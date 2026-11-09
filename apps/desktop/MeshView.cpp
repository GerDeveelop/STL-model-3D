#include "MeshView.h"
#include "core/io/StlReader.h"

#include <QMatrix4x4>
#include <QDebug>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>
#include <QShowEvent>
#include <QSurfaceFormat>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <numeric>
#include <stdexcept>

namespace {
constexpr float radians = std::numbers::pi_v<float> / 180.f;
constexpr float halfFov = 22.5f * radians;
ViewVertex vertex(const QVector3D& p, const QVector3D& n = {})
{ return {p.x(), p.y(), p.z(), n.x(), n.y(), n.z()}; }
}

std::shared_ptr<const ViewMesh> prepareViewMesh(const stlcut::Mesh& mesh, std::stop_token stop)
{
    if (stop.stop_requested()) { throw stlcut::ImportCancelled{}; }
    auto result = std::make_shared<ViewMesh>();
    const auto d = mesh.bounds.dimensions();
    const double extent = std::max({d.x, d.y, d.z});
    const double scale = extent > 0 ? extent : 1;
    const stlcut::Vec3 center{std::midpoint(mesh.bounds.min.x, mesh.bounds.max.x),
                              std::midpoint(mesh.bounds.min.y, mesh.bounds.max.y),
                              std::midpoint(mesh.bounds.min.z, mesh.bounds.max.z)};
    result->radius = std::max(0.01f, static_cast<float>(std::hypot(d.x/scale, d.y/scale, d.z/scale)));
    result->floor = static_cast<float>((mesh.bounds.min.z - center.z) / scale * 2);
    result->center = center;
    result->scale = scale;
    if (stop.stop_requested()) { throw stlcut::ImportCancelled{}; }
    if(mesh.paintedTriangles>mesh.triangles.size()||mesh.paintFacets.size()<mesh.paintedTriangles)
        throw std::runtime_error("Metadatos de pintura inconsistentes.");
    const auto displayCount=mesh.triangles.size()+mesh.paintFacets.size()-mesh.paintedTriangles;
    if(displayCount>stlcut::maxTriangles)throw std::runtime_error("Presentacion de pintura supera 2 millones de caras.");
    result->vertices.reserve(displayCount*3);
    auto emitTriangle=[&](const stlcut::Triangle& triangle,const std::array<std::int32_t,3>& labels,bool unresolved=false) {
        if(stop.stop_requested())throw stlcut::ImportCancelled{};
        QVector3D p[3];
        for(size_t i=0;i<3;++i) {
            const auto& v=triangle.vertices[i];
            p[i]={static_cast<float>((v.x-center.x)/scale*2),static_cast<float>((v.y-center.y)/scale*2),static_cast<float>((v.z-center.z)/scale*2)};
        }
        const auto normal=QVector3D::crossProduct(p[1]-p[0],p[2]-p[0]).normalized();
        for(size_t j=0;j<3;++j) {
            auto item=vertex(p[j],normal);const auto label=labels[j];
            if(label>=0&&static_cast<size_t>(label)<mesh.materials.size()) {
                const auto& rgba=mesh.materials[label].rgba;item.r=rgba[0]/255.f;item.g=rgba[1]/255.f;item.b=rgba[2]/255.f;
            } else if(unresolved)item.r=item.g=item.b=.5f;
            result->vertices.push_back(item);
        }
    };
    size_t paintIndex=0,painted=0;
    for(size_t i=0;i<mesh.triangles.size();++i) {
        const auto& source=mesh.triangles[i];
        if(paintIndex<mesh.paintFacets.size()&&mesh.paintFacets[paintIndex].sourceTriangle<i)
            throw std::runtime_error("Pintura fuera de orden respecto a la fuente.");
        if(paintIndex<mesh.paintFacets.size()&&mesh.paintFacets[paintIndex].sourceTriangle==i) {
            ++painted;
            while(paintIndex<mesh.paintFacets.size()&&mesh.paintFacets[paintIndex].sourceTriangle==i) {
                const auto& facet=mesh.paintFacets[paintIndex++];stlcut::Triangle triangle;
                for(size_t j=0;j<3;++j) {
                    const auto w=facet.barycentric.vertices[j];const auto a=source.vertices[0],b=source.vertices[1],c=source.vertices[2];
                    triangle.vertices[j]={w.x*a.x+w.y*b.x+w.z*c.x,w.x*a.y+w.y*b.y+w.z*c.y,w.x*a.z+w.y*b.z+w.z*c.z};
                }
                emitTriangle(triangle,{facet.material,facet.material,facet.material},facet.material<0);
            }
        } else emitTriangle(source,mesh.styles.size()==mesh.triangles.size()?mesh.styles[i].material:std::array<std::int32_t,3>{-1,-1,-1});
    }
    if(paintIndex!=mesh.paintFacets.size()||painted!=mesh.paintedTriangles)throw std::runtime_error("Pintura no coincide con la fuente de presentacion.");
    if (stop.stop_requested()) { throw stlcut::ImportCancelled{}; }
    return result;
}

