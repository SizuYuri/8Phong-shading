#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>  // HMODULE, GetModuleHandleA, GetModuleFileNameA, MAX_PATH
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring> // strcmp, snprintf

#ifdef USE_IMGUI
#include "imgui.h"
#endif

#define TINYFD_NOLIB
#include "tinyfiledialogs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shader.h"
#include "model.h"
#include "camera.h"
#include "lighting.h"
#include "gui_panel.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float  deltaTime = 0.0f, lastFrame = 0.0f;
Model* ourModel = nullptr;
GLuint normalMapTex = 0;
bool   useNormalMap = false;

glm::vec3 objectColor(0.8f);
float     shininess = 32.0f;

std::vector<LightCPU> lights;

// Вращение модели
static bool  g_RotateEnabled = true;
static bool  g_RotateX = true, g_RotateY = true, g_RotateZ = false;
static float g_RotateSpeed = 0.6f;

// Гизмы
static bool  g_ShowLightGizmos = true;

// Blender-навигация
static glm::vec3 g_OrbitCenter = glm::vec3(0.0f);
static float     g_OrbitDist = 5.0f;
static float     g_YawDeg = -90.0f;
static float     g_PitchDeg = 0.0f;

// Диагностика
static bool   g_ShowDiag = true;
static bool   g_VSync = false;
static bool   g_Stress = false;

// GPU timer query (ping-pong)
static bool   g_HasTimerQuery = false;
static GLuint g_TimerQuery[2] = { 0,0 };
static int    g_TimerWrite = 0;     // в какой query пишем сейчас
static double g_LastGpuMs = 0.0;

static double g_LastCpuMs = 0.0;
static double g_LastFps = 0.0;

static inline void updateCameraFromOrbit() {
    float yaw = glm::radians(g_YawDeg);
    float pitch = glm::radians(g_PitchDeg);
    glm::vec3 dir(
        cosf(yaw) * cosf(pitch),
        sinf(pitch),
        sinf(yaw) * cosf(pitch)
    );
    glm::vec3 pos = g_OrbitCenter - dir * g_OrbitDist;
    camera.Position = pos;
    camera.Front = glm::normalize(g_OrbitCenter - pos);
}

// ---------- callbacks ----------
static void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
#endif
    static double prevX = xpos, prevY = ypos;
    double dx = xpos - prevX;
    double dy = ypos - prevY;
    prevX = xpos; prevY = ypos;

    bool mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (mmb && !shift && !ctrl) {
        float sens = 0.2f;
        g_YawDeg -= (float)dx * sens;
        g_PitchDeg -= (float)dy * sens;
        g_PitchDeg = glm::clamp(g_PitchDeg, -89.5f, 89.5f);
        updateCameraFromOrbit();
        return;
    }
    if (mmb && shift) {
        float panSens = 0.0025f * g_OrbitDist;
        glm::vec3 right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
        glm::vec3 up = glm::normalize(glm::cross(right, camera.Front));
        g_OrbitCenter -= right * (float)dx * panSens;
        g_OrbitCenter += up * (float)dy * panSens;
        updateCameraFromOrbit();
        return;
    }
    if (mmb && ctrl) {
        float dollySens = 0.01f;
        g_OrbitDist *= (1.0f + (float)dy * dollySens);
        g_OrbitDist = glm::clamp(g_OrbitDist, 0.2f, 500.0f);
        updateCameraFromOrbit();
        return;
    }
}

static void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
#endif
    float zoomSens = 0.1f;
    g_OrbitDist *= (1.0f - (float)yoffset * zoomSens);
    g_OrbitDist = glm::clamp(g_OrbitDist, 0.2f, 500.0f);
    updateCameraFromOrbit();
}

// ---------- helpers ----------
static GLuint loadTexture2D(const char* path) {
    int w, h, n; stbi_uc* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) { std::cerr << "Texture load failed: " << path << std::endl; return 0; }
    GLenum format = (n == 1 ? GL_RED : (n == 3 ? GL_RGB : GL_RGBA));
    GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

static void showNormalMapDialog() {
    const char* patterns[] = { "*.png","*.jpg","*.jpeg","*.tga","*.bmp" };
    const char* fp = tinyfd_openFileDialog("Normal map (optional)", "", 5, patterns, "Images", 0);
    if (fp) { normalMapTex = loadTexture2D(fp); useNormalMap = (normalMapTex != 0); }
}

static void loadModel(const char* path) {
    delete ourModel; ourModel = new Model(path);
}

static void showModelDialog() {
    const char* patterns[] = { "*.obj","*.fbx","*.dae","*.3ds","*.ply" };
    const char* fp = tinyfd_openFileDialog("Choose model", "", 5, patterns, "Models", 0);
    if (fp) { std::cout << "Model: " << fp << std::endl; loadModel(fp); }
}

