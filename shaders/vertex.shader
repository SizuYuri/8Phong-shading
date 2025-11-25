#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
out VS_OUT{ vec3 FragPos; vec2 TexCoord; mat3 TBN; } vs_out;
uniform mat4 model, view, projection;
void main(){
    vec3 T = normalize(mat3(model)*aTangent);
    vec3 B = normalize(mat3(model)*aBitangent);
    vec3 N = normalize(mat3(model)*aNormal);
    vs_out.TBN = mat3(T,B,N);
    vec4 wp = model*vec4(aPos,1.0);
    vs_out.FragPos = wp.xyz;
    vs_out.TexCoord = aTex;
    gl_Position = projection*view*wp;
}