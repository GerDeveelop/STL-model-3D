#pragma once

#include "core/mesh/Mesh.h"
#include "core/cutting/PlaneCut.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointF>
#include <QVector3D>

#include <memory>
#include <stop_token>
#include <vector>

struct ViewVertex { float x, y, z, nx, ny, nz; float r{.28f},g{.72f},b{.91f}; };
struct ViewMesh {
    std::vector<ViewVertex> vertices;
    float radius{1};
    float floor{-1};
    stlcut::Vec3 center;
    double scale{1};
    std::size_t legacyZeroNormalTriangles{};
};

// Presentation adapter only: does not modify core mesh coordinates or statistics.
std::shared_ptr<const ViewMesh> prepareViewMesh(const stlcut::Mesh& mesh,
                                              std::stop_token stop = {},bool auditLegacyNormals = false);

class MeshView final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit MeshView(QWidget* parent = nullptr);
    ~MeshView() override;
    void setMesh(std::shared_ptr<const ViewMesh> mesh);
    void fitMesh();
    void resetCamera();
    void showGrid(bool show);
    void showAxes(bool show);
    void setCutPlane(stlcut::Plane plane, bool visible);
signals:
    void rendererStatus(const QString& message);
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent* event) override;
private:
    void releaseGL();
    void updateFitDistance();
    void fail(const QString& message);
    QVector3D eyeDirection() const;
    std::shared_ptr<const ViewMesh> mesh_;
    QOpenGLShaderProgram program_;
    QOpenGLBuffer meshBuffer_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer linesBuffer_{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject vao_;
    bool ready_{};
    bool meshDrawable_{};
    bool uploadPending_{};
    bool qualityPending_{true};
    bool grid_{true};
    bool axes_{true};
    bool autoFit_{true};
    float distance_{5};
    float azimuth_{45};
    float elevation_{30};
    QVector3D target_;
    QPointF lastMouse_;
    QString failure_;
    stlcut::Plane cutPlane_;
    bool cutPlaneVisible_{};
};
