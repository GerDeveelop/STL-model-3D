#include "PlaneCut.h"
#include "core/io/MeshReader.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <manifold/manifold.h>
#include <limits>

namespace stlcut {
namespace {
Vec3 sub(Vec3 a,Vec3 b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,double b) {return {a.x*b,a.y*b,a.z*b};}
double dot(Vec3 a,Vec3 b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b) {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
void check(std::stop_token stop) {if(stop.stop_requested())throw ImportCancelled{};}
[[noreturn]] void invalid(const char* text) {throw std::runtime_error(text);}

// Canonical points recover STL topology within the documented tolerance.
// The source Mesh is never modified; only the backend adapter uses these points.
class Vertices {
    using Key=std::array<long long,3>;
    std::map<Key,std::vector<size_t>> buckets_;
    double epsilon_;
    Vec3 origin_;
public:
    std::vector<Vec3> points;
    Vertices(double epsilon,Vec3 origin):epsilon_(epsilon),origin_(origin){}
    size_t id(Vec3 p) {
        const auto r=sub(p,origin_);
        const double coordinates[]={r.x/epsilon_,r.y/epsilon_,r.z/epsilon_};
        for(double c:coordinates)if(!std::isfinite(c)||std::abs(c)>9e18)
            invalid("Coordenadas fuera del rango de indexacion del corte.");
        Key key{static_cast<long long>(std::floor(coordinates[0])),static_cast<long long>(std::floor(coordinates[1])),static_cast<long long>(std::floor(coordinates[2]))};
        for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z) {
            const auto found=buckets_.find({key[0]+x,key[1]+y,key[2]+z});
            if(found!=buckets_.end())for(auto i:found->second) {
                const auto d=sub(points[i],p);
                if(std::hypot(d.x,d.y,d.z)<=epsilon_)return i;
            }
        }
        if(points.size()>=maxTriangles)invalid("Corte supera el presupuesto de 2 millones de vertices indexados.");
        const auto index=points.size();points.push_back(p);buckets_[key].push_back(index);return index;
    }
};
void closed(const Mesh& mesh,double epsilon,std::stop_token stop,
            std::vector<Vec3>* points=nullptr,std::vector<std::array<std::uint64_t,3>>* triangles=nullptr) {
    Vertices vertices(epsilon,mesh.bounds.min);
    struct Edge { unsigned count{}; int orientation{}; };
    std::map<std::pair<size_t,size_t>,Edge> edges;
    for(const auto& t:mesh.triangles) {
        check(stop);
        std::array<size_t,3> ids;
        for(int j=0;j<3;++j)ids[j]=vertices.id(t.vertices[j]);
        if(ids[0]==ids[1]||ids[1]==ids[2]||ids[0]==ids[2])invalid("Corte: triangulo degenerado a la tolerancia elegida.");
        if(triangles)triangles->push_back({ids[0],ids[1],ids[2]});
        const auto area=cross(sub(t.vertices[1],t.vertices[0]),sub(t.vertices[2],t.vertices[0]));
        if(std::hypot(area.x,area.y,area.z)<=epsilon*epsilon)invalid("Corte: triangulo sin area suficiente.");
        for(int j=0;j<3;++j) {
            auto a=ids[j],b=ids[(j+1)%3];auto& edge=edges[std::minmax(a,b)];
            ++edge.count;edge.orientation+=a<b ? 1 : -1;
        }
    }
    for(const auto& [key,e]:edges) {
        check(stop);
        if(e.count!=2||e.orientation!=0)
            throw std::runtime_error("Se requiere malla cerrada/manifold con orientacion coherente. Arista con "+
                std::to_string(e.count)+" caras y balance "+std::to_string(e.orientation)+". No se reparo el origen.");
    }
    if(points)*points=std::move(vertices.points);
}
}
double signedVolume(const Mesh& mesh) {
    if(mesh.triangles.empty())return 0;
    const auto origin=mesh.bounds.min;
    long double sum=0;
    for(const auto& t:mesh.triangles) {
        const auto a=sub(t.vertices[0],origin),b=sub(t.vertices[1],origin),c=sub(t.vertices[2],origin);
        sum+=static_cast<long double>(a.x)*(static_cast<long double>(b.y)*c.z-static_cast<long double>(b.z)*c.y)
            +static_cast<long double>(a.y)*(static_cast<long double>(b.z)*c.x-static_cast<long double>(b.x)*c.z)
            +static_cast<long double>(a.z)*(static_cast<long double>(b.x)*c.y-static_cast<long double>(b.y)*c.x);
    }
    return static_cast<double>(sum/6);
}
void verifyClosedMesh(const Mesh& mesh,double epsilon,std::stop_token stop) {
    if(!std::isfinite(epsilon)||epsilon<=0)invalid("Tolerancia de cierre no valida.");
    closed(mesh,epsilon,stop);
}
CutResult cutByPlane(const Mesh& mesh,Plane plane,std::stop_token stop,const ConnectorSpec* connector) {
    check(stop);
    if(mesh.triangles.empty()||mesh.triangles.size()>maxTriangles)
        invalid("No hay una malla cortable dentro del limite de 2 millones de triangulos.");
    const double length=std::hypot(plane.normal.x,plane.normal.y,plane.normal.z);
    if(!std::isfinite(length)||length<1e-15||!std::isfinite(plane.offset))
        invalid("Plano no valido: normal no nula y valores finitos requeridos.");
    plane.normal=mul(plane.normal,1/length);
    plane.offset/=length;
    const auto dimensions=mesh.bounds.dimensions();
    const double extent=std::max({dimensions.x,dimensions.y,dimensions.z});
    const double cubicExtent=extent*extent*extent;
    if(!std::isfinite(extent)||extent<=0||!std::isfinite(cubicExtent)||cubicExtent==0)
        invalid("Escala de malla fuera del rango numerico del corte.");
    const double coordinate=std::max({std::abs(mesh.bounds.min.x),std::abs(mesh.bounds.min.y),std::abs(mesh.bounds.min.z),
                                      std::abs(mesh.bounds.max.x),std::abs(mesh.bounds.max.y),std::abs(mesh.bounds.max.z)});
    const double epsilon=std::max(extent*1e-9,coordinate*2e-15);
    if(epsilon>extent*1e-5)invalid("Coordenadas demasiado alejadas del origen para un corte fiable.");
    const Vec3 center{std::midpoint(mesh.bounds.min.x,mesh.bounds.max.x),
                      std::midpoint(mesh.bounds.min.y,mesh.bounds.max.y),
                      std::midpoint(mesh.bounds.min.z,mesh.bounds.max.z)};
    const double normalizedOffset=(plane.offset-dot(plane.normal,center))/extent;
    if(!std::isfinite(normalizedOffset))invalid("Posicion del plano fuera del rango numerico.");
    CutResult result;
    result.tolerance=epsilon;
    result.originalVolume=signedVolume(mesh);
    if(!std::isfinite(result.originalVolume)||result.originalVolume<=cubicExtent*1e-15)
        invalid("Volumen nulo/negativo o fuera de rango; revisar orientacion y geometria antes de cortar.");

    // Counts constrain adapter buffers, not all internal allocations of Manifold.
    constexpr std::uint64_t maxAdapterBytes=512ULL*1024*1024;
    // While normalizing, canonical indices/points and MeshGL copies coexist.
    std::uint64_t adapterBytes=mesh.triangles.size()*(sizeof(Triangle)+6*sizeof(std::uint64_t));
    auto account=[&](std::uint64_t bytes) {
        if(bytes>maxAdapterBytes-adapterBytes)invalid("Corte supera el presupuesto de 512 MiB de buffers geometricos del adaptador.");
        adapterBytes+=bytes;
    };
    std::vector<Vec3> points;
    std::vector<std::array<std::uint64_t,3>> indices;
    indices.reserve(mesh.triangles.size());
    closed(mesh,epsilon,stop,&points,&indices);
    account(points.size()*6*sizeof(double));
    manifold::MeshGL64 input;
    input.numProp=3;
    input.vertProperties.reserve(points.size()*3);
    input.triVerts.reserve(indices.size()*3);
    for(const auto& p:points) {
        check(stop);
        const auto q=mul(sub(p,center),1/extent);
        input.vertProperties.insert(input.vertProperties.end(),{q.x,q.y,q.z});
    }
    for(const auto& triangle:indices) {
        check(stop);
        input.triVerts.insert(input.triVerts.end(),triangle.begin(),triangle.end());
    }
    // Release topology construction buffers before invoking the engine.
    std::vector<Vec3>().swap(points);
    std::vector<std::array<std::uint64_t,3>>().swap(indices);
    manifold::ExecutionContext context;
    std::stop_callback onStop(stop,[&context] {context.Cancel();});
    auto requireValid=[&](manifold::Manifold::Error error) {
        check(stop);
        if(error==manifold::Manifold::Error::Cancelled)throw ImportCancelled{};
        if(error!=manifold::Manifold::Error::NoError)
            throw std::runtime_error("Manifold rechazo la geometria (codigo "+std::to_string(static_cast<int>(error))+"). Se requiere una malla manifold valida; no se reparo el original.");
    };
    auto solid=context.FromMeshGL(input);
    requireValid(solid.Status());
    const double normalizedVolume=solid.Volume();
    if(!std::isfinite(normalizedVolume)||std::abs(normalizedVolume*cubicExtent-result.originalVolume)>
       std::max(result.originalVolume*1e-7,cubicExtent*1e-12))
        invalid("La conversion a Manifold altera el volumen fuera de tolerancia; corte descartado.");
    input=manifold::MeshGL64{};
    check(stop);
    // SplitByPlane is eager in 3.5.4. A cancellation received inside its Boolean
    // operation is honored at return; never terminate a worker thread unsafely.
    auto halves=solid.SplitByPlane({plane.normal.x,plane.normal.y,plane.normal.z},normalizedOffset);
    check(stop);
    requireValid(halves.first.Status());requireValid(halves.second.Status());
    // Drop provenance boundaries before simplifying coplanar output. Otherwise
    // zero-thickness remnants from coincident faces can survive as separate runs
    // and become non-manifold when STL loses indexed topology.
    halves.first=halves.first.AsOriginal().Simplify(epsilon/extent);
    check(stop);
    halves.second=halves.second.AsOriginal().Simplify(epsilon/extent);
    requireValid(halves.first.Status());requireValid(halves.second.Status());
    if(halves.first.IsEmpty()||halves.second.IsEmpty())
        invalid("El plano no produce dos piezas con volumen. Es exterior o solo tangente; el modelo se conserva.");

    double connectorDelta=0;
    if(connector) {
        const auto& c=*connector;
        if(!std::isfinite(c.diameter)||!std::isfinite(c.depth)||!std::isfinite(c.clearance)||!std::isfinite(c.wall)||
           c.diameter<=0||c.depth<=0||c.clearance<0||c.wall<=0||
           c.diameter>extent||c.depth>extent||c.clearance>extent||c.wall>extent)
            invalid("Parametros del conector invalidos para las dimensiones de la pieza.");
        const double radius=c.diameter/(2*extent),depth=c.depth/extent;
        const double gap=c.clearance/(2*extent),wall=c.wall/extent;
        if(std::min({radius,depth,wall})<epsilon/extent*100)
            invalid("Conector demasiado pequeno para la tolerancia del modelo.");
        const auto n=plane.normal;
        const Vec3 axis=std::abs(n.z)<.9 ? Vec3{0,0,1}:Vec3{1,0,0};
        auto u=cross(axis,n);u=mul(u,1/std::hypot(u.x,u.y,u.z));
        const auto v=cross(n,u);
        const auto base=mul(n,normalizedOffset);
        auto cylinder=[&](Vec3 origin,double height,double r) {
            return manifold::Manifold::Cylinder(height,r,r,64,false).Transform(
                manifold::mat3x4{{u.x,u.y,u.z},{v.x,v.y,v.z},{n.x,n.y,n.z},{origin.x,origin.y,origin.z}});
        };
        bool found=false;
        // Search a bounded set on the actual section, then verify containment by
        // Boolean difference. A box centre alone does not prove a safe contact.
        for(double a:{0.,-.15,.15,-.3,.3}) {
            for(double b:{0.,-.15,.15,-.3,.3}) {
                check(stop);
                const Vec3 p{base.x+a*u.x+b*v.x,base.y+a*u.y+b*v.y,base.z+a*u.z+b*v.z};
                const auto negativeStart=Vec3{p.x-n.x*(depth+gap+wall),p.y-n.y*(depth+gap+wall),p.z-n.z*(depth+gap+wall)};
                const double circumscribe=1/std::cos(std::acos(-1.)/64);
                const auto receiverGuard=cylinder(negativeStart,depth+gap+wall,(radius+gap+wall)*circumscribe);
                const auto anchorGuard=cylinder(p,wall,(radius+wall)*circumscribe);
                const auto receiverOutside=receiverGuard-halves.second;
                const auto anchorOutside=anchorGuard-halves.first;
                requireValid(receiverOutside.Status());requireValid(anchorOutside.Status());
                if(receiverOutside.Volume()>receiverGuard.Volume()*1e-8||anchorOutside.Volume()>anchorGuard.Volume()*1e-8)continue;
                const Vec3 pinStart{p.x-n.x*depth,p.y-n.y*depth,p.z-n.z*depth};
                const auto pin=cylinder(pinStart,depth+wall,radius);
                const auto socket=cylinder({p.x-n.x*(depth+gap),p.y-n.y*(depth+gap),p.z-n.z*(depth+gap)},depth+gap+wall,radius+gap);
                auto positive=halves.first+pin;
                auto negative=halves.second-socket;
                requireValid(positive.Status());requireValid(negative.Status());
                check(stop);
                if(positive.IsEmpty()||negative.IsEmpty())continue;
                const auto pinExtension=cylinder(pinStart,depth,radius);
                const auto removedSocket=cylinder({p.x-n.x*(depth+gap),p.y-n.y*(depth+gap),p.z-n.z*(depth+gap)},depth+gap,radius+gap);
                const double added=positive.Volume()-halves.first.Volume();
                const double removed=halves.second.Volume()-negative.Volume();
                if(std::abs(added-pinExtension.Volume())>pinExtension.Volume()*1e-7||
                   std::abs(removed-removedSocket.Volume())>removedSocket.Volume()*1e-7)
                    invalid("El conector no conserva sus dimensiones nominales; resultado descartado.");
                connectorDelta=(pinExtension.Volume()-removedSocket.Volume())*cubicExtent;
                halves={std::move(positive),std::move(negative)};found=true;break;
            }
            if(found)break;
        }
        if(!found)invalid("No se encontro un contacto con profundidad y pared suficientes para el conector; reduzca parametros o cambie el plano.");
    }

    auto convert=[&](const manifold::Manifold& half) {
        check(stop);
        const auto triangles=half.NumTri(),vertices=half.NumVert();
        if(triangles>maxTriangles||vertices>maxTriangles)
            invalid("Resultado del corte supera 2 millones de triangulos o vertices por pieza.");
        account(triangles*(sizeof(Triangle)+3*sizeof(std::uint64_t))+vertices*3*sizeof(double));
        const auto data=half.GetMeshGL64();
        check(stop);
        Mesh output;output.format="cut-manifold";output.units=mesh.units;output.warnings=mesh.warnings;
        output.unsupportedAppearance=mesh.unsupportedAppearance;
        if(!mesh.paintFacets.empty())output.warnings.push_back("Corte geometrico: la pintura superficial no se reproyecta a los resultados. El original con pintura permanece en el historial.");
        if(data.numProp<3||data.triVerts.size()%3)invalid("Manifold devolvio buffers de geometria inconsistentes.");
        output.triangles.reserve(data.triVerts.size()/3);
        for(size_t i=0;i<data.triVerts.size();i+=3) {
            check(stop);Triangle triangle;
            for(size_t j=0;j<3;++j) {
                const auto index=data.triVerts[i+j];
                if(index>=data.vertProperties.size()/data.numProp)invalid("Manifold devolvio un indice fuera de rango.");
                const auto p=index*data.numProp;
                triangle.vertices[j]={center.x+data.vertProperties[p]*extent,
                                      center.y+data.vertProperties[p+1]*extent,
                                      center.z+data.vertProperties[p+2]*extent};
            }
            output.triangles.push_back(triangle);
        }
        updateBounds(output);
        if(!mesh.styles.empty()) {
            const auto label=mesh.styles.front().material[0];
            const bool uniform=label>=0&&static_cast<size_t>(label)<mesh.materials.size()&&
                std::all_of(mesh.styles.begin(),mesh.styles.end(),[&](const TriangleStyle& s) {
                    return s.material[0]==label&&s.material[1]==label&&s.material[2]==label;
                });
            if(uniform) {
                output.materials.push_back(mesh.materials[label]);
                std::uint32_t instanceId=0;
                if(mesh.instances.size()==1) {
                    output.instances=mesh.instances;auto& instance=output.instances.front();
                    instance.firstTriangle=0;instance.triangleCount=static_cast<std::uint32_t>(output.triangles.size());instanceId=instance.id;
                }
                output.styles.resize(output.triangles.size(),TriangleStyle{instanceId,{0,0,0}});
            } else {
                output.unsupportedAppearance=true;
                output.warnings.push_back("Corte geometrico sobre varios colores: las etiquetas originales no se proyectan a las caras nuevas. Separe por material antes de cortar para conservarlas.");
            }
        }
        closed(output,epsilon,stop);
        return output;
    };
    result.positive=convert(halves.first);
    result.negative=convert(halves.second);
    result.positiveVolume=signedVolume(result.positive);
    result.negativeVolume=signedVolume(result.negative);
    if(!std::isfinite(result.positiveVolume)||!std::isfinite(result.negativeVolume)||
       result.positiveVolume<=0||result.negativeVolume<=0||
       std::abs(result.positiveVolume+result.negativeVolume-result.originalVolume-connectorDelta)>
       std::max(result.originalVolume*1e-7,cubicExtent*1e-12))
        invalid("El corte no conserva el volumen dentro de tolerancia; resultado descartado.");
    check(stop);return result;
}
}
