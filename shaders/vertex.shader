#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;     // world space
    vec2 TexCoord;
    mat3 TBN;         // world-space TBN
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// 3x3 normal matrix = inverse(transpose(mat3(model)))
mat3 computeNormalMatrix(mat4 m) {
    mat3 M = mat3(m);
    return transpose(inverse(M));
}

void main() {
    mat3 Nmat = computeNormalMatrix(model);

    // Adding attributes to the world-space
    vec3 N = normalize(Nmat * aNormal);
    vec3 T_raw = normalize(Nmat * aTangent);
    vec3 B_raw = normalize(Nmat * aBitangent);

    // Orthogonalization of T to N (Gram–Schmidt) and reconstruction of B with a sign
    vec3 T = normalize(T_raw - N * dot(N, T_raw));
    float handedness = (dot(cross(N, T), B_raw) < 0.0) ? -1.0 : 1.0;
    vec3 B = normalize(cross(N, T)) * handedness;

    vs_out.TBN = mat3(T, B, N);

    vec4 wp = model * vec4(aPos, 1.0);
    vs_out.FragPos = wp.xyz;
    vs_out.TexCoord = aTex;
    gl_Position = projection * view * wp;
}
