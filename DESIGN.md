# PolyShow 技术设计文档

## 1. 系统架构设计

### 1.1 整体架构

PolyShow 采用经典的 **Model-View-Controller (MVC)** 架构模式：

```
┌─────────────────────────────────────────────────────────────┐
│                      UI Layer (Qt Widgets)                 │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │ MainWindow  │ │  Toolbar    │ │   PropertyPanel     │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   Presentation Layer                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           GeometryViewer (QGraphicsView)             │  │
│  │  - View Management                                   │  │
│  │  - User Interaction (Zoom/Pan/Rotate)                │  │
│  │  - Rendering Options                                │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                    Business Logic Layer                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │        GeometryScene (QGraphicsScene)                │  │
│  │  - Scene Management                                   │  │
│  │  - Item Selection                                    │  │
│  │  - Layer Management                                  │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │ Measurement │ │   LayerMgr  │ │    SelectionMgr      │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                       Data Layer                            │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────────┐  │
│  │ GeometryData│ │  FileLoader │ │    GeometryItem      │  │
│  └─────────────┘ └─────────────┘ └──────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Parsers (PLY/STL/OBJ)                   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      IPC Layer                              │
│  ┌─────────────┐ ┌──────────────────────────────────────┐  │
│  │ IPCServer   │ │         IPCClient (Header-only)      │  │
│  │(In App)     │ │         (User's Project)             │  │
│  └─────────────┘ └──────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 模块职责

| 模块 | 职责 | 关键类 |
|------|------|--------|
| **Core** | 几何数据结构和场景管理 | GeometryScene, GeometryData |
| **UI** | 用户界面和交互 | MainWindow, GeometryViewer, Toolbar |
| **Parsers** | 文件格式解析 | PlyParser, STLParser, OBJParser |
| **IPC** | 进程间通信 | IPCServer, DebugClient |
| **Utils** | 工具函数和辅助类 | Measurement, SelectionManager |

## 2. 核心类设计

### 2.1 几何数据模型

```cpp
// include/core/geometry.h
namespace PolyShow {

struct Point {
    double x{0.0};
    double y{0.0};

    Point() = default;
    Point(double x, double y) : x(x), y(y) {}

    bool operator==(const Point& other) const {
        return qFuzzyCompare(x, other.x) && qFuzzyCompare(y, other.y);
    }

    QPointF toQPointF() const { return QPointF(x, y); }
};

struct Line {
    Point start;
    Point end;

    Line() = default;
    Line(const Point& s, const Point& e) : start(s), end(e) {}
};

struct Polygon {
    QVector<Point> vertices;

    Polygon() = default;
    explicit Polygon(const QVector<Point>& verts) : vertices(verts) {}

    bool isValid() const { return vertices.size() >= 3; }
};

} // namespace PolyShow
```

### 2.2 QGraphicsItem 子类

```cpp
// include/ui/geometry_items.h
class PointItem : public QGraphicsEllipseItem {
public:
    explicit PointItem(const PolyShow::Point& point, QGraphicsItem* parent = nullptr);
    void setHighlight(bool highlight);

private:
    PolyShow::Point m_point;
    bool m_highlight{false};
};

class LineItem : public QGraphicsLineItem {
public:
    explicit LineItem(const PolyShow::Line& line, QGraphicsItem* parent = nullptr);
    void setHighlight(bool highlight);

private:
    PolyShow::Line m_line;
    bool m_highlight{false};
};

class PolygonItem : public QGraphicsPolygonItem {
public:
    explicit PolygonItem(const PolyShow::Polygon& polygon, QGraphicsItem* parent = nullptr);
    void setHighlight(bool highlight);

private:
    PolyShow::Polygon m_polygon;
    bool m_highlight{false};
};
```

### 2.3 图形场景管理

```cpp
// include/core/scene.h
class GeometryScene : public QGraphicsScene {
    Q_OBJECT

public:
    enum class RenderMode {
        Solid,
        Wireframe,
        Points
    };
    Q_ENUM(RenderMode)

