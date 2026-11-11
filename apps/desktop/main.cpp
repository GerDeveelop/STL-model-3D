#include "core/io/StlReader.h"
#include "core/io/MeshReader.h"
#include "core/io/StlWriter.h"
#include "core/cutting/PlaneCut.h"
#include "core/cutting/ColorSplit.h"
#include "core/cutting/PaintVolume.h"
#include "core/projects/EditHistory.h"
#include "MeshView.h"
#include "ModelImport.h"
#include "core/io/PaintCodec.h"
#include "core/mesh/SurfaceIndex.h"
#include "core/mesh/MeshFeasibility.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QClipboard>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QSaveFile>
#include <QShortcut>
#include <QSignalBlocker>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QSplitter>
#include <QScrollArea>
#include <QThread>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <functional>
#include <cmath>
#include <numeric>
#include <memory>
#include <sstream>
#include <stop_token>
#include <map>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace {
stlcut::Mesh readStdin(std::stop_token stop = {}, const std::function<void()>& onAnalysis = {})
{
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        throw std::runtime_error("No se pudo configurar la entrada binaria.");
    }
    std::string data;
    std::array<char,65536> block;
    auto append = [&](std::size_t bytes) {
        if (data.size()+bytes>stlcut::maxFileBytes) {
            throw std::runtime_error("El archivo STL supera el límite de 256 MiB.");
        }
        data.append(block.data(),bytes);
    };
    if (stop.stop_possible()) {
        // Poll a pipe before reading so Cancel/Close also work if the producer stalls.
        const auto pipe=GetStdHandle(STD_INPUT_HANDLE);
        if (GetFileType(pipe)!=FILE_TYPE_PIPE) {
            throw std::runtime_error("--view-stdin requiere una tubería de bytes.");
        }
        for (;;) {
            if (stop.stop_requested()) { throw stlcut::ImportCancelled{}; }
            DWORD available{};
            if (!PeekNamedPipe(pipe,nullptr,0,nullptr,&available,nullptr)) {
                if (GetLastError()==ERROR_BROKEN_PIPE) { break; }
                throw std::runtime_error("No se pudo leer la tubería de entrada.");
            }
            if (!available) { Sleep(10); continue; }
            DWORD bytes{};
            if (!ReadFile(pipe,block.data(),std::min<DWORD>(available,static_cast<DWORD>(block.size())),&bytes,nullptr)) {
                throw std::runtime_error("Error de lectura en la tubería de entrada.");
            }
            append(bytes);
        }
    } else {
        while (std::cin.read(block.data(), block.size()) || std::cin.gcount()) {
            append(static_cast<std::size_t>(std::cin.gcount()));
        }
        if (std::cin.bad()) { throw std::runtime_error("Error leyendo stdin."); }
    }
    if (onAnalysis) { onAnalysis(); }
    return readModel(std::span(reinterpret_cast<const unsigned char*>(data.data()),data.size()),stop);
}
QJsonArray vectorJson(const stlcut::Vec3& p) { return {p.x, p.y, p.z}; }
QJsonObject summary(const stlcut::Mesh& mesh)
{
    QJsonArray warnings,materials,instances;
    const bool previewTruncated=mesh.instances.size()>64||mesh.materials.size()>64||
        std::any_of(mesh.instances.begin(),mesh.instances.end(),[](const auto& v){return v.name.size()>256;})||
        std::any_of(mesh.materials.begin(),mesh.materials.end(),[](const auto& v){return v.name.size()>256;});
    for(size_t i=0;i<std::min<size_t>(mesh.instances.size(),64);++i) {const auto& instance=mesh.instances[i];instances.append(QJsonObject{{"id",static_cast<qint64>(instance.id)},{"object",static_cast<qint64>(instance.object)},{"first_triangle",static_cast<qint64>(instance.firstTriangle)},{"triangles",static_cast<qint64>(instance.triangleCount)},{"name",QString::fromStdString(instance.name).left(256)}});}
    for(size_t i=0;i<std::min<size_t>(mesh.materials.size(),64);++i) {const auto& m=mesh.materials[i];materials.append(QJsonObject{{"source",m.resource?"3mf":"filament"},{"resource",static_cast<qint64>(m.resource)},{"property",static_cast<qint64>(m.property)},{"name",QString::fromStdString(m.name).left(256)},{"rgba",QJsonArray{m.rgba[0],m.rgba[1],m.rgba[2],m.rgba[3]}}});}
    for(const auto& warning:mesh.warnings)warnings.append(QString::fromStdString(warning));
    return {{"painted_triangles",static_cast<qint64>(mesh.paintedTriangles)},{"paint_facets",static_cast<qint64>(mesh.paintFacets.size())},{"unresolved_paint_facets",static_cast<qint64>(mesh.unresolvedPaintFacets)},{"instance_count",static_cast<qint64>(mesh.instances.size())},{"material_count",static_cast<qint64>(mesh.materials.size())},{"appearance_preview_truncated",previewTruncated},{"instances",instances},{"materials",materials},{"styled_triangles",static_cast<qint64>(mesh.styles.size())},{"unsupported_appearance",mesh.unsupportedAppearance},{"format", QString::fromStdString(mesh.format)},
            {"triangles", static_cast<qint64>(mesh.triangles.size())},
            {"degenerate_triangles", static_cast<qint64>(mesh.degenerateTriangles)},
            {"min", vectorJson(mesh.bounds.min)}, {"max", vectorJson(mesh.bounds.max)},
            {"dimensions", vectorJson(mesh.bounds.dimensions())},
            {"units", QString::fromStdString(mesh.units)},
            {"signed_volume", stlcut::signedVolume(mesh)}, {"warnings",warnings}};
}

QJsonObject historyJson(const stlcut::EditHistory& history)
{
    const auto& s=history.state();
    QJsonArray pieces;
    for(const auto& p:s.pieces) {
        auto item=summary(*p->mesh);
        item.insert("id",static_cast<qint64>(p->id));
        item.insert("parent_id",static_cast<qint64>(p->parentId));
        item.insert("name",QString::fromStdString(p->name).left(256));pieces.append(item);
    }
    return {{"pieces",pieces},{"selected",static_cast<qint64>(s.selected)},
            {"plane_normal",vectorJson(s.plane.normal)},{"plane_offset",s.plane.offset},{"plane_visible",s.planeVisible},
            {"connector",QJsonObject{{"enabled",s.connectorEnabled},{"diameter",s.connector.diameter},{"depth",s.connector.depth},{"clearance",s.connector.clearance},{"wall",s.connector.wall}}},
            {"paint_volume",QJsonObject{{"mode",s.paintVolume.mode==stlcut::PaintVolumeMode::Inserts?"inserts":"partition"},{"resolution",s.paintVolume.resolution},{"depth",s.paintVolume.depth}}},
            {"undo",static_cast<qint64>(history.undoCount())},{"redo",static_cast<qint64>(history.redoCount())},
            {"retained_bytes",static_cast<qint64>(history.retainedBytes())}};
}

