/*******************************************************************************
 * include/generators/geometric/geometric_2d.h
 *
 * Copyright (C) 2017 Sebastian Lamm <lamm@ira.uka.de>
 * Copyright (C) 2017 Daniel Funke <funke@ira.uka.de>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/
#pragma once

#include <google/dense_hash_map>
#include <iostream>
#include <tuple>
#include <vector>

#include <sampling/hash.hpp>

#include "kagen/context.h"
#include "kagen/definitions.h"
#include "kagen/generators/generator.h"
#include "kagen/tools/geometry.h"
#include "kagen/tools/mersenne.h"
#include "kagen/tools/rng_wrapper.h"
#include "libmorton/morton3D.h"

namespace kagen {
class Geometric3D : public virtual Generator, private EdgeListOnlyGenerator {
public:
    // n, x_off, y_off, z_off, generated, offset
    using Chunk = std::tuple<SInt, LPFloat, LPFloat, LPFloat, bool, SInt>;
    // n, x_off, y_off, z_off, generated, offset
    using Cell = std::tuple<SInt, LPFloat, LPFloat, LPFloat, bool, SInt>;
    // x, y, z, id
    using Vertex = std::tuple<LPFloat, LPFloat, LPFloat, SInt>;

    Geometric3D(const PGeneratorConfig& config, const PEID rank, const PEID size)
        : config_(config),
          rank_(rank),
          size_(size),
          rng_(config) {
        start_node_ = std::numeric_limits<SInt>::max();
        num_nodes_  = 0;
    }

protected:
    void GenerateEdgeList() final {
        // Generate point distribution
        for (SInt i = local_chunk_start_; i < local_chunk_end_; i++)
            ComputeChunk(i);

        // Generate local chunks and edges
        for (SInt i = local_chunk_start_; i < local_chunk_end_; i++)
            GenerateChunk(i);

        SetVertexRange(start_node_, start_node_ + num_nodes_);
    }

    // Config
    const PGeneratorConfig& config_;

    PEID rank_;
    PEID size_;

    // Variates
    RNGWrapper<> rng_;
    Mersenne     mersenne_;

    // Constants and variables
    LPFloat chunk_size_;
    SInt    total_chunks_, chunks_per_dim_, local_chunk_start_, local_chunk_end_;
    LPFloat cell_size_;
    SInt    cells_per_chunk_, cells_per_dim_;
    SInt    start_node_, num_nodes_;

    // Data structures
    // std::vector<Chunk> chunks_;
    google::dense_hash_map<SInt, Chunk> chunks_;
    // std::vector<Cell> cells_;
    google::dense_hash_map<SInt, Cell> cells_;
    // std::vector<std::vector<Vertex>> vertices_;
    google::dense_hash_map<SInt, std::vector<Vertex>> vertices_;

    virtual SInt computeNumberOfCells() const {
        return 1;
    }

    void InitDatastructures() {
        // Chunk distribution
        SInt leftover_chunks = total_chunks_ % size_;
        SInt local_chunks    = total_chunks_ / size_ + ((SInt)rank_ < leftover_chunks);

        local_chunk_start_ = rank_ * local_chunks + ((SInt)rank_ >= leftover_chunks ? leftover_chunks : 0);
        local_chunk_end_   = local_chunk_start_ + local_chunks;

        // Init data structures
        chunks_.set_empty_key(total_chunks_);
        cells_.set_empty_key(total_chunks_ * cells_per_chunk_);
        vertices_.set_empty_key(total_chunks_ * cells_per_chunk_);
        // chunks_.resize(total_chunks_);
        // cells_.resize(total_chunks_ * cells_per_chunk_);
        // vertices_.resize(total_chunks_ * cells_per_chunk_);
        // edge_file = fopen((config_.debug_output + std::to_string(rank_)).c_str(),
        // "w");
    }

    void ComputeChunk(const SInt chunk_id) {
        ComputeChunk(chunk_id, config_.n, chunks_per_dim_, chunks_per_dim_, chunks_per_dim_, 0, 0, 0, 1, 0);
    }

    void ComputeChunk(
        const SInt chunk_id, const SInt n, const SInt row_k, const SInt column_k, const SInt depth_k,
        const SInt chunk_start_row, const SInt chunk_start_column, const SInt chunk_start_depth, const SInt level,
        const SInt offset) {
        // Stop if chunk exists
        if (chunks_.find(chunk_id) != end(chunks_))
            return;

        // Stop if empty or already generated
        if (n <= 0 || n > config_.n)
            return;

        SInt chunk_row, chunk_column, chunk_depth;
        Decode(chunk_id, chunk_column, chunk_row, chunk_depth);

        SInt chunk_start = Encode(chunk_start_column, chunk_start_row, chunk_start_depth);

        // Base case
        if (row_k == 1 && column_k == 1 && depth_k == 1) {
            chunks_[chunk_start] = std::make_tuple(
                n, chunk_start_row * chunk_size_, chunk_start_column * chunk_size_, chunk_start_depth * chunk_size_,
                false, offset);
            if (IsLocalChunk(chunk_id)) {
                if (start_node_ > offset)
                    start_node_ = offset;
                num_nodes_ += n;
            }
            return;
        }

        // Find splitter
        SInt row_splitter    = (row_k + 1) / 2;
        SInt column_splitter = (column_k + 1) / 2;
        SInt depth_splitter  = (depth_k + 1) / 2;

        // Generate variate for upper/lower half
        SInt h         = sampling::Spooky::hash(config_.seed + chunk_start + level * total_chunks_);
        SInt v_variate = rng_.GenerateBinomial(h, n, (LPFloat)row_splitter / row_k);

        if (chunk_row < row_splitter + chunk_start_row) {
            // Generate variate for left/right half
            SInt h_variate = rng_.GenerateBinomial(h, v_variate, (LPFloat)column_splitter / column_k);

            if (chunk_column < column_splitter + chunk_start_column) {
                // Generate variate for front/back half
                SInt z_variate = rng_.GenerateBinomial(h, h_variate, (LPFloat)depth_splitter / depth_k);

                // Upper left front/back octant
                if (chunk_depth < depth_splitter + chunk_start_depth)
                    ComputeChunk(
                        chunk_id, z_variate, row_splitter, column_splitter, depth_splitter, chunk_start_row,
                        chunk_start_column, chunk_start_depth, level + 1, offset);
                else
                    ComputeChunk(
                        chunk_id, h_variate - z_variate, row_splitter, column_splitter, depth_k - depth_splitter,
                        chunk_start_row, chunk_start_column, chunk_start_depth + depth_splitter, level + 1,
                        offset + z_variate);
            } else {
                // Generate variate for front/back half
                SInt z_variate = rng_.GenerateBinomial(h, v_variate - h_variate, (LPFloat)depth_splitter / depth_k);

                if (chunk_depth < depth_splitter + chunk_start_depth)
                    // Upper right front/back octant
                    ComputeChunk(
                        chunk_id, z_variate, row_splitter, column_k - column_splitter, depth_splitter, chunk_start_row,
                        chunk_start_column + column_splitter, chunk_start_depth, level + 1, offset + h_variate);
                else
                    ComputeChunk(
                        chunk_id, v_variate - h_variate - z_variate, row_splitter, column_k - column_splitter,
                        depth_k - depth_splitter, chunk_start_row, chunk_start_column + column_splitter,
                        chunk_start_depth + depth_splitter, level + 1, offset + h_variate + z_variate);
            }
        } else {
            // Lower half
            // Generate variate for left/right half
            SInt h_variate = rng_.GenerateBinomial(h, n - v_variate, (LPFloat)column_splitter / column_k);

            if (chunk_column < column_splitter + chunk_start_column) {
                // Generate variate for front/back half
                SInt z_variate = rng_.GenerateBinomial(h, h_variate, (LPFloat)depth_splitter / depth_k);

                // Lower left front/back octant
                if (chunk_depth < depth_splitter + chunk_start_depth)
                    ComputeChunk(
                        chunk_id, z_variate, row_k - row_splitter, column_splitter, depth_splitter,
                        chunk_start_row + row_splitter, chunk_start_column, chunk_start_depth, level + 1,
                        offset + v_variate);
                else
                    ComputeChunk(
                        chunk_id, h_variate - z_variate, row_k - row_splitter, column_splitter,
                        depth_k - depth_splitter, chunk_start_row + row_splitter, chunk_start_column,
                        chunk_start_depth + depth_splitter, level + 1, offset + v_variate + z_variate);
            } else {
                // Generate variate for front/back half
                SInt z_variate = rng_.GenerateBinomial(h, n - v_variate - h_variate, (LPFloat)depth_splitter / depth_k);
                // Lower right front/back octant
                if (chunk_depth < depth_splitter + chunk_start_depth)
                    ComputeChunk(
                        chunk_id, z_variate, row_k - row_splitter, column_k - column_splitter, depth_splitter,
                        chunk_start_row + row_splitter, chunk_start_column + column_splitter, chunk_start_depth,
                        level + 1, offset + v_variate + h_variate);
                else
                    ComputeChunk(
                        chunk_id, n - v_variate - h_variate - z_variate, row_k - row_splitter,
                        column_k - column_splitter, depth_k - depth_splitter, chunk_start_row + row_splitter,
                        chunk_start_column + column_splitter, chunk_start_depth + depth_splitter, level + 1,
                        offset + v_variate + h_variate + z_variate);
            }
        }
    }

    void GenerateChunk(const SInt chunk_id) {
        SInt chunk_row, chunk_column, chunk_depth;
        Decode(chunk_id, chunk_column, chunk_row, chunk_depth);
        // Generate nodes, gather neighbors and add edges
        GenerateCells(chunk_id);
        for (SInt i = 0; i < cells_per_chunk_; ++i)
            GenerateVertices(chunk_id, i, true);
        // Generate edges and vertices on demand
        GenerateEdges(chunk_row, chunk_column, chunk_depth);
    }

    virtual void GenerateCells(const SInt chunk_id) {
        // Lazily compute chunk
        if (chunks_.find(chunk_id) == end(chunks_)) {
            ComputeChunk(chunk_id);
        }

        auto& chunk = chunks_[chunk_id];

        // Stop if cell distribution already generated
        if (std::get<4>(chunk))
            return;

        SInt    seed       = 0;
        SInt    n          = std::get<0>(chunk);
        SInt    offset     = std::get<5>(chunk);
        LPFloat total_area = chunk_size_ * chunk_size_ * chunk_size_;
        LPFloat cell_area  = cell_size_ * cell_size_ * cell_size_;

        for (SInt i = 0; i < cells_per_chunk_; ++i) {
            seed                  = config_.seed + chunk_id * cells_per_chunk_ + i + total_chunks_ * cells_per_chunk_;
            SInt    h             = sampling::Spooky::hash(seed);
            SInt    cell_vertices = rng_.GenerateBinomial(h, n, cell_area / total_area);
            LPFloat cell_start_x  = std::get<1>(chunk) + ((i / cells_per_dim_) % cells_per_dim_) * cell_size_;
            LPFloat cell_start_y  = std::get<2>(chunk) + (i % cells_per_dim_) * cell_size_;
            LPFloat cell_start_z  = std::get<3>(chunk) + (i / (cells_per_dim_ * cells_per_dim_)) * cell_size_;

            // Only store cells that are adjacent to local ones
            if (cell_vertices != 0) {
                cells_[ComputeGlobalCellId(chunk_id, i)] =
                    std::make_tuple(cell_vertices, cell_start_x, cell_start_y, cell_start_z, false, offset);
            }

            // Update for multinomial
            n -= cell_vertices;
            offset += cell_vertices;
            total_area -= cell_area;
        }
        std::get<4>(chunk) = true;
    }

    void GenerateVertices(const SInt chunk_id, const SInt cell_id, const bool push_coordinates) {
        // Lazily compute chunk
        if (chunks_.find(chunk_id) == end(chunks_)) {
            ComputeChunk(chunk_id);
        }

        auto& chunk = chunks_[chunk_id];

        // Lazily compute cell distribution
        if (!std::get<4>(chunk))
            GenerateCells(chunk_id);

        // Stop if cell already generated
        SInt global_cell_id = ComputeGlobalCellId(chunk_id, cell_id);
        if (cells_.find(global_cell_id) == end(cells_))
            return;
        auto& cell = cells_[global_cell_id];
        if (std::get<4>(cell))
            return;

        // Compute vertex distribution
        SInt    n       = std::get<0>(cell);
        SInt    offset  = std::get<5>(cell);
        LPFloat start_x = std::get<1>(cell);
        LPFloat start_y = std::get<2>(cell);
        LPFloat start_z = std::get<3>(cell);

        SInt seed = config_.seed + chunk_id * cells_per_chunk_ + cell_id;
        SInt h    = sampling::Spooky::hash(seed);
        mersenne_.RandomInit(h);
        std::vector<Vertex>& cell_vertices = vertices_[global_cell_id];
        cell_vertices.reserve(n);
        for (SInt i = 0; i < n; ++i) {
            // Compute coordinates
            LPFloat x = mersenne_.Random() * cell_size_ + start_x;
            LPFloat y = mersenne_.Random() * cell_size_ + start_y;
            LPFloat z = mersenne_.Random() * cell_size_ + start_z;

            cell_vertices.emplace_back(x, y, z, offset + i);
            // fprintf(edge_file, "v %f %f\n", x, y);

            if (push_coordinates && config_.coordinates) {
                PushCoordinate(x, y, z);
            }
        }
        std::get<4>(cell) = true;
    }

    void GenerateVertices(const SInt chunk_id, const SInt cell_id, std::vector<Vertex>& vertex_buffer) {
        // Lazily compute chunk
        if (chunks_.find(chunk_id) == end(chunks_)) {
            ComputeChunk(chunk_id);
        }

        const auto& chunk = chunks_[chunk_id];

        // Lazily compute cell distribution
        if (!std::get<4>(chunk))
            GenerateCells(chunk_id);

        // Compute vertex distribution
        SInt global_cell_id = ComputeGlobalCellId(chunk_id, cell_id);
        if (cells_.find(global_cell_id) == end(cells_))
            return;
        const auto& cell = cells_[global_cell_id];
        if (std::get<4>(cell))
            return;

        SInt    n       = std::get<0>(cell);
        SInt    offset  = std::get<5>(cell);
        LPFloat start_x = std::get<1>(cell);
        LPFloat start_y = std::get<2>(cell);
        LPFloat start_z = std::get<3>(cell);

        SInt seed = config_.seed + chunk_id * cells_per_chunk_ + cell_id;
        SInt h    = sampling::Spooky::hash(seed);
        mersenne_.RandomInit(h);
        vertex_buffer.clear();
        vertex_buffer.reserve(n);
        for (SInt i = 0; i < n; ++i) {
            // Compute coordinates
            LPFloat x = mersenne_.Random() * cell_size_ + start_x;
            LPFloat y = mersenne_.Random() * cell_size_ + start_y;
            LPFloat z = mersenne_.Random() * cell_size_ + start_z;

            // vertex_buffer.emplace_back(x, y, z);
            vertex_buffer[i] = std::make_tuple(x, y, z, offset + i);
            // fprintf(edge_file, "v %f %f\n", x, y);
        }
    }

    virtual void GenerateEdges(const SInt chunk_row, const SInt chunk_column, const SInt chunk_depth) = 0;

    inline SInt ComputeGlobalCellId(const SInt chunk_id, const SInt cell_id) const {
        return chunk_id * cells_per_chunk_ + cell_id;
    }

    inline bool IsLocalChunk(const SInt chunk_id) const {
        return (chunk_id >= local_chunk_start_ && chunk_id < local_chunk_end_);
    }

    // Chunk coding
    inline SInt Encode(const SInt x, const SInt y, const SInt z) const {
        return libmorton::m3D_e_sLUT<SInt>(z, x, y);
        // return x + y * chunks_per_dim_ + z * (chunks_per_dim_ * chunks_per_dim_);
    }

    inline void Decode(const SInt id, SInt& x, SInt& y, SInt& z) const {
        libmorton::m3D_d_sLUT(id, z, x, y);
        // x = id % chunks_per_dim_;
        // y = (id / chunks_per_dim_) % chunks_per_dim_;
        // z = id / (chunks_per_dim_ * chunks_per_dim_);
    }
};
} // namespace kagen
