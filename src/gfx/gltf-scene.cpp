#include "pch.h"

#include "gltf-scene.h"

#include "gltf-loader.h"
#include "tinygltf/tinygltf.h"
#include "stb_image.h"
#include "async-loader.h"
#include "rendering/rendering-utils.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
static uint8_t *getBufferPtr(tinygltf::Model *model, const tinygltf::Accessor &accessor) {
    tinygltf::BufferView &bufferView = model->bufferViews[accessor.bufferView];
    return model->buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;
}

void GLTFScene::ParseMaterial(tinygltf::Model *model, MaterialInfo *component, uint32_t matIndex) {
    if (matIndex == -1)
        return;

    tinygltf::Material &material = model->materials[matIndex];
    // component->alphaCutoff = (float)material.alphaCutoff;
    /*
    if (material.alphaMode == "BLEND")
        component->alphaMode = ALPHAMODE_BLEND;
    else if (material.alphaMode == "MASK")
        component->alphaMode = ALPHAMODE_MASK;
        */

    // Parse Material value
    tinygltf::PbrMetallicRoughness &pbr = material.pbrMetallicRoughness;
    std::vector<double> &baseColor = pbr.baseColorFactor;
    component->albedo = glm::vec4((float)baseColor[0], (float)baseColor[1], (float)baseColor[2], (float)baseColor[3]);
    component->transparency = (float)baseColor[3];
    component->metallic = (float)pbr.metallicFactor;
    component->roughness = (float)pbr.roughnessFactor;
    std::vector<double> &emissiveColor = material.emissiveFactor;
    component->emissive = glm::vec4((float)emissiveColor[0], (float)emissiveColor[1], (float)emissiveColor[2], 1.0f);

    // Parse Material texture
    /*
    auto loadTexture = [&](uint32_t index) {
        tinygltf::Texture& texture = model->textures[index];
        tinygltf::Image& image = model->images[texture.source];
        const std::string& name = image.uri.length() == 0 ? image.name : image.uri;
        return TextureCache::LoadTexture(name, image.width, image.height, image.image.data(), image.component, true);
        };
    */
    auto loadTexture = [&](uint32_t index) -> uint32_t {
        tinygltf::Texture &texture = model->textures[index];
        tinygltf::Image &image = model->images[texture.source];
        if (image.uri.length() > 0) {
            // @TODO implement better mechanism
            std::string texturePath = _meshBasePath + image.uri;
            uint32_t hash = DJB2Hash(texturePath);
            LOG("Path: " + texturePath + " Hash: " + std::to_string(hash));
            auto found = textureMap.find(hash);
            if (found != textureMap.end()) {
                return (uint32_t)found->second.id;
            } else {
                int width, height, comp;
                int res = stbi_info(texturePath.c_str(), &width, &height, &comp);

                RD::SamplerDescription samplerDesc = RD::SamplerDescription::Initialize();
                RD::TextureDescription desc = RD::TextureDescription::Initialize(width, height);
                desc.usageFlags = RD::TEXTURE_USAGE_SAMPLED_BIT | RD::TEXTURE_USAGE_TRANSFER_DST_BIT;
                if (res == 0) {
                    LOGE("Failed to get texture info from the file" + texturePath);
                    return ~0u;
                }
                desc.format = RD::FORMAT_R8G8B8A8_UNORM;
                desc.mipMaps = 1;
                desc.samplerDescription = &samplerDesc;

                TextureID textureId = device->CreateTexture(&desc, image.uri);
                textureMap[hash] = textureId;
                asyncLoader->RequestTextureLoad(texturePath, textureId);
                return (uint32_t)textureId.id;
            }
        }
        return ~0u;
    };

    if (pbr.baseColorTexture.index >= 0)
        component->albedoMap = loadTexture(pbr.baseColorTexture.index);
    /*
    if (pbr.metallicRoughnessTexture.index >= 0)
        component->metallicRoughnessMap = loadTexture(pbr.metallicRoughnessTexture.index);

    if (material.emissiveTexture.index >= 0)
        component->emissiveMap = loadTexture(material.emissiveTexture.index);
        */
}

