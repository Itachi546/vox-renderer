#include "mesh.h"

#include <iostream>
#include <vector>

namespace gfx {

    Mesh::Mesh(float *vertices, uint32_t vertexCount, uint32_t *indices, uint32_t indexCount) : vertexBuffer(~0u),
                                                                                                indexBuffer(~0u),
                                                                                                numVertices(0) {
        Initialize(vertices, vertexCount, indices, indexCount);
    }

    void Mesh::Initialize(float *vertices, uint32_t vertexCount, uint32_t *indices, uint32_t indexCount) {
        if (vertexCount == 0 || indexCount == 0) {
            std::cerr << "VertexCount: " << vertexCount << " IndexCount: " << indexCount << std::endl;
            return;
        }

        RD *device = RD::GetInstance();

        uint32_t vertexSize = static_cast<uint32_t>(sizeof(float) * vertexCount);
        uint32_t indexSize = sizeof(uint32_t) * indexCount;

        BufferID stagingBuffer = device->CreateBuffer(vertexSize + indexSize, RD::BUFFER_USAGE_TRANSFER_SRC_BIT, RD::MEMORY_ALLOCATION_TYPE_CPU, "StagingBuffer");
        void *sbPtr = device->MapBuffer(stagingBuffer);

        std::memcpy(sbPtr, vertices, vertexSize);
        std::memcpy((uint8_t *)sbPtr + vertexSize, indices, indexSize);

        vertexBuffer = device->CreateBuffer(vertexSize, RD::BUFFER_USAGE_TRANSFER_DST_BIT | RD::BUFFER_USAGE_STORAGE_BUFFER_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "MeshVertexBuffer");
        indexBuffer = device->CreateBuffer(sizeof(uint32_t) * indexCount, RD::BUFFER_USAGE_TRANSFER_DST_BIT | RD::BUFFER_USAGE_INDEX_BUFFER_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "MeshIndexBuffer");

        device->ImmediateSubmit([&](CommandBufferID commandBuffer) {
            RD::BufferCopyRegion copyRegion{0, 0, vertexSize};
            device->CopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, &copyRegion);
            copyRegion.srcOffset = vertexSize;
            copyRegion.size = indexSize;
            device->CopyBuffer(commandBuffer, stagingBuffer, indexBuffer, &copyRegion);
        });
        device->Destroy(stagingBuffer);

        numVertices = indexCount;
    }

    void Mesh::CreatePlane(Mesh *mesh, int width, int height) {
        std::vector<Vertex> vertices;
        float invWidth = 1.0f / float(width);
        float invHeight = 1.0f / float(height);

        float tX = -width * 0.5f;
        float tY = -height * 0.5f;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                glm::vec3 position{float(x) + tX, 0.0f, float(y) + tY};
                glm::vec2 uv{position.x * invWidth, position.y * invHeight};
                vertices.emplace_back(Vertex{position, glm::vec3{0.0f, 1.0f, 0.0f}, uv});
            }
        }

        std::vector<uint32_t> indices;
        for (int i = 0; i < height - 1; ++i) {
            for (int j = 0; j < width - 1; ++j) {
                int p0 = i * width + j;
                int p1 = p0 + 1;
                int p2 = (i + 1) * width + j;
                int p3 = p2 + 1;
                indices.push_back(p2);
                indices.push_back(p1);
                indices.push_back(p0);

                indices.push_back(p2);
                indices.push_back(p3);
                indices.push_back(p1);
            }
        }
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size()) * 8;
        uint32_t indexCount = static_cast<uint32_t>(indices.size());
        mesh->Initialize((float *)vertices.data(), vertexCount, indices.data(), indexCount);
    }

    void Mesh::CreateCube(Mesh *mesh) {
        std::vector<Vertex> vertices{
            // Front face
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)}, // Bottom-left
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},  // Bottom-right
            {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},   // Top-right
            {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},  // Top-left
            // Back face
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f)}, // Bottom-left
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f)},  // Bottom-right
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f)},   // Top-right
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f)},  // Top-left
            // Top face
            {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},  // Top-left
            {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},   // Top-right
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // Bottom-right
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)}, // Bottom-left
            // Bottom face
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},  // Top-left
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},   // Top-right
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // Bottom-right
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)}, // Bottom-left
            // Right face
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},  // Bottom-left
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)}, // Bottom-right
            {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},   // Top-right
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // Top-left
            // Left face
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},  // Bottom-left
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)}, // Bottom-right
            {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},   // Top-right
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},  // Top-left
        };

        std::vector<unsigned int> indices = {
            0, 1, 2, // Front face
            0, 2, 3,
            4, 6, 5, // Back face
            6, 4, 7,
            8, 9, 10, // Top face
            8, 10, 11,
            13, 12, 14, // Bottom face
            14, 12, 15,
            16, 17, 18, // Right face
            18, 17, 19,
            20, 22, 21, // Left face
            21, 22, 23};

        uint32_t vertexCount = static_cast<uint32_t>(vertices.size()) * 8;
        uint32_t indexCount = static_cast<uint32_t>(indices.size());
        mesh->Initialize((float *)vertices.data(), vertexCount, indices.data(), indexCount);
    }

    Mesh::~Mesh() {
        RD *device = RD::GetInstance();
        device->Destroy(vertexBuffer);
        device->Destroy(indexBuffer);
    }

} // namespace gfx
