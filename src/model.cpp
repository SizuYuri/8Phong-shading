#include "model.h"
#include "shader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

Model::Model(const std::string& path){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate|aiProcess_FlipUVs|aiProcess_CalcTangentSpace);
    if(!scene || !scene->mRootNode){ std::cerr<<"ASSIMP: "<<importer.GetErrorString()<<std::endl; return; }
    for(unsigned i=0;i<scene->mNumMeshes;i++){
        aiMesh* m=scene->mMeshes[i];
        for(unsigned j=0;j<m->mNumVertices;j++){
            Vertex v{};
            v.Position = { (float)m->mVertices[j].x,(float)m->mVertices[j].y,(float)m->mVertices[j].z };
            if(m->HasNormals()) v.Normal = { (float)m->mNormals[j].x,(float)m->mNormals[j].y,(float)m->mNormals[j].z };
            if(m->mTextureCoords[0]) v.TexCoords = { (float)m->mTextureCoords[0][j].x,(float)m->mTextureCoords[0][j].y };
            if(m->HasTangentsAndBitangents()){
                v.Tangent = { (float)m->mTangents[j].x,(float)m->mTangents[j].y,(float)m->mTangents[j].z };
                v.Bitangent = { (float)m->mBitangents[j].x,(float)m->mBitangents[j].y,(float)m->mBitangents[j].z };
            } else { v.Tangent={1,0,0}; v.Bitangent={0,1,0}; }
            vertices.push_back(v);
        }
        for(unsigned f=0; f<m->mNumFaces; ++f){
            aiFace face = m->mFaces[f];
            for(unsigned k=0;k<face.mNumIndices;k++) indices.push_back(face.mIndices[k]);
        }
    }
    setupMesh();
}

void Model::setupMesh(){
    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO); glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,Normal));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,TexCoords));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,Tangent));
    glEnableVertexAttribArray(4); glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,Bitangent));
    glBindVertexArray(0);
}
void Model::Draw(Shader&){ glBindVertexArray(VAO); glDrawElements(GL_TRIANGLES,(GLsizei)indices.size(),GL_UNSIGNED_INT,0); glBindVertexArray(0); }
