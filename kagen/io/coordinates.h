#pragma once

#include <mpi.h>

#include "kagen/io/graph_writer.h"

namespace kagen {
class CoordinatesWriter : public SequentialGraphWriter {
public:
    CoordinatesWriter(Graph& graph, MPI_Comm comm);

    std::string DefaultExtension() const final;

protected:
    int Requirements() const final;

    void AppendHeaderTo(const std::string& filename, const SInt n, const SInt m) final;

    void AppendTo(const std::string& filename) final;
};
} // namespace kagen
