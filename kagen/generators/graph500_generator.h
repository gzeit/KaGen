#pragma once

#include "kagen/generators/generator.h"

#include <mpi.h>

namespace kagen {
class Graph500Generator : public virtual Generator, private EdgeListOnlyGenerator {
public:
    Graph500Generator(const PGeneratorConfig& config);

protected:
    void FinalizeEdgeList(MPI_Comm comm) final;

    inline void PushLocalEdge(const int from, const int to) {
        if (config_.self_loops || from != to) {
            local_edges_.emplace_back(from, to);
        }
        if (!config_.directed && from != to) {
            local_edges_.emplace_back(to, from);
        }
    }

private:
    const PGeneratorConfig&           config_;
    std::vector<std::tuple<int, int>> local_edges_;
};
} // namespace kagen
