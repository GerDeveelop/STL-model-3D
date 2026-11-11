#include "PaintVolume.h"
#include "PlaneCut.h"
#include "core/io/MeshReader.h"
#include "core/io/StlWriter.h"
#include "core/mesh/SurfaceIndex.h"
#include <manifold/manifold.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace stlcut {
namespace {
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,double b){return {a.x*b,a.y*b,a.z*b};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
}
std::vector<Mesh> splitPaintVolume(const Mesh& mesh,const PaintVolumeSpec& spec,std::stop_token stop,const ConnectorSpec* connector,std::vector<RegionConnection>* connections) {
    auto check=[&]{if(stop.stop_requested())throw ImportCancelled{};};check();
    auto fail=[](const char* text){throw std::runtime_error(text);};
    if(spec.mode!=PaintVolumeMode::Inserts&&spec.mode!=PaintVolumeMode::Partition)fail("Modo volumetrico invalido.");
    if(!std::isfinite(spec.resolution)||spec.resolution<=0||!std::isfinite(spec.depth)||spec.depth<=0)
        fail("Resolucion y profundidad deben ser finitas y positivas.");
    if(mesh.triangles.empty()||mesh.triangles.size()>100000||mesh.paintFacets.size()>100000)
        fail("Reconstruccion limitada a 100000 caras fuente y 100000 regiones de pintura.");
    const auto dimensions=mesh.bounds.dimensions();const double extent=std::max({dimensions.x,dimensions.y,dimensions.z});
    if(!std::isfinite(extent)||extent<=0||!std::isfinite(extent*extent*extent))fail("Escala volumetrica invalida.");
    const auto origin=mesh.bounds.min;
    const double coordinate=std::max({std::abs(origin.x),std::abs(origin.y),std::abs(origin.z),std::abs(mesh.bounds.max.x),std::abs(mesh.bounds.max.y),std::abs(mesh.bounds.max.z)});
    if(coordinate*2e-15>extent*1e-5)fail("Coordenadas demasiado alejadas del origen.");
    verifyClosedMesh(mesh,extent*1e-9,stop);
    const double volume=signedVolume(mesh);
    if(!std::isfinite(volume)||volume<=extent*extent*extent*1e-15)fail("Se requiere volumen positivo cerrado.");
    if(mesh.styles.size()!=mesh.triangles.size()||mesh.materials.empty()||mesh.unresolvedPaintFacets)
        fail("Faltan colores resueltos para reconstruir el volumen.");
    if(mesh.unsupportedAppearance&&mesh.paintFacets.empty())fail("Apariencia no interpretada; no se inventa su color.");
    if(spec.resolution<extent/64)fail("Resolucion demasiado fina: maximo 64 celdas por eje.");
    std::array<int,3> n;const double dims[]={dimensions.x,dimensions.y,dimensions.z};
    for(int k=0;k<3;++k)n[k]=std::max(1,static_cast<int>(std::ceil(dims[k]/spec.resolution)));
    const std::size_t cells=static_cast<size_t>(n[0])*n[1]*n[2];
    auto expandedData=surfaceData(mesh,{},stop);std::set<int> labels;
    auto normalize=[&](Vec3 p){return mul(sub(p,origin),1/extent);};
    for(const auto& facet:expandedData.facets) {
        check();const auto& t=facet.triangle;const auto material=facet.material;
        if(material<0||static_cast<size_t>(material)>=mesh.materials.size()) {
            const auto& m=mesh.styles[facet.sourceTriangle].material;
            if(m[0]!=m[1]||m[0]!=m[2])fail("Colores interpolados por vertice no admitidos para reconstruccion.");
            fail("Color superficial ausente o invalido.");
        }
        const auto a=sub(t.vertices[1],t.vertices[0]),b=sub(t.vertices[2],t.vertices[0]);
        const double area=dot(a,a)*dot(b,b)-dot(a,b)*dot(a,b);
        if(area<=1e-24)fail("Region pintada demasiado pequena o degenerada.");
        labels.insert(material);
    }
    if(expandedData.facets.empty()||expandedData.facets.size()>100000||labels.size()>64)fail("Reconstruccion limitada a 100000 superficies y 64 materiales.");
    if(cells>262144)fail("Reconstruccion supera 262144 celdas.");
    SurfaceIndex index(std::move(expandedData),stop);SurfaceWork work;
    const double h[]={dims[0]/extent/n[0],dims[1]/extent/n[1],dims[2]/extent/n[2]};
    const double error=std::hypot(h[0],h[1],h[2])*extent/2;
    if(spec.mode==PaintVolumeMode::Inserts&&spec.depth<=error)fail("Profundidad menor o igual al error espacial de la celda; refine la resolucion.");
    // Every box is assigned, including centers outside the solid. Intersecting
    // later preserves thin exterior features rather than dropping those cells.
    std::vector<int> grid(cells);std::map<int,std::vector<manifold::Manifold>> boxes;
    for(int z=0;z<n[2];++z)for(int y=0;y<n[1];++y)for(int x=0;x<n[0];++x) {
        check();const Vec3 p{(x+.5)*h[0],(y+.5)*h[1],(z+.5)*h[2]};
        const auto hit=index.nearest(add(origin,mul(p,extent)),work,stop);int material=hit.material;
        if(spec.mode==PaintVolumeMode::Inserts&&hit.distance>spec.depth)material=-1;
        grid[(static_cast<size_t>(z)*n[1]+y)*n[0]+x]=material;
    }
    std::size_t runs=0;
    for(int z=0;z<n[2];++z)for(int y=0;y<n[1];++y)for(int x=0;x<n[0];) {
        check();const auto start=x;const int label=grid[(static_cast<size_t>(z)*n[1]+y)*n[0]+x++];
        while(x<n[0]&&grid[(static_cast<size_t>(z)*n[1]+y)*n[0]+x]==label)++x;
        if(++runs>8192)fail("Reconstruccion supera 8192 bloques; aumente el tamano de celda.");
        // Use shared endpoint arithmetic so adjacent boxes meet exactly.
        const double left=start*h[0],right=x*h[0],bottom=y*h[1],top=(y+1)*h[1],back=z*h[2],front=(z+1)*h[2];
        boxes[label].push_back(manifold::Manifold::Cube({right-left,top-bottom,front-back}).Translate({left,bottom,back}));
    }
    manifold::MeshGL64 input;input.numProp=3;std::map<std::array<double,3>,std::uint64_t> vertices;
    for(const auto& t:mesh.triangles)for(const auto& p:t.vertices) {
        check();const auto q=normalize(p);const std::array<double,3> key{q.x,q.y,q.z};
        auto [it,inserted]=vertices.emplace(key,vertices.size());
        if(inserted)input.vertProperties.insert(input.vertProperties.end(),key.begin(),key.end());
        input.triVerts.push_back(it->second);
    }
    manifold::ExecutionContext context;std::stop_callback cancel(stop,[&]{context.Cancel();});
    auto valid=[&](const manifold::Manifold& m){check();if(m.Status()!=manifold::Manifold::Error::NoError)fail("Manifold rechazo la reconstruccion volumetrica; sesion conservada.");};
    auto solid=context.FromMeshGL(input);valid(solid);
    const double tolerance=std::max(volume*1e-7,extent*extent*extent*1e-12);
    if(std::abs(solid.Volume()*extent*extent*extent-volume)>tolerance)fail("Conversion volumetrica altera el volumen original.");
    std::vector<Mesh> pieces;double total=0;std::set<int> survived;size_t totalTriangles=0;
    for(auto& [label,blocks]:boxes) {
        check();auto mask=manifold::Manifold::BatchBoolean(blocks,manifold::OpType::Add);valid(mask);
        auto region=(solid^mask).AsOriginal().Simplify(1e-9);valid(region);
        if(region.IsEmpty())continue;
        // Decompose separates surface shells, including negatively oriented
        // cavity shells. Clip the full region to each positive enclosure so a
        // cavity stays with its owner. Smallest enclosures claim nested islands
        // first; subtract them before processing the surrounding enclosure.
        auto shells=region.Decompose();
        std::erase_if(shells,[&](const auto& shell){valid(shell);return shell.Volume()<=0;});
        std::sort(shells.begin(),shells.end(),[](const auto& a,const auto& b){return a.Volume()<b.Volume();});
        auto remaining=region;
        for(const auto& shell:shells) {
            auto component=(remaining^shell).AsOriginal().Simplify(1e-9);valid(component);
            if(component.IsEmpty())continue;
            remaining=remaining-component;valid(remaining);
            check();valid(component);if(pieces.size()>=256)fail("Reconstruccion supera 256 piezas.");
            if(component.NumTri()>maxTriangles-totalTriangles)fail("Reconstruccion supera 2 millones de triangulos totales.");
            totalTriangles+=component.NumTri();const auto data=component.GetMeshGL64();Mesh output;
            output.units=mesh.units;output.format=spec.mode==PaintVolumeMode::Inserts?"paint-insert-grid":"paint-partition-grid";
            output.warnings=mesh.warnings;
            output.warnings.push_back("Interfaces internas aproximadas por celdas; error espacial maximo del muestreo: "+std::to_string(error)+" unidades. Exterior recortado contra el original."+(connector?std::string{}:" No se generaron conectores."));
            if(label<0)output.materials.push_back({0,0,{160,160,160,255},"Nucleo sin color asignado (no es un filamento inferido)"});
            else output.materials.push_back(mesh.materials[label]);
            for(size_t i=0;i<data.triVerts.size();i+=3) {
                check();Triangle t;for(int j=0;j<3;++j){const auto k=data.triVerts[i+j]*data.numProp;t.vertices[j]=add(origin,mul({data.vertProperties[k],data.vertProperties[k+1],data.vertProperties[k+2]},extent));}
                output.triangles.push_back(t);
            }
            updateBounds(output);verifyClosedMesh(output,extent*1e-9,stop);
            const double v=signedVolume(output);if(!std::isfinite(v)||v<=0)fail("Pieza reconstruida sin volumen positivo.");
            // Reject float32 topology damage before any history transaction.
            (void)validatedStl(output,true,stop);total+=v;
            output.styles.resize(output.triangles.size(),TriangleStyle{0,{0,0,0}});
            survived.insert(label);pieces.push_back(std::move(output));
        }
        if(!remaining.IsEmpty()&&std::abs(remaining.Volume()*extent*extent*extent)>tolerance)
            fail("No se pudieron asociar las cavidades a sus piezas exteriores.");
    }
    for(int label:labels)if(!survived.contains(label))fail("La resolucion elimina un color; refine las celdas. No se publico ninguna pieza.");
    if(spec.mode==PaintVolumeMode::Inserts&&!survived.contains(-1))fail("La profundidad no deja un nucleo; reduzca profundidad o use particion completa.");
    if(pieces.size()<2)fail("No se obtuvieron dos piezas; se conserva el original.");
    if(std::abs(total-volume)>tolerance)fail("Reconstruccion no conserva el volumen total.");
    if(connector) {
        auto connected=connectRegions(pieces,*connector,stop);check();
        pieces=std::move(connected.pieces);if(connections)*connections=std::move(connected.connections);
    } else if(connections)connections->clear();
    check();return pieces;
}
}
