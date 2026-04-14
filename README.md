# PolyShow - 二维几何图形查看器

[![Qt](https://img.shields.io/badge/Qt-6.9-blue)](https://www.qt.io)
[![C++](https://img.shields.io/badge/C++-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)]()

## 项目概述

PolyShow 是一个面向二维几何调试场景的查看器，用来快速打开和检查点、折线、多边形等图元。

当前版本只支持 **PolyShow 自定义 `.ply` 二维文本格式**。这里的 `.ply` 只是沿用扩展名，**已经不是标准的 Polygon File Format / Stanford PLY**。

## 当前能力

- 打开并解析 PolyShow 自定义 `.ply` 文本文件
- 显示 `point`、`polyline`、`polygon` 三类图元
- 支持 `Solid / Wireframe / Points` 三种渲染模式
- 支持图元级样式：颜色、填充、线宽、点大小
- 显示当前文件路径与 `Points / Polylines / Polygons` 统计信息

## 构建

### Windows

```bash
cmake -G "Visual Studio 17 2022" -B build -DCMAKE_PREFIX_PATH="D:/Qt/6.9.1/msvc2022_64"
cmake --build build --config Release
cmake --build build --config Debug
```

### Linux

```bash
cmake -G "Unix Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## PolyShow `.ply` 格式说明

### 定位说明

- 文件扩展名继续使用 `.ply`
- 文件内容为 UTF-8 文本
- 这不是标准 PLY 头格式
- 旧的 `ply / format ascii / element vertex / element face` 方案已经不再支持

### 基本规则

| 规则 | 说明 |
|------|------|
| 坐标行 | 一行固定写两个数字：`x y` |
| 图形分隔 | 用单独一行 `NEXT` 分隔相邻图形 |
| 空行 | 忽略 |
| 注释 | `#` 开头整行忽略 |
| 文件结尾 | 最后一个图形后面可以不写 `NEXT` |

### 图元判定

| 点数规则 | 结果 |
|---------|------|
| 一个点 | `point` |
| 两个及以上点，且首尾不同 | `polyline` |
| 四个及以上点，且首尾相同 | `polygon` |

补充说明：

- `polygon` 必须至少有 3 个唯一顶点
- 闭合多边形需要显式把起点再写一遍作为终点
- 解析后最后一个重复闭合点不会重复存储

### 样式指令

样式指令是“粘性”的，写一次后会持续作用于后续图形，直到被新的同类指令覆盖。样式指令只能写在某个图形开始之前，不能插入到图形坐标中间。

| 指令 | 说明 | 默认值 |
|------|------|--------|
| `COLOR #RRGGBB` | 设置点/线/轮廓颜色 | `#2259B4` |
| `COLOR #RRGGBBAA` | 设置带透明度的颜色 | `#2259B4FF` |
| `FILL #RRGGBB` | 设置 polygon 填充色 | 默认跟随 `COLOR`，透明度为 80 |
| `FILL #RRGGBBAA` | 设置带透明度的填充色 | 同上 |
| `FILL none` | 关闭 polygon 填充 | 无填充 |
| `WIDTH number` | 设置线宽 | `1.5` |
| `POINT_SIZE number` | 设置点半径 | `2.5` |

补充说明：

- 如果没有显式写 `FILL`，填充色会跟随当前 `COLOR`
- 一旦写过 `FILL none` 或具体颜色，后续 `COLOR` 不会自动改写当前填充设置
- `WIDTH` 和 `POINT_SIZE` 必须大于 `0`

### 有效示例

#### 1. 单点

```text
12 18
NEXT
```

#### 2. 开放折线

```text
0 0
40 20
90 20
120 60
NEXT
```

#### 3. 闭合多边形

```text
0 0
120 0
60 90
0 0
NEXT
```

#### 4. 带样式的混合图形

```text
# 蓝色点
COLOR #2259B4
POINT_SIZE 4
20 20
NEXT

# 橙色折线
COLOR #E66A2C
WIDTH 3
0 0
50 40
90 10
140 60
NEXT

# 绿色半透明多边形
COLOR #278A5B
FILL #278A5B66
0 0
80 0
80 80
0 80
0 0
NEXT
```

### 错误规则

以下内容会直接报错，并在界面中提示具体行号：

- 未知指令
- 图形中途插入样式指令
- 坐标行不是两个数字
- 数值解析失败
- 闭合图形点数不足
- 图形只包含重复点

## 渲染规则

- `Solid`
  - `point` 显示为点
  - `polyline` 显示为线
  - `polygon` 显示为描边 + 填充
- `Wireframe`
  - `point` 显示为点
  - `polyline` 显示为线
  - `polygon` 只显示描边
- `Points`
  - 所有图元只显示顶点点位

## 测试数据

项目内置了一批手工验证文件，位于 `test_data/ply`：

### 有效文件

- `triangle.ply`
- `square.ply`
- `pentagon.ply`
- `points_only.ply`
- `multi_shape.ply`
- `polyline_open.ply`
- `mixed_primitives.ply`
- `style_colors.ply`
- `style_sizes.ply`
- `comments_and_blanks.ply`
- `fill_none.ply`

### 无效文件

- `invalid_unknown_command.ply`
- `invalid_bad_number.ply`
- `invalid_mid_shape_style.ply`
- `invalid_degenerate_polygon.ply`

## 迁移说明

如果你手里还保留旧版本样例，请把原来的标准 PLY 头格式：

```text
ply
format ascii 1.0
element vertex ...
element face ...
...
```

改写成现在的“逐行坐标 + `NEXT` 分段”格式。PolyShow 当前不会再解析旧头格式。

## 开发规范

项目协作规范见 `AGENTS.md`。
