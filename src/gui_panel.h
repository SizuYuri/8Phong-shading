/*
 * Annotated for clarity:
 *  - This project implements Phong shading with optional normal mapping,
 *    supports up to 8 lights (directional/point/spot), and uses ImGui for GUI.
 */

#pragma once
#ifdef USE_IMGUI

#include <vector>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "lighting.h"

class GuiPanel {
public:
    GuiPanel(GLFWwindow* window,
             glm::vec3& objectColor, float& shininess, bool& useNormalMap,
             std::vector<LightCPU>& lights,
             glm::vec3& camPosRef, glm::vec3& camDirRef,
             bool& showLightGizmos,
             bool& rotateEnabled,
             bool& rotX, bool& rotY, bool& rotZ,
             float& rotateSpeed,
             glm::vec3& orbitCenterRef);

    void init();
    void shutdown();
// ImGui: start a new UI frame.
    void beginFrame();
// ImGui: render the UI draw data.
    void endFrame();

// ImGui: build the Lighting & Material panel and controls.
    void draw(); //  "Lighting & Material"

private:
    void drawMaterialSection();
    void drawLightsSection();
    void addLight(int type);

private:
    GLFWwindow* window_;
    glm::vec3& objectColor_;
    float& shininess_;
    bool& useNormalMap_;
    std::vector<LightCPU>& lights_;
    glm::vec3& camPosRef_;
    glm::vec3& camDirRef_;
    bool& showLightGizmos_;
    bool& rotateEnabled_;
    bool& rotX_;
    bool& rotY_;
    bool& rotZ_;
    float& rotateSpeed_;
    glm::vec3& orbitCenterRef_;

    int selectedLight_ = 0;
};

#endif // USE_IMGUI
