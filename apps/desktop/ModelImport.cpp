#include "ModelImport.h"
#include "core/io/PaintCodec.h"
#include "core/io/ZipParts.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QXmlStreamReader>
#include <algorithm>
#include <fstream>
#include <map>
#include <stdexcept>

namespace {
void check(std::stop_token stop) {if(stop.stop_requested())throw stlcut::ImportCancelled{};}
unsigned number(QStringView value,unsigned maximum=4096) {
    bool ok=false;const auto n=value.toUInt(&ok);
    if(!ok||n>maximum)throw std::runtime_error("Identificador de metadatos 3MF invalido.");
    return n;
}
struct PaintObject {unsigned triangles{};std::vector<std::pair<unsigned,std::string>> codes;};
using ObjectKey=std::pair<std::string,unsigned>;
using Objects=std::map<ObjectKey,PaintObject>;
std::string partName(std::string path) {if(path.starts_with('/'))path.erase(0,1);return path;}
void guardXml(QXmlStreamReader& xml,unsigned& depth,std::stop_token stop) {
    check(stop);
    if(xml.isDTD()||xml.isEntityReference())throw std::runtime_error("DTD/entidades no admitidas en metadatos 3MF.");
    if(xml.isStartElement()) {
        if(++depth>128||xml.attributes().size()>64)throw std::runtime_error("XML 3MF supera profundidad o atributos admitidos.");
    } else if(xml.isEndElement()) {if(!depth)throw std::runtime_error("XML 3MF inconsistente.");--depth;}
}
void readPaintXml(const std::string& path,const std::vector<unsigned char>& bytes,Objects& objects,size_t& codeBytes,std::stop_token stop) {
    QXmlStreamReader xml(QByteArray::fromRawData(reinterpret_cast<const char*>(bytes.data()),static_cast<qsizetype>(bytes.size())));
    unsigned depth=0,object=0,objectDepth=0,meshDepth=0;
    while(!xml.atEnd()) {
        xml.readNext();guardXml(xml,depth,stop);
        if(xml.isStartElement()&&xml.namespaceUri()==u"http://schemas.microsoft.com/3dmanufacturing/core/2015/02") {
            if(xml.name()==u"object") {
                object=number(xml.attributes().value(u"id"),0x7fffffff);objectDepth=depth;
                if(!objects.emplace(ObjectKey{path,object},PaintObject{}).second)throw std::runtime_error("Objeto XML 3MF duplicado.");
            } else if(xml.name()==u"mesh"&&objectDepth)meshDepth=depth;
            else if(xml.name()==u"triangle"&&meshDepth) {
                auto& obj=objects.at({path,object});
                if(obj.triangles>=stlcut::maxTriangles)throw std::runtime_error("Pintura: demasiadas caras por objeto.");
                QString code;bool found=false;
                for(const auto& attribute:xml.attributes())if(attribute.name()==u"paint_color"||attribute.name()==u"mmu_segmentation") {
                    if(found)throw std::runtime_error("Dos representaciones de pintura en una cara; ambiguo.");
                    found=true;code=attribute.value().toString();
                }
                if(found) {
                    const auto encoded=code.toUtf8();
                    if(encoded.size()>262144||static_cast<size_t>(encoded.size())>64*1024*1024-codeBytes)
                        throw std::runtime_error("Pintura supera limite de codigo por cara o 64 MiB de metadatos.");
                    codeBytes+=encoded.size();obj.codes.emplace_back(obj.triangles,encoded.toStdString());
                }
                ++obj.triangles;
            }
        } else if(xml.isEndElement()) {
            if(meshDepth&&depth<meshDepth)meshDepth=0;
            if(objectDepth&&depth<objectDepth){objectDepth=0;object=0;}
        }
    }
    if(xml.hasError())throw std::runtime_error("XML de apariencia 3MF mal formado.");
}
struct Defaults {
    std::map<unsigned,unsigned> objects;
    std::map<std::pair<unsigned,unsigned>,unsigned> parts;
};
Defaults readDefaults(const std::vector<unsigned char>& bytes,std::stop_token stop) {
    Defaults defaults;
    QXmlStreamReader xml(QByteArray::fromRawData(reinterpret_cast<const char*>(bytes.data()),static_cast<qsizetype>(bytes.size())));
    unsigned depth=0,object=0,objectDepth=0,part=0,partDepth=0;
    while(!xml.atEnd()) {
        xml.readNext();guardXml(xml,depth,stop);
        if(xml.isStartElement()) {
            if(xml.name()==u"object") {object=number(xml.attributes().value(u"id"),0x7fffffff);objectDepth=depth;}
            else if(xml.name()==u"part"&&objectDepth){part=number(xml.attributes().value(u"id"),0x7fffffff);partDepth=depth;}
            else if(xml.name()==u"metadata"&&objectDepth&&xml.attributes().value(u"key")==u"extruder") {
                const auto value=number(xml.attributes().value(u"value"));
                if(partDepth) {
                    auto [it,inserted]=defaults.parts.emplace(std::make_pair(object,part),value);
                    if(!inserted&&it->second!=value)throw std::runtime_error("Filamento de pieza ambiguo.");
                } else {
                    auto [it,inserted]=defaults.objects.emplace(object,value);
                    if(!inserted&&it->second!=value)throw std::runtime_error("Filamento de objeto ambiguo.");
                }
            }
        } else if(xml.isEndElement()) {
            if(partDepth&&depth<partDepth){partDepth=0;part=0;}
            if(objectDepth&&depth<objectDepth){objectDepth=0;object=0;}
        }
    }
    if(xml.hasError())throw std::runtime_error("XML de asignacion de filamentos mal formado.");
    return defaults;
}
void applyPaint(std::span<const unsigned char> data,stlcut::Mesh& mesh,std::stop_token stop) {
    const auto parts=stlcut::readAppearanceParts(data,stop);
    Objects objects;size_t codeBytes=0;
    for(const auto& [path,bytes]:parts)if(path.ends_with(".model"))readPaintXml(path,bytes,objects,codeBytes,stop);
    Defaults defaults;
    if(const auto found=parts.find("Metadata/model_settings.config");found!=parts.end())defaults=readDefaults(found->second,stop);
    std::vector<std::int32_t> filamentMaterials;
    if(const auto found=parts.find("Metadata/project_settings.config");found!=parts.end()) {
        QJsonParseError error;
        const auto doc=QJsonDocument::fromJson(QByteArray::fromRawData(reinterpret_cast<const char*>(found->second.data()),static_cast<qsizetype>(found->second.size())),&error);
        if(error.error==QJsonParseError::NoError&&doc.isObject()&&doc.object().value("filament_colour").isArray()) {
            const auto colors=doc.object().value("filament_colour").toArray();
            if(colors.size()>4096||mesh.materials.size()+colors.size()>4096)throw std::runtime_error("Paleta de filamentos supera 4096 materiales.");
            for(qsizetype i=0;i<colors.size();++i) {
                const auto color=colors[i].toString();
                if((color.size()!=7&&color.size()!=9)||!color.startsWith('#'))throw std::runtime_error("Color de filamento no valido.");
                for(qsizetype k=1;k<color.size();++k)if(!((color[k]>=u'0'&&color[k]<=u'9')||(color[k]>=u'A'&&color[k]<=u'F')||(color[k]>=u'a'&&color[k]<=u'f')))
                    throw std::runtime_error("Color de filamento no hexadecimal.");
                const auto value=color.mid(1).toULongLong(nullptr,16);
                const auto rgba=color.size()==7 ? (value<<8)|255:value;
                filamentMaterials.push_back(static_cast<std::int32_t>(mesh.materials.size()));
                mesh.materials.push_back({0,static_cast<std::uint32_t>(i+1),{static_cast<std::uint8_t>(rgba>>24),static_cast<std::uint8_t>(rgba>>16),static_cast<std::uint8_t>(rgba>>8),static_cast<std::uint8_t>(rgba)},"Filamento "+std::to_string(i+1)});
            }
        } else mesh.warnings.push_back("Paleta de filamentos ausente/no interpretable: no se inventan colores de pintura.");
    }
    auto resolve=[&](unsigned filament)->std::int32_t {
        return filament&&filament<=filamentMaterials.size() ? filamentMaterials[filament-1]:-1;
    };
    bool warnedAmbiguous=false;
    for(const auto& instance:mesh.instances) {
        check(stop);
        const auto found=objects.find({partName(instance.partPath),instance.modelObject});
        if(found==objects.end()||found->second.triangles!=instance.triangleCount)
            throw std::runtime_error("No se pudo asociar la apariencia XML al objeto/instancia importado.");
        unsigned inherited=0;
        bool ambiguous=false;
        for(const auto object:instance.objectHierarchy)if(const auto value=defaults.parts.find({instance.buildObject,object});value!=defaults.parts.end()&&value->second) {
            if(inherited&&inherited!=value->second)ambiguous=true;
            inherited=value->second;
        }
        if(ambiguous) {
            inherited=0;
            if(!warnedAmbiguous){mesh.warnings.push_back("Asignacion de filamento ambigua en jerarquia; se conserva sin resolver.");warnedAmbiguous=true;}
        } else if(!inherited)if(const auto value=defaults.objects.find(instance.buildObject);value!=defaults.objects.end())inherited=value->second;
        if(resolve(inherited)>=0)for(size_t i=instance.firstTriangle;i<static_cast<size_t>(instance.firstTriangle)+instance.triangleCount;++i)
            mesh.styles[i].material={resolve(inherited),resolve(inherited),resolve(inherited)};
        for(const auto& [face,code]:found->second.codes) {
            const auto source=instance.firstTriangle+face;
            auto leaves=stlcut::decodePaint(code,stop);
            if(mesh.paintFacets.size()+leaves.size()>stlcut::maxTriangles||
               mesh.triangles.size()+mesh.paintFacets.size()+leaves.size()-mesh.paintedTriangles-1>stlcut::maxTriangles)
                throw std::runtime_error("Pintura expandida supera 2 millones de caras de presentacion.");
            ++mesh.paintedTriangles;
            for(auto& leaf:leaves) {
                const auto filament=leaf.filament ? leaf.filament:inherited;
                auto material=resolve(filament);
                if(!leaf.filament&&material<0&&!ambiguous) {
                    const auto& style=mesh.styles[source].material;
                    if(style[0]>=0&&style[0]==style[1]&&style[1]==style[2])material=style[0];
                }
                if(instance.reflected) {
                    for(auto& v:leaf.barycentric.vertices)std::swap(v.y,v.z);
                    std::swap(leaf.barycentric.vertices[1],leaf.barycentric.vertices[2]);
                }
                if(material<0)++mesh.unresolvedPaintFacets;
                mesh.paintFacets.push_back({leaf.barycentric,source,material,static_cast<std::uint16_t>(filament)});
            }
        }
    }
    if(mesh.paintedTriangles) {
        mesh.unsupportedAppearance=true; // Reading paint is not volumetric reconstruction.
        mesh.warnings.push_back("Pintura decodificada en regiones superficiales; la malla original no se subdivide. La construccion de volumen es una operacion separada con resolucion y profundidad explicitas.");
        if(mesh.unresolvedPaintFacets)mesh.warnings.push_back("Hay regiones de pintura sin filamento/color heredado resoluble; no se les asigna un color inventado.");
    }
}
}
stlcut::Mesh readModel(std::span<const unsigned char> data,std::stop_token stop) {
    auto mesh=stlcut::readMesh(data,stop);
    if(mesh.format=="3mf")applyPaint(data,mesh,stop);
    check(stop);return mesh;
}
stlcut::Mesh readModel(const std::filesystem::path& path,std::stop_token stop) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("No se pudo abrir el modelo.");
    const auto size=file.tellg();
    if(size<0||static_cast<std::uint64_t>(size)>stlcut::maxFileBytes)throw std::runtime_error("Archivo supera 256 MiB.");
    file.seekg(0);std::vector<unsigned char> data(static_cast<size_t>(size));
    for(size_t offset=0;offset<data.size();) {
        check(stop);const auto count=std::min<size_t>(65536,data.size()-offset);
        if(!file.read(reinterpret_cast<char*>(data.data()+offset),static_cast<std::streamsize>(count)))throw std::runtime_error("Lectura incompleta del modelo.");
        offset+=count;
    }
    return readModel(data,stop);
}