static void uploadLights(Shader& sh) {
    int n = (int)lights.size(); if (n > 8) n = 8;
    glUniform1i(glGetUniformLocation(sh.ID, "numLights"), n);
    for (int i = 0; i < n; ++i) {
        std::string b = "lights[" + std::to_string(i) + "].";
        sh.setInt(b + "type", (int)lights[i].type); // enum class -> int
        sh.setVec3(b + "position", lights[i].position);
        sh.setVec3(b + "direction", glm::normalize(lights[i].direction));
        sh.setFloat(b + "innerCutoff", lights[i].innerCutoff);
        sh.setFloat(b + "outerCutoff", lights[i].outerCutoff);
        sh.setFloat(b + "constant", lights[i].constant);
        sh.setFloat(b + "linear", lights[i].linear);
        sh.setFloat(b + "quadratic", lights[i].quadratic);
        sh.setVec3(b + "color", lights[i].color);
        sh.setFloat(b + "ambient", lights[i].ambient);
        sh.setFloat(b + "diffuse", lights[i].diffuse);
        sh.setFloat(b + "specular", lights[i].specular);
    }
}

// ---------- таймеры: runtime-детекция без GLAD_GL_* макросов ----------
static bool HasExtension(const char* name) {
    if (!name || !glad_glGetStringi) return false;
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; ++i) {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && std::strcmp(ext, name) == 0) return true;
    }
    return false;
}

static void InitGpuTimersIfAvailable() {
    bool has_fn = glad_glBeginQuery &&
        glad_glEndQuery &&
        glad_glGetQueryObjectui64v;

    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    bool ver_ok = (major > 3) || (major == 3 && minor >= 3);

    bool has_ext = HasExtension("GL_ARB_timer_query") || HasExtension("GL_EXT_timer_query");

    g_HasTimerQuery = has_fn && (ver_ok || has_ext);
    if (g_HasTimerQuery) glGenQueries(2, g_TimerQuery); // ping-pong
}

static void processInput(GLFWwindow* window) {
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;
#endif
    static bool prevF = false;
    bool nowF = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (nowF && !prevF) {
        g_OrbitCenter = glm::vec3(0.0f);
        updateCameraFromOrbit();
    }
    prevF = nowF;
}

