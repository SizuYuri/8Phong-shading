# 8Phong Shading — Phong + Normal Mapping + up to 8 Lights

> OpenGL demo featuring per-fragment **Phong** lighting, **normal mapping**, **directional/point/spot** lights (up to 8), and an **ImGui** panel with **Blender-like** camera navigation.

## ✨ Features

- **Phong** lighting (ambient / diffuse / specular) in the fragment shader.  
- **Normal mapping** in tangent space (TBN); toggle at runtime with **`N`**.
- **Up to 8 lights** simultaneously:
  - **Directional** (infinite)
  - **Point** (with attenuation)
  - **Spot** (inner/outer cutoff for soft edges)
- **ImGui GUI** (optional): tweak light color/type/transform/attenuation/intensities; add/remove lights; material color & shininess.
- **Blender-like camera**:
  - MMB = orbit
  - Shift + MMB = pan
  - Ctrl + MMB = dolly
  - Mouse wheel = zoom
  - `F` = frame origin (0,0,0)
- **Diagnostics overlay** (GPU time if supported via `GL_TIME_ELAPSED`).

## 🧭 Controls

| Action | Mapping |
|---|---|
| Orbit | **MMB** |
| Pan | **Shift + MMB** |
| Dolly | **Ctrl + MMB** |
| Zoom | **Mouse Wheel** |
| Frame Origin | **F** |
| Toggle Normal Map | **N** |
| Reset Camera (if enabled) | **R** |

## 📦 Dependencies

- **GLAD** (OpenGL loader)
- **GLFW** (window/input)
- **GLM** (math)
- **Dear ImGui** + backends `imgui_impl_glfw.*`, `imgui_impl_opengl3.*`
- **stb_image.h** (image loading)
- **tinyfiledialogs** (native file dialogs)
- **Assimp** (model loading) — used by the `Model` class

> These can be vendored or installed via your preferred package manager. The repo assumes a **Visual Studio / Windows** setup and ships source files to build them directly.

## 🛠 Build (Visual Studio, Win64)

1. Open the solution (or create a new Win64 Console app and add sources under `src/`).
2. Set language standard to **C++17** (or newer).
3. **Runtime Library**: `/MDd` for Debug, `/MD` for Release.  
   > If you see `LNK4098: default library 'MSVCRT' conflicts…`, align CRT flags across all libraries or rebuild third-party libs to match.
4. Add/include third-party sources (if vendored):
   - ImGui core + `imgui_impl_glfw.cpp`, `imgui_impl_opengl3.cpp`
   - `glad.c`
   - `tinyfiledialogs.c`
   - GLM headers
5. Linker → **Additional Dependencies** *(adjust to your setup)*:
   - `opengl32.lib`, `glfw3dll.lib` (or your `glfw3.lib`), `assimp.lib`
   - Possibly `Comdlg32.lib`, `Shell32.lib`, `Ole32.lib` (for dialogs)
6. Preprocessor definitions:
   - `USE_IMGUI`
   - `IMGUI_IMPL_OPENGL_LOADER_GLAD`
7. Place `glfw3.dll` (if using DLL build) next to the `.exe`.

> **Shader paths:** the code loads `shaders/vertex.shader` and `shaders/fragment.shader` (relative to the working directory).

## 🧪 Build (CMake) — optional

If you prefer CMake, add a minimal `CMakeLists.txt` and vendor dependencies or use package finders. Example skeleton:

```cmake
cmake_minimum_required(VERSION 3.20)
project(8Phong CXX)

set(CMAKE_CXX_STANDARD 17)
add_executable(8Phong
  src/8Fong.cpp
  src/gui_panel.cpp src/gui_panel.h
  src/shader.cpp src/shader.h
  src/model.cpp src/model.h
  src/camera.cpp src/camera.h
  src/lighting.h
  third_party/glad.c
  third_party/tinyfiledialogs.c
  # + imgui sources and backends
)

# target_include_directories(8Phong PRIVATE third_party/include ...)
# target_link_libraries(8Phong PRIVATE opengl32 glfw3 assimp)
```

## 🧰 Project Layout

```
/ (repo root)
  ├─ src/            # C/C++ sources and headers
  ├─ shaders/        # GLSL shaders (vertex.shader, fragment.shader)
  ├─ assets/         # models/textures (optional)
  ├─ README.md
  ├─ .gitignore
  └─ (optional) .sln/.vcxproj for Visual Studio
```

## 🧩 Implementation Notes

- **Lighting:** classic Phong with ambient + diffuse (Lambert) + specular (Blinn/Phong-style).  
- **Normal Mapping:** tangent-space normals via **TBN**; if disabled, falls back to interpolated vertex normals.  
- **Lights:** passed as a uniform array (`lights[i]`), with fields for type, transform, color, attenuation, and spot cutoff.  
- **ImGui** panel: lets you add/remove lights, change type, toggle gizmos, adjust material & rotation parameters.

## ❗ Troubleshooting

- **`LNK4098: default library 'MSVCRT' conflicts …`**  
  Your CRT flags are mixed. Use `/MDd` for Debug and `/MD` for Release everywhere (your project **and** third-party libs).
- **Black screen or no UI:** confirm GL 3.3+ context, GLAD loaded, and ImGui backends are compiled and initialized.
- **Shaders not found:** check working directory; paths are `shaders/vertex.shader`, `shaders/fragment.shader`.

## 📄 License

Recommend **MIT** for demo/educational use; add `LICENSE` to the repo root.  
If you vendor third-party code, include their licenses under `third_party/`.

## 🙌 Acknowledgements

- [GLFW](https://www.glfw.org/), [GLAD](https://glad.dav1d.de/), [GLM](https://github.com/g-truc/glm)  
- [Dear ImGui](https://github.com/ocornut/imgui)  
- [stb_image.h](https://github.com/nothings/stb)  
- [Assimp](https://github.com/assimp/assimp)  
