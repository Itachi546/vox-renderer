#version 460

#extension GL_GOOGLE_include_directive : enable

struct InstanceData {
    float x, y, z, w;
};

struct VertexData {
    float px, py, pz;
    float nx, ny, nz;
    float tu, tv;
};

layout(binding = 0, set = 1) readonly buffer Vertices {
    VertexData vertices[];
};

layout(binding = 1, set = 1) readonly buffer Instances {
    InstanceData instanceData[];
};

layout(push_constant) uniform PushConstant {
    mat4 VP;
};

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vUV;
layout(location = 2) out flat vec3 vColor;

void main() {
    VertexData vertex = vertices[gl_VertexIndex];
    InstanceData data = instanceData[gl_InstanceIndex];

    vec3 position = vec3(vertex.px, vertex.py, vertex.pz);
    vec3 normal = vec3(vertex.nx, vertex.ny, vertex.nz);

    const float scale = 0.5f;
    vec3 worldPos = position * scale + vec3(data.x, data.y, data.z);

    vNormal = normal;
    vUV = vec2(vertex.tu, vertex.tv) * 2.0f - 1.0f;

    uint color = uint(data.w);
    vColor = unpackUnorm4x8(color).rgb;

    gl_Position = VP * vec4(worldPos, 1.0f);
}
