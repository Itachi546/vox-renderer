#version 460

#extension GL_GOOGLE_include_directive : enable 
#include "voxelizer.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 vWorldPos[];

layout(location = 0) out vec3 gVoxelWorldPos;

void main() {

    vec3 p0 = vWorldPos[0];
    vec3 p1 = vWorldPos[1];
    vec3 p2 = vWorldPos[2];

    vec3 normal = cross(p1 - p0, p2 - p0);
    uint dominantAxis = FindDominantAxis(normal);
  
    for (int i = 0; i < 3; ++i) {
        // Convert to clipspace position and project it along dominant axis
        vec3 voxelSpacePos = ToVoxelSpace(vWorldPos[i]);
        gVoxelWorldPos = clamp(voxelSpacePos * 0.5 + 0.5, vec3(0), vec3(1)) * VOXEL_GRID_SIZE;

        vec3 projectedPosition = ProjectAlongDominantAxis(voxelSpacePos, dominantAxis);
        
        gl_Position = vec4(projectedPosition, 1.0f);
        EmitVertex();
    }

    EndPrimitive();
}