// Production headless editing: model bytes on stdin, bounded JSON command list
// in the argument. Uses the same history, cutter and serializer as the window.
int editSession(const QString& text)
{
    QJsonObject result;QJsonArray states,exports,connectorReports;
    std::uint64_t exportBytes=0;
    std::unique_ptr<stlcut::EditHistory> history;
    int code=0;
    try {
        QJsonParseError error;
        const auto document=QJsonDocument::fromJson(text.toUtf8(),&error);
        if(error.error!=QJsonParseError::NoError||!document.isObject())throw std::runtime_error("Se requiere un objeto JSON de edicion.");
        const auto request=document.object();
        auto integer=[](const QJsonValue& value)->std::uint64_t {
            const auto n=value.toDouble(-1);
            if(!std::isfinite(n)||n<0||n>9007199254740991.0||std::floor(n)!=n)throw std::runtime_error("Entero JSON invalido.");
            return static_cast<std::uint64_t>(n);
        };
        stlcut::HistoryLimits limits;
        if(request.contains("limits")) {
            if(!request.value("limits").isObject())throw std::runtime_error("Limites JSON invalidos.");
            const auto l=request.value("limits").toObject();
            if(l.contains("bytes"))limits.bytes=integer(l.value("bytes"));
            if(l.contains("steps"))limits.cuts=static_cast<size_t>(integer(l.value("steps")));
            if(l.contains("pieces"))limits.pieces=static_cast<size_t>(integer(l.value("pieces")));
        }
        history=std::make_unique<stlcut::EditHistory>(limits);
        history->reset(std::make_shared<const stlcut::Mesh>(readStdin()),"Original");
        if(!request.value("commands").isArray())throw std::runtime_error("Falta la lista commands.");
        const auto commands=request.value("commands").toArray();
        if(commands.size()>256)throw std::runtime_error("Maximo 256 comandos por llamada.");
        states.append(historyJson(*history));
        for(const auto& value:commands) {
            if(!value.isObject())throw std::runtime_error("Comando JSON invalido.");
            const auto command=value.toObject();const auto op=command.value("op").toString();
            if(command.contains("piece"))history->select(integer(command.value("piece")));
            if(op=="plane"||op=="cut") {
                const auto normal=command.value("normal").toArray();
                if(normal.size()!=3||!command.value("offset").isDouble())throw std::runtime_error("Plano JSON invalido.");
                for(const auto& n:normal)if(!n.isDouble())throw std::runtime_error("Normal JSON invalida.");
                history->setPlane({{normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()},command.value("offset").toDouble()},command.value("visible").toBool(true));
                if(op=="cut") {
                    if(!history->selectedIsActive())throw std::runtime_error("Seleccione una pieza activa.");
                    stlcut::ConnectorSpec spec;
                    const bool withConnector=command.contains("connector");
                    if(withConnector) {
                        if(!command.value("connector").isObject())throw std::runtime_error("Conector JSON invalido.");
                        const auto fields=command.value("connector").toObject();
                        double* values[]={&spec.diameter,&spec.depth,&spec.clearance,&spec.wall};
                        const char* names[]={"diameter","depth","clearance","wall"};
                        for(int i=0;i<4;++i) {
                            if(!fields.value(names[i]).isDouble())throw std::runtime_error("Faltan parametros numericos del conector.");
                            *values[i]=fields.value(names[i]).toDouble();
                        }
                    }
                    history->setConnector(spec,withConnector);
                    auto cut=stlcut::cutByPlane(*history->selected()->mesh,history->state().plane,{},withConnector ? &spec:nullptr);
                    const auto commit=history->commitCut(std::make_shared<const stlcut::Mesh>(std::move(cut.positive)),
                                                         std::make_shared<const stlcut::Mesh>(std::move(cut.negative)));
                    result.insert("discarded_steps",static_cast<qint64>(commit.discardedSteps));
                }
            } else if(op=="paint-settings") {
                auto spec=history->state().paintVolume;const auto mode=command.value("mode").toString();
                if(mode!="inserts"&&mode!="partition")throw std::runtime_error("Modo requerido: inserts o partition.");
                spec.mode=mode=="inserts"?stlcut::PaintVolumeMode::Inserts:stlcut::PaintVolumeMode::Partition;
                if(!command.value("resolution").isDouble()||(mode=="inserts"&&!command.value("depth").isDouble()))
                    throw std::runtime_error("Indique resolution y, para insertos, depth numericos.");
                spec.resolution=command.value("resolution").toDouble();
                if(command.contains("depth")) {
                    if(!command.value("depth").isDouble())throw std::runtime_error("Profundidad invalida.");
                    spec.depth=command.value("depth").toDouble();
                }
                history->setPaintVolume(spec);
            } else if(op=="split-paint") {
                if(!history->selectedIsActive())throw std::runtime_error("Seleccione una pieza activa.");
                stlcut::ConnectorSpec connector;const bool withConnector=command.contains("connector");
                if(withConnector) {
                    if(!command.value("connector").isObject())throw std::runtime_error("Conector JSON invalido.");
                    const auto fields=command.value("connector").toObject();
                    double* values[]={&connector.diameter,&connector.depth,&connector.clearance,&connector.wall};
                    const char* names[]={"diameter","depth","clearance","wall"};
                    for(int i=0;i<4;++i) {
                        if(!fields.value(names[i]).isDouble())throw std::runtime_error("Faltan parametros numericos del conector.");
                        *values[i]=fields.value(names[i]).toDouble();
                    }
                }
                auto spec=history->state().paintVolume;
                const auto mode=command.value("mode").toString();
                if(mode!="inserts"&&mode!="partition")throw std::runtime_error("Modo requerido: inserts o partition.");
                spec.mode=mode=="inserts"?stlcut::PaintVolumeMode::Inserts:stlcut::PaintVolumeMode::Partition;
                if(!command.value("resolution").isDouble()||(mode=="inserts"&&!command.value("depth").isDouble()))
                    throw std::runtime_error("Indique resolution y, para insertos, depth numericos.");
                spec.resolution=command.value("resolution").toDouble();
                if(command.contains("depth")) {
                    if(!command.value("depth").isDouble())throw std::runtime_error("Profundidad invalida.");
                    spec.depth=command.value("depth").toDouble();
                }
                std::vector<stlcut::RegionConnection> connections;
                auto pieces=stlcut::splitPaintVolume(*history->selected()->mesh,spec,{},withConnector?&connector:nullptr,&connections);
                std::vector<std::shared_ptr<const stlcut::Mesh>> meshes;
                for(auto& mesh:pieces)meshes.push_back(std::make_shared<const stlcut::Mesh>(std::move(mesh)));
                auto next=*history;next.setConnector(connector,withConnector);const auto ids=next.commitSplit(meshes);next.setPaintVolume(spec);*history=std::move(next);
                if(withConnector) {
                    QJsonArray links;
                    for(const auto& link:connections) {
                        QJsonObject item{{"first_piece",static_cast<qint64>(ids.at(link.first))},{"second_piece",static_cast<qint64>(ids.at(link.second))},
                            {"connected",link.connected},{"reason",QString::fromStdString(link.reason)}};
                        if(link.connected) {
                            item.insert("donor_piece",static_cast<qint64>(ids.at(link.donor)));item.insert("receiver_piece",static_cast<qint64>(ids.at(link.receiver)));
                            item.insert("position",vectorJson(link.position));item.insert("normal",vectorJson(link.normal));
                            item.insert("added_volume",link.addedVolume);item.insert("removed_volume",link.removedVolume);
                        }
                        links.append(item);
                    }
                    connectorReports.append(QJsonObject{{"step",states.size()},{"connections",links}});
                    result.insert("connector_reports",connectorReports);
                }
            } else if(op=="split-materials") {
                if(!history->selectedIsActive())throw std::runtime_error("Seleccione una pieza activa.");
                stlcut::ConnectorSpec spec;const bool withConnector=command.contains("connector");
                if(withConnector) {
                    if(!command.value("connector").isObject())throw std::runtime_error("Conector JSON invalido.");
                    const auto fields=command.value("connector").toObject();
                    double* values[]={&spec.diameter,&spec.depth,&spec.clearance,&spec.wall};
                    const char* names[]={"diameter","depth","clearance","wall"};
                    for(int i=0;i<4;++i) {
                        if(!fields.value(names[i]).isDouble())throw std::runtime_error("Faltan parametros numericos del conector.");
                        *values[i]=fields.value(names[i]).toDouble();
                    }
                }
                history->setConnector(spec,withConnector);
                auto pieces=stlcut::splitByMaterial(*history->selected()->mesh,{},withConnector ? &spec:nullptr);
                std::vector<std::shared_ptr<const stlcut::Mesh>> meshes;
                for(auto& mesh:pieces)meshes.push_back(std::make_shared<const stlcut::Mesh>(std::move(mesh)));
                history->commitSplit(meshes);
            } else if(op=="undo") {history->undo();}
            else if(op=="redo") {history->redo();}
            else if(op=="export") {
                const auto bytesNeeded=84+history->selected()->mesh->triangles.size()*50;
                if(bytesNeeded>stlcut::maxFileBytes-exportBytes)throw std::runtime_error("Exportaciones JSON superan 256 MiB de STL en una llamada.");
                exportBytes+=bytesNeeded;
                const auto piece=history->selected();
                const auto bytes=stlcut::validatedStl(*piece->mesh,piece->parentId!=0);
                exports.append(QJsonObject{{"piece",static_cast<qint64>(history->state().selected)},
                    {"stl_base64",QString::fromLatin1(QByteArray(bytes.data(),static_cast<qsizetype>(bytes.size())).toBase64())}});
            } else if(op!="select"&&op!="status")throw std::runtime_error("Operacion de edicion desconocida.");
            states.append(historyJson(*history));
        }
        result.insert("ok",true);
    } catch(const std::exception& e) {result.insert("ok",false);result.insert("error",QString::fromUtf8(e.what()));code=1;}
    if(history&&!history->empty())result.insert("current",historyJson(*history));
    result.insert("states",states);result.insert("exports",exports);
    std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';
    return code;
}

