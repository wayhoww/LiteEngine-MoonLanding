# Moon Landing
一个 Direct3D 11 图形编程练习项目


## Build

适用环境：Windows 10/11, Visual Studio 2019 或更高版本

依赖：NuGet 包 DirectXTeX。在联网条件下，Visual Studio 
会自行下载

构建步骤：
1. 右键单击 MoonLanding 项目，将其设置为启动项目
2. 如果要在 Visual Studio 中直接启动或调试该项目，那么需
   要右键单击 MoonLanding 项目，将 属性 - 调试 - 工作目
   录设置为 "$(TargetDir)"。因为 Visual Studio 不会将
   该项设置保存到 VC 项目文件，所以需要手动设置该项。
3. 构建 / 启动 / 调试


## Run

构建完成后，项目所需的所有资源文件（如 cso 文件，3D 模型文
件）都和可执行程序位于同一目录下。以该目录为工作目录启动程序
即可。


## Dependencies

0. Windows API
1. NuGet 包 DirectXTeX。Visual Studio 会自动安装
2. TinyGLTF，用于加载 glTF 3D 模型文件。是一个 header-only
   的库


## Structures

* LiteEngine: 简单封装了 DirectX 的 API。称不上 Engine
* MoonLanding: 登陆月球小游戏

## LiteEngine

### Modeling
使用 Blender 建模（或导入外部模型），导出到 glTF 2.0 格式。
在导出界面，需要勾选 Normals、Tangents；需要取消勾选 +Y Up
(否则导入的相机会有问题)；可以勾选 Punctual Lights（方向光、
点光、聚光灯）、Cameras（相机）。

如果一个 Mesh 没有显式设置 TANGENT，并且没有任何 UV，那么 
Blender 导出插件不会导出 TANGENT。这种情况下，可以在 Blender 
中手动添加 UV Map 后再导出。

在缺少 NORMAL 或者 TANGENT 的时候，为了简单明确起见，
LiteEngine 会直接报错。

### glTF Model Loader Feature
会导入:
1. Mesh
2. 灯光
3. 材质和贴图
4. 相机

不会导入：
1. 动画

需要 glTF 文件中存在：
1. 法线
2. 正切线

不支持：
1. 非三角形的信息
2. 从稀疏缓冲区中读取数据



## Reference
主要参考资料
* [Direct3D 11 Graphics](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
* [Blender Reference Manual](https://docs.blender.org/manual/en/latest/)
* [glTF 2.0 Specification](https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html)
* [glTF `KHR_lights_punctual` Extension 文档](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md)
