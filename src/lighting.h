#pragma once
#include <glm/glm.hpp>

enum class LightType : int { Directional = 0, Point = 1, Spot = 2 };

struct LightCPU {
    LightType type = LightType::Directional;

    glm::vec3 position{ 0.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };

    float innerCutoff = 0.0f;  // cos(inner)
    float outerCutoff = 0.0f;  // cos(outer)

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    glm::vec3 color{ 1.0f };
    float ambient = 0.05f;
    float diffuse = 0.9f;
    float specular = 0.3f;

    bool drawGizmo = true;
    bool followCamera = false; // для прожектора, если нужно «прилипить» к камере
};
