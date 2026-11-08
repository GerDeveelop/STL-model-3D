#include "core/io/StlReader.h"
#include "core/io/MeshReader.h"
#include "core/io/StlWriter.h"
#include "core/cutting/PlaneCut.h"
#include "MeshView.h"

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
    return stlcut::readMesh(std::span(reinterpret_cast<const unsigned char*>(data.data()),data.size()),stop);
}
QJsonArray vectorJson(const stlcut::Vec3& p) { return {p.x, p.y, p.z}; }
QJsonObject summary(const stlcut::Mesh& mesh)
{
    QJsonArray warnings;
    for(const auto& warning:mesh.warnings)warnings.append(QString::fromStdString(warning));
    return {{"format", QString::fromStdString(mesh.format)},
            {"triangles", static_cast<qint64>(mesh.triangles.size())},
            {"degenerate_triangles", static_cast<qint64>(mesh.degenerateTriangles)},
            {"min", vectorJson(mesh.bounds.min)}, {"max", vectorJson(mesh.bounds.max)},
            {"dimensions", vectorJson(mesh.bounds.dimensions())},
            {"units", QString::fromStdString(mesh.units)},
            {"signed_volume", stlcut::signedVolume(mesh)}, {"warnings",warnings}};
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
        if (args.size() == 2 && args[1] == QStringLiteral("--inspect-stdin")) {
            mesh = readStdin();
        } else if (args.size() == 3 && args[1] == QStringLiteral("--inspect")) {
            mesh = stlcut::readMesh(std::filesystem::path(args[2].toStdWString()));
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
            stlcut::writeStl(std::cout,args[6]=="positive" ? cut.positive:cut.negative);
            return 0;
        } else {
            throw std::runtime_error("Uso: --inspect <modelo.stl|3mf>, --inspect-stdin o --cut-stdin nx ny nz offset positive|negative");
        }
        result = summary(mesh);
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
        auto* cutButton=new QPushButton(QStringLiteral("Cortar original en dos piezas"),cutting);
        form->addRow(cutButton);connect(cutButton,&QPushButton::clicked,this,[this]{startCut();});
        pieces_=new QComboBox(cutting);
        pieces_->addItems({QStringLiteral("Original"),QStringLiteral("Parte positiva (+)"),QStringLiteral("Parte negativa (−)")});
        form->addRow(QStringLiteral("Mostrar / exportar"),pieces_);
        connect(pieces_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int){selectPiece();});
        auto* exportButton=new QPushButton(QStringLiteral("Exportar pieza visible a STL…"),cutting);
        form->addRow(exportButton);connect(exportButton,&QPushButton::clicked,this,[this]{exportPiece();});
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
        auto* viewMenu=menuBar()->addMenu(QStringLiteral("&Vista"));
        auto* toolbar=addToolBar(QStringLiteral("Vista"));
        toolbar->setMovable(false);
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
                }) : stlcut::readMesh(
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
                    {"retained_triangles",mesh_ ? static_cast<qint64>(mesh_->triangles.size()) : 0}
                }).toJson(QJsonDocument::Compact);
            } else if (!job->error.isEmpty()) {
                outcome_->setText(QStringLiteral("No se pudo importar: %1\nSe conserva el modelo anterior.")
                                      .arg(job->error));
                qInfo().noquote() << "STL_IMPORT_ERROR_METRIC retained_triangles="
                                  << (mesh_ ? mesh_->triangles.size() : 0);
            } else {
                qInfo().noquote() << "STL_IMPORT_METRIC" << QJsonDocument(QJsonObject{
                    {"triangles",static_cast<qint64>(job->mesh->triangles.size())},
                    {"input_ms",job->inputMs},{"presentation_ms",job->presentationMs},
                    {"presentation_bytes",static_cast<qint64>(job->view->vertices.size()*sizeof(ViewVertex))}
                }).toJson(QJsonDocument::Compact);
                mesh_ = std::move(job->mesh);
                sourcePath_=path;
                cutResult_.reset();
                originalView_=std::move(job->view);
                pieces_->setCurrentIndex(0);
                view_->setMesh(originalView_);
                const auto centerZ=std::midpoint(mesh_->bounds.min.z,mesh_->bounds.max.z);
                planeInputs_[0]->setValue(0);planeInputs_[1]->setValue(0);planeInputs_[2]->setValue(1);
                planeInputs_[3]->setValue(centerZ);
                updatePlane();
                const auto& mesh = *mesh_;
                info_->setText(QStringLiteral("Archivo: %1\nFormato: %2\nTriángulos: %3\n"
                                              "Dimensiones: %4\nCaja mínima: %5\nCaja máxima: %6\n\n"
                                              "Unidades: %7 (3MF convertido a mm; STL sin unidad definida).")
                                   .arg(path, QString::fromStdString(mesh.format))
                                   .arg(static_cast<qulonglong>(mesh.triangles.size()))
                                   .arg(vectorText(mesh.bounds.dimensions()), vectorText(mesh.bounds.min),
                                        vectorText(mesh.bounds.max)).arg(QString::fromStdString(mesh.units)));
                outcome_->setText(QStringLiteral("Importación completada. Triángulos degenerados: %1.\n"
                                                 "Se conservan todos los triángulos. No se verificó cierre, "
                                                 "conectividad ni autointersecciones. Normales de archivo ignoradas.")
                                      .arg(static_cast<qulonglong>(mesh.degenerateTriangles)));
                for(const auto& warning:mesh.warnings)outcome_->setText(outcome_->text()+"\n"+QString::fromStdString(warning));
            }
            statusBar()->showMessage(mesh_ ? QStringLiteral("Modelo cargado · Unidades: %1").arg(QString::fromStdString(mesh_->units))
                                          : QStringLiteral("Sin modelo cargado"));
            job_.reset();
            cutControls_->setEnabled(bool(mesh_));
        });
        worker_->start();
    }