#ifdef USE_IMGUI
// --- 2D гизмосы света поверх сцены ---
static bool project_to_screen(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj, ImVec2 display, ImVec2& out) {
    glm::vec4 clip = proj * view * glm::vec4(p, 1.0);
    if (clip.w <= 0.0f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.z < -1 || ndc.z > 1) return false;
    out.x = (ndc.x * 0.5f + 0.5f) * display.x;
    out.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * display.y;
    return true;
}
static ImU32 col_from(glm::vec3 rgb, float alpha = 1.0f) {
    rgb = glm::clamp(rgb, glm::vec3(0.0f), glm::vec3(1.0f));
    return IM_COL32((int)(rgb.r * 255.0f), (int)(rgb.g * 255.0f), (int)(rgb.b * 255.0f), (int)(alpha * 255.0f));
}
static void draw_light_gizmos_2d(const glm::mat4& view, const glm::mat4& proj) {
    if (!g_ShowLightGizmos) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 disp = ImGui::GetIO().DisplaySize;

    const glm::vec3 anchor = g_OrbitCenter;

    for (size_t i = 0; i < lights.size(); ++i) {
        const auto& L = lights[i];
        if (!L.drawGizmo) continue;
        glm::vec3 dir = glm::normalize(L.direction);
        bool isDir = (L.type == LightType::Directional);
        glm::vec3 base = isDir ? anchor : L.position;

        float camDist = glm::length(base - camera.Position);
        float len = glm::clamp(camDist * 0.25f, 0.8f, 4.0f);
        glm::vec3 tip = base + dir * len;

        ImVec2 ps;
        if (project_to_screen(base, view, proj, disp, ps)) {
            ImU32 c = col_from(L.color, 1.0f);
            dl->AddCircleFilled(ps, 5.0f, c);
            dl->AddCircle(ps, 7.0f, IM_COL32(255, 255, 255, 200), 0, 1.0f);
            char buf[8]; std::snprintf(buf, sizeof(buf), "%zu", i + 1);
            dl->AddText(ImVec2(ps.x + 8, ps.y - 8), IM_COL32(255, 255, 255, 230), buf);
        }

        ImVec2 a, b;
        bool okBase = project_to_screen(base, view, proj, disp, a);
        bool okTip = project_to_screen(tip, view, proj, disp, b);
        if (okBase && okTip) {
            ImU32 c = col_from(L.color, 1.0f);
            dl->AddLine(a, b, c, 2.0f);
            glm::vec3 refUp = fabsf(dir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            glm::vec3 side = glm::normalize(glm::cross(dir, refUp));
            glm::vec3 h1 = tip - dir * (len * 0.2f) + side * (len * 0.12f);
            glm::vec3 h2 = tip - dir * (len * 0.2f) - side * (len * 0.12f);
            ImVec2 h1s, h2s;
            if (project_to_screen(h1, view, proj, disp, h1s)) dl->AddLine(b, h1s, c, 2.0f);
            if (project_to_screen(h2, view, proj, disp, h2s)) dl->AddLine(b, h2s, c, 2.0f);
        }
        else if (okTip) {
            ImU32 c = col_from(L.color, 1.0f);
            dl->AddCircle(b, 6.0f, c, 0, 2.0f);
        }
    }
}
#endif

int main() {
    setlocale(LC_ALL, "ru");

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Phong + NormalMap + Lights + GUI", nullptr, nullptr);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    // VSync
    glfwSwapInterval(g_VSync ? 1 : 0);

    // GPU timer query (runtime detection)
    InitGpuTimersIfAvailable();

    Shader shader("shaders/vertex.shader", "shaders/fragment.shader");

#ifdef USE_IMGUI
    GuiPanel gui(window, objectColor, shininess, useNormalMap, lights,
        (glm::vec3&)camera.Position, (glm::vec3&)camera.Front,
        g_ShowLightGizmos, g_RotateEnabled, g_RotateX, g_RotateY, g_RotateZ, g_RotateSpeed,
        g_OrbitCenter);
    gui.init();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#endif

    // инициализация орбиты
    {
        g_OrbitCenter = glm::vec3(0.0f);
        g_OrbitDist = glm::length(camera.Position - g_OrbitCenter);
        glm::vec3 f = glm::normalize(camera.Front);
        g_PitchDeg = glm::degrees(asinf(f.y));
        g_YawDeg = glm::degrees(atan2f(f.z, f.x));
        updateCameraFromOrbit();
    }

    showModelDialog();
    if (!ourModel) { return 0; }
    showNormalMapDialog();

    // initial lights
    lights.clear();
    lights.push_back({ LightType::Directional, {0,0,0}, glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)),
        0.9f, 0.85f, 1.0f, 0.0f, 0.0f, {1,1,1}, 0.15f, 1.0f, 0.3f, true, false });

    lights.push_back({ LightType::Point, { 2,2,2 }, {0,-1,0},
        0.9f, 0.85f, 1.0f, 0.09f, 0.032f, {1.0f,0.9f,0.8f}, 0.08f, 0.8f, 0.25f, true, false });

    lights.push_back({ LightType::Point, {-2,2,2 }, {0,-1,0},
        0.9f, 0.85f, 1.0f, 0.09f, 0.032f, {0.8f,0.9f,1.0f}, 0.06f, 0.7f, 0.20f, true, false });

    lights.push_back({ LightType::Spot, camera.Position, camera.Front,
        cos(glm::radians(12.5f)), cos(glm::radians(17.5f)),
        1.0f, 0.09f, 0.032f, {1,1,1}, 0.00f, 1.0f, 0.3f, true, false });

    while (!glfwWindowShouldClose(window)) {
        float t = (float)glfwGetTime(); deltaTime = t - lastFrame; lastFrame = t;

        processInput(window);
        updateCameraFromOrbit();

        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef USE_IMGUI
        if (g_ShowDiag) ImGui::SetNextWindowBgAlpha(0.9f);
        gui.beginFrame();
        gui.draw();
#endif

        // ---- CPU timer start
        double cpuStart = glfwGetTime();

        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        if (g_RotateEnabled) {
            float a = g_RotateSpeed * t;
            if (g_RotateX) model = glm::rotate(model, a, glm::vec3(1, 0, 0));
            if (g_RotateY) model = glm::rotate(model, a * 0.7f, glm::vec3(0, 1, 0));
            if (g_RotateZ) model = glm::rotate(model, a * 1.3f, glm::vec3(0, 0, 1));
        }
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        for (auto& L : lights) {
            if (L.type == LightType::Spot && L.followCamera) {
                L.position = camera.Position;
                L.direction = glm::normalize(camera.Front);
            }
        }

        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("objectColor", objectColor);
        shader.setFloat("shininess", shininess);

        uploadLights(shader);

        shader.setBool("useNormalMap", useNormalMap);
        if (useNormalMap && normalMapTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, normalMapTex);
            shader.setInt("normalMap", 0);
        }

        // --- GPU timer start (в активный query)
        if (g_HasTimerQuery) glBeginQuery(GL_TIME_ELAPSED, g_TimerQuery[g_TimerWrite]);

        if (ourModel) ourModel->Draw(shader);

        // стресс-нагрузка (x10)
        if (g_Stress) {
            for (int i = 0; i < 10; ++i) {
                shader.setFloat("shininess", shininess + i * 0.01f);
                if (ourModel) ourModel->Draw(shader);
            }
        }

        // --- GPU timer end
        if (g_HasTimerQuery) glEndQuery(GL_TIME_ELAPSED);

        // --- GPU timer read (читаем предыдущий query, если готов)
        if (g_HasTimerQuery) {
            int readIdx = 1 - g_TimerWrite;
            if (g_TimerQuery[readIdx]) {
                GLuint available = 0;
                glGetQueryObjectuiv(g_TimerQuery[readIdx], GL_QUERY_RESULT_AVAILABLE, &available);
                if (available) {
                    GLuint64 ns = 0;
                    glGetQueryObjectui64v(g_TimerQuery[readIdx], GL_QUERY_RESULT, &ns);
                    g_LastGpuMs = ns / 1e6;
                }
            }
            g_TimerWrite = 1 - g_TimerWrite; // переключаемся
        }

        // ---- CPU timer end
        g_LastCpuMs = (glfwGetTime() - cpuStart) * 1000.0;
        g_LastFps = (deltaTime > 0.0 ? 1.0 / deltaTime : 0.0);

