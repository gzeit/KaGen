/*******************************************************************************
 * include/generator_config.h
 *
 * Copyright (C) 2016-2017 Sebastian Lamm <lamm@ira.uka.de>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/
#pragma once

#include <iostream>
#include <limits>
#include <ostream>
#include <string>
#include <unordered_map>

#include "kagen/definitions.h"

namespace kagen {
enum class OutputFormat {
    NONE,
    EDGE_LIST,
    BINARY_EDGE_LIST,
    BINARY_EDGE_LIST32,
    METIS,
    HMETIS,
    DOT,
    DOT_DIRECTED,
    COORDINATES,
    BINARY_PARHIP,
};

std::unordered_map<std::string, OutputFormat> GetOutputFormatMap();

std::ostream& operator<<(std::ostream& out, OutputFormat format);

enum class OutputHeader {
    ALWAYS,
    ROOT,
    NEVER,
};

std::unordered_map<std::string, OutputHeader> GetOutputHeaderMap();

std::ostream& operator<<(std::ostream& out, OutputHeader output_header);

enum class GeneratorType {
    GNM_DIRECTED,
    GNM_UNDIRECTED,
    GNP_DIRECTED,
    GNP_UNDIRECTED,
    RGG_2D,
    RGG_3D,
#ifdef KAGEN_CGAL_FOUND
    RDG_2D,
    RDG_3D,
#endif // KAGEN_CGAL_FOUND
    GRID_2D,
    GRID_3D,
    BA,
    KRONECKER,
    RHG,
    RMAT,
    IMAGE_MESH,
    STATIC_GRAPH,
};

std::unordered_map<std::string, GeneratorType> GetGeneratorTypeMap();

std::ostream& operator<<(std::ostream& out, GeneratorType generator_type);

enum class StatisticsLevel : std::uint8_t {
    NONE     = 0,
    BASIC    = 1,
    ADVANCED = 2,
};

bool operator<=(StatisticsLevel a, StatisticsLevel b);

std::unordered_map<std::string, StatisticsLevel> GetStatisticsLevelMap();

std::ostream& operator<<(std::ostream& out, StatisticsLevel statistics_level);

enum class ImageMeshWeightModel : std::uint8_t {
    L2         = 0,
    INV_L2     = 1,
    RATIO      = 2,
    INV_RATIO  = 3,
    SIMILARITY = 4,
};

std::unordered_map<std::string, ImageMeshWeightModel> GetImageMeshWeightModelMap();

std::ostream& operator<<(std::ostream& out, ImageMeshWeightModel weight_model);

struct ImageMeshConfig {
    std::string          filename             = "";
    ImageMeshWeightModel weight_model         = ImageMeshWeightModel::RATIO;
    double               weight_multiplier    = 255.0;
    double               weight_offset        = 0.0;
    double               weight_min_threshold = 1.0;
    double               weight_max_threshold = std::numeric_limits<double>::max();
    SInt                 neighborhood         = 4;
    SInt                 max_grid_x           = 0;
    SInt                 max_grid_y           = 0;
    SInt                 grid_x               = 0;
    SInt                 grid_y               = 0;
    SInt                 cols_per_pe          = 0;
    SInt                 rows_per_pe          = 0;
};

enum class StaticGraphDistribution {
    BALANCE_VERTICES,
    BALANCE_EDGES,
};

std::unordered_map<std::string, StaticGraphDistribution> GetStaticGraphDistributionMap();

std::ostream& operator<<(std::ostream& out, StaticGraphDistribution distribution);

enum class StaticGraphFormat {
    METIS,
    BINARY_PARHIP,
};

std::unordered_map<std::string, StaticGraphFormat> GetStaticGraphFormatMap();

std::ostream& operator<<(std::ostream& out, StaticGraphFormat format);

struct StaticGraphConfig {
    std::string             filename     = "";
    StaticGraphFormat       format       = StaticGraphFormat::METIS;
    StaticGraphDistribution distribution = StaticGraphDistribution::BALANCE_VERTICES;
};

// Configuration for the generator.
struct PGeneratorConfig {
    // General settings
    bool            quiet                 = false; // Disable all console output
    bool            validate_simple_graph = false; // Validate that the result is a simple graph
    StatisticsLevel statistics_level      = StatisticsLevel::BASIC;
    bool            skip_postprocessing   = false;
    bool            print_header          = true;

    // Generator settings
    GeneratorType generator;          // Generator type
    SInt          n          = 0;     // Number of nodes
    SInt          m          = 0;     // Number of edges
    SInt          k          = 0;     // Number of chunks
    double        p          = 0.0;   // Edge probability
    double        r          = 0.0;   // Edge radius
    bool          self_loops = false; // Allow self loops
    double        plexp      = 2.6;   // Power law exponent
    double        avg_degree = 0.0;   // Average degree
    double        thres      = 0.0;   // Clique threshold (RHG)
    bool          query_both = false; // Query strategy (RHG) -- should be set to false
    SInt          min_degree = 0.0;   // Minimum degree (BA)
    SInt          grid_x     = 0;     // Grid x dimension (Grid2D, Grid3D)
    SInt          grid_y     = 0;     // Grid y dimension (Grid2D, Grid3D)
    SInt          grid_z     = 0;     // Grid z dimension (Grid3D)
    bool          periodic   = false; // Use periodic boundary (Grid2D, Grid3D)
    int           hp_floats  = 0;     // Use 80 bit floating point numbers for RHG generator, 0 for auto
    double        rmat_a     = 0.0;
    double        rmat_b     = 0.0;
    double        rmat_c     = 0.0;
    bool          directed   = false;

    double max_vertex_imbalance = 0.1; // RGG, RDG, RHG

    bool coordinates = false; // Store vertex coordinates

    // Image mesh generator settings
    ImageMeshConfig image_mesh{};

    // Settings for the static graph pseudo-generator
    StaticGraphConfig static_graph{};

    // Hashing / sampling settings
    int   seed        = 1;      // Seed for PRNG
    bool  hash_sample = false;  // Use hash tryagain sampling
    bool  use_binom   = false;  // Use binomial approximation to hypergeomtry
    ULONG precision   = 32;     // Floating-point precision
    ULONG base_size   = 1 << 8; // Sampler base size
    ULONG hyp_base    = 1 << 8;

    // IO settings
    OutputFormat output_format      = OutputFormat::EDGE_LIST; // Output format
    OutputHeader output_header      = OutputHeader::ROOT;      // PEs that print file headers
    std::string  output_file        = "out";                   // Output filename
    bool         output_single_file = true;                    // Collect all graphs in a single output file
};

std::ostream& operator<<(std::ostream& out, const PGeneratorConfig& config);

PGeneratorConfig CreateConfigFromString(const std::string& options_str, PGeneratorConfig config = {});
} // namespace kagen
