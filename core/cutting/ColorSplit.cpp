#include "ColorSplit.h"
#include "PlaneCut.h"
#include "core/io/MeshReader.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <map>
#include <numeric>
#include <tuple>

namespace stlcut {
std::vector<Mesh> splitByMaterial(const Mesh& mesh,std::stop_token stop,const ConnectorSpec* connector) {
    auto check=[&] {if(stop.stop_requested())throw ImportCancelled{};};
    check();
    if(connector&&(!std::isfinite(connector->diameter)||!std::isfinite(connector->depth)||
       !std::isfinite(connector->clearance)||!std::isfinite(connector->wall)||connector->diameter<=0||
       connector->depth<=0||connector->clearance<0||connector->wall<=0))
        throw std::runtime_error("Parametros del conector invalidos.");
    if(mesh.unsupportedAppearance)throw std::runtime_error("Hay pintura/atributos de fabricante o materiales no interpretados; no se puede separar por color de forma fiable todavia.");
    if(mesh.styles.size()!=mesh.triangles.size()||mesh.materials.empty())
        throw std::runtime_error("El modelo no tiene propiedades de color/material estandar para separar.");
    // Map nodes/edges are additional to Mesh storage; bound the operation before
    // constructing its temporary topology. This is not a process RSS quota.
    if(mesh.triangles.size()>500000)throw std::runtime_error("Separacion por color limitada a 500000 triangulos para acotar topologia temporal.");
    const auto count=mesh.triangles.size();
    std::vector<size_t> parents(count);std::iota(parents.begin(),parents.end(),0);
    auto root=[&](size_t n) {while(parents[n]!=n){parents[n]=parents[parents[n]];n=parents[n];}return n;};
    using Point=std::tuple<std::uint32_t,double,double,double>;
    std::map<Point,size_t> vertices;
    using Edge=std::tuple<std::uint32_t,std::int32_t,size_t,size_t>;
    std::map<Edge,size_t> edges;
    for(size_t i=0;i<count;++i) {
        check();const auto& style=mesh.styles[i];const auto material=style.material[0];
        if(material<0||static_cast<size_t>(material)>=mesh.materials.size()||
           style.material[1]!=material||style.material[2]!=material)
            throw std::runtime_error("Color ausente o interpolado por vertice: se requiere reconstruccion volumetrica, no una simple separacion de cuerpos.");
        std::array<size_t,3> ids;
        for(int j=0;j<3;++j) {
            const auto& p=mesh.triangles[i].vertices[j];
            auto [entry,inserted]=vertices.emplace(Point{style.instance,p.x,p.y,p.z},vertices.size());
            ids[j]=entry->second;
        }
        for(int j=0;j<3;++j) {
            const auto [a,b]=std::minmax(ids[j],ids[(j+1)%3]);
            auto [entry,inserted]=edges.emplace(Edge{style.instance,material,a,b},i);
            if(!inserted)parents[root(i)]=root(entry->second);
        }
    }
    std::map<size_t,size_t> groups;std::vector<Mesh> pieces;
    for(size_t i=0;i<count;++i) {
        check();const auto id=root(i);
        auto [entry,inserted]=groups.emplace(id,pieces.size());
        if(inserted) {
            if(pieces.size()>=256)throw std::runtime_error("Separacion por color supera 256 regiones conectadas.");
            Mesh p;p.units=mesh.units;p.format="3mf-material-region";
            p.materials.push_back(mesh.materials[mesh.styles[i].material[0]]);p.warnings=mesh.warnings;
            for(const auto& instance:mesh.instances)if(instance.id==mesh.styles[i].instance) {
                p.instances.push_back(instance);p.instances.back().firstTriangle=0;p.instances.back().triangleCount=0;break;
            }
            pieces.push_back(std::move(p));
        }
        auto& p=pieces[entry->second];p.triangles.push_back(mesh.triangles[i]);
        p.styles.push_back({mesh.styles[i].instance,{0,0,0}});
    }
    if(pieces.size()<2)throw std::runtime_error("No hay dos regiones de material separables; se conserva la pieza actual.");
    double total=0;
    for(auto& p:pieces) {
        check();updateBounds(p);
        if(!p.instances.empty())p.instances.front().triangleCount=static_cast<std::uint32_t>(p.triangles.size());
        const auto d=p.bounds.dimensions();const double extent=std::max({d.x,d.y,d.z});
        verifyClosedMesh(p,extent*1e-9,stop);
        const auto volume=signedVolume(p);
        if(!std::isfinite(volume)||volume<=0)throw std::runtime_error("Region de color sin volumen positivo; requiere reconstruccion volumetrica.");
        total+=volume;
    }
    const auto d=mesh.bounds.dimensions();const auto extent=std::max({d.x,d.y,d.z});
    if(std::abs(total-signedVolume(mesh))>std::max(std::abs(total)*1e-7,extent*extent*extent*1e-12))
        throw std::runtime_error("Separacion por material no conserva el volumen; se conserva la sesion.");
    if(connector) {
        if(pieces.size()!=2)throw std::runtime_error("Conectores entre regiones: este incremento admite exactamente dos regiones con contacto plano alineado con los ejes.");
        const auto& a=pieces[0];const auto& b=pieces[1];const double epsilon=extent*1e-9;
        const double amin[]={a.bounds.min.x,a.bounds.min.y,a.bounds.min.z},amax[]={a.bounds.max.x,a.bounds.max.y,a.bounds.max.z};
        const double bmin[]={b.bounds.min.x,b.bounds.min.y,b.bounds.min.z},bmax[]={b.bounds.max.x,b.bounds.max.y,b.bounds.max.z};
        int axis=-1;double sign=1,offset=0;
        for(int k=0;k<3;++k) {
            if(amax[k]<bmin[k]-epsilon||bmax[k]<amin[k]-epsilon) {
                for(auto& p:pieces)p.warnings.push_back("Regiones separadas sin contacto: no se necesitan conectores entre ellas.");
                check();return pieces;
            }
            if(std::abs(amax[k]-bmin[k])<=epsilon) {axis=k;sign=-1;offset=-amax[k];}
            else if(std::abs(amin[k]-bmax[k])<=epsilon) {axis=k;sign=1;offset=amin[k];}
        }
        if(axis<0)throw std::runtime_error("No se detecto un contacto plano entre regiones alineado con los ejes; no se generan conectores a ciegas.");
        Plane plane{{0,0,0},offset};double* normal[]={&plane.normal.x,&plane.normal.y,&plane.normal.z};*normal[axis]=sign;
        Mesh assembly;assembly.units=mesh.units;assembly.format="material-contact";
        auto coordinate=[&](const Vec3& v) {return axis==0?v.x:(axis==1?v.y:v.z);};
        for(const auto& piece:pieces)for(const auto& triangle:piece.triangles) {
            check();const bool onPlane=std::all_of(triangle.vertices.begin(),triangle.vertices.end(),[&](const Vec3& v){return std::abs(sign*coordinate(v)-offset)<=epsilon;});
            if(!onPlane)assembly.triangles.push_back(triangle);
        }
        updateBounds(assembly);
        // Removing the common planar skins is valid only if the remaining
        // assembly closes. cutByPlane checks that before creating any connector.
        auto connected=cutByPlane(assembly,plane,stop,connector);
        std::array<Mesh*,2> output{&connected.positive,&connected.negative};
        for(size_t i=0;i<2;++i) {
            auto& p=*output[i];p.format="3mf-material-connected";p.materials=pieces[i].materials;p.warnings=pieces[i].warnings;
            p.instances=pieces[i].instances;std::uint32_t instanceId=0;
            if(!p.instances.empty()) {auto& instance=p.instances.front();instance.firstTriangle=0;instance.triangleCount=static_cast<std::uint32_t>(p.triangles.size());instanceId=instance.id;}
            p.styles.resize(p.triangles.size(),TriangleStyle{instanceId,{0,0,0}});
        }
        pieces[0]=std::move(connected.positive);pieces[1]=std::move(connected.negative);
    }
    check();return pieces;
}
}