private:
    stlcut::Plane plane() const {return {{planeInputs_[0]->value(),planeInputs_[1]->value(),planeInputs_[2]->value()},planeInputs_[3]->value()};}
    void updatePlane() {if(view_)view_->setCutPlane(plane(),preview_->isChecked());}
    const stlcut::Mesh* selectedMesh() const {
        if(cutResult_&&pieces_->currentIndex()==1)return &cutResult_->positive;
        if(cutResult_&&pieces_->currentIndex()==2)return &cutResult_->negative;
        return mesh_.get();
    }
    void selectPiece() {
        if(!mesh_||worker_)return;
        const auto index=pieces_->currentIndex();
        if(index&& !cutResult_) {pieces_->setCurrentIndex(0);return;}
        view_->setMesh(index==0 ? originalView_:index==1 ? positiveView_:negativeView_);
        const auto* m=selectedMesh();
        info_->setText(QStringLiteral("%1\nTriángulos: %2\nDimensiones: %3\nUnidades: %4")
            .arg(pieces_->currentText()).arg(static_cast<qulonglong>(m->triangles.size()))
            .arg(vectorText(m->bounds.dimensions()),QString::fromStdString(m->units)));
        updatePlane();
    }
    void startCut() {
        if(worker_||!mesh_)return;
        const auto requestedPlane=plane();
        const auto source=mesh_;
        auto result=std::make_shared<stlcut::CutResult>();
        auto views=std::make_shared<std::array<std::shared_ptr<const ViewMesh>,2>>();
        job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([source,result,views,job,requestedPlane] {
            try {
                const auto stop=job->cancellation.get_token();
                *result=stlcut::cutByPlane(*source,requestedPlane,stop);
                (*views)[0]=prepareViewMesh(result->positive,stop);
                (*views)[1]=prepareViewMesh(result->negative,stop);
            }catch(const stlcut::ImportCancelled&) {job->cancelled=true;}
            catch(const std::exception& e) {job->error=QString::fromUtf8(e.what());}
        });
        worker_->setParent(this);open_->setEnabled(false);cutControls_->setEnabled(false);
        cancel_->setText(QStringLiteral("Cancelar operación (Ctrl+.)"));cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(QStringLiteral("Cortando y comprobando cierre y conservación de volumen…"));
        connect(worker_,&QThread::finished,this,[this,job,result,views] {
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            if(job->cancelled||job->cancellation.stop_requested())outcome_->setText(QStringLiteral("Corte cancelado. Se conserva el estado anterior."));
            else if(!job->error.isEmpty())outcome_->setText(QStringLiteral("Corte descartado: %1").arg(job->error));
            else {
                cutResult_=result;positiveView_=(*views)[0];negativeView_=(*views)[1];
                pieces_->setCurrentIndex(1);selectPiece();
                outcome_->setText(QStringLiteral("Corte completado. Volumen original: %1; +: %2; −: %3.\nTolerancia espacial: %4. Seleccione cada pieza para exportarla.\nNo se certifica ausencia de autointersecciones de la malla original.")
                    .arg(result->originalVolume,0,'g',12).arg(result->positiveVolume,0,'g',12).arg(result->negativeVolume,0,'g',12).arg(result->tolerance,0,'g',6));
            }
            job_.reset();open_->setEnabled(true);cutControls_->setEnabled(true);cancel_->hide();progress_->hide();
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
        const bool isCutPiece=bool(cutResult_)&&pieces_->currentIndex()!=0;
        const std::shared_ptr<const void> owner=isCutPiece ? std::shared_ptr<const void>(cutResult_):std::shared_ptr<const void>(mesh_);
        auto selected=std::shared_ptr<const stlcut::Mesh>(owner,selectedMesh());
        job_=std::make_shared<ImportJob>();const auto job=job_;
        worker_=QThread::create([selected,path,job,isCutPiece] {
            try {
                const auto stop=job->cancellation.get_token();
                std::ostringstream out(std::ios::binary);stlcut::writeStl(out,*selected,stop);
                const auto bytes=out.str();
                const auto roundTrip=stlcut::readMesh(std::span(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size()),stop);
                if(roundTrip.triangles.size()!=selected->triangles.size())throw std::runtime_error("Reimportacion STL inconsistente.");
                const auto original=stlcut::signedVolume(*selected),volume=stlcut::signedVolume(roundTrip);
                const auto dims=selected->bounds.dimensions();const auto extent=std::max({dims.x,dims.y,dims.z});
                for(size_t i=0;i<selected->triangles.size();++i) {
                    if(stop.stop_requested())throw stlcut::ImportCancelled{};
                    for(size_t j=0;j<3;++j) {
                        const auto a=selected->triangles[i].vertices[j],b=roundTrip.triangles[i].vertices[j];
                        if(std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)>extent*1e-6)
                            throw std::runtime_error("La precision float32 de STL altera demasiado las coordenadas.");
                    }
                }
                if(isCutPiece)stlcut::verifyClosedMesh(roundTrip,extent*1e-9,stop);
                if(!std::isfinite(volume)||std::abs(volume-original)>std::max(std::abs(original)*1e-5,extent*extent*extent*1e-8))
                    throw std::runtime_error("La precision float32 de STL altera demasiado el volumen; exportacion descartada.");
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
        worker_->setParent(this);open_->setEnabled(false);cutControls_->setEnabled(false);
        cancel_->setText(QStringLiteral("Cancelar exportación (Ctrl+.)"));
        cancel_->setEnabled(true);cancel_->show();progress_->show();
        outcome_->setText(QStringLiteral("Exportando STL y comprobando reimportación…"));
        connect(worker_,&QThread::finished,this,[this,job,path] {
            worker_->wait();worker_->deleteLater();worker_=nullptr;
            outcome_->setText(job->cancelled ? QStringLiteral("Exportación cancelada antes de guardar."):
                !job->error.isEmpty() ? QStringLiteral("No se pudo exportar: %1").arg(job->error):
                QStringLiteral("STL exportado y reimportado para comprobar triángulos y volumen: %1\nSTL no guarda unidades, colores ni materiales.").arg(path));
            job_.reset();open_->setEnabled(true);cutControls_->setEnabled(true);cancel_->hide();progress_->hide();
        });
        worker_->start();
    }
    QString sourcePath_;
    std::shared_ptr<stlcut::CutResult> cutResult_;
    std::shared_ptr<const ViewMesh> originalView_,positiveView_,negativeView_;
    QWidget* cutControls_{};
    std::array<QDoubleSpinBox*,4> planeInputs_{};
    QCheckBox* preview_{};
    QComboBox* pieces_{};
    std::shared_ptr<stlcut::Mesh> mesh_;
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
    if (argc > 1 && (std::strcmp(argv[1], "--inspect-stdin") == 0 ||
                     std::strcmp(argv[1], "--inspect") == 0 || std::strcmp(argv[1], "--cut-stdin") == 0)) {
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
