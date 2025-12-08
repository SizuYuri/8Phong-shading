#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoord;
    mat3 TBN;   // world-space
} fs_in;

struct Light {
    int   type;        // 0=Directional, 1=Point, 2=Spot
    vec3  position;    // for point/spot (world)
    vec3  direction;   // for dir/spot  (world, from light pointing OUT)
    float innerCutoff; // cos(innerAngle)
    float outerCutoff; // cos(outerAngle)
    float constant, linear, quadratic;
    vec3  color;
    float ambient, diffuse, specular;
};

uniform int   numLights;
uniform Light lights[8];

uniform vec3  viewPos;       // world
uniform vec3  objectColor;   // albedo
uniform float shininess;

uniform sampler2D normalMap;
uniform bool  useNormalMap;
uniform bool  flipNormalY;   // ON if the card is from D3D/Unreal (green is inverted)

vec3 fetchNormalTS(vec2 uv) {
    vec3 n = texture(normalMap, uv).rgb;
    // from [0,1] -> [-1,1]
    n = n * 2.0 - 1.0;
    if (flipNormalY) n.g = -n.g;
    return normalize(n);
}

vec3 getWorldNormal() {
    if (useNormalMap) {
        vec3 n_ts = fetchNormalTS(fs_in.TexCoord);
        return normalize(fs_in.TBN * n_ts); // TS -> world
    } else {
        return normalize(fs_in.TBN[2]);     // column N
    }
}

void main() {
    vec3 N = getWorldNormal();
    vec3 V = normalize(viewPos - fs_in.FragPos);

    vec3 total = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        Light L = lights[i];

        vec3 Ldir;        // the direction from the fragment to the source
        float attenuation = 1.0;
        float spotMask    = 1.0;

        if (L.type == 0) {
            // directional: its direction looks FROM the source, we need to go to the fragment
            Ldir = normalize(-L.direction);
        } else {
            vec3 toL = L.position - fs_in.FragPos;
            float dist = length(toL);
            Ldir = toL / max(dist, 1e-6);

            attenuation = 1.0 / max(L.constant + L.linear * dist + L.quadratic * dist * dist, 1e-6);

            if (L.type == 2) {
                // the angle between the spotlight axis (looking FROM the source) and the beam TOWARDS the fragment
                float theta = dot(normalize(-Ldir), normalize(L.direction));
                float eps = max(L.innerCutoff - L.outerCutoff, 1e-5);
                spotMask = clamp((theta - L.outerCutoff) / eps, 0.0, 1.0);
            }
        }

        // Phong
        float NdotL = max(dot(N, Ldir), 0.0);
        vec3  diffuse  = L.diffuse  * NdotL * L.color;

        vec3  R = reflect(-Ldir, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        vec3  specular = L.specular * spec * L.color;

        vec3 ambient = L.ambient * L.color;

        total += (ambient + (diffuse + specular) * spotMask) * attenuation;
    }

    FragColor = vec4(total * objectColor, 1.0);
}
