#include "StlWriter.h"
#include "StlReader.h"
#include "MeshReader.h"
#include "core/cutting/PlaneCut.h"
#include <algorithm>
#include <sstream>
#include <bit>
#include <cmath>
#include <limits>

namespace stlcut {
std::string validatedStl(const Mesh& mesh,bool requireClosed,std::stop_token stop) {
    std::ostringstream out(std::ios::binary);
    writeStl(out,mesh,stop);
    auto bytes=out.str();
    const auto roundTrip=readMesh(std::span(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size()),stop);
    if(roundTrip.triangles.size()!=mesh.triangles.size())throw std::runtime_error("Reimportacion STL inconsistente.");
    const auto dims=mesh.bounds.dimensions();const auto extent=std::max({dims.x,dims.y,dims.z});
    for(size_t i=0;i<mesh.triangles.size();++i) {
        if(stop.stop_requested())throw ImportCancelled{};
        for(size_t j=0;j<3;++j) {
            const auto a=mesh.triangles[i].vertices[j],b=roundTrip.triangles[i].vertices[j];
            if(std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)>extent*1e-6)
                throw std::runtime_error("La precision float32 de STL altera demasiado las coordenadas; exportacion descartada.");
        }
    }
    if(requireClosed) {
        try {verifyClosedMesh(roundTrip,extent*1e-9,stop);}
        catch(const ImportCancelled&) {throw;}
        catch(const std::runtime_error& e) {
            throw std::runtime_error(std::string("STL float32 no conserva una pieza cerrada y valida; exportacion descartada: ")+e.what());
        }
    }
    const auto original=signedVolume(mesh),volume=signedVolume(roundTrip);
    if(!std::isfinite(volume)||std::abs(volume-original)>std::max(std::abs(original)*1e-5,extent*extent*extent*1e-8))
        throw std::runtime_error("La precision float32 de STL altera demasiado el volumen; exportacion descartada.");
    if(stop.stop_requested())throw ImportCancelled{};
    return bytes;
}
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
