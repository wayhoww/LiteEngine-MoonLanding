# Moon Landing
登陆月球小游戏


## Build

适用环境：Windows 10/11, Visual Studio 2019 或更高版本

依赖：NuGet 包 DirectXTeX。在联网条件下，Visual Studio 会自行下载

构建步骤：
1. 右键单击 MoonLanding 项目，将其设置为启动项目
2. 如果要在 Visual Studio 中直接启动或调试该项目，那么需要右键单击 MoonLanding 项目，
   将 属性 - 调试 - 工作目录 设置为 "$(TargetDir)"。
   因为 Visual Studio 不会将该项设置保存到 VC 项目文件，所以需要手动设置该项。
3. 构建 / 启动 / 调试


## Run

构建完成后，项目所需的所有资源文件（如 cso 文件，3D 模型文件）都和可执行程序位于同一目录下。
以该目录为工作目录启动程序即可。


## Dependencies

0. Windows API
1. NuGet 包 DirectXTeX。Visual Studio 会自动安装
2. TinyGLTF，用于加载 glTF 3D 模型文件。是一个 header-only 的库


## Structures

* LiteEngine: "迷你引擎"，简单封装了 DirectX 的 API
* MoonLanding: 登陆月球小游戏