QJsonObject feasibilityJson(const stlcut::Mesh& mesh,std::stop_token stop={})
{
    QElapsedTimer timer;timer.start();QJsonArray instances;
    for(const auto& f:stlcut::meshFeasibility(mesh,stop))instances.append(QJsonObject{
        {"instance",static_cast<qint64>(f.instance)},{"triangles",static_cast<qint64>(f.triangles)},
        {"surface_facets",static_cast<qint64>(f.surfaceFacets)},{"unresolved_facets",static_cast<qint64>(f.unresolvedFacets)},
        {"materials",static_cast<qint64>(f.materials)},{"topology_analyzed",f.topologyAnalyzed},
        {"closed_oriented",f.topologyAnalyzed?QJsonValue(f.closedOriented):QJsonValue(QJsonValue::Null)},
        {"signed_volume",f.topologyAnalyzed?QJsonValue(f.volume):QJsonValue(QJsonValue::Null)},
        {"positive_volume",f.topologyAnalyzed?QJsonValue(f.positiveVolume):QJsonValue(QJsonValue::Null)},
        {"surface_components",static_cast<qint64>(f.components)},{"degenerate_triangles",static_cast<qint64>(f.degenerate)},
        {"boundary_edges",static_cast<qint64>(f.boundaryEdges)},{"nonmanifold_edges",static_cast<qint64>(f.nonmanifoldEdges)},
        {"inconsistent_edges",static_cast<qint64>(f.inconsistentEdges)},{"current_reconstruction_budget",f.currentReconstructionBudget},
        {"reason",QString::fromStdString(f.reason)}});
    return {{"instances",instances},{"elapsed_ms",timer.nsecsElapsed()/1e6},{"exact_coordinate_screening",true},{"self_intersections_checked",false},{"source_modified",false}};
}
QJsonObject spatialJson(const stlcut::Mesh& mesh,const QString& text)
{
    QJsonParseError error;const auto doc=QJsonDocument::fromJson(text.toUtf8(),&error);
    if(error.error!=QJsonParseError::NoError||!doc.isObject())throw std::runtime_error("Se requiere objeto JSON de consulta espacial.");
    const auto req=doc.object();std::optional<std::uint32_t> instance;
    auto integer=[](QJsonValue v,double max){const auto d=v.toDouble(-1);if(!v.isDouble()||!std::isfinite(d)||d<0||d>max||std::floor(d)!=d)throw std::runtime_error("Entero espacial invalido.");return static_cast<std::uint32_t>(d);};
    if(req.contains("instance"))instance=integer(req.value("instance"),4294967295.);
    stlcut::SurfaceWork indexed,brute;
    if(req.contains("work")){indexed.limit=integer(req.value("work"),100000000);brute.limit=indexed.limit;}
    if(!indexed.limit)throw std::runtime_error("Presupuesto espacial debe ser positivo.");
    if(req.contains("verify")&&!req.value("verify").isBool())throw std::runtime_error("verify debe ser booleano.");
    if(!req.value("points").isArray()||req.value("points").toArray().isEmpty()||req.value("points").toArray().size()>1024)throw std::runtime_error("Indique entre 1 y 1024 puntos espaciales.");
    QElapsedTimer timer;timer.start();stlcut::SurfaceIndex index(stlcut::surfaceData(mesh,instance));const double buildMs=timer.nsecsElapsed()/1e6;
    double indexedMs=0,bruteMs=0;QJsonArray hits;const bool verify=req.value("verify").toBool();
    for(const auto& value:req.value("points").toArray()) {
        if(!value.isArray()||value.toArray().size()!=3)throw std::runtime_error("Punto espacial requiere tres numeros.");
        const auto a=value.toArray();for(const auto& v:a)if(!v.isDouble()||!std::isfinite(v.toDouble()))throw std::runtime_error("Punto espacial no finito o no numerico.");
        const stlcut::Vec3 p{a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};timer.restart();const auto hit=index.nearest(p,indexed);indexedMs+=timer.nsecsElapsed()/1e6;
        if(verify){timer.restart();const auto reference=index.nearest(p,brute,{},true);bruteMs+=timer.nsecsElapsed()/1e6;if(std::abs(hit.distance-reference.distance)>std::max(1e-10,std::abs(reference.distance)*1e-10)||hit.facet!=reference.facet)throw std::runtime_error("La consulta indexada difiere de la referencia exhaustiva.");}
        hits.append(QJsonObject{{"distance",hit.distance},{"material",hit.material},{"source_triangle",static_cast<qint64>(hit.sourceTriangle)},{"instance",static_cast<qint64>(hit.instance)},{"facet",static_cast<qint64>(hit.facet)}});
    }
    return {{"hits",hits},{"facets",static_cast<qint64>(index.facets())},{"nodes",static_cast<qint64>(index.nodes())},{"index_bytes",static_cast<qint64>(index.bytes())},
        {"node_visits",static_cast<qint64>(indexed.nodes)},{"triangle_tests",static_cast<qint64>(indexed.triangles)},{"brute_triangle_tests",static_cast<qint64>(brute.triangles)},
        {"build_ms",buildMs},{"query_ms",indexedMs},{"reference_ms",bruteMs},{"verified_exhaustive",verify},{"source_modified",false}};
}
// Useful for local automation: same production reader, JSON over redirected stdout.
// No GUI, no source-file changes and no auxiliary files are required.
int inspect(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QJsonObject result;
    int code = 0;
    try {
        stlcut::Mesh mesh;
        const auto args = app.arguments();
        if((args.size()==3&&args[1]==QStringLiteral("--decode-paint"))||(args.size()==2&&args[1]==QStringLiteral("--decode-paint-stdin"))) {
            std::string paintCode;
            if(args.size()==3)paintCode=args[2].toUtf8().toStdString();
            else {
                if(_setmode(_fileno(stdin),_O_BINARY)==-1)throw std::runtime_error("No se pudo leer codigo de pintura.");
                std::array<char,65536> buffer;
                while(std::cin.read(buffer.data(),buffer.size())||std::cin.gcount()) {
                    const auto count=static_cast<size_t>(std::cin.gcount());
                    if(count>262144-paintCode.size())throw std::runtime_error("Pintura: codigo supera 262144 caracteres.");
                    paintCode.append(buffer.data(),count);
                }
                if(std::cin.bad())throw std::runtime_error("Error leyendo codigo de pintura.");
            }
            const auto leaves=stlcut::decodePaint(paintCode);
            QJsonArray preview;std::map<unsigned,double> areas;
            for(const auto& leaf:leaves) {
                const auto& a=leaf.barycentric.vertices[0];const auto& b=leaf.barycentric.vertices[1];const auto& c=leaf.barycentric.vertices[2];
                areas[leaf.filament]+=std::abs((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y));
                if(preview.size()<64)preview.append(QJsonObject{{"filament",leaf.filament},{"barycentric",QJsonArray{vectorJson(a),vectorJson(b),vectorJson(c)}}});
            }
            QJsonArray fractions;for(const auto& [filament,area]:areas)fractions.append(QJsonObject{{"filament",static_cast<int>(filament)},{"fraction",area}});
            result={{"ok",true},{"leaf_count",static_cast<qint64>(leaves.size())},{"leaves",preview},{"filament_fractions",fractions}};
            std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
        }
        if(args.size()==3&&args[1]==QStringLiteral("--edit-stdin"))return editSession(args[2]);
        if((args.size()==2&&args[1]==QStringLiteral("--inspect-feasibility-stdin"))||(args.size()==3&&args[1]==QStringLiteral("--inspect-spatial-stdin"))) {
            mesh=readStdin();result=args[1]==QStringLiteral("--inspect-feasibility-stdin")?feasibilityJson(mesh):spatialJson(mesh,args[2]);
            result.insert("ok",true);std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
        }
        if (args.size() == 2 && (args[1] == QStringLiteral("--inspect-stdin")||args[1] == QStringLiteral("--inspect-view-stdin"))) {
            mesh = readStdin();
        } else if (args.size() == 3 && args[1] == QStringLiteral("--inspect")) {
            mesh = readModel(std::filesystem::path(args[2].toStdWString()));
        } else if (args.size() == 7 && args[1] == QStringLiteral("--cut-stdin")) {
            stlcut::Plane plane;
            double* values[]={&plane.normal.x,&plane.normal.y,&plane.normal.z,&plane.offset};
            for(int i=0;i<4;++i) {
                bool ok=false;*values[i]=args[i+2].toDouble(&ok);
                if(!ok)throw std::runtime_error("Parametros del plano invalidos.");
            }
            if(args[6]!="positive"&&args[6]!="negative")throw std::runtime_error("Lado: positive o negative.");
            const auto cut=stlcut::cutByPlane(readStdin(),plane);
            _setmode(_fileno(stdout),_O_BINARY);
            const auto bytes=stlcut::validatedStl(args[6]=="positive" ? cut.positive:cut.negative,true);
            std::cout.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
            if(!std::cout)throw std::runtime_error("No se pudo escribir el STL.");
            return 0;
        } else {
            throw std::runtime_error("Uso: --inspect <modelo.stl|3mf>, --inspect-stdin o --cut-stdin nx ny nz offset positive|negative");
        }
        result = summary(mesh);
        if(args.contains(QStringLiteral("--inspect-view-stdin"))) {
            const auto view=prepareViewMesh(mesh,{},true);
            std::size_t zeroNormals=0,nonfiniteNormals=0;double minNormal=2,maxNormal=0;
            for(const auto& v:view->vertices) {
                const auto length=std::hypot(v.nx,v.ny,v.nz);
                if(!std::isfinite(length))++nonfiniteNormals;
                else {if(length==0)++zeroNormals;minNormal=std::min(minNormal,static_cast<double>(length));maxNormal=std::max(maxNormal,static_cast<double>(length));}
            }
            result.insert("view_legacy_zero_normal_triangles",static_cast<qint64>(view->legacyZeroNormalTriangles));
            result.insert("view_zero_normal_vertices",static_cast<qint64>(zeroNormals));result.insert("view_nonfinite_normal_vertices",static_cast<qint64>(nonfiniteNormals));
            result.insert("view_normal_length_min",view->vertices.empty()?0:minNormal);result.insert("view_normal_length_max",maxNormal);
            struct ColorInfo {size_t count{};std::array<float,3> minimum{},maximum{};};
            std::map<std::array<int,3>,ColorInfo> colors;
            for(const auto& vertex:view->vertices) {
                auto& info=colors[{static_cast<int>(std::lround(vertex.r*255)),static_cast<int>(std::lround(vertex.g*255)),static_cast<int>(std::lround(vertex.b*255))}];
                const std::array<float,3> position{vertex.x,vertex.y,vertex.z};
                if(!info.count)info.minimum=info.maximum=position;
                for(size_t k=0;k<3;++k){info.minimum[k]=std::min(info.minimum[k],position[k]);info.maximum[k]=std::max(info.maximum[k],position[k]);}++info.count;
            }
            QJsonArray preview;for(const auto& [rgb,info]:colors)if(preview.size()<64)preview.append(QJsonObject{{"rgb",QJsonArray{rgb[0],rgb[1],rgb[2]}},{"vertices",static_cast<qint64>(info.count)},
                {"min",QJsonArray{info.minimum[0],info.minimum[1],info.minimum[2]}},{"max",QJsonArray{info.maximum[0],info.maximum[1],info.maximum[2]}}});
            result.insert("view_vertices",static_cast<qint64>(view->vertices.size()));result.insert("view_bytes",static_cast<qint64>(view->vertices.size()*sizeof(ViewVertex)));result.insert("view_colors",preview);
        }
        result.insert("ok", true);
    } catch (const std::exception& error) {
        if(app.arguments().contains(QStringLiteral("--cut-stdin"))) {std::cerr<<error.what()<<'\n';return 1;}
        result = {{"ok", false}, {"error", QString::fromUtf8(error.what())}};
        code = 1;
    }
    std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << '\n';
    return code;
}

