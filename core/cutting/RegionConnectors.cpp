#include "RegionConnectors.h"
#include "core/io/MeshReader.h"
#include "core/io/StlWriter.h"
#include <manifold/manifold.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>

namespace stlcut {
namespace {
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,double b){return {a.x*b,a.y*b,a.z*b};}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3 a){return std::hypot(a.x,a.y,a.z);}
bool near(const Bounds& a,const Bounds& b,double e) {
    return a.min.x<=b.max.x+e&&b.min.x<=a.max.x+e&&a.min.y<=b.max.y+e&&b.min.y<=a.max.y+e&&a.min.z<=b.max.z+e&&b.min.z<=a.max.z+e;
}
struct Site {Vec3 position,normal;double area;size_t triangle;};
}
RegionConnectorResult connectRegions(const std::vector<Mesh>& input,const ConnectorSpec& c,std::stop_token stop) {
    auto check=[&]{if(stop.stop_requested())throw ImportCancelled{};};check();
    auto fail=[](const char* s){throw std::runtime_error(s);};
    if(input.size()<2||input.size()>32)fail("Conectores de regiones requieren entre 2 y 32 piezas.");
    if(!std::isfinite(c.diameter)||!std::isfinite(c.depth)||!std::isfinite(c.clearance)||!std::isfinite(c.wall)||c.diameter<=0||c.depth<=0||c.clearance<0||c.wall<=0)
        fail("Parametros de conectores de regiones invalidos.");
    Bounds bounds=input.front().bounds;size_t triangles=0;
    for(const auto& m:input) {
        triangles+=m.triangles.size();if(triangles>100000)fail("Conectores de regiones limitados a 100000 triangulos de entrada totales.");
        bounds.min={std::min(bounds.min.x,m.bounds.min.x),std::min(bounds.min.y,m.bounds.min.y),std::min(bounds.min.z,m.bounds.min.z)};
        bounds.max={std::max(bounds.max.x,m.bounds.max.x),std::max(bounds.max.y,m.bounds.max.y),std::max(bounds.max.z,m.bounds.max.z)};
    }
    const auto d=bounds.dimensions();const double extent=std::max({d.x,d.y,d.z}),cubic=extent*extent*extent;
    if(!std::isfinite(cubic)||cubic<=0)fail("Escala de conectores de regiones invalida.");
    if(std::max({c.diameter,c.depth,c.clearance,c.wall})>extent||std::min({c.diameter/2,c.depth,c.wall})<extent*1e-7)
        fail("Parametros de conectores fuera de escala para las regiones.");
    const auto origin=bounds.min;
    const double radius=c.diameter/(2*extent),depth=c.depth/extent,gap=c.clearance/(2*extent),wall=c.wall/extent;
    const double collisionTolerance=std::min(1e-12,radius*radius*depth*1e-8);
    const double circumscribe=1/std::cos(std::acos(-1.)/64);
    auto normalize=[&](Vec3 p){return mul(sub(p,origin),1/extent);};
    manifold::ExecutionContext context;std::stop_callback cancel(stop,[&]{context.Cancel();});
    auto valid=[&](const manifold::Manifold& m){check();if(m.Status()!=manifold::Manifold::Error::NoError)fail("Manifold rechazo la operacion de conectores de regiones.");};
    auto intersection=[&](const manifold::Manifold& a,const manifold::Manifold& b){auto result=a^b;valid(result);return result;};
    auto outside=[&](const manifold::Manifold& a,const manifold::Manifold& b){auto result=a-b;valid(result);return result;};
    auto cylinder=[&](Vec3 p,Vec3 n,double height,double r) {
        const auto axis=std::abs(n.z)<.9?Vec3{0,0,1}:Vec3{1,0,0};auto u=cross(axis,n);u=mul(u,1/length(u));const auto v=cross(n,u);
        auto solid=manifold::Manifold::Cylinder(height,r,r,64,false).Transform(manifold::mat3x4{{u.x,u.y,u.z},{v.x,v.y,v.z},{n.x,n.y,n.z},{p.x,p.y,p.z}});valid(solid);return solid;
    };
    std::vector<manifold::Manifold> originals,parts;std::vector<std::vector<Site>> sites;
    std::vector<std::vector<manifold::Manifold>> guards(input.size()),pins(input.size());
    double initialVolume=0;
    for(const auto& mesh:input) {
        check();if(mesh.triangles.empty()||mesh.materials.size()!=1||mesh.styles.size()!=mesh.triangles.size())fail("Cada region debe ser cerrada y tener un material uniforme.");
        if(!std::all_of(mesh.styles.begin(),mesh.styles.end(),[](const auto& s){return s.material==std::array<std::int32_t,3>{0,0,0};}))fail("Region con etiquetas no uniformes.");
        verifyClosedMesh(mesh,extent*1e-9,stop);
        manifold::MeshGL64 data;data.numProp=3;std::map<std::array<double,3>,std::uint64_t> vertices;std::vector<Site> candidates;
        for(size_t i=0;i<mesh.triangles.size();++i) {
            check();const auto& t=mesh.triangles[i];
            for(auto p:t.vertices) {
                p=normalize(p);const std::array<double,3> key{p.x,p.y,p.z};
                auto [it,inserted]=vertices.emplace(key,vertices.size());if(inserted)data.vertProperties.insert(data.vertProperties.end(),key.begin(),key.end());data.triVerts.push_back(it->second);
            }
            const auto a=normalize(t.vertices[0]),b=normalize(t.vertices[1]),q=normalize(t.vertices[2]);auto n=cross(sub(b,a),sub(q,a));const double area=length(n);
            if(area>1e-18)candidates.push_back({mul(add(a,add(b,q)),1./3),mul(n,1/area),area,i});
        }
        auto solid=context.FromMeshGL(data);valid(solid);const double volume=signedVolume(mesh);
        if(volume<=0||std::abs(solid.Volume()*cubic-volume)>std::max(volume*1e-7,cubic*1e-12))fail("Conversion de regiones altera su volumen.");
        initialVolume+=volume;originals.push_back(solid);parts.push_back(solid);
        std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.area>b.area;});
        // Largest faces first, then spread the remaining half across the mesh.
        std::vector<Site> selected;
        if(candidates.size()<=32)selected=candidates;
        else {selected.insert(selected.end(),candidates.begin(),candidates.begin()+16);for(size_t i=0;i<16;++i)selected.push_back(candidates[16+i*(candidates.size()-16)/16]);}
        sites.push_back(std::move(selected));
    }
    std::vector<std::pair<size_t,size_t>> pairs;
    for(size_t i=0;i<input.size();++i)for(size_t j=i+1;j<input.size();++j)if(near(input[i].bounds,input[j].bounds,extent*1e-9)) {
        check();if(pairs.size()>=64)fail("Conectores de regiones superan 64 parejas candidatas.");
        const auto overlap=intersection(parts[i],parts[j]);
        if(overlap.Volume()>collisionTolerance)fail("Las regiones se solapan antes de crear conectores.");
        pairs.emplace_back(i,j);
    }
    // Conservative sweep of the unmodified body, plus individual convex pins.
    // This checks the last depth+clearance of straight insertion, not a global
    // disassembly plan or arbitrary rotations of the whole contact graph.
    auto clearPath=[&](size_t donor,Vec3 normal,const std::vector<manifold::Manifold>& proposed,const std::vector<std::vector<manifold::Manifold>>& proposedPins) {
        const auto shift=mul(normal,-(depth+gap));const manifold::vec3 offset{shift.x,shift.y,shift.z};
        std::vector<manifold::Manifold> sweeps;
        sweeps.push_back(manifold::Manifold::Hull({originals[donor],originals[donor].Translate(offset)}));valid(sweeps.back());
        for(const auto& pin:proposedPins[donor]){sweeps.push_back(manifold::Manifold::Hull({pin,pin.Translate(offset)}));valid(sweeps.back());}
        for(size_t j=0;j<proposed.size();++j)if(j!=donor)for(const auto& sweep:sweeps) {
            check();if(intersection(sweep,proposed[j]).Volume()>collisionTolerance)return false;
        }
        return true;
    };
    RegionConnectorResult result;double delta=0;
    for(const auto [first,second]:pairs) {
        check();RegionConnection link;link.first=first;link.second=second;bool found=false;
        link.reason="Busqueda acotada sin contacto local seguro: revise area, pared, profundidad o recorrido recto.";
        for(int direction=0;direction<2&&!found;++direction) {
            const size_t donor=direction?second:first,receiver=direction?first:second;
            for(const auto& site:sites[donor]) {
                check();const auto p=site.position,n=site.normal;
                const auto anchor=cylinder(sub(p,mul(n,wall)),n,wall,(radius+wall)*circumscribe);
                const auto guard=cylinder(p,n,depth+gap+wall,(radius+gap+wall)*circumscribe);
                if(outside(anchor,parts[donor]).Volume()>anchor.Volume()*1e-8||outside(guard,parts[receiver]).Volume()>guard.Volume()*1e-8)continue;
                bool separated=true;
                for(const auto& old:guards[donor])if(intersection(anchor,old).Volume()>collisionTolerance){separated=false;break;}
                for(const auto& old:guards[receiver])if(intersection(guard,old).Volume()>collisionTolerance){separated=false;break;}
                if(!separated){link.reason="Separacion insuficiente respecto a otro conector o a su pared de guarda.";continue;}
                const auto pin=cylinder(sub(p,mul(n,wall)),n,wall+depth,radius);
                const auto socket=cylinder(sub(p,mul(n,wall)),n,wall+depth+gap,radius+gap);
                auto candidate=parts;candidate[donor]=parts[donor]+pin;candidate[receiver]=outside(parts[receiver],socket);valid(candidate[donor]);
                const double added=candidate[donor].Volume()-parts[donor].Volume(),removed=parts[receiver].Volume()-candidate[receiver].Volume();
                const double expectedPin=cylinder(p,n,depth,radius).Volume(),expectedSocket=cylinder(p,n,depth+gap,radius+gap).Volume();
                if(std::abs(added-expectedPin)>expectedPin*1e-7||std::abs(removed-expectedSocket)>expectedSocket*1e-7){link.reason="No se verificaron las dimensiones nominales del pin o de la cavidad.";continue;}
                auto candidatePins=pins;candidatePins[donor].push_back(pin);
                if(!clearPath(donor,n,candidate,candidatePins)){link.reason="Recorrido recto conservador bloqueado por otra pieza; no se certifica insercion en esa direccion.";continue;}
                bool priorPaths=true;
                for(const auto& previous:result.connections)if(previous.connected&&!clearPath(previous.donor,previous.normal,candidate,candidatePins)){priorPaths=false;break;}
                if(!priorPaths){link.reason="La colocacion bloquearia el recorrido verificado de un conector anterior.";continue;}
                parts.swap(candidate);pins.swap(candidatePins);guards[donor].push_back(anchor);guards[receiver].push_back(guard);
                link.connected=true;link.donor=donor;link.receiver=receiver;link.position=add(origin,mul(p,extent));link.normal=n;
                link.addedVolume=added*cubic;link.removedVolume=removed*cubic;delta+=link.addedVolume-link.removedVolume;
                link.reason="Pin/cavidad y tramo recto final verificados; no certifica orden global de montaje.";found=true;break;
            }
        }
        result.connections.push_back(std::move(link));
    }
    result.pieces=input;size_t totalTriangles=0;double finalVolume=0;
    for(size_t i=0;i<parts.size();++i) {
        check();auto& output=result.pieces[i];
        const bool changed=std::any_of(result.connections.begin(),result.connections.end(),[&](const auto& l){return l.connected&&(l.first==i||l.second==i);});
        if(changed) {
            auto simplified=parts[i].AsOriginal().Simplify(1e-9);valid(simplified);
            if(simplified.NumTri()>maxTriangles-totalTriangles)fail("Conectores superan 2 millones de triangulos resultantes.");
            const auto data=simplified.GetMeshGL64();output.triangles.clear();
            for(size_t t=0;t<data.triVerts.size();t+=3) {
                check();Triangle triangle;for(size_t k=0;k<3;++k){const auto v=data.triVerts[t+k]*data.numProp;triangle.vertices[k]=add(origin,mul({data.vertProperties[v],data.vertProperties[v+1],data.vertProperties[v+2]},extent));}output.triangles.push_back(triangle);
            }
            output.styles.assign(output.triangles.size(),TriangleStyle{0,{0,0,0}});output.instances.clear();output.format+="-connected";
            updateBounds(output);verifyClosedMesh(output,extent*1e-9,stop);(void)validatedStl(output,true,stop);
        }
        totalTriangles+=output.triangles.size();if(totalTriangles>maxTriangles)fail("Conectores superan 2 millones de triangulos resultantes.");
        const double volume=signedVolume(output);if(!std::isfinite(volume)||volume<=0)fail("Conector produjo una pieza sin volumen positivo.");finalVolume+=volume;
        for(const auto& l:result.connections)if(l.first==i||l.second==i)
            output.warnings.push_back("Union regiones "+std::to_string(l.first+1)+"/"+std::to_string(l.second+1)+": "+(l.connected?"conector generado. ":"sin conector. ")+l.reason);
        if(pairs.empty())output.warnings.push_back("Sin parejas candidatas en contacto; no se generaron conectores.");
    }
    if(std::abs(finalVolume-initialVolume-delta)>std::max(initialVolume*1e-7,cubic*1e-12))fail("Conectores no conservan el delta de volumen esperado.");
    // Check final intersections, including non-neighbor pieces and other pins.
    for(size_t i=0;i<parts.size();++i)for(size_t j=i+1;j<parts.size();++j) {
        check();if(intersection(parts[i],parts[j]).Volume()>collisionTolerance)fail("Se detecto colision entre las piezas conectadas.");
    }
    check();return result;
}
}
