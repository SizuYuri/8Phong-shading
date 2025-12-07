/*
 * Annotated for clarity:
 *  - This project implements Phong shading with optional normal mapping,
 *    supports up to 8 lights (directional/point/spot), and uses ImGui for GUI.
 */

#include "gui_panel.h"
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cmath>
#include <algorithm>
#include <string>

static inline float Deg2Rad(float d) { return d * 3.1415926535f / 180.0f; }
static inline float Rad2Deg(float r) { return r * 180.0f / 3.1415926535f; }

GuiPanel::GuiPanel(GLFWwindow* window,
    glm::vec3& objectColor, float& shininess, bool& useNormalMap,
    std::vector<LightCPU>& lights,
    glm::vec3& camPosRef, glm::vec3& camDirRef,
    bool& showLightGizmos,
    bool& rotateEnabled,
    bool& rotX, bool& rotY, bool& rotZ,
    float& rotateSpeed,
    glm::vec3& orbitCenterRef)
    : window_(window),
    objectColor_(objectColor), shininess_(shininess), useNormalMap_(useNormalMap),
    lights_(lights), camPosRef_(camPosRef), camDirRef_(camDirRef),
    showLightGizmos_(showLightGizmos),
    rotateEnabled_(rotateEnabled), rotX_(rotX), rotY_(rotY), rotZ_(rotZ), rotateSpeed_(rotateSpeed),
    orbitCenterRef_(orbitCenterRef) {
}

void GuiPanel::init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void GuiPanel::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ImGui: start a new UI frame.
void GuiPanel::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// ImGui: render the UI draw data.
void GuiPanel::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiPanel::addLight(int type) {
    if ((int)lights_.size() >= 8) return;
    LightCPU L{};
    L.type = static_cast<LightType>(type);
    L.position = camPosRef_ + camDirRef_ * 2.0f;
    L.direction = glm::normalize(camDirRef_);
    L.innerCutoff = std::cos(Deg2Rad(12.5f));
    L.outerCutoff = std::cos(Deg2Rad(17.5f));
    L.constant = 1.0f; L.linear = 0.09f; L.quadratic = 0.032f;
    L.color = { 1,1,1 };
    L.ambient = 0.05f; L.diffuse = 0.9f; L.specular = 0.3f;
    L.drawGizmo = true;
    lights_.push_back(L);
    selectedLight_ = (int)lights_.size() - 1;
}

void GuiPanel::drawMaterialSection() {
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Object Color", (float*)&objectColor_);
        ImGui::SliderFloat("Shininess", &shininess_, 1.0f, 256.0f);
        ImGui::Checkbox("Use Normal Map (N)", &useNormalMap_);

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Model / Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Rotate model", &rotateEnabled_);
            ImGui::SameLine();
            if (ImGui::Button(rotateEnabled_ ? "Pause" : "Resume"))
                rotateEnabled_ = !rotateEnabled_;
            ImGui::SliderFloat("Speed (rad/s)", &rotateSpeed_, 0.0f, 3.0f);
            ImGui::Checkbox("Rotate X", &rotX_); ImGui::SameLine();
            ImGui::Checkbox("Rotate Y", &rotY_); ImGui::SameLine();
            ImGui::Checkbox("Rotate Z", &rotZ_);
        }
    }
}

