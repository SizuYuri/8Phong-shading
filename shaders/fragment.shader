#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoord;
    mat3 TBN;
} fs_in;

struct Light {
    int  type;         // 0=Directional,1=Point,2=Spot
    vec3 position;
    vec3 direction;
    float innerCutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
};

uniform int numLights;
uniform Light lights[8];

uniform vec3 viewPos;
uniform vec3 objectColor;
uniform float shininess;

uniform sampler2D normalMap;
uniform bool useNormalMap;

vec3 getNormal() {
    if (useNormalMap) {
        vec3 n = texture(normalMap, fs_in.TexCoord).rgb;
        n = normalize(n * 2.0 - 1.0);
        return normalize(fs_in.TBN * n);
    } else {
        return normalize(fs_in.TBN[2]);
    }
}

void main() {
    vec3 N = getNormal();
    vec3 V = normalize(viewPos - fs_in.FragPos);

    vec3 total = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        Light Lgt = lights[i];

        vec3 Ldir;
        float attenuation = 1.0;
        float spot = 1.0;

        if (Lgt.type == 0) {
            Ldir = normalize(-Lgt.direction);
        } else if (Lgt.type == 1) {
            vec3 toL = Lgt.position - fs_in.FragPos;
            float dist = length(toL);
            Ldir = toL / dist;
            attenuation = 1.0 / (Lgt.constant + Lgt.linear * dist + Lgt.quadratic * dist * dist);
        } else {
            vec3 toL = Lgt.position - fs_in.FragPos;
            float dist = length(toL);
            Ldir = toL / dist;
            float theta = dot(normalize(-Ldir), normalize(Lgt.direction));
            float eps = max(Lgt.innerCutoff - Lgt.outerCutoff, 1e-4);
            spot = clamp((theta - Lgt.outerCutoff) / eps, 0.0, 1.0);
            attenuation = 1.0 / (Lgt.constant + Lgt.linear * dist + Lgt.quadratic * dist * dist);
        }

        vec3 ambient  = Lgt.ambient * Lgt.color;
        float diff = max(dot(N, Ldir), 0.0);
        vec3 diffuse = Lgt.diffuse * diff * Lgt.color;
        vec3 R = reflect(-Ldir, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        vec3 specular = Lgt.specular * spec * Lgt.color;

        total += (ambient + diffuse + specular) * attenuation * spot;
    }

    FragColor = vec4(total * objectColor, 1.0);
}
