#pragma once

#include <string>

#include <mpi.h>

#include "kagen/context.h"
#include "kagen/definitions.h"

namespace kagen {
class GraphWriter {
public:
    GraphWriter(Graph& graph, MPI_Comm comm);

    virtual ~GraphWriter();

    virtual std::string DefaultExtension() const = 0;

    virtual void Write(const PGeneratorConfig& config) = 0;

protected:
    bool HasVertexWeights() const;
    bool HasEdgeWeights() const;

    EdgeList&      edges_;
    VertexRange&   vertex_range_;
    Coordinates&   coordinates_;
    VertexWeights& vertex_weights_;
    EdgeWeights&   edge_weights_;
    MPI_Comm       comm_;

    bool has_vertex_weights_;
    bool has_edge_weights_;
};

class SequentialGraphWriter : public GraphWriter {
protected:
    enum Requirement {
        NONE              = 0,
        SORTED_EDGES      = 1 << 1,
        COORDINATES       = 1 << 2,
        COORDINATES_2D    = 1 << 3,
        COORDINATES_3D    = 1 << 4,
        NO_VERTEX_WEIGHTS = 1 << 5,
        NO_EDGE_WEIGHTS   = 1 << 6,
    };

public:
    SequentialGraphWriter(Graph& graph, MPI_Comm comm);

    virtual void Write(const PGeneratorConfig& config) override;

protected:
    virtual void AppendHeaderTo(const std::string& filename, SInt n, SInt m) = 0;

    virtual void AppendTo(const std::string& filename) = 0;

    virtual void AppendFooterTo(const std::string& filename);

    virtual int Requirements() const;

private:
    static void CreateFile(const std::string& filename);
};

class NoopWriter : public GraphWriter {
public:
    NoopWriter(Graph& graph, MPI_Comm comm);

    std::string DefaultExtension() const final;

    void Write(const PGeneratorConfig& config) final;
};
} // namespace kagen
