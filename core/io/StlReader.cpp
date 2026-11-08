#include "core/io/StlReader.h"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace stlcut {
namespace {
void checkStop(std::stop_token stop)
{ if (stop.stop_requested()) { throw ImportCancelled{}; } }

[[noreturn]] void invalid(const char* message)
{ throw std::runtime_error(message); }

std::uint32_t little32(const unsigned char* p)
{
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

void addTriangle(Mesh& mesh, const Triangle& triangle)
{
    if (mesh.triangles.size() >= maxTriangles) {
        invalid("El STL supera el límite de 2 millones de triángulos.");
    }
    for (const auto& p : triangle.vertices) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
            invalid("El STL contiene coordenadas no finitas o fuera de rango.");
        }
    }
    if (mesh.triangles.empty()) {
        mesh.bounds.min = mesh.bounds.max = triangle.vertices[0];
    }
    for (const auto& p : triangle.vertices) {
        auto& lo = mesh.bounds.min;
        auto& hi = mesh.bounds.max;
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    const auto& a = triangle.vertices[0];
    const auto& b = triangle.vertices[1];
    const auto& c = triangle.vertices[2];
    // Normalize first to avoid overflowing cross products for large finite values.
    const double scale = std::max({std::abs(a.x), std::abs(a.y), std::abs(a.z),
                                   std::abs(b.x), std::abs(b.y), std::abs(b.z),
                                   std::abs(c.x), std::abs(c.y), std::abs(c.z)});
    bool degenerate = scale == 0;
    if (!degenerate) {
        const Vec3 u{b.x / scale - a.x / scale, b.y / scale - a.y / scale,
                     b.z / scale - a.z / scale};
        const Vec3 v{c.x / scale - a.x / scale, c.y / scale - a.y / scale,
                     c.z / scale - a.z / scale};
        const double cross = std::hypot(u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z,
                                        u.x*v.y-u.y*v.x);
        degenerate = cross <= 1e-12 * std::hypot(u.x,u.y,u.z) * std::hypot(v.x,v.y,v.z);
    }
    mesh.degenerateTriangles += degenerate ? 1 : 0;
    mesh.triangles.push_back(triangle);
}

class Tokens {
public:
    Tokens(std::istream& input, std::stop_token stop) : input_(input), stop_(stop) {}
    std::string next()
    {
        checkStop(stop_);
        std::string token;
        char ch{};
        std::size_t skipped{};
        while (input_.get(ch)) {
            if ((++skipped & 4095) == 0) { checkStop(stop_); }
            if (!space(ch)) { token.push_back(ch); break; }
        }
        while (input_.get(ch)) {
            if (space(ch)) { input_.unget(); break; }
            if (token.size() >= 128) { invalid("Token STL ASCII demasiado largo."); }
            token.push_back(ch);
        }
        if (input_.bad()) { invalid("Error de lectura del STL."); }
        return token;
    }
    void expect(const char* expected)
    { if (next() != expected) { invalid("Sintaxis STL ASCII inválida o archivo truncado."); } }
    double number()
    {
        auto token = next();
        const char* begin = token.data();
        const char* end = begin + token.size();
        if (begin != end && *begin == '+') {
            ++begin;
            if (begin != end && (*begin == '+' || *begin == '-')) {
                invalid("Signo numérico STL inválido.");
            }
        }
        double value{};
        auto result = std::from_chars(begin, end, value);
        if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
            invalid("Valor numérico STL inválido, no finito o fuera de rango.");
        }
        return value;
    }
    void nameLine()
    {
        char ch{};
        std::size_t length{};
        while (input_.get(ch) && ch != '\n') {
            if (++length > 4096) { invalid("Nombre de sólido STL demasiado largo."); }
        }
    }
private:
    static bool space(char ch)
    { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' || ch == '\v'; }
    std::istream& input_;
    std::stop_token stop_;
};

Mesh ascii(std::istream& input, std::stop_token stop)
{
    Mesh mesh;
    mesh.format = "ASCII";
    Tokens tokens(input, stop);
    tokens.expect("solid");
    tokens.nameLine();
    for (;;) {
        auto token = tokens.next();
        if (token == "endsolid") {
            tokens.nameLine();
            if (!tokens.next().empty()) { invalid("Datos adicionales después de endsolid."); }
            break;
        }
        if (token != "facet") { invalid("Se esperaba facet o endsolid en STL ASCII."); }
        tokens.expect("normal");
        // Ignore supplied normals after validating; geometry is defined by vertices.
        tokens.number(); tokens.number(); tokens.number();
        tokens.expect("outer"); tokens.expect("loop");
        Triangle triangle;
        for (auto& p : triangle.vertices) {
            tokens.expect("vertex");
            p = {tokens.number(), tokens.number(), tokens.number()};
        }
        tokens.expect("endloop"); tokens.expect("endfacet");
        addTriangle(mesh, triangle);
    }
    return mesh;
}

Mesh binary(std::istream& input, std::uint32_t count, std::stop_token stop)
{
    Mesh mesh;
    mesh.format = "binario";
    checkStop(stop);
    mesh.triangles.reserve(count);
    input.seekg(84);
    for (std::uint32_t index = 0; index < count; ++index) {
        checkStop(stop);
        unsigned char record[50];
        if (!input.read(reinterpret_cast<char*>(record), sizeof(record))) {
            invalid("STL binario truncado durante la lectura.");
        }
        auto scalar = [&](std::size_t offset) -> double {
            const float value = std::bit_cast<float>(little32(record + offset));
            if (!std::isfinite(value)) { invalid("STL binario contiene valores no finitos."); }
            return value;
        };
        scalar(0); scalar(4); scalar(8);
        Triangle triangle;
        for (std::size_t vertex = 0; vertex < 3; ++vertex) {
            const auto offset = 12 + vertex * 12;
            triangle.vertices[vertex] = {scalar(offset), scalar(offset+4), scalar(offset+8)};
        }
        addTriangle(mesh, triangle);
        // Attribute bytes (including vendor color conventions) are not interpreted.
    }
    return mesh;
}
}