    explicit GeometryScene(QObject* parent = nullptr);

    void loadPLY(const QString& filePath);
    void loadSTL(const QString& filePath);
    void loadOBJ(const QString& filePath);

    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const { return m_render_mode; }

    void clearScene();
    void showGrid(bool show);
    void showAxes(bool show);

signals:
    void fileLoaded(const QString& filePath, int itemCount);
    void selectionChanged(QList<QGraphicsItem*> items);
    void errorOccurred(const QString& message);

private:
    void setupGrid();
    void setupAxes();

    RenderMode m_render_mode{RenderMode::Solid};
    QGraphicsItem* m_gridItem{nullptr};
    QGraphicsItem* m_axesItem{nullptr};
};
```

### 2.4 图形视图控制器

```cpp
// include/ui/viewer.h
class GeometryViewer : public QGraphicsView {
    Q_OBJECT

public:
    explicit GeometryViewer(GeometryScene* scene, QWidget* parent = nullptr);

    void zoomIn();
    void zoomOut();
    void fitInView();
    void resetView();

    void setPanMode(bool enabled);
    void setSelectMode(bool enabled);
    void setMeasureMode(bool enabled);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class ViewMode {
        Pan,
        Select,
        Measure
    };

    void updateTransform();
    void startMeasurement(const QPointF& pos);

    ViewMode m_viewMode{ViewMode::Pan};
    QPointF m_last_mouse_position;
    bool m_is_panning{false};
    qreal m_zoom_factor{1.0};
};
```

## 3. 文件解析器设计

### 3.1 PLY 解析器

```cpp
// include/parsers/PlyParser.h
class PlyParser {
public:
    struct PLYData {
        QVector<PolyShow::Point> vertices;
        QVector<PolyShow::Polygon> faces;
        QVector<QVector3D> normals;
        QVector<QColor> colors;
    };

    static bool parse(const QString& filePath, PLYData& data, QString* error = nullptr);

private:
    PlyParser() = default;

    static bool parseASCII(QFile& file, PLYData& data);
    static bool parseBinary(QFile& file, PLYData& data);
};
```

### 3.2 STL 解析器

```cpp
// include/parsers/stl_parser.h
class STLParser {
public:
    struct STLData {
        QVector<PolyShow::Triangle> triangles;
        QVector<QVector3D> normals;
    };

    static bool parse(const QString& filePath, STLData& data, QString* error = nullptr);

private:
    static bool parseASCII(QFile& file, STLData& data);
    static bool parseBinary(QFile& file, STLData& data);
};
```

## 4. IPC 通信设计

### 4.1 协议定义

使用简单的 JSON 格式进行通信：

```json
{
    "type": "points",
    "data": [
        {"x": 0.0, "y": 0.0},
        {"x": 1.0, "y": 1.0}
    ]
}
```

```json
{
    "type": "lines",
    "data": [
        {"start": {"x": 0, "y": 0}, "end": {"x": 1, "y": 1}}
    ]
}
```

```json
{
    "type": "polygon",
    "data": {
        "vertices": [
            {"x": 0, "y": 0},
            {"x": 1, "y": 1},
            {"x": 2, "y": 0}
        ]
    }
}
```

### 4.2 服务器端（查看器）

```cpp
// include/ipc/server.h
class IPCServer : public QObject {
    Q_OBJECT

public:
    explicit IPCServer(QObject* parent = nullptr);
    ~IPCServer();

    bool start(const QString& serverName = "PolyShowServer");
    void stop();

    void setScene(GeometryScene* scene);

signals:
    void dataReceived(const QString& type, const QJsonObject& data);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QLocalServer* m_server;
    QLocalSocket* m_socket;
    GeometryScene* m_scene{nullptr};
};
```

### 4.3 客户端（纯头文件）

```cpp
// include/ipc/debug_client.h
class PolyShowDebug {
public:
    PolyShowDebug();
    ~PolyShowDebug();

    bool connect(const QString& serverName = "PolyShowServer");
    void disconnect();

