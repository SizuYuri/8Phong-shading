# Project: Phong + Normal Mapping + up to 8 Lights (v1)

## What’s included
- Per-fragment **Phong** lighting (ambient / diffuse / specular).
- **Normal mapping** (TBN, tangent space), toggle with `N`.
- Up to **8 lights** simultaneously.
- **Light types:** Directional, Point (with attenuation), Spot (inner/outer cutoff for soft edge).
- **Dear ImGui GUI** (optional): edit light color, type, position/direction, attenuation, intensities; add/remove lights.  
  Material controls: object color and shininess.

## Controls
- **Blender-like camera:**
  - **MMB** – orbit
  - **Shift + MMB** – pan
  - **Ctrl + MMB** – dolly
  - **Mouse wheel** – zoom
  - **F** – frame origin (0,0,0)
- **N** – toggle normal map  
- **R** – reset camera (if enabled in your build)

## Build
- Dependencies: **GLAD**, **GLFW**, **GLM**, **Assimp**, **tinyfiledialogs**, **stb_image.h**.  
  (If you currently use a minimal `stb_image.h` placeholder, replace it with the full header.)
- Core files touched in this version: `src/8Fong.cpp`, `src/model.h/.cpp`, `shaders/vertex.shader`, `shaders/fragment.shader`, `stb_image.h`, this README.

### Enabling the GUI (Dear ImGui)
1. Add Dear ImGui core and backends: `imgui_impl_glfw.*`, `imgui_impl_opengl3.*`.
2. Define the macro **`USE_IMGUI`** and add the ImGui sources to your project.
3. Build and link with GLFW and OpenGL (if you use GLAD, also define `IMGUI_IMPL_OPENGL_LOADER_GLAD`).

> Without `USE_IMGUI` the app builds and runs headless (no panel).

## GitHub-ready Layout

```
/ (repo root)
  ├─ src/            # C/C++ sources and headers
  ├─ shaders/        # GLSL shaders (vertex.shader, fragment.shader)
  ├─ assets/         # models/textures (optional)
  ├─ README.md
  ├─ .gitignore
  └─ (optional) .sln/.vcxproj for Visual Studio
```

> The code loads shaders from `shaders/vertex.shader` and `shaders/fragment.shader`.