MeshView::MeshView(QWidget* parent) : QOpenGLWidget(parent)
{
    setObjectName(QStringLiteral("meshViewer"));
    setAccessibleName(QStringLiteral("Visor 3D"));
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    setFormat(format);
    setToolTip(QStringLiteral("Izquierdo: orbitar · Derecho/central: desplazar · Rueda: zoom · F: encuadrar"));
}

MeshView::~MeshView()
{
    if (context()) {
        disconnect(context(), nullptr, this, nullptr);
        releaseGL();
    }
}

void MeshView::releaseGL()
{
    makeCurrent();
    meshBuffer_.destroy();
    linesBuffer_.destroy();
    vao_.destroy();
    program_.removeAllShaders();
    ready_ = false;
    meshDrawable_ = false;
    doneCurrent();
}

void MeshView::fail(const QString& message)
{
    failure_ = message;
    emit rendererStatus(message);
}

void MeshView::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    QTimer::singleShot(500, this, [this] {
        if (!isValid()) {
            fail(QStringLiteral("No se pudo iniciar OpenGL 3.3. Las estadísticas STL siguen disponibles."));
        }
    });
}

void MeshView::initializeGL()
{
    initializeOpenGLFunctions();
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &MeshView::releaseGL,
            Qt::DirectConnection);
    failure_.clear();
    const char* vertexShader = R"(
        #version 330 core
        layout(location=0) in vec3 position;
        layout(location=1) in vec3 normal;
        layout(location=2) in vec3 vertexColor;
        uniform mat4 mvp;
        out vec3 faceNormal;
        out vec3 surfaceColor;
        void main() { gl_Position = mvp * vec4(position,1.0); faceNormal=normal; surfaceColor=vertexColor; }
    )";
    const char* fragmentShader = R"(
        #version 330 core
        in vec3 faceNormal;
        in vec3 surfaceColor;
        uniform vec3 color;
        uniform bool lighting;
        out vec4 fragColor;
        void main() {
            float shade=1.0;
            if(lighting && length(faceNormal)>0.00001) {
                vec3 n=normalize(faceNormal);
                if(!gl_FrontFacing) n=-n;
                shade=0.30+0.70*max(dot(n,normalize(vec3(0.4,-0.6,1.0))),0.0);
            }
            fragColor=vec4((lighting ? surfaceColor : color)*shade,1.0);
        }
    )";
    if (!program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader) ||
        !program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader) ||
        !program_.link() || !meshBuffer_.create() || !linesBuffer_.create() || !vao_.create()) {
        fail(QStringLiteral("No se pudo preparar el visor OpenGL: %1").arg(program_.log()));
        return;
    }
    meshBuffer_.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ready_ = true;
    uploadPending_ = true;
    emit rendererStatus(QStringLiteral("OpenGL %1 · %2")
                            .arg(QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_VERSION))),
                                 QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_RENDERER)))));
}

void MeshView::setMesh(std::shared_ptr<const ViewMesh> mesh)
{
    mesh_ = std::move(mesh);
    meshDrawable_=false;
    uploadPending_ = true;
    resetCamera();
}

void MeshView::updateFitDistance()
{
    const float radius = mesh_ ? mesh_->radius : 1.75f;
    const float aspect = static_cast<float>(std::max(1, width())) / std::max(1, height());
    const float angle = std::min(halfFov, std::atan(std::tan(halfFov) * aspect));
    distance_ = radius / std::sin(angle) * 1.15f;
}

void MeshView::fitMesh()
{
    target_ = {};
    autoFit_ = true;
    updateFitDistance();
    update();
}

void MeshView::resetCamera()
{ azimuth_=45; elevation_=30; fitMesh(); }
void MeshView::showGrid(bool show) { grid_=show; update(); }
void MeshView::showAxes(bool show) { axes_=show; update(); }
void MeshView::setCutPlane(stlcut::Plane plane, bool visible)
{ cutPlane_=plane; cutPlaneVisible_=visible; update(); }
void MeshView::resizeGL(int, int) { if (autoFit_) { updateFitDistance(); } }

QVector3D MeshView::eyeDirection() const
{
    const float a=azimuth_*radians, e=elevation_*radians;
    return {std::cos(e)*std::cos(a), std::cos(e)*std::sin(a), std::sin(e)};
}