Mesh readStl(std::istream& input, std::uint64_t bytes, std::stop_token stop)
{
    checkStop(stop);
    if (bytes == 0) { invalid("El archivo STL está vacío."); }
    if (bytes > maxFileBytes) { invalid("El archivo STL supera el límite de 256 MiB."); }
    std::uint32_t count{};
    bool exactBinary{};
    if (bytes >= 84) {
        input.seekg(80);
        unsigned char raw[4];
        if (!input.read(reinterpret_cast<char*>(raw), 4)) { invalid("No se pudo leer el STL."); }
        count = little32(raw);
        exactBinary = bytes == 84ULL + 50ULL * count;
    }
    input.clear(); input.seekg(0);
    Mesh mesh;
    if (exactBinary) {
        if (count > maxTriangles) { invalid("El STL supera el límite de 2 millones de triángulos."); }
        mesh = binary(input, count, stop);
    } else {
        // Exact binary length wins even when the 80-byte header begins with solid.
        // Otherwise require a complete ASCII grammar; never accept partial binary.
        mesh = ascii(input, stop);
    }
    checkStop(stop);
    if (mesh.triangles.empty()) { invalid("El STL no contiene triángulos."); }
    const auto d = mesh.bounds.dimensions();
    if (!std::isfinite(d.x) || !std::isfinite(d.y) || !std::isfinite(d.z)) {
        invalid("Las dimensiones STL exceden el rango numérico admitido.");
    }
    return mesh;
}

Mesh readStl(const std::filesystem::path& path, std::stop_token stop)
{
    checkStop(stop);
    if (!std::filesystem::is_regular_file(path)) { invalid("La ruta no es un archivo regular."); }
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) { invalid("No se pudo abrir el STL para lectura."); }
    const auto end = input.tellg();
    if (end < 0) { invalid("No se pudo determinar el tamaño del STL."); }
    input.seekg(0);
    auto mesh = readStl(input, static_cast<std::uint64_t>(end), stop);
    // Detect changes in length while reading without modifying the original file.
    input.clear(); input.seekg(0, std::ios::end);
    if (input.tellg() != end) { invalid("El archivo STL cambió durante la importación."); }
    return mesh;
}
}