enum class ImportPhase { Receiving, Analyzing, Preparing, Ready };
const char* phaseName(ImportPhase phase)
{
    switch (phase) {
    case ImportPhase::Receiving: return "receiving";
    case ImportPhase::Analyzing: return "analysis";
    case ImportPhase::Preparing: return "preparation";
    case ImportPhase::Ready: return "ready";
    }
    return "unknown";
}

struct ImportJob {
    std::stop_source cancellation;
    std::shared_ptr<stlcut::Mesh> mesh;
    std::shared_ptr<const ViewMesh> view;
    QString error;
    bool cancelled{};
    double inputMs{};
    double presentationMs{};
    std::atomic<ImportPhase> phase{ImportPhase::Receiving};
    QString cancelRequestPhase;
    QElapsedTimer cancelTimer;
    void setPhase(ImportPhase value)
    {
        phase.store(value);
        qInfo().noquote() << "STL_IMPORT_PHASE" << phaseName(value);
    }
};

QString vectorText(const stlcut::Vec3& p)
{
    return QStringLiteral("X: %1 · Y: %2 · Z: %3")
        .arg(p.x, 0, 'g', 12).arg(p.y, 0, 'g', 12).arg(p.z, 0, 'g', 12);
}

class MainWindow final : public QMainWindow {
public:
    MainWindow()
    {
        setWindowTitle(QStringLiteral("STL Cut Studio AI %1").arg(QApplication::applicationVersion()));
        resize(1250, 800);
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        info_ = new QLabel(QStringLiteral("Abra un STL o 3MF para visualizarlo y consultar su geometría."), central);
        info_->setObjectName(QStringLiteral("meshStatistics"));
        info_->setTextFormat(Qt::PlainText);
        info_->setWordWrap(true);
        info_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        outcome_ = new QLabel(central);
        outcome_->setObjectName(QStringLiteral("importOutcome"));
        outcome_->setTextFormat(Qt::PlainText);
        outcome_->setWordWrap(true);
        progress_ = new QProgressBar(central);
        progress_->setRange(0, 0);
        progress_->hide();
        cancel_ = new QPushButton(QStringLiteral("Cancelar importación (Ctrl+.)"), central);
        // QAbstractButton::setText resets its mnemonic shortcut. Keep Ctrl+.
        // on the window so operation-specific button captions cannot remove it.
        auto* cancelShortcut=new QShortcut(QKeySequence(QStringLiteral("Ctrl+.")),this);
        connect(cancelShortcut,&QShortcut::activated,this,[this] {
            if(cancel_->isVisible()&&cancel_->isEnabled())cancel_->click();
        });
        cancel_->setEnabled(false);
        cancel_->hide();
        layout->addWidget(info_);
        layout->addWidget(outcome_);
        auto* cutting=new QWidget(central);
        cutControls_=cutting;
        auto* form=new QFormLayout(cutting);
        const QString names[]={QStringLiteral("Normal X"),QStringLiteral("Normal Y"),QStringLiteral("Normal Z"),QStringLiteral("Posición d (n·p=d)")};
        for(int i=0;i<4;++i) {
            planeInputs_[i]=new QDoubleSpinBox(cutting);
            planeInputs_[i]->setRange(-1e12,1e12);
            planeInputs_[i]->setDecimals(8);
            planeInputs_[i]->setValue(i==2 ? 1:0);
            form->addRow(names[i],planeInputs_[i]);
            connect(planeInputs_[i],qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updatePlane();});
        }
        preview_=new QCheckBox(QStringLiteral("Mostrar plano y normal +"),cutting);
        preview_->setChecked(true);form->addRow(preview_);
        connect(preview_,&QCheckBox::toggled,this,[this]{updatePlane();});
        connectors_=new QCheckBox(QStringLiteral("Añadir pin y cavidad al cortar / separar"),cutting);
        form->addRow(connectors_);
        const QString connectorNames[]={QStringLiteral("Diámetro pin (unidades modelo)"),QStringLiteral("Profundidad pin"),QStringLiteral("Holgura diametral"),QStringLiteral("Pared mínima")};
        const double connectorDefaults[]={3,4,.2,1};
        for(int i=0;i<4;++i) {
            connectorInputs_[i]=new QDoubleSpinBox(cutting);connectorInputs_[i]->setRange(i==2 ? 0:.001,1e6);
            connectorInputs_[i]->setDecimals(3);connectorInputs_[i]->setValue(connectorDefaults[i]);
            form->addRow(connectorNames[i],connectorInputs_[i]);
            connect(connectorInputs_[i],qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updateConnector();});
        }
        connect(connectors_,&QCheckBox::toggled,this,[this]{updateConnector();});
        cutButton_=new QPushButton(QStringLiteral("Cortar pieza seleccionada"),cutting);
        form->addRow(cutButton_);connect(cutButton_,&QPushButton::clicked,this,[this]{startCut();});
        splitMaterials_=new QPushButton(QStringLiteral("Separar cuerpos por material / color"),cutting);
        form->addRow(splitMaterials_);connect(splitMaterials_,&QPushButton::clicked,this,[this]{startMaterialSplit();});
        auto* feasibility=new QPushButton(QStringLiteral("Analizar viabilidad por instancia"),cutting);
        form->addRow(feasibility);connect(feasibility,&QPushButton::clicked,this,[this]{startFeasibility();});
        paintMode_=new QComboBox(cutting);
        paintMode_->addItems({QStringLiteral("Partición completa por color"),QStringLiteral("Insertos y núcleo sin color asignado")});
        form->addRow(QStringLiteral("Volumen desde pintura"),paintMode_);
        paintResolution_=new QDoubleSpinBox(cutting);paintResolution_->setDecimals(4);paintResolution_->setRange(.0001,1e6);paintResolution_->setValue(1);
        form->addRow(QStringLiteral("Tamaño máximo de celda"),paintResolution_);
        paintDepth_=new QDoubleSpinBox(cutting);paintDepth_->setDecimals(4);paintDepth_->setRange(.0001,1e6);paintDepth_->setValue(2);
        form->addRow(QStringLiteral("Profundidad del inserto"),paintDepth_);paintDepth_->setEnabled(false);
        connect(paintMode_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){paintDepth_->setEnabled(i==1);updatePaintVolume();});
        connect(paintResolution_,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updatePaintVolume();});
        connect(paintDepth_,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{updatePaintVolume();});
        auto* paintHint=new QLabel(QStringLiteral("Interfaces escalonadas según celda; exterior recortado al original. Active la casilla de conectores para buscar pines/cavidades seguros. Se informan las uniones omitidas; no se certifica el orden global de montaje."),cutting);
        paintHint->setWordWrap(true);form->addRow(paintHint);
        splitPaint_=new QPushButton(QStringLiteral("Construir piezas desde pintura"),cutting);
        form->addRow(splitPaint_);connect(splitPaint_,&QPushButton::clicked,this,[this]{startMaterialSplit(true);});
        pieces_=new QComboBox(cutting);
        pieces_->setObjectName(QStringLiteral("pieceSelection"));
        form->addRow(QStringLiteral("Mostrar / exportar"),pieces_);
        connect(pieces_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int){selectPiece();});
        auto* exportButton=new QPushButton(QStringLiteral("Exportar pieza visible a STL…"),cutting);
        form->addRow(exportButton);connect(exportButton,&QPushButton::clicked,this,[this]{exportPiece();});
        historyStatus_=new QLabel(cutting);historyStatus_->setWordWrap(true);historyStatus_->setObjectName(QStringLiteral("historyStatus"));
        form->addRow(historyStatus_);
        cutting->setEnabled(false);layout->addWidget(cutting);
        layout->addStretch();
        layout->addWidget(progress_);
        layout->addWidget(cancel_);
        viewerStatus_=new QLabel(central);
        viewerStatus_->setTextFormat(Qt::PlainText);
        viewerStatus_->setWordWrap(true);
        viewerStatus_->setObjectName(QStringLiteral("rendererStatus"));
        layout->addWidget(viewerStatus_);
        auto* sidebar=new QScrollArea(this);
        sidebar->setWidgetResizable(true);
        sidebar->setWidget(central);
        sidebar->setMinimumWidth(280);
        sidebar->setMaximumWidth(400);
        auto* split=new QSplitter(this);
        view_=new MeshView(split);
        split->addWidget(view_);
        split->addWidget(sidebar);
        split->setSizes({900,330});
        setCentralWidget(split);
        connect(view_,&MeshView::rendererStatus,viewerStatus_,&QLabel::setText);

        auto* fileMenu = menuBar()->addMenu(QStringLiteral("&Archivo"));
        open_ = fileMenu->addAction(QStringLiteral("Abrir &STL / 3MF…"));
        open_->setShortcut(QKeySequence::Open);
        connect(open_, &QAction::triggered, this, [this] {
            const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Abrir modelo"),
                {}, QStringLiteral("Modelos 3D (*.stl *.3mf);;STL (*.stl);;3MF (*.3mf);;Todos los archivos (*)"));
            if (!path.isEmpty()) { importFile(path); }
        });
        fileMenu->addSeparator();
        auto* quit = fileMenu->addAction(QStringLiteral("&Salir"));
        quit->setShortcut(QKeySequence::Quit);
        connect(quit, &QAction::triggered, this, &QMainWindow::close);
        auto* editMenu=menuBar()->addMenu(QStringLiteral("&Editar"));
        undo_=editMenu->addAction(QStringLiteral("Deshacer corte"));undo_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Z")));
        redo_=editMenu->addAction(QStringLiteral("Rehacer corte"));redo_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Y")));
        connect(undo_,&QAction::triggered,this,[this]{restoreHistory(false);});
        connect(redo_,&QAction::triggered,this,[this]{restoreHistory(true);});
        undo_->setEnabled(false);redo_->setEnabled(false);
        auto* viewMenu=menuBar()->addMenu(QStringLiteral("&Vista"));
        auto* toolbar=addToolBar(QStringLiteral("Vista"));
        toolbar->setMovable(false);
        toolbar->addAction(undo_);toolbar->addAction(redo_);toolbar->addSeparator();
        auto* fit=viewMenu->addAction(QStringLiteral("Encuadrar"));
        fit->setShortcut(QKeySequence(Qt::Key_F));
        connect(fit,&QAction::triggered,view_,&MeshView::fitMesh);
        toolbar->addAction(fit);
        auto* reset=viewMenu->addAction(QStringLiteral("Restablecer cámara"));
        reset->setShortcut(QKeySequence(Qt::Key_Home));
        connect(reset,&QAction::triggered,view_,&MeshView::resetCamera);
        toolbar->addAction(reset);
        auto* grid=viewMenu->addAction(QStringLiteral("Cuadrícula"));
        grid->setCheckable(true); grid->setChecked(true);
        connect(grid,&QAction::toggled,view_,&MeshView::showGrid);
        toolbar->addAction(grid);
        auto* axes=viewMenu->addAction(QStringLiteral("Ejes"));
        axes->setCheckable(true); axes->setChecked(true);
        connect(axes,&QAction::toggled,view_,&MeshView::showAxes);
        toolbar->addAction(axes);
        auto* capture=viewMenu->addAction(QStringLiteral("Copiar imagen"));
        capture->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
        toolbar->addAction(capture);
        connect(capture,&QAction::triggered,this,[this] {
            const auto image=view_->grabFramebuffer();
            if (image.isNull()) {
                statusBar()->showMessage(QStringLiteral("No se pudo capturar el visor"),5000);
            } else {
                QApplication::clipboard()->setImage(image);
                statusBar()->showMessage(QStringLiteral("Imagen del visor copiada al portapapeles"),5000);
            }
        });
        auto* help = menuBar()->addMenu(QStringLiteral("A&yuda"));
        auto* about = help->addAction(QStringLiteral("&Acerca de…"));
        connect(about, &QAction::triggered, this, [this] {
            QMessageBox::about(this, QStringLiteral("STL Cut Studio AI"),
                               QStringLiteral("Versión %1\nImportación STL/3MF, corte por plano y exportación STL.\n"
                                              "Visor 3D OpenGL con órbita, zoom y paneo.").arg(QApplication::applicationVersion()));
        });
        connect(cancel_, &QPushButton::clicked, this, [this] {
            if (!job_) { return; }
            job_->cancelRequestPhase=QString::fromLatin1(phaseName(job_->phase.load()));
            job_->cancelTimer.start();
            job_->cancellation.request_stop();
            qInfo().noquote() << "STL_CANCEL_REQUEST" << job_->cancelRequestPhase;
            cancel_->setEnabled(false);
            outcome_->setText(QStringLiteral("Cancelando operación…"));
        });
        statusBar()->showMessage(QStringLiteral("Sin modelo cargado · STL no define unidades"));
    }

    ~MainWindow() override
    {
        if (worker_) {
            job_->cancellation.request_stop();
            worker_->wait();
        }
    }

    void importFile(const QString& path, bool fromStdin = false)
    {
        if (worker_) { return; }
        job_ = std::make_shared<ImportJob>();
        const auto job = job_;
        worker_ = QThread::create([job, path, fromStdin] {
            try {
                const auto stop=job->cancellation.get_token();
                QElapsedTimer timer;
                timer.start();
                job->setPhase(fromStdin ? ImportPhase::Receiving : ImportPhase::Analyzing);
                job->mesh = std::make_shared<stlcut::Mesh>(fromStdin ? readStdin(stop, [job] {
                    job->setPhase(ImportPhase::Analyzing);
                }) : readModel(
                    std::filesystem::path(path.toStdWString()), stop));
                job->inputMs=static_cast<double>(timer.nsecsElapsed())/1e6;
                timer.restart();
                job->setPhase(ImportPhase::Preparing);
                job->view=prepareViewMesh(*job->mesh,stop);
                job->presentationMs=static_cast<double>(timer.nsecsElapsed())/1e6;
                job->setPhase(ImportPhase::Ready);
            } catch (const stlcut::ImportCancelled&) {
                job->cancelled = true;
            } catch (const std::bad_alloc&) {
                job->error = QStringLiteral("Memoria insuficiente para importar el STL.");
            } catch (const std::exception& error) {
                job->error = QString::fromUtf8(error.what());
            } catch (...) {
                job->error = QStringLiteral("Error inesperado durante la importación STL.");
            }
        });
        worker_->setParent(this);
        open_->setEnabled(false);
        cutControls_->setEnabled(false);
        updateHistoryActions();
        cancel_->setText(QStringLiteral("Cancelar importación (Ctrl+.)"));
        cancel_->setEnabled(true);
        cancel_->show();
        progress_->show();
        outcome_->setText(QStringLiteral("Importando %1…").arg(QFileInfo(path).fileName()));
        statusBar()->showMessage(QStringLiteral("Importación en segundo plano"));
        connect(worker_, &QThread::finished, this, [this, job, path] {
            // Synchronize with thread termination before consuming its result.
            worker_->wait();
            worker_->deleteLater();
            worker_ = nullptr;
            open_->setEnabled(true);
            progress_->hide();
            cancel_->setEnabled(false);
            cancel_->hide();
            if (job->cancelled || job->cancellation.stop_requested()) {
                outcome_->setText(QStringLiteral("Importación cancelada. Se conserva el modelo anterior."));
                qInfo().noquote() << "STL_CANCELLED_METRIC" << QJsonDocument(QJsonObject{
                    {"request_phase",job->cancelRequestPhase},
                    {"worker_phase",QString::fromLatin1(phaseName(job->phase.load()))},
                    {"latency_ms",job->cancelTimer.isValid() ? static_cast<double>(job->cancelTimer.nsecsElapsed())/1e6 : -1},
                    {"retained_triangles",selectedMesh() ? static_cast<qint64>(selectedMesh()->triangles.size()) : 0}
                }).toJson(QJsonDocument::Compact);
            } else if (!job->error.isEmpty()) {
                outcome_->setText(QStringLiteral("No se pudo importar: %1\nSe conserva el modelo anterior.")
                                      .arg(job->error));
                qInfo().noquote() << "STL_IMPORT_ERROR_METRIC retained_triangles="
                                  << (selectedMesh() ? selectedMesh()->triangles.size() : 0);
            } else {
                qInfo().noquote() << "STL_IMPORT_METRIC" << QJsonDocument(QJsonObject{
                    {"triangles",static_cast<qint64>(job->mesh->triangles.size())},
                    {"input_ms",job->inputMs},{"presentation_ms",job->presentationMs},
                    {"presentation_bytes",static_cast<qint64>(job->view->vertices.size()*sizeof(ViewVertex))}
                }).toJson(QJsonDocument::Compact);
                try {
                    auto next=history_;
                    next.reset(job->mesh,QFileInfo(path).fileName().toStdString(),viewBytes(*job->view));
                    std::map<stlcut::PieceId,std::shared_ptr<const ViewMesh>> views{{next.original()->id,job->view}};
                    history_=std::move(next);pieceViews_.swap(views);sourcePath_=path;
                    refreshScene();
                    outcome_->setText(QStringLiteral("Importación completada. Triángulos degenerados: %1.\nEl historial anterior se reinició. No se comprobó cierre ni ausencia de autointersecciones.")
                        .arg(static_cast<qulonglong>(history_.original()->mesh->degenerateTriangles)));
                    for(const auto& warning:history_.original()->mesh->warnings)outcome_->setText(outcome_->text()+"\n"+QString::fromStdString(warning));
                }catch(const std::exception& e) {
                    outcome_->setText(QStringLiteral("No se pudo adoptar el modelo: %1. Se conserva la sesión anterior.").arg(QString::fromUtf8(e.what())));
                }
            }
            statusBar()->showMessage(history_.empty() ? QStringLiteral("Sin modelo cargado") : QStringLiteral("Sesión con %1 piezas activas").arg(history_.state().pieces.size()));
            job_.reset();
            updateHistoryActions();
        });
        worker_->start();
    }
