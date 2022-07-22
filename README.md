## About
Shader editor made using `Vulkan` which supports both `GLSL` and `HLSL` shaders. Project is written for the `C++11` standard and `x64` system. Currently the code is tested only on `Windows`, using `MSVC` (Visual Studio) compiler. `Linux` and `Mac` are not completely supported at the moment, but it should be easy to port, since all third party libraries are cross platform.

![Example](https://github.com/milkru/data_resources/blob/main/foton/universe.png "Example")

## Feature overview
* Rendering backend written using `Vulkan 1.0`
* GUI mostly written using [Dear ImGui](https://github.com/ocornut/imgui)
* Window handling using [GLFW](https://github.com/glfw/glfw)
* `GLSL` and `HLSL` live shader compilation using [Glslang](https://github.com/KhronosGroup/glslang.git)
* Live coding editor window
* Log output window
* Shader bindings window
* Automatic descriptor set layout creation with [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect)
* Image loading using [stb](https://github.com/nothings/stb.git)
* Dialog windows are handled by [Native File Dialog Extended](https://github.com/btzy/nativefiledialog-extended.git)
* Meta file serialization is written using [rapidjson](https://github.com/Tencent/rapidjson)

## Installation
This project uses [CMake](https://cmake.org/download/) as a build tool. Since the project is built using `Vulkan`, the latest [Vulkan SDK](https://vulkan.lunarg.com) is required. Dependency management is handled by [Bootstrap](https://github.com/corporateshark/bootstrapping), which requires [Python](https://www.python.org/downloads/) and [Git](https://git-scm.com/downloads) installed.

## License
Distributed under the MIT License. See `LICENSE` for more information.
