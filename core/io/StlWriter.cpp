#include "StlWriter.h"
#include "StlReader.h"
#include <bit>
#include <cmath>
#include <limits>

namespace stlcut {
void writeStl(std::ostream& out,const Mesh& mesh,std::stop_token stop) {
    if(mesh.triangles.empty()||mesh.triangles.size()>maxTriangles)throw std::runtime_error("Cantidad de triangulos no exportable.");
    auto integer=[&](std::uint32_t n) { for(int i=0;i<4;++i)out.put(static_cast<char>((n>>(8*i))&255)); };
    auto number=[&](double n) {
        if(!std::isfinite(n)||std::abs(n)>std::numeric_limits<float>::max())throw std::runtime_error("Coordenada fuera del rango STL float32.");
        integer(std::bit_cast<std::uint32_t>(static_cast<float>(n)));
    };
    char header[80]="STL Cut Studio AI - binary STL; units supplied by user; no materials";
    out.write(header,80); integer(static_cast<std::uint32_t>(mesh.triangles.size()));
    for(const auto& t:mesh.triangles) {
        if(stop.stop_requested())throw ImportCancelled{};
        const auto a=t.vertices[0],b=t.vertices[1],c=t.vertices[2];
        Vec3 u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
        Vec3 n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
        const auto len=std::hypot(n.x,n.y,n.z);
        if(!std::isfinite(len))throw std::runtime_error("Normal STL fuera de rango.");
        number(len ? n.x/len : 0);number(len ? n.y/len : 0);number(len ? n.z/len : 0);
        for(const auto& p:t.vertices) {number(p.x);number(p.y);number(p.z);}
        out.put(0);out.put(0);
        if(!out)throw std::runtime_error("No se pudo escribir el STL.");
    }
}
}