bool GLTFScene::ParseMesh(tinygltf::Model *model, tinygltf::Mesh &mesh, MeshGroup *meshGroup, const glm::mat4 &transform) {
    for (auto &primitive : mesh.primitives) {
        // Parse position
        const tinygltf::Accessor &positionAccessor = model->accessors[primitive.attributes["POSITION"]];
        float *positions = (float *)getBufferPtr(model, positionAccessor);
        uint32_t numPosition = (uint32_t)positionAccessor.count;

        // Parse normals
        float *normals = nullptr;
        auto normalAttributes = primitive.attributes.find("NORMAL");
        if (normalAttributes != primitive.attributes.end()) {
            const tinygltf::Accessor &normalAccessor = model->accessors[normalAttributes->second];
            assert(numPosition == normalAccessor.count);
            normals = (float *)getBufferPtr(model, normalAccessor);
        }

        // Parse tangents
        float *tangents = nullptr;
        auto tangentAttributes = primitive.attributes.find("TANGENT");
        if (tangentAttributes != primitive.attributes.end()) {
            const tinygltf::Accessor &tangentAccessor = model->accessors[normalAttributes->second];
            assert(numPosition == tangentAccessor.count);
            tangents = (float *)getBufferPtr(model, tangentAccessor);
        }

        // Parse UV
        float *uvs = nullptr;
        auto uvAttributes = primitive.attributes.find("TEXCOORD_0");
        if (uvAttributes != primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor = model->accessors[uvAttributes->second];
            assert(numPosition == uvAccessor.count);
            uvs = (float *)getBufferPtr(model, uvAccessor);
        }

        std::vector<Vertex> &vertices = meshGroup->vertices;
        std::vector<uint32_t> &indices = meshGroup->indices;
        uint32_t vertexOffset = (uint32_t)(vertices.size() * sizeof(Vertex));
        uint32_t indexOffset = (uint32_t)(indices.size() * sizeof(uint32_t));

        for (uint32_t i = 0; i < numPosition; ++i) {
            Vertex vertex;
            vertex.position = {positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]};

            if (normals)
                vertex.normal = {normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]};
            else
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

            if (uvs)
                vertex.uv = {uvs[i * 2 + 0], 1.0f - uvs[i * 2 + 1]};
            else
                vertex.uv = {0.0f, 0.0f};

            vertices.push_back(std::move(vertex));
        }

        const tinygltf::Accessor &indicesAccessor = model->accessors[primitive.indices];
        uint32_t indexCount = (uint32_t)indicesAccessor.count;
        if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            uint32_t *indicesPtr = (uint32_t *)getBufferPtr(model, indicesAccessor);
            indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
        } else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            uint16_t *indicesPtr = (uint16_t *)getBufferPtr(model, indicesAccessor);
            indices.insert(indices.end(), indicesPtr, indicesPtr + indexCount);
        } else {
            std::cerr << "Undefined indices componentType: " + std::to_string(indicesAccessor.componentType) << std::endl;
            assert(0);
        }

        glm::vec3 minExtent = glm::vec3(positionAccessor.minValues[0], positionAccessor.minValues[1], positionAccessor.minValues[2]);
        glm::vec3 maxExtent = glm::vec3(positionAccessor.maxValues[0], positionAccessor.maxValues[1], positionAccessor.maxValues[2]);

        meshGroup->transforms.push_back(transform);
        RD::DrawElementsIndirectCommand drawCommand = {};
        drawCommand.count = indexCount;
        drawCommand.instanceCount = 1;
        drawCommand.firstIndex = indexOffset / sizeof(uint32_t);
        drawCommand.baseVertex = vertexOffset / sizeof(Vertex);
        drawCommand.baseInstance = 0;
        drawCommand.drawId = (uint32_t)meshGroup->drawCommands.size();
        meshGroup->drawCommands.push_back(std::move(drawCommand));

        MaterialInfo material = {};
        std::string materialName = model->materials[primitive.material].name;
        meshGroup->names.push_back(materialName);
        ParseMaterial(model, &material, primitive.material);
        meshGroup->materials.push_back(std::move(material));
    }

    return true;
}

void GLTFScene::ParseNodeHierarchy(tinygltf::Model *model, int nodeIndex, MeshGroup *meshGroup) {
    tinygltf::Node &node = model->nodes[nodeIndex];

    // Create entity and write the transforms
    glm::mat4 translation = glm::mat4(1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    if (node.translation.size() > 0)
        translation = glm::translate(glm::mat4(1.0f), glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
    if (node.rotation.size() > 0)
        rotation = glm::mat4_cast(glm::fquat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]));
    if (node.scale.size() > 0)
        scale = glm::scale(glm::mat4(1.0f), glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));

    glm::mat4 transform = translation * rotation * scale;
    // Update MeshData
    if (node.mesh >= 0) {
        tinygltf::Mesh &mesh = model->meshes[node.mesh];
        ParseMesh(model, mesh, meshGroup, transform);
    }

    for (auto child : node.children)
        ParseNodeHierarchy(model, child, meshGroup);
}

