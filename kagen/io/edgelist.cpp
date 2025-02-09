#include "kagen/io/edgelist.h"

#include "kagen/definitions.h"
#include "kagen/io/buffered_writer.h"
#include "kagen/io/graph_writer.h"

namespace kagen {
EdgeListWriter::EdgeListWriter(Graph& graph, MPI_Comm comm) : SequentialGraphWriter(graph, comm) {}

std::string EdgeListWriter::DefaultExtension() const {
    return "edgelist";
}

int EdgeListWriter::Requirements() const {
    return Requirement::NO_VERTEX_WEIGHTS | Requirement::NO_EDGE_WEIGHTS;
}

void EdgeListWriter::AppendHeaderTo(const std::string& filename, const SInt n, const SInt m) {
    BufferedTextOutput<> out(tag::append, filename);
    out.WriteString("p ").WriteInt(n).WriteChar(' ').WriteInt(m).WriteChar('\n').Flush();
}

void EdgeListWriter::AppendTo(const std::string& filename) {
    BufferedTextOutput<> out(tag::append, filename);
    for (const auto& [from, to]: edges_) {
        out.WriteString("e ").WriteInt(from + 1).WriteChar(' ').WriteInt(to + 1).WriteChar('\n').Flush();
    }
}

BinaryEdgeListWriter::BinaryEdgeListWriter(Graph& graph, MPI_Comm comm, int width)
    : SequentialGraphWriter(graph, comm),
      width_(width) {}

std::string BinaryEdgeListWriter::DefaultExtension() const {
    return "binaryedgelist";
}

int BinaryEdgeListWriter::Requirements() const {
    return Requirement::NO_VERTEX_WEIGHTS | Requirement::NO_EDGE_WEIGHTS;
}

void BinaryEdgeListWriter::AppendHeaderTo(const std::string& filename, const SInt n, const SInt m) {
    auto* fout = std::fopen(filename.c_str(), "ab");
    std::fwrite(&n, sizeof(SInt), 1, fout);
    std::fwrite(&m, sizeof(SInt), 1, fout);
    std::fclose(fout);
}

void BinaryEdgeListWriter::AppendTo(const std::string& filename) {
    auto* fout = std::fopen(filename.c_str(), "ab");
    for (const auto& [from, to]: edges_) {
        if (width_ == 64) {
            const std::uint64_t edges[2] = {static_cast<std::uint64_t>(from), static_cast<std::uint64_t>(to)};
            std::fwrite(edges, sizeof(std::uint64_t), 2, fout);
        } else { // 32
            const std::uint32_t edges[2] = {static_cast<std::uint32_t>(from), static_cast<std::uint32_t>(to)};
            std::fwrite(edges, sizeof(std::uint32_t), 2, fout);
        }
    }
    fclose(fout);
}

} // namespace kagen