#ifdef USE_IMGUI
        draw_light_gizmos_2d(view, projection);

        // Diagnostics
        if (g_ShowDiag) {
            if (ImGui::Begin("Diagnostics", &g_ShowDiag, ImGuiWindowFlags_AlwaysAutoResize)) {
                const char* vendor = (const char*)glGetString(GL_VENDOR);
                const char* renderer = (const char*)glGetString(GL_RENDERER);
                const char* version = (const char*)glGetString(GL_VERSION);

                ImGui::Text("GL_VENDOR:   %s", vendor ? vendor : "(null)");
                ImGui::Text("GL_RENDERER: %s", renderer ? renderer : "(null)");
                ImGui::Text("GL_VERSION:  %s", version ? version : "(null)");
#ifdef _WIN32
                HMODULE hGL = GetModuleHandleA("opengl32.dll");
                if (hGL) {
                    char path[MAX_PATH] = { 0 };
                    GetModuleFileNameA(hGL, path, MAX_PATH);
                    ImGui::Text("opengl32.dll: %s", path);
                }
#endif
                ImGui::Separator();
                ImGui::Checkbox("VSync", &g_VSync); ImGui::SameLine();
                if (ImGui::Button("Apply")) glfwSwapInterval(g_VSync ? 1 : 0);
                ImGui::Checkbox("Stress scene (x10 draws)", &g_Stress);

                ImGui::Separator();
                ImGui::Text("CPU frame: %.2f ms (%.0f FPS)", g_LastCpuMs, g_LastFps);
                if (g_HasTimerQuery) ImGui::Text("GPU time:  %.2f ms", g_LastGpuMs);
                else ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "GPU timer not supported");

                // простая индикация
                std::string V = vendor ? vendor : "";
                for (auto& c : V) c = (char)tolower(c);
                bool isMesa = (V.find("mesa") != std::string::npos || V.find("x.org") != std::string::npos);
                bool isNV = (V.find("nvidia") != std::string::npos);
                bool isAMD = (V.find("advanced micro devices") != std::string::npos || V.find("ati") != std::string::npos);
                bool isIntel = (V.find("intel") != std::string::npos);
                ImGui::Separator();
                ImGui::Text("Detected: "); ImGui::SameLine();
                if (isMesa)  ImGui::TextColored(ImVec4(0, 1, 0, 1), "CPU (Mesa)");
                else if (isNV || isAMD) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Discrete GPU");
                else if (isIntel)       ImGui::TextColored(ImVec4(1, 1, 0, 1), "Integrated GPU");
                else                    ImGui::Text("Unknown");
            }
            ImGui::End();
        }

        // Help
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Help", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs)) {
            ImGui::SetWindowPos({ 10,10 }, ImGuiCond_Always);
            ImGui::Text("Blender navigation:");
            ImGui::Text("MMB: Orbit   | Shift+MMB: Pan");
            ImGui::Text("Ctrl+MMB: Dolly   | Wheel: Zoom");
            ImGui::Text("F: Frame origin (orbit center = 0,0,0)");
        }
        ImGui::End();

        gui.endFrame();
#endif

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#ifdef USE_IMGUI
    if (g_HasTimerQuery) glDeleteQueries(2, g_TimerQuery);
    gui.shutdown();
#endif
    glfwTerminate();
    return 0;
}