void GLTFScene::ParseScene(tinygltf::Model *model,
                           tinygltf::Scene *scene,
                           MeshGroup *meshGroup) {
    for (auto node : scene->nodes)
        ParseNodeHierarchy(model, node, meshGroup);
}

bool GLTFScene::LoadFile(const std::string &filename, MeshGroup *meshGroup) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!ret) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
        if (!ret) {
            if (!warn.empty()) {
                LOGW(warn);
            }
            if (!err.empty()) {
                LOGE("Error::" + err);
            }
            return false;
        }
    }

    for (auto &scene : model.scenes)
        ParseScene(&model, &scene, meshGroup);

    return true;
}

bool GLTFScene::Initialize(const std::vector<std::string> &filenames, std::shared_ptr<AsyncLoader> loader, BufferID globalUB) {
    device = RD::GetInstance();
    asyncLoader = loader;

    RD::UniformBinding vsBindings[] = {
        {RD::BINDING_TYPE_UNIFORM_BUFFER, 0, 0},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, 0},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, 1},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, 2},
    };

    RD::UniformBinding fsBindings[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, 3},
    };

    ShaderID shaders[2] = {
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/mesh.vert.spv", vsBindings, (uint32_t)std::size(vsBindings), nullptr, 0),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/mesh.frag.spv", fsBindings, (uint32_t)std::size(fsBindings), nullptr, 0),
    };

    RD::Format colorAttachmentFormat[] = {RD::FORMAT_B8G8R8A8_UNORM};
    RD::BlendState blendState = RD::BlendState::Create();
    RD::RasterizationState rasterizationState = RD::RasterizationState::Create();
    rasterizationState.frontFace = RD::FRONT_FACE_COUNTER_CLOCKWISE;
    RD::DepthState depthState = RD::DepthState::Create();
    depthState.enableDepthWrite = true;
    depthState.enableDepthTest = true;

    renderPipeline = device->CreateGraphicsPipeline(shaders,
                                                    (uint32_t)std::size(shaders),
                                                    RD::TOPOLOGY_TRIANGLE_LIST,
                                                    &rasterizationState,
                                                    &depthState,
                                                    colorAttachmentFormat,
                                                    &blendState,
                                                    1,
                                                    RD::FORMAT_D24_UNORM_S8_UINT,
                                                    "MeshDrawPipeline");

    device->Destroy(shaders[0]);
    device->Destroy(shaders[1]);

    RD::BoundUniform globalBinding{
        .bindingType = RD::BINDING_TYPE_UNIFORM_BUFFER,
        .binding = 0,
        .resourceID = globalUB,
        .offset = 0,
    };

    globalSet = device->CreateUniformSet(renderPipeline, &globalBinding, 1, 0, "GlobalUniformSet");

    for (int i = 0; i < filenames.size(); ++i) {
        _meshBasePath = std::filesystem::path(filenames[i]).remove_filename().string();
        if (!LoadFile(filenames[i].c_str(), &meshGroup))
            return false;
    }
    return true;
}

