#include "MeshReader.h"
#include <lib3mf_implicit.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>

namespace stlcut {
namespace {
void check(std::stop_token stop) { if (stop.stop_requested()) throw ImportCancelled{}; }
// Inspect the central directory before the SDK decompresses anything. No files are extracted.
void checkZip(std::span<const unsigned char> b) {
    auto u16=[&](size_t p)->unsigned { if(p+2>b.size()) throw std::runtime_error("3MF ZIP truncado."); return b[p]|(unsigned(b[p+1])<<8); };
    auto u32=[&](size_t p)->std::uint64_t { return u16(p)|(std::uint64_t(u16(p+2))<<16); };
    if(b.size()<22) throw std::runtime_error("3MF ZIP incompleto.");
    size_t e=b.size()-22;
    const auto first=b.size()>65557 ? b.size()-65557 : 0;
    while(u32(e)!=0x06054b50 || e+22+u16(e+20)!=b.size()) {
        if(e==first) throw std::runtime_error("3MF ZIP sin directorio valido.");
        --e;
    }
    const auto count=u16(e+10);
    if(u16(e+4)||u16(e+6)||u16(e+8)!=count||count==65535||count>4096)
        throw std::runtime_error("3MF: ZIP64/multivolumen o demasiadas entradas no admitidos.");
    size_t p=static_cast<size_t>(u32(e+16));
    const auto end=p+u32(e+12);
    if(end!=e) throw std::runtime_error("Directorio ZIP 3MF invalido.");
    std::uint64_t total=0;
    for(unsigned i=0;i<count;++i) {
        if(p+46>end||u32(p)!=0x02014b50) throw std::runtime_error("Entrada ZIP 3MF invalida.");
        total+=u32(p+24);
        if(total>maxFileBytes||u32(p+20)==0xffffffff||u32(p+42)==0xffffffff||u16(p+34)||
           (u16(p+8)&1)||(u16(p+10)!=0&&u16(p+10)!=8))
            throw std::runtime_error("3MF: limite descomprimido 256 MiB o ZIP no admitido.");
        p+=46+u16(p+28)+u16(p+30)+u16(p+32);
    }
    if(p!=end) throw std::runtime_error("Longitud de directorio ZIP invalida.");
}
using Matrix=std::array<std::array<double,4>,4>;
Matrix identity() { Matrix m{}; for(int i=0;i<4;++i)m[i][i]=1; return m; }
Matrix matrix(const Lib3MF::sTransform& t) {
    auto m=identity();
    for(int i=0;i<4;++i)for(int j=0;j<3;++j) {
        m[i][j]=t.m_Fields[i][j];
        if(!std::isfinite(m[i][j])) throw std::runtime_error("Transformacion 3MF no finita.");
    }
    return m;
}
Matrix compose(const Matrix& a,const Matrix& b) {
    Matrix c{};
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)c[i][j]+=a[i][k]*b[k][j];
    return c;
}
Mesh read3mf(std::span<const unsigned char> data,std::stop_token stop) {
    checkZip(data); check(stop);
    auto wrapper=Lib3MF::CWrapper::loadLibrary();
    Lib3MF_uint32 major{},minor{},patch{}; wrapper->GetLibraryVersion(major,minor,patch);
    if(major!=2||minor!=5||patch!=0) throw std::runtime_error("Se requiere lib3mf 2.5.0.");
    auto model=wrapper->CreateModel(); auto reader=model->QueryReader("3mf");
    reader->SetStrictModeActive(false);
    reader->SetProgressCallback([](bool* abort,double,Lib3MF::eProgressIdentifier,void* user) {
        *abort=static_cast<std::stop_token*>(user)->stop_requested();
    },&stop);
    try { reader->ReadFromBuffer(Lib3MF::CInputVector<Lib3MF_uint8>(data.data(),data.size())); }
    catch(...) { check(stop); throw; }
    check(stop);
    Mesh result; result.format="3mf"; result.units="millimeter";
    double scale=1;
    switch(model->GetUnit()) {
    case Lib3MF::eModelUnit::MicroMeter: scale=.001; break;
    case Lib3MF::eModelUnit::MilliMeter: break;
    case Lib3MF::eModelUnit::CentiMeter: scale=10; break;
    case Lib3MF::eModelUnit::Inch: scale=25.4; break;
    case Lib3MF::eModelUnit::Foot: scale=304.8; break;
    case Lib3MF::eModelUnit::Meter: scale=1000; break;
    default: throw std::runtime_error("Unidad 3MF no admitida.");
    }
    result.warnings.push_back("3MF: geometria en mm; colores/materiales estandar conservados para la sesion. Pintura del laminador, texturas y ajustes no se interpretan. STL exportado no conserva color. Instancias sin union booleana.");
    // lib3mf v2.5.0 Include/Common/NMR_ErrorConst.h: tolerate vendor metadata/
    // attributes, but never silently accept geometry warnings or required extensions.
    std::set<unsigned> reportedWarnings;
    for(unsigned i=0;i<reader->GetWarningCount();++i) {
        check(stop);
        Lib3MF_uint32 code{}; const auto warning=reader->GetWarning(i,code);
        if(code!=0x80B2 && code!=0x80A7)
            throw std::runtime_error("3MF no admitido: "+warning);
        if(code==0x80A7)result.unsupportedAppearance=true;
        if(reportedWarnings.insert(code).second)
            result.warnings.push_back(std::to_string(code)+": "+warning);
    }
    std::set<unsigned> active;
    size_t instances=0;
    std::uint32_t buildObject=0;
    std::vector<std::uint32_t> objectHierarchy;
    std::map<std::pair<unsigned,unsigned>,std::int32_t> palette;
    auto material=[&](unsigned resource,unsigned property)->std::int32_t {
        if(!resource)return -1;
        const auto key=std::make_pair(resource,property);
        if(const auto found=palette.find(key);found!=palette.end())return found->second;
        const auto type=model->GetPropertyTypeByID(resource);
        Lib3MF::sColor color{};std::string name;
        if(type==Lib3MF::ePropertyType::BaseMaterial) {
            const auto group=model->GetBaseMaterialGroupByID(resource);
            color=group->GetDisplayColor(property);name=group->GetName(property);
        } else if(type==Lib3MF::ePropertyType::Colors)color=model->GetColorGroupByID(resource)->GetColor(property);
        else {result.unsupportedAppearance=true;return -1;}
        if(name.size()>4096)throw std::runtime_error("Nombre de material 3MF supera 4096 bytes.");
        if(result.materials.size()>=4096)throw std::runtime_error("3MF supera 4096 propiedades de color/material usadas.");
        const auto index=static_cast<std::int32_t>(result.materials.size());
        result.materials.push_back({resource,property,{color.m_Red,color.m_Green,color.m_Blue,color.m_Alpha},name});
        palette.emplace(key,index);return index;
    };
    std::function<void(Lib3MF::PObject,const Matrix&,unsigned)> visit;
    visit=[&](Lib3MF::PObject object,const Matrix& t,unsigned depth) {
        check(stop);
        if(depth>64||++instances>100000||!active.insert(object->GetUniqueResourceID()).second)
            throw std::runtime_error("3MF: ciclo o limite de componentes excedido.");
        const auto id=object->GetUniqueResourceID();
        objectHierarchy.push_back(object->GetModelResourceID());
        if(object->IsMeshObject()) {
            const auto mesh=model->GetMeshObjectByID(id);
            const auto count=mesh->GetTriangleCount();
            if(count>maxTriangles-result.triangles.size()||mesh->GetVertexCount()>6000000)
                throw std::runtime_error("3MF supera 2 millones de triangulos o 6 millones de vertices.");
            if(mesh->BeamLattice()->GetBeamCount()) throw std::runtime_error("3MF con beam lattice no admitido.");
            const auto instance=static_cast<std::uint32_t>(instances);
            auto objectName=mesh->GetName();
            if(objectName.size()>4096)throw std::runtime_error("Nombre de objeto 3MF supera 4096 bytes.");
            result.instances.push_back({instance,id,static_cast<std::uint32_t>(result.triangles.size()),count,std::move(objectName),
                mesh->PackagePart()->GetPath(),mesh->GetModelResourceID(),buildObject,false,objectHierarchy});
            Lib3MF_uint32 defaultResource{},defaultProperty{};
            const bool hasDefault=mesh->GetObjectLevelProperty(defaultResource,defaultProperty);
            const double det=t[0][0]*(t[1][1]*t[2][2]-t[1][2]*t[2][1])-t[0][1]*(t[1][0]*t[2][2]-t[1][2]*t[2][0])+t[0][2]*(t[1][0]*t[2][1]-t[1][1]*t[2][0]);
            if(!std::isfinite(det)||std::abs(det)<1e-18) throw std::runtime_error("Transformacion 3MF singular.");
            result.instances.back().reflected=det<0;
            for(unsigned i=0;i<count;++i) {
                check(stop); Triangle tri; const auto indices=mesh->GetTriangle(i);
                for(int j=0;j<3;++j) {
                    if(indices.m_Indices[j]>=mesh->GetVertexCount()) throw std::runtime_error("Indice 3MF invalido.");
                    const auto v=mesh->GetVertex(indices.m_Indices[j]);
                    double p[3]{};
                    for(int k=0;k<3;++k) p[k]=scale*(v.m_Coordinates[0]*t[0][k]+v.m_Coordinates[1]*t[1][k]+v.m_Coordinates[2]*t[2][k]+t[3][k]);
                    tri.vertices[j]={p[0],p[1],p[2]};
                }
                Lib3MF::sTriangleProperties properties{};mesh->GetTriangleProperties(i,properties);
                TriangleStyle style;style.instance=instance;
                if(!properties.m_ResourceID&&hasDefault) {
                    properties.m_ResourceID=defaultResource;
                    for(auto& p:properties.m_PropertyIDs)p=defaultProperty;
                }
                for(int j=0;j<3;++j)style.material[j]=material(properties.m_ResourceID,properties.m_PropertyIDs[j]);
                if(det<0) {std::swap(tri.vertices[1],tri.vertices[2]);std::swap(style.material[1],style.material[2]);}
                result.triangles.push_back(tri);
                result.styles.push_back(style);
            }
        } else if(object->IsComponentsObject()) {
            auto components=model->GetComponentsObjectByID(id);
            for(unsigned i=0;i<components->GetComponentCount();++i) {
                auto c=components->GetComponent(i);
                visit(c->GetObjectResource(),compose(matrix(c->GetTransform()),t),depth+1);
            }
        } else throw std::runtime_error("Tipo de objeto 3MF no admitido (solo mallas/componentes).");
        active.erase(id);
        objectHierarchy.pop_back();
    };
    auto items=model->GetBuildItems();
    while(items->MoveNext()) { auto item=items->GetCurrent();buildObject=item->GetObjectResource()->GetModelResourceID();visit(item->GetObjectResource(),matrix(item->GetObjectTransform()),0); }
    updateBounds(result); return result;
}
}
void updateBounds(Mesh& mesh) {
    if(mesh.triangles.empty()) throw std::runtime_error("La malla no contiene triangulos.");
    mesh.bounds={mesh.triangles[0].vertices[0],mesh.triangles[0].vertices[0]};
    mesh.degenerateTriangles=0;
    for(const auto& triangle:mesh.triangles) {
        for(const auto& v:triangle.vertices) {
            if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))throw std::runtime_error("Coordenada no finita.");
            auto& b=mesh.bounds;
            b.min={std::min(b.min.x,v.x),std::min(b.min.y,v.y),std::min(b.min.z,v.z)};
            b.max={std::max(b.max.x,v.x),std::max(b.max.y,v.y),std::max(b.max.z,v.z)};
        }
        const auto a=triangle.vertices[0],b=triangle.vertices[1],c=triangle.vertices[2];
        const Vec3 u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
        if(std::hypot(u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x)==0)++mesh.degenerateTriangles;
    }
    const auto d=mesh.bounds.dimensions();
    if(!std::isfinite(d.x)||!std::isfinite(d.y)||!std::isfinite(d.z))throw std::runtime_error("Dimensiones no finitas.");
}
Mesh readMesh(std::span<const unsigned char> data,std::stop_token stop) {
    check(stop);
    if(data.size()>maxFileBytes)throw std::runtime_error("Archivo supera 256 MiB.");
    bool binaryStl=false;
    if(data.size()>=84) {
        const auto count=std::uint64_t(data[80])|(std::uint64_t(data[81])<<8)|
            (std::uint64_t(data[82])<<16)|(std::uint64_t(data[83])<<24);
        binaryStl=count<=maxTriangles&&data.size()==84+count*50;
    }
    // An arbitrary binary STL header may itself start with PK. Preserve the
    // exact-length binary STL rule before interpreting a ZIP signature.
    if(!binaryStl&&data.size()>=4&&data[0]=='P'&&data[1]=='K')return read3mf(data,stop);
    std::istringstream input(std::string(reinterpret_cast<const char*>(data.data()),data.size()),std::ios::binary);
    return readStl(input,data.size(),stop);
}
Mesh readMesh(const std::filesystem::path& path,std::stop_token stop) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f)throw std::runtime_error("No se pudo abrir el modelo.");
    const auto size=f.tellg();
    if(size<0||static_cast<std::uint64_t>(size)>maxFileBytes)throw std::runtime_error("Archivo supera 256 MiB.");
    f.seekg(0); std::vector<unsigned char> data(static_cast<size_t>(size));
    for(size_t p=0;p<data.size();) {
        check(stop); const auto n=std::min<size_t>(65536,data.size()-p);
        if(!f.read(reinterpret_cast<char*>(data.data()+p),n))throw std::runtime_error("Lectura de modelo incompleta.");
        p+=n;
    }
    return readMesh(data,stop);
}
}