    void sendPoints(const std::vector<PolyShow::Point>& points);
    void sendLines(const std::vector<PolyShow::Line>& lines);
    void sendPolygons(const std::vector<PolyShow::Polygon>& polygons);

    void clear();
    void setLineColor(const QColor& color);
    void setFillColor(const QColor& color);

private:
    QLocalSocket* m_socket;
};
```

## 5. 用户界面设计

### 5.1 主窗口布局

```
┌────────────────────────────────────────────────────────────┐
│ Menu Bar: File | View | Tools | Help                      │
├──────────┬───────────────────────────────────────────────┤
│ Toolbar  │                                               │
│ ├────────┤                                               │
│          │                                               │
│          │            Graphics View                     │
│          │                                               │
│ Layer    │                                               │
│ Panel    │                                               │
│ ├────────┤                                               │
│ Property │                                               │
│ Panel    │                                               │
└──────────┴───────────────────────────────────────────────┘
│ Status Bar: X: 123.45 Y: 678.90 | Items: 156 | Ready   │
└────────────────────────────────────────────────────────────┘
```

### 5.2 工具栏设计

- **文件操作**: 打开, 保存, 导出截图
- **视图控制**: 放大, 缩小, 适应视图, 重置视图
- **模式切换**: 平移模式, 选择模式, 测量模式
- **显示选项**: 网格, 坐标轴, 线框/实体切换

### 5.3 属性面板

```
┌─────────────────────┐
│ Selected Item       │
├─────────────────────┤
│ Type: Polygon       │
│ Vertices: 5         │
│ Area: 123.45        │
│ Perimeter: 67.89    │
├─────────────────────┤
│ Position            │
│ X: 123.45           │
│ Y: 678.90           │
└─────────────────────┘
```

## 6. 性能优化策略

### 6.1 渲染优化

- **BSP 树**: QGraphicsView 内置空间索引，优化元素查找
- **LOD (Level of Detail)**: 远距离渲染简化几何体
- **视口裁剪**: 只渲染可见区域元素
- **OpenGL 加速**: 使用 QOpenGLWidget 作为视口

### 6.2 数据加载优化

- **流式加载**: 大文件分块加载
- **延迟解析**: 按需解析文件内容
- **缓存机制**: 缓存已解析的几何数据
- **内存映射**: 使用内存映射文件处理超大文件

### 6.3 交互优化

- **区域选择**: 支持框选多元素
- **批量操作**: 批量修改属性
- **异步加载**: 文件加载不阻塞UI

## 7. 测试策略

### 7.1 单元测试

- **几何计算**: 点、线、面的数学运算
- **文件解析**: 各格式的正确性验证
- **测量工具**: 距离、角度、面积计算精度

### 7.2 集成测试

- **文件加载流程**: 完整的打开-显示-关闭流程
- **UI交互**: 视图控制、工具切换
- **IPC通信**: 客户端-服务器数据传输

### 7.3 性能测试

- **大文件加载**: 百万级元素的加载时间
- **渲染性能**: 不同元素数量的FPS
- **内存占用**: 不同场景的内存使用情况

## 8. 部署方案

### 8.1 Windows

- 使用 WinSparkle 进行自动更新
- NSIS 或 WiX 创建安装程序
- 打包 Qt DLL 和依赖库

### 8.2 Linux

- AppImage 格式分发
- DEB/RPM 包构建
- PPA 仓库发布

## 9. 未来扩展

### 9.1 三维支持

- 使用 Qt Quick 3D 替代 QGraphicsView
- 支持三维几何体的渲染和交互
- 添加 2D/3D 视图切换

### 9.2 高级功能

- 动画播放（时间序列数据）
- 几何变换（旋转、缩放、平移）
- 几何运算（布尔运算、偏移、倒角）
- 导出多种格式（DXF, SVG, PDF）

### 9.3 插件系统

- 支持自定义文件格式解析器
- 支持自定义渲染效果
- 支持自定义工具和命令

---

**文档版本**: 1.0
**最后更新**: 2026-02-01
**维护者**: PolyShow Team
