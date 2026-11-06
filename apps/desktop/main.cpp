#include "core/io/StlReader.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QThread>
#include <QVBoxLayout>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stop_token>

namespace {
QJsonArray vectorJson(const stlcut::Vec3& p) { return {p.x, p.y, p.z}; }
QJsonObject summary(const stlcut::Mesh& mesh)
{
    return {{"format", QString::fromStdString(mesh.format)},
            {"triangles", static_cast<qint64>(mesh.triangles.size())},
            {"degenerate_triangles", static_cast<qint64>(mesh.degenerateTriangles)},
            {"min", vectorJson(mesh.bounds.min)}, {"max", vectorJson(mesh.bounds.max)},
            {"dimensions", vectorJson(mesh.bounds.dimensions())},
            {"units", QStringLiteral("unspecified")}};
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
            if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
                throw std::runtime_error("No se pudo configurar la entrada binaria.");
            }
            std::string data;
            std::array<char, 65536> block;
            while (std::cin.read(block.data(), block.size()) || std::cin.gcount()) {
                const auto bytes = static_cast<std::size_t>(std::cin.gcount());
                if (data.size() + bytes > stlcut::maxFileBytes) {
                    throw std::runtime_error("El archivo STL supera el límite de 256 MiB.");
                }
                data.append(block.data(), bytes);
            }
            if (std::cin.bad()) { throw std::runtime_error("Error leyendo stdin."); }
            std::istringstream input(std::move(data), std::ios::in | std::ios::binary);
            input.seekg(0, std::ios::end);
            const auto size = input.tellg();
            input.seekg(0);
            mesh = stlcut::readStl(input, static_cast<std::uint64_t>(size));
        } else if (args.size() == 3 && args[1] == QStringLiteral("--inspect")) {
            mesh = stlcut::readStl(std::filesystem::path(args[2].toStdWString()));
        } else {
            throw std::runtime_error("Uso: --inspect <archivo.stl> o --inspect-stdin");
        }
        result = summary(mesh);
        result.insert("ok", true);
    } catch (const std::exception& error) {
        result = {{"ok", false}, {"error", QString::fromUtf8(error.what())}};
        code = 1;
    }
    std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << '\n';
    return code;
}

struct ImportJob {
    std::stop_source cancellation;
    std::shared_ptr<stlcut::Mesh> mesh;
    QString error;
    bool cancelled{};
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
        resize(1100, 720);
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        info_ = new QLabel(QStringLiteral("Abra un STL para consultar su geometría.\n"
                                          "El visor 3D está pendiente de implementación."), central);
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
        cancel_ = new QPushButton(QStringLiteral("Cancelar importación"), central);
        cancel_->hide();
        layout->addWidget(info_);
        layout->addWidget(outcome_);
        layout->addStretch();
        layout->addWidget(progress_);
        layout->addWidget(cancel_);
        setCentralWidget(central);

        auto* fileMenu = menuBar()->addMenu(QStringLiteral("&Archivo"));
        open_ = fileMenu->addAction(QStringLiteral("Abrir &STL…"));
        open_->setShortcut(QKeySequence::Open);
        connect(open_, &QAction::triggered, this, [this] {
            const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Abrir STL"),
                                                         {}, QStringLiteral("STL (*.stl);;Todos los archivos (*)"));
            if (!path.isEmpty()) { importFile(path); }
        });
        fileMenu->addSeparator();
        auto* quit = fileMenu->addAction(QStringLiteral("&Salir"));
        quit->setShortcut(QKeySequence::Quit);
        connect(quit, &QAction::triggered, this, &QMainWindow::close);
        auto* help = menuBar()->addMenu(QStringLiteral("A&yuda"));
        auto* about = help->addAction(QStringLiteral("&Acerca de…"));
        connect(about, &QAction::triggered, this, [this] {
            QMessageBox::about(this, QStringLiteral("STL Cut Studio AI"),
                               QStringLiteral("Versión %1\nImportación STL ASCII/binaria y estadísticas.\n"
                                              "Visor 3D pendiente.").arg(QApplication::applicationVersion()));
        });
        connect(cancel_, &QPushButton::clicked, this, [this] {
            if (job_) { job_->cancellation.request_stop(); }
            cancel_->setEnabled(false);
            outcome_->setText(QStringLiteral("Cancelando importación…"));
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

    void importFile(const QString& path)
    {
        if (worker_) { return; }
        job_ = std::make_shared<ImportJob>();
        const auto job = job_;
        worker_ = QThread::create([job, path] {
            try {
                job->mesh = std::make_shared<stlcut::Mesh>(stlcut::readStl(
                    std::filesystem::path(path.toStdWString()), job->cancellation.get_token()));
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
            cancel_->hide();
            if (job->cancelled || job->cancellation.stop_requested()) {
                outcome_->setText(QStringLiteral("Importación cancelada. Se conserva el modelo anterior."));
            } else if (!job->error.isEmpty()) {
                outcome_->setText(QStringLiteral("No se pudo importar: %1\nSe conserva el modelo anterior.")
                                      .arg(job->error));
            } else {
                mesh_ = std::move(job->mesh);
                const auto& mesh = *mesh_;
                info_->setText(QStringLiteral("Archivo: %1\nFormato: %2\nTriángulos: %3\n"
                                              "Dimensiones: %4\nCaja mínima: %5\nCaja máxima: %6\n\n"
                                              "Unidades no especificadas por STL; escala original sin conversión.\n"
                                              "Visor 3D pendiente de implementación.")
                                   .arg(path, QString::fromStdString(mesh.format))
                                   .arg(static_cast<qulonglong>(mesh.triangles.size()))
                                   .arg(vectorText(mesh.bounds.dimensions()), vectorText(mesh.bounds.min),
                                        vectorText(mesh.bounds.max)));
                outcome_->setText(QStringLiteral("Importación completada. Triángulos degenerados: %1.\n"
                                                 "Se conservan todos los triángulos. No se verificó cierre, "
                                                 "conectividad ni autointersecciones. Normales de archivo ignoradas.")
                                      .arg(static_cast<qulonglong>(mesh.degenerateTriangles)));
            }
            statusBar()->showMessage(mesh_ ? QStringLiteral("Modelo cargado · Unidades no especificadas")
                                          : QStringLiteral("Sin modelo cargado"));
            job_.reset();
        });
        worker_->start();
    }
private:
    std::shared_ptr<stlcut::Mesh> mesh_;
    std::shared_ptr<ImportJob> job_;
    QThread* worker_{};
    QLabel* info_{};
    QLabel* outcome_{};
    QProgressBar* progress_{};
    QPushButton* cancel_{};
    QAction* open_{};
};
}

int main(int argc, char* argv[])
{
    if (argc > 1 && (std::strcmp(argv[1], "--inspect-stdin") == 0 ||
                     std::strcmp(argv[1], "--inspect") == 0)) {
        return inspect(argc, argv);
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("STL Cut Studio AI"));
    QApplication::setApplicationVersion(QStringLiteral(STL_CUT_STUDIO_VERSION));
    QApplication::setOrganizationName(QStringLiteral("STLCutStudioAI"));
    MainWindow window;
    window.show();
    const auto args = app.arguments();
    if (args.size() == 2) { window.importFile(args[1]); }
    return app.exec();
}
