#include "SurfaceIndex.h"
#include "core/io/StlReader.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace stlcut {
namespace {
void check(std::stop_token stop){if(stop.stop_requested())throw ImportCancelled{};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,double b){return {a.x*b,a.y*b,a.z*b};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool finite(Vec3 a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
void include(Bounds& b,Vec3 p){b.min={std::min(b.min.x,p.x),std::min(b.min.y,p.y),std::min(b.min.z,p.z)};b.max={std::max(b.max.x,p.x),std::max(b.max.y,p.y),std::max(b.max.z,p.z)};}
double coordinate(Vec3 v,int axis){return axis==0?v.x:axis==1?v.y:v.z;}
double boxDistance(Vec3 p,const Bounds& b){const double x=std::max({b.min.x-p.x,0.,p.x-b.max.x}),y=std::max({b.min.y-p.y,0.,p.y-b.max.y}),z=std::max({b.min.z-p.z,0.,p.z-b.max.z});return x*x+y*y+z*z;}
double distance2(Vec3 p,const Triangle& t) {
    const auto a=t.vertices[0],b=t.vertices[1],c=t.vertices[2];const auto ab=sub(b,a),ac=sub(c,a),ap=sub(p,a);
    const double d1=dot(ab,ap),d2=dot(ac,ap);if(d1<=0&&d2<=0)return dot(ap,ap);
    const auto bp=sub(p,b);const double d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3)return dot(bp,bp);
    const double vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0){const auto q=sub(p,add(a,mul(ab,d1/(d1-d3))));return dot(q,q);}
    const auto cp=sub(p,c);const double d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6)return dot(cp,cp);
    const double vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0){const auto q=sub(p,add(a,mul(ac,d2/(d2-d6))));return dot(q,q);}
    const double va=d3*d6-d5*d4;if(va<=0&&d4-d3>=0&&d5-d6>=0){const auto q=sub(p,add(b,mul(sub(c,b),(d4-d3)/((d4-d3)+(d5-d6)))));return dot(q,q);}
    const double sum=va+vb+vc;if(!(sum>0))throw std::runtime_error("Faceta degenerada en consulta espacial.");
    const auto q=sub(p,add(a,add(mul(ab,vb/sum),mul(ac,vc/sum))));return dot(q,q);
}
}
SurfaceData surfaceData(const Mesh& mesh,std::optional<std::uint32_t> instance,std::stop_token stop) {
    check(stop);if(mesh.triangles.empty()||mesh.triangles.size()>maxTriangles)throw std::runtime_error("Malla vacia o fuera de presupuesto espacial.");
    if(instance&&*instance!=0&&std::none_of(mesh.instances.begin(),mesh.instances.end(),[&](const auto& i){return i.id==*instance;}))throw std::runtime_error("Instancia desconocida para consulta espacial.");
    auto selected=[&](size_t i){return !instance||(mesh.styles.size()==mesh.triangles.size()?mesh.styles[i].instance:0)==*instance;};
    Bounds bounds;bool found=false;
    for(size_t i=0;i<mesh.triangles.size();++i)if(selected(i))for(const auto& p:mesh.triangles[i].vertices) {
        check(stop);if(!finite(p))throw std::runtime_error("Coordenada no finita en indice espacial.");if(!found){bounds={p,p};found=true;}else include(bounds,p);
    }
    if(!found)throw std::runtime_error("La instancia no contiene superficies.");
    const auto d=bounds.dimensions();SurfaceData result;result.origin=bounds.min;result.scale=std::max({d.x,d.y,d.z});
    if(!std::isfinite(result.scale)||result.scale<=0)throw std::runtime_error("Escala espacial no valida.");
    auto append=[&](Triangle triangle,std::int32_t material,size_t source) {
        check(stop);
        if(result.facets.size()>=1000000)throw std::runtime_error("Indice espacial limitado a un millon de facetas expandidas.");
        for(auto& p:triangle.vertices)p=mul(sub(p,result.origin),1/result.scale);
        const auto id=mesh.styles.size()==mesh.triangles.size()?mesh.styles[source].instance:0;
        result.facets.push_back({triangle,material,static_cast<std::uint32_t>(source),id});
    };
    size_t next=0,painted=0;
    for(size_t i=0;i<mesh.triangles.size();++i) {
        check(stop);const auto& source=mesh.triangles[i];
        if(next<mesh.paintFacets.size()&&mesh.paintFacets[next].sourceTriangle<i)throw std::runtime_error("Pintura fuera de orden en indice espacial.");
        if(next<mesh.paintFacets.size()&&mesh.paintFacets[next].sourceTriangle==i) {
            ++painted;
            while(next<mesh.paintFacets.size()&&mesh.paintFacets[next].sourceTriangle==i) {
                const auto& f=mesh.paintFacets[next++];if(!selected(i))continue;Triangle t;
                for(size_t k=0;k<3;++k){const auto b=f.barycentric.vertices[k];t.vertices[k]=add(mul(source.vertices[0],b.x),add(mul(source.vertices[1],b.y),mul(source.vertices[2],b.z)));}
                append(t,f.material,i);
            }
        } else if(selected(i)) {
            std::int32_t material=-1;if(mesh.styles.size()==mesh.triangles.size()){const auto& labels=mesh.styles[i].material;if(labels[0]==labels[1]&&labels[0]==labels[2])material=labels[0];}
            append(source,material,i);
        }
    }
    if(next!=mesh.paintFacets.size()||painted!=mesh.paintedTriangles)throw std::runtime_error("Metadatos de pintura inconsistentes en indice espacial.");
    check(stop);return result;
}
SurfaceIndex::SurfaceIndex(SurfaceData data,std::stop_token stop):data_(std::move(data)) {
    check(stop);if(data_.facets.empty()||data_.facets.size()>1000000||!finite(data_.origin)||!std::isfinite(data_.scale)||data_.scale<=0)throw std::runtime_error("Datos de indice espacial invalidos.");
    for(const auto& f:data_.facets) {
        check(stop);for(const auto& p:f.triangle.vertices)if(!finite(p)||std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>2)throw std::runtime_error("Faceta espacial fuera de coordenadas normalizadas.");
        const auto u=sub(f.triangle.vertices[1],f.triangle.vertices[0]),v=sub(f.triangle.vertices[2],f.triangle.vertices[0]);
        if(std::hypot(u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x)<=1e-30)throw std::runtime_error("Faceta espacial degenerada o demasiado pequena.");
    }
    order_.resize(data_.facets.size());std::iota(order_.begin(),order_.end(),0U);nodes_.reserve(order_.size()/2+1);build(0,static_cast<std::uint32_t>(order_.size()),stop);
    if(bytes()>256ULL*1024*1024)throw std::runtime_error("Indice espacial supera 256 MiB de buffers contabilizados.");
}
std::uint32_t SurfaceIndex::build(std::uint32_t begin,std::uint32_t end,std::stop_token stop) {
    check(stop);const auto id=static_cast<std::uint32_t>(nodes_.size());const auto p=data_.facets[order_[begin]].triangle.vertices[0];Node node;node.bounds={p,p};node.begin=begin;node.count=end-begin;
    for(auto i=begin;i<end;++i){if((i&1023)==0)check(stop);for(auto v:data_.facets[order_[i]].triangle.vertices)include(node.bounds,v);}
    nodes_.push_back(node);if(end-begin<=8)return id;
    const auto d=node.bounds.dimensions();const int axis=d.x>=d.y&&d.x>=d.z?0:d.y>=d.z?1:2;const auto middle=begin+(end-begin)/2;size_t comparisons=0;
    auto centroid=[&](std::uint32_t i){const auto& t=data_.facets[i].triangle;return (coordinate(t.vertices[0],axis)+coordinate(t.vertices[1],axis)+coordinate(t.vertices[2],axis))/3;};
    std::nth_element(order_.begin()+begin,order_.begin()+middle,order_.begin()+end,[&](auto a,auto b){if((++comparisons&1023)==0)check(stop);const auto x=centroid(a),y=centroid(b);return x<y||(x==y&&a<b);});
    const auto left=build(begin,middle,stop),right=build(middle,end,stop);nodes_[id].count=0;nodes_[id].left=left;nodes_[id].right=right;return id;
}
std::size_t SurfaceIndex::bytes() const{return sizeof(*this)+data_.facets.capacity()*sizeof(SurfaceFacet)+order_.capacity()*sizeof(std::uint32_t)+nodes_.capacity()*sizeof(Node);}
SurfaceHit SurfaceIndex::nearest(Vec3 point,SurfaceWork& work,std::stop_token stop,bool bruteForce) const {
    check(stop);if(!finite(point)||!work.limit||work.limit>100000000)throw std::runtime_error("Consulta o presupuesto espacial invalido.");
    const auto p=mul(sub(point,data_.origin),1/data_.scale);if(!finite(p)||std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>1e150)throw std::runtime_error("Punto fuera de rango numerico espacial.");
    auto charge=[&](bool triangle){check(stop);if(work.nodes+work.triangles>=work.limit)throw std::runtime_error("Consulta supera el presupuesto de trabajo espacial.");if(triangle)++work.triangles;else ++work.nodes;};
    double best=std::numeric_limits<double>::infinity();std::uint32_t chosen=std::numeric_limits<std::uint32_t>::max();
    auto key=[&](std::uint32_t i){const auto& f=data_.facets[i];return std::tuple{f.material,f.instance,f.sourceTriangle,i};};
    // First obtain the exact minimum, then resolve all normalized squared
    // distance ties within 1e-15. Independent of BVH traversal order.
    for(int pass=0;pass<2;++pass) {
        auto test=[&](std::uint32_t i){charge(true);const double d=distance2(p,data_.facets[i].triangle);if(pass==0){best=std::min(best,d);}else if(d<=best+1e-15&&(chosen==std::numeric_limits<std::uint32_t>::max()||key(i)<key(chosen)))chosen=i;};
        if(bruteForce){for(std::uint32_t i=0;i<data_.facets.size();++i)test(i);continue;}
        std::array<std::uint32_t,64> stack;size_t count=1;stack[0]=0;
        while(count) {
            const auto id=stack[--count];charge(false);const auto& node=nodes_[id];
            if(boxDistance(p,node.bounds)>best+1e-15+std::abs(best)*8*std::numeric_limits<double>::epsilon())continue;
            if(node.count){for(auto i=node.begin;i<node.begin+node.count;++i)test(order_[i]);}
            else {
                auto a=node.left,b=node.right;if(boxDistance(p,nodes_[a].bounds)>boxDistance(p,nodes_[b].bounds))std::swap(a,b);
                if(count+2>stack.size())throw std::runtime_error("Profundidad de indice espacial fuera de presupuesto.");stack[count++]=b;stack[count++]=a;
            }
        }
    }
    if(chosen==std::numeric_limits<std::uint32_t>::max()||!std::isfinite(best))throw std::runtime_error("Indice espacial no encontro superficie valida.");
    const auto distance=std::sqrt(best)*data_.scale;if(!std::isfinite(distance))throw std::runtime_error("Distancia espacial fuera de rango numerico.");
    const auto& f=data_.facets[chosen];return {distance,f.material,f.sourceTriangle,f.instance,chosen};
}
}
