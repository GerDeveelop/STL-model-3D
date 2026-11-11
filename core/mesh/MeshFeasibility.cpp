#include "MeshFeasibility.h"
#include "core/io/StlReader.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <unordered_map>
namespace stlcut {
namespace {
struct Point {std::array<std::uint64_t,3> bits;bool operator==(const Point&) const=default;};
struct Hash {size_t operator()(const Point& p) const {size_t h=0;for(auto x:p.bits)h^=std::hash<std::uint64_t>{}(x)+0x9e3779b9+(h<<6)+(h>>2);return h;}};
}
std::vector<InstanceFeasibility> meshFeasibility(const Mesh& mesh,std::stop_token stop) {
    auto check=[&]{if(stop.stop_requested())throw ImportCancelled{};};check();
    if(mesh.triangles.empty()||mesh.triangles.size()>maxTriangles)throw std::runtime_error("No hay malla dentro del presupuesto de diagnostico.");
    if(!mesh.styles.empty()&&mesh.styles.size()!=mesh.triangles.size())throw std::runtime_error("Etiquetas de instancia inconsistentes.");
    std::map<std::uint32_t,std::vector<std::uint32_t>> groups;
    for(std::uint32_t i=0;i<mesh.triangles.size();++i){check();const auto id=mesh.styles.empty()?0:mesh.styles[i].instance;groups[id].push_back(i);if(groups.size()>256)throw std::runtime_error("Diagnostico limitado a 256 instancias.");}
    std::vector<bool> painted(mesh.triangles.size());std::map<std::uint32_t,size_t> expanded,unresolved;std::map<std::uint32_t,std::set<std::int32_t>> labels;
    for(const auto& f:mesh.paintFacets){check();if(f.sourceTriangle>=mesh.triangles.size())throw std::runtime_error("Referencia de pintura invalida en diagnostico.");painted[f.sourceTriangle]=true;const auto id=mesh.styles.empty()?0:mesh.styles[f.sourceTriangle].instance;++expanded[id];if(f.material<0||static_cast<size_t>(f.material)>=mesh.materials.size())++unresolved[id];else labels[id].insert(f.material);}
    for(size_t i=0;i<mesh.triangles.size();++i)if(!painted[i]){const auto id=mesh.styles.empty()?0:mesh.styles[i].instance;++expanded[id];if(mesh.styles.empty())++unresolved[id];else {const auto& m=mesh.styles[i].material;if(m[0]<0||static_cast<size_t>(m[0])>=mesh.materials.size()||m[0]!=m[1]||m[0]!=m[2])++unresolved[id];else labels[id].insert(m[0]);}}
    std::vector<InstanceFeasibility> result;
    for(const auto& [id,faces]:groups) {
        check();InstanceFeasibility item;item.instance=id;item.triangles=faces.size();item.surfaceFacets=expanded[id];item.unresolvedFacets=unresolved[id];item.materials=labels[id].size();item.currentReconstructionBudget=faces.size()<=100000&&item.surfaceFacets<=100000;
        if(faces.size()>500000){item.reason="Topologia no analizada: mas de 500000 caras en una instancia.";result.push_back(item);continue;}
        const auto origin=mesh.triangles[faces.front()].vertices[0];
        std::unordered_map<Point,std::uint32_t,Hash> vertices;
        struct Edge {std::uint32_t count{},first{};std::int32_t balance{};};std::unordered_map<std::uint64_t,Edge> edges;
        std::vector<std::uint32_t> parents(faces.size());std::iota(parents.begin(),parents.end(),0U);std::vector<bool> valid(faces.size());
        auto root=[&](std::uint32_t n){while(parents[n]!=n){parents[n]=parents[parents[n]];n=parents[n];}return n;};long double volume=0;
        for(std::uint32_t local=0;local<faces.size();++local) {
            check();const auto& t=mesh.triangles[faces[local]];std::array<std::uint32_t,3> ids;
            for(size_t j=0;j<3;++j){const auto p=t.vertices[j];if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))throw std::runtime_error("Coordenada no finita en diagnostico.");const Point key{{std::bit_cast<std::uint64_t>(p.x==0?0.:p.x),std::bit_cast<std::uint64_t>(p.y==0?0.:p.y),std::bit_cast<std::uint64_t>(p.z==0?0.:p.z)}};auto [it,inserted]=vertices.emplace(key,static_cast<std::uint32_t>(vertices.size()));ids[j]=it->second;}
            const auto a=t.vertices[0],b=t.vertices[1],c=t.vertices[2];const double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
            if(ids[0]==ids[1]||ids[1]==ids[2]||ids[0]==ids[2]||std::hypot(uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx)==0){++item.degenerate;continue;}
            valid[local]=true;
            const long double ax=static_cast<long double>(a.x)-origin.x,ay=static_cast<long double>(a.y)-origin.y,az=static_cast<long double>(a.z)-origin.z;
            const long double bx=static_cast<long double>(b.x)-origin.x,by=static_cast<long double>(b.y)-origin.y,bz=static_cast<long double>(b.z)-origin.z;
            const long double cx=static_cast<long double>(c.x)-origin.x,cy=static_cast<long double>(c.y)-origin.y,cz=static_cast<long double>(c.z)-origin.z;
            volume+=ax*(by*cz-bz*cy)+ay*(bz*cx-bx*cz)+az*(bx*cy-by*cx);
            for(size_t j=0;j<3;++j){const auto u=ids[j],v=ids[(j+1)%3];const auto key=(static_cast<std::uint64_t>(std::min(u,v))<<32)|std::max(u,v);auto [it,inserted]=edges.try_emplace(key,Edge{0,local,0});auto& edge=it->second;if(!inserted)parents[root(local)]=root(edge.first);++edge.count;edge.balance+=u<v?1:-1;}
        }
        for(const auto& [key,e]:edges){check();if(e.count==1)++item.boundaryEdges;else if(e.count!=2)++item.nonmanifoldEdges;if(e.count==2&&e.balance!=0)++item.inconsistentEdges;}
        std::set<std::uint32_t> components;for(std::uint32_t i=0;i<faces.size();++i)if(valid[i])components.insert(root(i));item.components=components.size();item.topologyAnalyzed=true;
        item.volume=static_cast<double>(volume/6);item.positiveVolume=std::isfinite(item.volume)&&item.volume>0;item.closedOriented=!item.degenerate&&!item.boundaryEdges&&!item.nonmanifoldEdges&&!item.inconsistentEdges;
        if(!item.closedOriented)item.reason="Revisar topologia: caras degeneradas, bordes abiertos/no manifold u orientacion inconsistente.";
        else if(!item.positiveVolume)item.reason="Volumen no positivo; revisar orientacion y cavidades.";
        else if(item.unresolvedFacets)item.reason="Topologia preliminar compatible; colores ausentes o interpolados requieren resolver seleccion.";
        else if(!item.currentReconstructionBudget)item.reason="Topologia preliminar compatible; supera el presupuesto actual de reconstruccion de 100000 caras/facetas.";
        else item.reason="Compatible con filtros preliminares; falta validacion tolerante/Manifold y contactos antes de extraer.";
        result.push_back(std::move(item));
    }
    check();return result;
}
}