void MeshView::paintGL()
{
    glClearColor(0.065f, 0.085f, 0.12f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!ready_) { return; }
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Show both sides of open/inconsistently oriented meshes.
    QMatrix4x4 projection;
    const float aspect=static_cast<float>(std::max(1,width()))/std::max(1,height());
    const float radius=mesh_ ? mesh_->radius : 1.75f;
    projection.perspective(45, aspect, std::max(0.0001f,radius*0.001f),
                           distance_ + target_.length() + radius*20 + 10);
    QMatrix4x4 view;
    view.lookAt(target_+eyeDirection()*distance_, target_, {0,0,1});
    program_.bind();
    program_.setUniformValue("mvp", projection*view);
    QOpenGLVertexArrayObject::Binder vao(&vao_);
    auto attributes = [this] {
        program_.enableAttributeArray(0);
        program_.enableAttributeArray(1);
        program_.enableAttributeArray(2);
        program_.setAttributeBuffer(0, GL_FLOAT, offsetof(ViewVertex,x), 3, sizeof(ViewVertex));
        program_.setAttributeBuffer(1, GL_FLOAT, offsetof(ViewVertex,nx), 3, sizeof(ViewVertex));
        program_.setAttributeBuffer(2, GL_FLOAT, offsetof(ViewVertex,r), 3, sizeof(ViewVertex));
    };
    if (mesh_ && !mesh_->vertices.empty()) {
        meshBuffer_.bind();
        if (uploadPending_) {
            meshDrawable_=false;
            while (glGetError() != GL_NO_ERROR) {}
            const auto bytes=mesh_->vertices.size()*sizeof(ViewVertex);
            if (bytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                fail(QStringLiteral("La malla supera el tamaño de buffer admitido por el visor."));
            } else {
                QElapsedTimer uploadTimer;
                uploadTimer.start();
                meshBuffer_.allocate(mesh_->vertices.data(), static_cast<int>(bytes));
                if (glGetError() != GL_NO_ERROR) {
                    fail(QStringLiteral("No hay memoria gráfica suficiente para dibujar esta malla."));
                } else {
                    meshDrawable_=true;
                    qInfo().noquote() << "STL_GPU_UPLOAD_METRIC bytes=" << bytes
                                      << "submit_ms=" << static_cast<double>(uploadTimer.nsecsElapsed())/1e6;
                    if (!failure_.isEmpty()) {
                        failure_.clear();
                        emit rendererStatus(QStringLiteral("Visor OpenGL disponible"));
                    }
                }
            }
            uploadPending_=false;
        }
        if (meshDrawable_) {
            attributes();
            program_.setUniformValue("lighting", true);
            program_.setUniformValue("color", QVector3D(0.28f,0.72f,0.91f));
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1,1);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh_->vertices.size()));
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        meshBuffer_.release();
    }
    const float floor=mesh_ ? mesh_->floor : 0;
    std::vector<ViewVertex> lines;
    if (grid_) {
        for (int i=-10; i<=10; ++i) {
            const float p=static_cast<float>(i)*0.25f;
            lines.push_back(vertex({-2.5f,p,floor})); lines.push_back(vertex({2.5f,p,floor}));
            lines.push_back(vertex({p,-2.5f,floor})); lines.push_back(vertex({p,2.5f,floor}));
        }
    }
    const auto gridCount=static_cast<GLsizei>(lines.size());
    if (axes_) {
        lines.push_back(vertex({0,0,floor})); lines.push_back(vertex({1.5f,0,floor}));
        lines.push_back(vertex({0,0,floor})); lines.push_back(vertex({0,1.5f,floor}));
        lines.push_back(vertex({0,0,floor})); lines.push_back(vertex({0,0,floor+1.5f}));
    }
    const auto baseCount=static_cast<GLsizei>(lines.size());
    if(cutPlaneVisible_ && mesh_) {
        const auto p=cutPlane_;
        const double length=std::hypot(p.normal.x,p.normal.y,p.normal.z);
        if(std::isfinite(length)&&length>1e-15) {
            const QVector3D n(static_cast<float>(p.normal.x/length),static_cast<float>(p.normal.y/length),static_cast<float>(p.normal.z/length));
            const auto c=mesh_->center;
            const double offset=(p.offset/length-(p.normal.x*c.x+p.normal.y*c.y+p.normal.z*c.z)/length)/mesh_->scale*2;
            if(std::isfinite(offset)&&std::abs(offset)<1e6) {
                const auto u=QVector3D::crossProduct(n,std::abs(n.x())<.8f?QVector3D(1,0,0):QVector3D(0,1,0)).normalized()*mesh_->radius*1.2f;
                const auto v=QVector3D::crossProduct(n,u);
                const auto center=n*static_cast<float>(offset);
                const QVector3D corners[]={center-u-v,center+u-v,center+u+v,center-u+v};
                for(int i=0;i<4;++i) {lines.push_back(vertex(corners[i]));lines.push_back(vertex(corners[(i+1)%4]));}
                lines.push_back(vertex(center));lines.push_back(vertex(center+n*mesh_->radius*.4f));
            }
        }
    }
    if (!lines.empty()) {
        linesBuffer_.bind();
        linesBuffer_.allocate(lines.data(), static_cast<int>(lines.size()*sizeof(ViewVertex)));
        attributes();
        program_.setUniformValue("lighting",false);
        program_.setUniformValue("color",QVector3D(0.22f,0.28f,0.35f));
        glDrawArrays(GL_LINES,0,gridCount);
        if (axes_) {
            glDepthFunc(GL_LEQUAL);
            const QVector3D colors[]={{1,0.3f,0.3f},{0.35f,0.9f,0.4f},{0.3f,0.55f,1}};
            for (int i=0; i<3; ++i) {
                program_.setUniformValue("color",colors[i]);
                glDrawArrays(GL_LINES,gridCount+i*2,2);
            }
            glDepthFunc(GL_LESS);
        }
        if(static_cast<GLsizei>(lines.size())>baseCount) {
            program_.setUniformValue("color",QVector3D(1,.7f,.15f));
            glDrawArrays(GL_LINES,baseCount,static_cast<GLsizei>(lines.size())-baseCount);
        }
        linesBuffer_.release();
    }
    program_.release();
    glDisable(GL_DEPTH_TEST);
    QPainter painter(this);
    painter.setPen(QColor(210,220,235));
    painter.drawText(14,22,QStringLiteral("X rojo · Y verde · Z azul   |   Z ↑"));
    if (axes_) {
        const QVector3D directions[]={{1,0,0},{0,1,0},{0,0,1}};
        const QColor colors[]={QColor(255,77,77),QColor(89,230,102),QColor(77,140,255)};
        const QString labels[]={QStringLiteral("X"),QStringLiteral("Y"),QStringLiteral("Z")};
        const QPointF origin(64,80);
        for (int i=0; i<3; ++i) {
            const auto direction=view.mapVector(directions[i]);
            const QPointF end=origin+QPointF(direction.x()*35,-direction.y()*35);
            painter.setPen(QPen(colors[i],2));
            painter.drawLine(origin,end);
            painter.drawText(end+QPointF(3,-3),labels[i]);
        }
        painter.setPen(QColor(210,220,235));
    }
    if (!mesh_) { painter.drawText(rect(),Qt::AlignCenter,QStringLiteral("Abra un STL o 3MF para visualizar la malla")); }
    painter.drawText(14,height()-14,QStringLiteral("Orbitar: izquierdo   ·   Paneo: derecho/central   ·   Zoom: rueda   ·   Encuadrar: F"));
}