void GLTFScene::PrepareDraws() {
    uint32_t vertexSize = static_cast<uint32_t>(meshGroup.vertices.size() * sizeof(Vertex));
    vertexBuffer = device->CreateBuffer(vertexSize, RD::BUFFER_USAGE_STORAGE_BUFFER_BIT | RD::BUFFER_USAGE_TRANSFER_DST_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "VertexBuffer");

    uint32_t indexSize = static_cast<uint32_t>(meshGroup.indices.size() * sizeof(uint32_t));
    indexBuffer = device->CreateBuffer(indexSize, RD::BUFFER_USAGE_INDEX_BUFFER_BIT | RD::BUFFER_USAGE_TRANSFER_DST_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "IndexBuffer");

    uint32_t drawCommandSize = static_cast<uint32_t>(meshGroup.drawCommands.size() * sizeof(RD::DrawElementsIndirectCommand));
    drawCommandBuffer = device->CreateBuffer(drawCommandSize, RD::BUFFER_USAGE_STORAGE_BUFFER_BIT | RD::BUFFER_USAGE_TRANSFER_DST_BIT | RD::BUFFER_USAGE_INDIRECT_BUFFER_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "DrawCommandBuffer");

    uint32_t transformSize = static_cast<uint32_t>(meshGroup.transforms.size() * sizeof(glm::mat4));
    transformBuffer = device->CreateBuffer(transformSize, RD::BUFFER_USAGE_STORAGE_BUFFER_BIT | RD::BUFFER_USAGE_TRANSFER_DST_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "TransformBuffer");

    uint32_t materialSize = static_cast<uint32_t>(meshGroup.materials.size() * sizeof(MaterialInfo));
    materialBuffer = device->CreateBuffer(materialSize, RD::BUFFER_USAGE_STORAGE_BUFFER_BIT | RD::BUFFER_USAGE_TRANSFER_DST_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "Material Buffer");

    uint32_t maxSize = std::max({vertexSize, indexSize, transformSize, materialSize, drawCommandSize});

    BufferID stagingBuffer = device->CreateBuffer(maxSize, RD::BUFFER_USAGE_TRANSFER_SRC_BIT, RD::MEMORY_ALLOCATION_TYPE_CPU, "Temp Staging Buffer");
    uint8_t *stagingBufferPtr = device->MapBuffer(stagingBuffer);

    BufferUploadRequest uploadRequests[] = {
        {meshGroup.vertices.data(), vertexBuffer, vertexSize},
        {meshGroup.indices.data(), indexBuffer, indexSize},
        {meshGroup.drawCommands.data(), drawCommandBuffer, drawCommandSize},
        {meshGroup.transforms.data(), transformBuffer, transformSize},
        {meshGroup.materials.data(), materialBuffer, materialSize},
    };

    // Move to transfer queue
    for (auto &request : uploadRequests) {
        std::memcpy(stagingBufferPtr, request.data, request.size);

        device->ImmediateSubmit([&](CommandBufferID commandBuffer) {
            RD::BufferCopyRegion copyRegion{
                0, 0, request.size};
            device->CopyBuffer(commandBuffer, stagingBuffer, request.bufferId, &copyRegion);
        },
                                nullptr);
    }

    device->Destroy(stagingBuffer);

    RD::BoundUniform boundedUniform[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, vertexBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, drawCommandBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 2, transformBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 3, materialBuffer},
    };
    meshBindingSet = device->CreateUniformSet(renderPipeline, boundedUniform, static_cast<uint32_t>(std::size(boundedUniform)), 1, "MeshBindingSet");
}

void GLTFScene::Render(CommandBufferID commandBuffer) {
    if (drawCommandBuffer && meshGroup.drawCommands.size() > 0) {
        device->BindPipeline(commandBuffer, renderPipeline);

        UniformSetID uniformSets[] = {globalSet, meshBindingSet};
        device->BindUniformSet(commandBuffer, renderPipeline, uniformSets, (uint32_t)std::size(uniformSets));
        device->BindIndexBuffer(commandBuffer, indexBuffer);
        device->DrawIndexedIndirect(commandBuffer, drawCommandBuffer, 0, (uint32_t)meshGroup.drawCommands.size(), sizeof(RD::DrawElementsIndirectCommand));
    }
}

void GLTFScene::UpdateTextures(CommandBufferID commandBuffer) {
    // Transfer Queue Ownership
    QueueID transferQueue = device->GetDeviceQueue(RD::QUEUE_TYPE_TRANSFER);
    QueueID mainQueue = device->GetDeviceQueue(RD::QUEUE_TYPE_GRAPHICS);

    std::lock_guard lock{textureUpdateMutex};
    uint32_t textureUpdateCount = static_cast<uint32_t>(texturesToUpdate.size());
    std::vector<RD::TextureBarrier> barriers(textureUpdateCount);
    for (uint32_t i = 0; i < textureUpdateCount; ++i) {
        barriers[i].texture = texturesToUpdate[i];
        barriers[i].srcAccess = 0,
        barriers[i].dstAccess = RD::BARRIER_ACCESS_SHADER_READ_BIT;
        barriers[i].newLayout = RD::TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[i].srcQueueFamily = transferQueue;
        barriers[i].dstQueueFamily = mainQueue;
        device->UpdateBindlessTexture(texturesToUpdate[i]);
    }
    device->PipelineBarrier(commandBuffer, RD::PIPELINE_STAGE_TOP_OF_PIPE_BIT, RD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barriers.data(), textureUpdateCount);
}

void GLTFScene::Shutdown() {
    device->Destroy(globalSet);
    device->Destroy(meshBindingSet);
    device->Destroy(renderPipeline);
    if (meshGroup.drawCommands.size() > 0) {
        device->Destroy(vertexBuffer);
        device->Destroy(indexBuffer);
        device->Destroy(transformBuffer);
        device->Destroy(drawCommandBuffer);
    }
    device->Destroy(materialBuffer);
    for (auto &[key, val] : textureMap)
        device->Destroy(val);
}
