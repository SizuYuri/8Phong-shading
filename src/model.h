#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class Shader;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    Vertex():Position(0),Normal(0),TexCoords(0),Tangent(0),Bitangent(0) {}
};

class Model {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Model(const std::string& path);
    void Draw(Shader& shader);
private:
    unsigned int VAO=0,VBO=0,EBO=0;
    void setupMesh();
};
#endif