private:
    stlcut::Plane plane() const {return {{planeInputs_[0]->value(),planeInputs_[1]->value(),planeInputs_[2]->value()},planeInputs_[3]->value()};}
    void updateConnector() {
        if(!history_.empty())history_.setConnector({connectorInputs_[0]->value(),connectorInputs_[1]->value(),connectorInputs_[2]->value(),connectorInputs_[3]->value()},connectors_->isChecked());
    }
    void updatePaintVolume() {
        if(!history_.empty())history_.setPaintVolume({paintMode_->currentIndex()==1?stlcut::PaintVolumeMode::Inserts:stlcut::PaintVolumeMode::Partition,paintResolution_->value(),paintDepth_->value()});
    }
    void updatePlane() {
        if(!history_.empty())history_.setPlane(plane(),preview_->isChecked());
        if(view_)view_->setCutPlane(plane(),preview_->isChecked());
    }
    static std::uint64_t viewBytes(const ViewMesh& view) {
        return sizeof(ViewMesh)+view.vertices.capacity()*sizeof(ViewVertex);
    }
    const stlcut::Mesh* selectedMesh() const {
        return history_.empty() ? nullptr:history_.selected()->mesh.get();
    }
    void updateHistoryActions() {
        const bool available=!worker_&&!history_.empty();
        cutControls_->setEnabled(available);
        cutButton_->setEnabled(available&&history_.selectedIsActive());
        splitMaterials_->setEnabled(available&&history_.selectedIsActive());
        splitPaint_->setEnabled(available&&history_.selectedIsActive());
        undo_->setEnabled(available&&history_.undoCount()>0);
        redo_->setEnabled(available&&history_.redoCount()>0);
        if(available)statusBar()->showMessage(history_.state().pieces.size()==1 ? QStringLiteral("Sesión con 1 pieza activa"):
            QStringLiteral("Sesión con %1 piezas activas").arg(history_.state().pieces.size()));
        if(!history_.empty())historyStatus_->setText(QStringLiteral("%1 piezas activas · Deshacer: %2 · Rehacer: %3\nHistorial: %4 MiB / 1024 MiB · máximo 32 pasos y 256 piezas")
            .arg(history_.state().pieces.size()).arg(history_.undoCount()).arg(history_.redoCount())
            .arg(history_.retainedBytes()/1048576.0,0,'f',2));
    }
    void showSelectedPiece() {
        if(history_.empty())return;
        const auto piece=history_.selected();const auto& mesh=*piece->mesh;
        view_->setMesh(pieceViews_.at(piece->id));
        auto text=QStringLiteral("Pieza: %1 [#%2]\nOrigen: %3\nFormato: %4\nTriángulos: %5\nDimensiones: %6\nCaja mínima: %7\nCaja máxima: %8\nUnidades: %9")
            .arg(QString::fromStdString(piece->name)).arg(piece->id)
            .arg(piece->parentId ? QString::fromStdString(piece->parentName):QStringLiteral("Archivo importado"))
            .arg(QString::fromStdString(mesh.format))
            .arg(static_cast<qulonglong>(mesh.triangles.size())).arg(vectorText(mesh.bounds.dimensions()),vectorText(mesh.bounds.min),vectorText(mesh.bounds.max),QString::fromStdString(mesh.units));
        text+=QStringLiteral("\nPropiedades de color/material: %1").arg(mesh.materials.size());
        text+=QStringLiteral("\nPintura: %1 caras fuente / %2 regiones, %3 sin color resuelto").arg(mesh.paintedTriangles).arg(mesh.paintFacets.size()).arg(mesh.unresolvedPaintFacets);
        if(mesh.unsupportedAppearance)text+=QStringLiteral("\nHay pintura/atributos no interpretados; separación exacta no disponible.");
        if(!history_.selectedIsActive())text+=QStringLiteral("\nOriginal de referencia: seleccione una pieza activa para cortar.");
        info_->setText(text);
        view_->setCutPlane(plane(),preview_->isChecked());
    }
    void refreshScene() {
        if(history_.empty())return;
        const QSignalBlocker blockPieces(pieces_),bx(planeInputs_[0]),by(planeInputs_[1]),bz(planeInputs_[2]),bd(planeInputs_[3]),bp(preview_);
        const QSignalBlocker bc(connectors_),c0(connectorInputs_[0]),c1(connectorInputs_[1]),c2(connectorInputs_[2]),c3(connectorInputs_[3]);
        const QSignalBlocker pm(paintMode_),pr(paintResolution_),pd(paintDepth_);
        pieces_->clear();
        const auto& state=history_.state();
        connectors_->setChecked(state.connectorEnabled);
        connectorInputs_[0]->setValue(state.connector.diameter);connectorInputs_[1]->setValue(state.connector.depth);
        connectorInputs_[2]->setValue(state.connector.clearance);connectorInputs_[3]->setValue(state.connector.wall);
        paintMode_->setCurrentIndex(state.paintVolume.mode==stlcut::PaintVolumeMode::Inserts?1:0);
        paintResolution_->setValue(state.paintVolume.resolution);paintDepth_->setValue(state.paintVolume.depth);
        paintDepth_->setEnabled(paintMode_->currentIndex()==1);
        const auto root=history_.original();
        const bool rootActive=std::any_of(state.pieces.begin(),state.pieces.end(),[&](const auto& p){return p->id==root->id;});
        if(!rootActive)pieces_->addItem(QStringLiteral("Original (referencia)"),QVariant::fromValue<qulonglong>(root->id));
        for(const auto& p:state.pieces)pieces_->addItem(QString::fromStdString(p->name)+QStringLiteral(" · #%1").arg(p->id),QVariant::fromValue<qulonglong>(p->id));
        pieces_->setCurrentIndex(pieces_->findData(QVariant::fromValue<qulonglong>(state.selected)));
        planeInputs_[0]->setValue(state.plane.normal.x);planeInputs_[1]->setValue(state.plane.normal.y);
        planeInputs_[2]->setValue(state.plane.normal.z);planeInputs_[3]->setValue(state.plane.offset);preview_->setChecked(state.planeVisible);
        // Match any clipping/rounding imposed by the visible spin controls.
        history_.setPlane(plane(),preview_->isChecked());
        showSelectedPiece();updateHistoryActions();
    }
    void selectPiece() {
        if(history_.empty()||worker_||pieces_->currentIndex()<0)return;
        history_.select(pieces_->currentData().toULongLong());
        showSelectedPiece();updateHistoryActions();
    }
    void restoreHistory(bool redo) {
        if(worker_||history_.empty())return;
        if(redo ? history_.redo():history_.undo()) {
            refreshScene();outcome_->setText(redo ? QStringLiteral("Operación rehecha: piezas y parámetros recuperados."):
                QStringLiteral("Operación deshecha: piezas y parámetros recuperados."));
        }
    }
    void startCut() {
        if(worker_||history_.empty()||!history_.selectedIsActive())return;
        history_.setPlane(plane(),preview_->isChecked());
        const auto requestedPlane=plane();
        const bool withConnector=connectors_->isChecked();
        const stlcut::ConnectorSpec connector{connectorInputs_[0]->value(),connectorInputs_[1]->value(),connectorInputs_[2]->value(),connectorInputs_[3]->value()};
        const auto source=history_.selected()->mesh;
        auto result=std::make_shared<stlcut::CutResult>();
        auto views=std::make_shared<std::array<std::shared_ptr<const ViewMesh>,2>>();
        job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([source,result,views,job,requestedPlane,withConnector,connector] {
            try {
                const auto stop=job->cancellation.get_token();
                *result=stlcut::cutByPlane(*source,requestedPlane,stop,withConnector ? &connector:nullptr);
                (*views)[0]=prepareViewMesh(result->positive,stop);
                (*views)[1]=prepareViewMesh(result->negative,stop);
            }catch(const stlcut::ImportCancelled&) {job->cancelled=true;}
            catch(const std::exception& e) {job->error=QString::fromUtf8(e.what());}
        });
        worker_->setParent(this);open_->setEnabled(false);cutControls_->setEnabled(false);updateHistoryActions();
        cancel_->setText(QStringLiteral("Cancelar operación (Ctrl+.)"));cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(QStringLiteral("Cortando y comprobando cierre y conservación de volumen…"));
        connect(worker_,&QThread::finished,this,[this,job,result,views] {
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            if(job->cancelled||job->cancellation.stop_requested())outcome_->setText(QStringLiteral("Corte cancelado. Se conserva el estado anterior."));
            else if(!job->error.isEmpty())outcome_->setText(QStringLiteral("Corte descartado: %1").arg(job->error));
            else {
                try {
                    auto next=history_;
                    const auto commit=next.commitCut(std::make_shared<const stlcut::Mesh>(std::move(result->positive)),
                        std::make_shared<const stlcut::Mesh>(std::move(result->negative)),viewBytes(*(*views)[0]),viewBytes(*(*views)[1]));
                    auto cache=pieceViews_;cache[commit.positive]=(*views)[0];cache[commit.negative]=(*views)[1];
                    const auto retained=next.retainedIds();
                    std::erase_if(cache,[&](const auto& entry){return !std::binary_search(retained.begin(),retained.end(),entry.first);});
                    history_=std::move(next);pieceViews_.swap(cache);refreshScene();
                    outcome_->setText(QStringLiteral("Operación completada (conectores si se solicitaron). Volumen original de la pieza: %1; +: %2; −: %3.\nLas demás piezas se conservan. Seleccione otra pieza para continuar o exportar.\nNo se certifica ausencia de autointersecciones del origen.")
                        .arg(result->originalVolume,0,'g',12).arg(result->positiveVolume,0,'g',12).arg(result->negativeVolume,0,'g',12));
                    if(commit.discardedSteps)outcome_->setText(outcome_->text()+QStringLiteral("\nSe retiraron %1 pasos antiguos por los límites de historial.").arg(commit.discardedSteps));
                }catch(const std::exception& e) {
                    outcome_->setText(QStringLiteral("No se pudo aplicar el corte: %1. Se conserva la sesión anterior.").arg(QString::fromUtf8(e.what())));
                }
            }
            job_.reset();open_->setEnabled(true);cancel_->hide();progress_->hide();updateHistoryActions();
        });
        worker_->start();
    }
    void startFeasibility() {
        if(worker_||history_.empty())return;const auto source=history_.selected()->mesh;
        auto report=std::make_shared<QJsonObject>();job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([source,report,job]{try{*report=feasibilityJson(*source,job->cancellation.get_token());}catch(const stlcut::ImportCancelled&){job->cancelled=true;}catch(const std::exception& e){job->error=QString::fromUtf8(e.what());}});
        worker_->setParent(this);open_->setEnabled(false);updateHistoryActions();cancel_->setText(QStringLiteral("Cancelar diagnóstico (Ctrl+.)"));cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(QStringLiteral("Analizando topología y límites por instancia; la sesión se conserva…"));
        connect(worker_,&QThread::finished,this,[this,job,report]{
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            if(job->cancelled||job->cancellation.stop_requested())outcome_->setText(QStringLiteral("Diagnóstico cancelado; sesión conservada."));
            else if(!job->error.isEmpty())outcome_->setText(QStringLiteral("No se completó el diagnóstico: %1").arg(job->error));
            else {QString text=QStringLiteral("Diagnóstico preliminar por coordenadas exactas; no certifica ausencia de autointersecciones ni repara la malla.");
                for(const auto& value:report->value("instances").toArray()){const auto f=value.toObject();text+=QStringLiteral("\nInstancia %1: %2 caras, %3 facetas de color; %4").arg(f.value("instance").toInteger()).arg(f.value("triangles").toInteger()).arg(f.value("surface_facets").toInteger()).arg(f.value("reason").toString());}outcome_->setText(text);}
            job_.reset();open_->setEnabled(true);cancel_->hide();progress_->hide();updateHistoryActions();
        });worker_->start();
    }
    void startMaterialSplit(bool paint=false) {
        if(worker_||history_.empty()||!history_.selectedIsActive())return;
        const stlcut::PaintVolumeSpec paintSpec{paintMode_->currentIndex()==1?stlcut::PaintVolumeMode::Inserts:stlcut::PaintVolumeMode::Partition,paintResolution_->value(),paintDepth_->value()};
        const auto source=history_.selected()->mesh;
        const bool withConnector=connectors_->isChecked();
        const stlcut::ConnectorSpec connector{connectorInputs_[0]->value(),connectorInputs_[1]->value(),connectorInputs_[2]->value(),connectorInputs_[3]->value()};
        auto meshes=std::make_shared<std::vector<std::shared_ptr<const stlcut::Mesh>>>();
        auto views=std::make_shared<std::vector<std::shared_ptr<const ViewMesh>>>();
        auto connections=std::make_shared<std::vector<stlcut::RegionConnection>>();
        job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([source,meshes,views,connections,job,withConnector,connector,paint,paintSpec] {
            try {
                const auto stop=job->cancellation.get_token();
                auto pieces=paint?stlcut::splitPaintVolume(*source,paintSpec,stop,withConnector?&connector:nullptr,connections.get()):stlcut::splitByMaterial(*source,stop,withConnector ? &connector:nullptr);
                for(auto& mesh:pieces) {
                    views->push_back(prepareViewMesh(mesh,stop));
                    meshes->push_back(std::make_shared<const stlcut::Mesh>(std::move(mesh)));
                }
            }catch(const stlcut::ImportCancelled&) {job->cancelled=true;}
            catch(const std::exception& e) {job->error=QString::fromUtf8(e.what());}
        });
        worker_->setParent(this);open_->setEnabled(false);updateHistoryActions();
        cancel_->setText(QStringLiteral("Cancelar separación (Ctrl+.)"));cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(paint?QStringLiteral("Reconstruyendo volúmenes por color y validando cierre, volumen y exportación…"):QStringLiteral("Separando regiones de material y comprobando cierre…"));
        connect(worker_,&QThread::finished,this,[this,job,meshes,views,connections,paint,paintSpec,withConnector] {
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            if(job->cancelled||job->cancellation.stop_requested())outcome_->setText(QStringLiteral("Separación cancelada. Se conserva la sesión."));
            else if(!job->error.isEmpty())outcome_->setText(QStringLiteral("Separación descartada: %1").arg(job->error));
            else try {
                auto next=history_;std::vector<std::uint64_t> bytes;
                for(const auto& view:*views)bytes.push_back(viewBytes(*view));
                const auto ids=next.commitSplit(*meshes,bytes);
                if(paint)next.setPaintVolume(paintSpec);
                auto cache=pieceViews_;for(size_t i=0;i<ids.size();++i)cache[ids[i]]=(*views)[i];
                const auto retained=next.retainedIds();
                std::erase_if(cache,[&](const auto& entry){return !std::binary_search(retained.begin(),retained.end(),entry.first);});
                history_=std::move(next);pieceViews_.swap(cache);refreshScene();
                const bool connected=!meshes->empty()&&meshes->front()->format=="3mf-material-connected";
                outcome_->setText(QStringLiteral("Separación completada: %1 regiones sólidas. %2\nSTL no conserva color.").arg(ids.size()).arg(connected ? QStringLiteral("Pin y cavidad generados."):QStringLiteral("No se generaron conectores.")));
                if(paint&&withConnector) {
                    const auto generated=std::count_if(connections->begin(),connections->end(),[](const auto& link){return link.connected;});
                    QString text=QStringLiteral("Separación completada: %1 piezas. Conectores: %2 generados; %3 parejas candidatas omitidas.\nNo se certifica el orden global de montaje. STL no conserva color.")
                        .arg(ids.size()).arg(generated).arg(connections->size()-static_cast<size_t>(generated));
                    for(const auto& link:*connections)if(!link.connected)text+=QStringLiteral("\nRegiones %1/%2 sin conector: %3").arg(link.first+1).arg(link.second+1).arg(QString::fromStdString(link.reason));
                    if(connections->empty())text+=QStringLiteral("\nNo hay parejas candidatas en contacto.");outcome_->setText(text);
                }
            }catch(const std::exception& e) {outcome_->setText(QStringLiteral("No se aplicó la separación: %1").arg(QString::fromUtf8(e.what())));}
            job_.reset();open_->setEnabled(true);cancel_->hide();progress_->hide();updateHistoryActions();
        });
        worker_->start();
    }
    void exportPiece() {
        if(worker_||!selectedMesh())return;
        const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("Exportar pieza STL"),{},QStringLiteral("STL binario (*.stl)"));
        if(path.isEmpty())return;
        const auto canonical=QFileInfo(path).canonicalFilePath();
        if(!sourcePath_.isEmpty()&&!canonical.isEmpty()&&canonical.compare(QFileInfo(sourcePath_).canonicalFilePath(),Qt::CaseInsensitive)==0) {
            QMessageBox::warning(this,QStringLiteral("Conservar origen"),QStringLiteral("Elija otro destino para conservar el archivo fuente."));return;
        }
        const auto piece=history_.selected();
        const bool isCutPiece=piece->parentId!=0;
        const auto selected=piece->mesh;
        job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([selected,path,job,isCutPiece] {
            try {
                const auto stop=job->cancellation.get_token();
                const auto bytes=stlcut::validatedStl(*selected,isCutPiece,stop);
                QSaveFile file(path);file.setDirectWriteFallback(false);
                if(!file.open(QIODevice::WriteOnly))throw std::runtime_error(file.errorString().toStdString());
                for(size_t p=0;p<bytes.size();) {
                    if(stop.stop_requested())throw stlcut::ImportCancelled{};
                    const auto count=std::min<size_t>(65536,bytes.size()-p);
                    if(file.write(bytes.data()+p,static_cast<qint64>(count))!=static_cast<qint64>(count))throw std::runtime_error("Escritura STL incompleta.");
                    p+=count;
                }
                if(stop.stop_requested())throw stlcut::ImportCancelled{};
                if(!file.commit())throw std::runtime_error(file.errorString().toStdString());
            }catch(const stlcut::ImportCancelled&) {job->cancelled=true;}
            catch(const std::exception& e) {job->error=QString::fromUtf8(e.what());}
        });
        worker_->setParent(this);open_->setEnabled(false);cutControls_->setEnabled(false);updateHistoryActions();
        cancel_->setText(QStringLiteral("Cancelar exportación (Ctrl+.)"));
        cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(QStringLiteral("Exportando STL y comprobando reimportación…"));
        connect(worker_,&QThread::finished,this,[this,job,path] {
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            outcome_->setText(job->cancelled ? QStringLiteral("Exportación cancelada antes de guardar."):
                !job->error.isEmpty() ? QStringLiteral("No se pudo exportar: %1").arg(job->error):
                QStringLiteral("STL exportado y reimportado para comprobar triángulos y volumen: %1\nSTL no guarda unidades, colores ni materiales.").arg(path));
            job_.reset();open_->setEnabled(true);cancel_->hide();progress_->hide();updateHistoryActions();
        });
        worker_->start();
    }
    QString sourcePath_;
    QPushButton* splitMaterials_{};
    QPushButton* splitPaint_{};
    QComboBox* paintMode_{};
    QDoubleSpinBox* paintResolution_{};
    QDoubleSpinBox* paintDepth_{};
    QCheckBox* connectors_{};
    QDoubleSpinBox* connectorInputs_[4]{};
    stlcut::EditHistory history_;
    std::map<stlcut::PieceId,std::shared_ptr<const ViewMesh>> pieceViews_;
    QAction* undo_{};
    QAction* redo_{};
    QPushButton* cutButton_{};
    QLabel* historyStatus_{};
    QWidget* cutControls_{};
    std::array<QDoubleSpinBox*,4> planeInputs_{};
    QCheckBox* preview_{};
    QComboBox* pieces_{};
    std::shared_ptr<ImportJob> job_;
    QThread* worker_{};
    QLabel* info_{};
    QLabel* outcome_{};
    QProgressBar* progress_{};
    QPushButton* cancel_{};
    QAction* open_{};
    MeshView* view_{};
    QLabel* viewerStatus_{};
};
}

int main(int argc, char* argv[])
{
    if (argc > 1 && (std::strcmp(argv[1], "--inspect-stdin") == 0 || std::strcmp(argv[1], "--inspect-view-stdin") == 0 || std::strcmp(argv[1], "--inspect-feasibility-stdin") == 0 || std::strcmp(argv[1], "--inspect-spatial-stdin") == 0 ||
                     std::strcmp(argv[1], "--decode-paint") == 0 || std::strcmp(argv[1], "--decode-paint-stdin") == 0 || std::strcmp(argv[1], "--inspect") == 0 || std::strcmp(argv[1], "--cut-stdin") == 0 || std::strcmp(argv[1], "--edit-stdin") == 0)) {
        return inspect(argc, argv);
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("STL Cut Studio AI"));
    QApplication::setApplicationVersion(QStringLiteral(STL_CUT_STUDIO_VERSION));
    QApplication::setOrganizationName(QStringLiteral("STLCutStudioAI"));
    MainWindow window;
    window.show();
    const auto args = app.arguments();
    if (args.size() == 2) {
        if (args[1]==QStringLiteral("--view-stdin")) {
            window.importFile(QStringLiteral("Entrada estándar"),true);
        } else { window.importFile(args[1]); }
    }
    return app.exec();
}