void MeshView::mousePressEvent(QMouseEvent* event)
{ lastMouse_=event->position(); setFocus(); event->accept(); }

void MeshView::mouseMoveEvent(QMouseEvent* event)
{
    const auto delta=event->position()-lastMouse_;
    lastMouse_=event->position();
    if (event->buttons() & (Qt::RightButton | Qt::MiddleButton)) {
        const auto direction=eyeDirection();
        const auto right=QVector3D::crossProduct({0,0,1},direction).normalized();
        const auto up=QVector3D::crossProduct(direction,right).normalized();
        const float factor=2*distance_*std::tan(halfFov)/std::max(1,height());
        target_ += right * static_cast<float>(-delta.x())*factor + up*static_cast<float>(delta.y())*factor;
        if (target_.length()>100) { target_=target_.normalized()*100; }
        autoFit_=false;
    } else if (event->buttons() & Qt::LeftButton) {
        azimuth_=std::fmod(azimuth_-static_cast<float>(delta.x())*0.4f,360.f);
        elevation_=std::clamp(elevation_+static_cast<float>(delta.y())*0.4f,-89.f,89.f);
    }
    update(); event->accept();
}

void MeshView::wheelEvent(QWheelEvent* event)
{
    const float radius=mesh_ ? mesh_->radius : 1.75f;
    const float steps=static_cast<float>(event->angleDelta().y())/120;
    distance_=std::clamp(distance_*std::exp(-steps*0.15f),radius*1.05f,radius*100);
    autoFit_=false;
    update(); event->accept();
}