void GuiPanel::drawLightsSection() {
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show light gizmos", &showLightGizmos_);

        if (ImGui::Button("+ Directional")) addLight((int)LightType::Directional);
        ImGui::SameLine();
        if (ImGui::Button("+ Point")) addLight((int)LightType::Point);
        ImGui::SameLine();
        if (ImGui::Button("+ Spot")) addLight((int)LightType::Spot);

        if (!lights_.empty()) {
            ImGui::Separator();
            ImGui::Text("Active light:");
            ImGui::SliderInt("Index", &selectedLight_, 0, (int)lights_.size() - 1);

            LightCPU& L = lights_[selectedLight_];
            if (ImGui::Button("Place at camera")) {
                L.position = camPosRef_ + camDirRef_ * 2.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Focus camera on light")) {
                orbitCenterRef_ = L.position;
            }
            if (ImGui::Button("Aim to origin")) {
                L.direction = glm::normalize(-L.position);
            }
            ImGui::SameLine();
            if (ImGui::Button("Aim to camera dir")) {
                L.direction = glm::normalize(camDirRef_);
            }
            ImGui::SameLine();
            if (ImGui::Button("Aim to orbit center")) {
                L.direction = glm::normalize(orbitCenterRef_ - L.position);
            }
        }

        for (int i = 0; i < (int)lights_.size(); ++i) {
            ImGui::Separator();
            ImGui::PushID(i);

            const char* types[] = { "Directional","Point","Spot" };
            ImGui::Text("Light %d", i);

            bool* giz = &lights_[i].drawGizmo;
            std::string gizLbl = std::string("Gizmo visible##gizmo_") + std::to_string(i);
            ImGui::Checkbox(gizLbl.c_str(), giz);

            std::string typeLbl = std::string("Type##type_") + std::to_string(i);
            int t = (int)lights_[i].type;
            if (ImGui::Combo(typeLbl.c_str(), &t, types, IM_ARRAYSIZE(types))) {
                lights_[i].type = static_cast<LightType>(t);
            }

            if (lights_[i].type != LightType::Directional) {
                std::string posLbl = std::string("Position##pos_") + std::to_string(i);
                ImGui::DragFloat3(posLbl.c_str(), (float*)&lights_[i].position, 0.05f);
                std::string attC = std::string("Constant##attc_") + std::to_string(i);
                std::string attL = std::string("Linear##attl_") + std::to_string(i);
                std::string attQ = std::string("Quadratic##attq_") + std::to_string(i);
                ImGui::DragFloat(attC.c_str(), &lights_[i].constant, 0.005f, 0.0f, 5.0f);
                ImGui::DragFloat(attL.c_str(), &lights_[i].linear, 0.001f, 0.0f, 2.0f);
                ImGui::DragFloat(attQ.c_str(), &lights_[i].quadratic, 0.001f, 0.0f, 2.0f);
            }
            if (lights_[i].type != LightType::Point) {
                std::string dirLbl = std::string("Direction##dir_") + std::to_string(i);
                ImGui::DragFloat3(dirLbl.c_str(), (float*)&lights_[i].direction, 0.01f);
            }
            if (lights_[i].type == LightType::Spot) {
                float inner = Rad2Deg(std::acos(std::clamp(lights_[i].innerCutoff, -1.0f, 1.0f)));
                float outer = Rad2Deg(std::acos(std::clamp(lights_[i].outerCutoff, -1.0f, 1.0f)));
                std::string inLbl = std::string("Inner (deg)##in_") + std::to_string(i);
                std::string ouLbl = std::string("Outer (deg)##ou_") + std::to_string(i);
                ImGui::SliderFloat(inLbl.c_str(), &inner, 0.0f, 45.0f);
                ImGui::SliderFloat(ouLbl.c_str(), &outer, inner, 60.0f);
                if (outer < inner) outer = inner;
                lights_[i].innerCutoff = std::cos(Deg2Rad(inner));
                lights_[i].outerCutoff = std::cos(Deg2Rad(outer));
            }

            std::string colLbl = std::string("Color##col_") + std::to_string(i);
            ImGui::ColorEdit3(colLbl.c_str(), (float*)&lights_[i].color);
            std::string ambLbl = std::string("Ambient##amb_") + std::to_string(i);
            std::string difLbl = std::string("Diffuse##dif_") + std::to_string(i);
            std::string speLbl = std::string("Specular##spe_") + std::to_string(i);
            ImGui::SliderFloat(ambLbl.c_str(), &lights_[i].ambient, 0.0f, 1.0f);
            ImGui::SliderFloat(difLbl.c_str(), &lights_[i].diffuse, 0.0f, 2.0f);
            ImGui::SliderFloat(speLbl.c_str(), &lights_[i].specular, 0.0f, 2.0f);

            if (ImGui::Button("Select")) selectedLight_ = i;
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                lights_.erase(lights_.begin() + i);
                selectedLight_ = std::min(selectedLight_, (int)lights_.size() - 1);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
}

// ImGui: build the Lighting & Material panel and controls.
void GuiPanel::draw() {
    if (ImGui::Begin("Lighting & Material")) {
        drawMaterialSection();
        drawLightsSection();
    }
    ImGui::End();
}
#endif
