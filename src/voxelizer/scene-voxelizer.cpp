#include "pch.h"
#include "voxelizer.h"

#include "gfx/render-scene.h"
#include "gfx/gltf-scene.h"
#include "gfx/camera.h"
#include "rendering/rendering-utils.h"
#include "scene-voxelizer.h"

SceneVoxelizer::SceneVoxelizer(std::shared_ptr<RenderScene> scene) : scene(scene) {
}

void SceneVoxelizer::Initialize(uint32_t resolution) {
    this->voxelResolution = resolution;

    device = RD::GetInstance();

    voxelCountBuffer = device->CreateBuffer(static_cast<uint32_t>(sizeof(uint32_t) * 2),
                                            RD::BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            RD::MEMORY_ALLOCATION_TYPE_CPU,
                                            "Voxel Count Buffer");
    countBufferPtr = (uint32_t *)device->MapBuffer(voxelCountBuffer);
    std::memset(countBufferPtr, 0, sizeof(uint32_t) * 2);

    InitializePrepassResources();
    InitializeMainResources();
    // InitializeRayMarchResources();
}

void SceneVoxelizer::InitializePrepassResources() {
    RD::UniformBinding vsBindings[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 1},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 2},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 3},
    };

    RD::PushConstant pushConstant[] = {
        {0, static_cast<uint32_t>(sizeof(float)) * 2},
        {8, static_cast<uint32_t>(sizeof(uint32_t))},
    };

    std::shared_ptr<GLTFScene>
        gltfScene = std::static_pointer_cast<GLTFScene>(scene);

    RD::RasterizationState rasterizationState = RD::RasterizationState::Create();
    rasterizationState.enableConservative = enableConservativeRasterization;
    rasterizationState.cullMode = RD::CULL_MODE_NONE;
    RD::DepthState depthState = RD::DepthState::Create();
    depthState.enableDepthWrite = false;
    depthState.enableDepthTest = false;

    // Create prepass pipeline
    RD::UniformBinding fsBindings[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 4},
    };
    ShaderID shaders[3] = {
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer-prepass.vert.spv", vsBindings, (uint32_t)std::size(vsBindings), nullptr, 0),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer-prepass.geom.spv", nullptr, 0, &pushConstant[0], 1),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer-prepass.frag.spv", fsBindings, (uint32_t)std::size(fsBindings), &pushConstant[1], 1),
    };
    prepassPipeline = device->CreateGraphicsPipeline(shaders,
                                                     (uint32_t)std::size(shaders),
                                                     RD::TOPOLOGY_TRIANGLE_LIST,
                                                     &rasterizationState,
                                                     &depthState,
                                                     nullptr,
                                                     nullptr,
                                                     0,
                                                     RD::FORMAT_UNDEFINED,
                                                     false,
                                                     "SceneVoxelizer Prepass");

    device->Destroy(shaders[0]);
    device->Destroy(shaders[1]);
    device->Destroy(shaders[2]);

    RD::BoundUniform boundedUniform[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 1, gltfScene->vertexBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 2, gltfScene->drawCommandBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 3, gltfScene->transformBuffer},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 4, voxelCountBuffer},
    };
    prepassSet = device->CreateUniformSet(prepassPipeline, boundedUniform, static_cast<uint32_t>(std::size(boundedUniform)), 0, "Prepass Binding");
}

void SceneVoxelizer::InitializeMainResources() {
    // Create prepass pipeline
    RD::UniformBinding vsBindings[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 1},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 2},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 3},
    };

    RD::UniformBinding fsBindings[] = {
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 4},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 5},
        {RD::BINDING_TYPE_STORAGE_BUFFER, 0, 6},
        /*{RD::BINDING_TYPE_IMAGE, 0, 7},*/
    };
    RD::PushConstant pushConstant[] = {
        {0, static_cast<uint32_t>(sizeof(float)) * 2},
        {8, static_cast<uint32_t>(sizeof(uint32_t))},
    };

    ShaderID shaders[3] = {
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer.vert.spv", vsBindings, (uint32_t)std::size(vsBindings), nullptr, 0),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer.geom.spv", nullptr, 0, &pushConstant[0], 1),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/voxelizer.frag.spv", fsBindings, (uint32_t)std::size(fsBindings), &pushConstant[1], 1),
    };

    RD::RasterizationState rasterizationState = RD::RasterizationState::Create();
    rasterizationState.cullMode = RD::CULL_MODE_NONE;
    rasterizationState.enableConservative = enableConservativeRasterization;
    RD::DepthState depthState = RD::DepthState::Create();
    depthState.enableDepthWrite = false;
    depthState.enableDepthTest = false;

    mainPipeline = device->CreateGraphicsPipeline(shaders,
                                                  (uint32_t)std::size(shaders),
                                                  RD::TOPOLOGY_TRIANGLE_LIST,
                                                  &rasterizationState,
                                                  &depthState,
                                                  nullptr,
                                                  nullptr,
                                                  0,
                                                  RD::FORMAT_UNDEFINED,
                                                  true,
                                                  "SceneVoxelizer Main Pass");

    device->Destroy(shaders[0]);
    device->Destroy(shaders[1]);
    device->Destroy(shaders[2]);
    /*
    {
        // @TODO Temp
        RD::SamplerDescription samplerDesc = RD::SamplerDescription::Initialize();
        samplerDesc.addressMode = RD::ADDRESS_MODE_CLAMP_TO_EDGE;

        RD::TextureDescription description = RD::TextureDescription::Initialize(size, size, size);
        description.format = RD::FORMAT_R8G8B8A8_UNORM;
        description.usageFlags = RD::TEXTURE_USAGE_STORAGE_BIT;
        description.textureType = RD::TEXTURE_TYPE_3D;
        description.samplerDescription = &samplerDesc;
        texture = device->CreateTexture(&description, "VoxelTexture");

        RD::UniformBinding csBindings = {
            .bindingType = RD::BINDING_TYPE_IMAGE,
            .set = 0,
            .binding = 0,
        };

        ShaderID cs = RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/clear3d-texture.comp.spv", &csBindings, 1, nullptr, 0);
        clearTexturePipeline = device->CreateComputePipeline(cs, false, "Clear3DTexture Pipeline");
        device->Destroy(cs);

        RD::BoundUniform textureBinding = {RD::BINDING_TYPE_IMAGE, 0, texture, 0, 0};
        clearTextureSet = device->CreateUniformSet(clearTexturePipeline, &textureBinding, 1, 0, "ClearTextureBinding");
    }
    */
}
/*
void SceneVoxelizer::InitializeRayMarchResources() {
    // Create prepass pipeline
    RD::UniformBinding fsBindings[] = {
        {RD::BINDING_TYPE_IMAGE, 0, 0},
    };

    RD::PushConstant pushConstants = {0, sizeof(RayMarchPushConstant)};

    ShaderID shaders[2] = {
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/raycast-grid.vert.spv", nullptr, 0, nullptr, 0),
        RenderingUtils::CreateShaderModuleFromFile("assets/SPIRV/raycast-grid.frag.spv", fsBindings, (uint32_t)std::size(fsBindings), &pushConstants, 1),
    };

    RD::RasterizationState rasterizationState = RD::RasterizationState::Create();
    RD::DepthState depthState = RD::DepthState::Create();
    depthState.enableDepthWrite = false;
    depthState.enableDepthTest = false;

    RD::Format colorAttachmentFormat = RD::FORMAT_B8G8R8A8_UNORM;
    RD::BlendState blendState = RD::BlendState::Create();

    raymarchPipeline = device->CreateGraphicsPipeline(shaders,
                                                      (uint32_t)std::size(shaders),
                                                      RD::TOPOLOGY_TRIANGLE_LIST,
                                                      &rasterizationState,
                                                      &depthState,
                                                      &colorAttachmentFormat,
                                                      &blendState,
                                                      1,
                                                      RD::FORMAT_D24_UNORM_S8_UINT,
                                                      false,
                                                      "RayMarch Pass");

    device->Destroy(shaders[0]);
    device->Destroy(shaders[1]);

    RD::BoundUniform textureBinding = {RD::BINDING_TYPE_IMAGE, 0, texture, 0, 0};
    raymarchSet = device->CreateUniformSet(raymarchPipeline, &textureBinding, 1, 0, "Raymarch Set");
}
*/

void SceneVoxelizer::DrawVoxelScene(CommandBufferID commandBuffer, PipelineID pipeline, UniformSetID *uniformSet, uint32_t uniformSetCount) {
    RD::RenderingInfo renderingInfo = {
        .width = voxelResolution,
        .height = voxelResolution,
        .layerCount = 1,
        .colorAttachmentCount = 0,
        .pColorAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
    };

    device->BeginRenderPass(commandBuffer, &renderingInfo);
    device->SetViewport(commandBuffer, 0, 0, static_cast<float>(renderingInfo.width), static_cast<float>(renderingInfo.height));
    device->SetScissor(commandBuffer, 0, 0, renderingInfo.width, renderingInfo.height);

    device->BindPipeline(commandBuffer, pipeline);
    device->BindUniformSet(commandBuffer, pipeline, uniformSet, uniformSetCount);

    AABB aabb = std::static_pointer_cast<GLTFScene>(scene)->GetBoundingBox();
    float extents[2] = {
        std::min({aabb.min.x, aabb.min.y, aabb.min.z}),
        std::max({aabb.max.x, aabb.max.y, aabb.max.z}),
    };
    device->BindPushConstants(commandBuffer, pipeline, RD::SHADER_STAGE_GEOMETRY, extents, 0, sizeof(float) * 2);
    device->BindPushConstants(commandBuffer, pipeline, RD::SHADER_STAGE_FRAGMENT, &voxelResolution, 8, sizeof(uint32_t));

    device->BindIndexBuffer(commandBuffer, scene->indexBuffer);
    uint32_t drawCount = static_cast<uint32_t>(scene->meshGroup.drawCommands.size());
    device->DrawIndexedIndirect(commandBuffer, scene->drawCommandBuffer, 0, drawCount, sizeof(RD::DrawElementsIndirectCommand));

    device->EndRenderPass(commandBuffer);
}

void SceneVoxelizer::ExecuteVoxelPrepass(CommandPoolID cp, CommandBufferID cb, FenceID waitFence) {
    RD::ImmediateSubmitInfo submitInfo = {
        .queue = device->GetDeviceQueue(RD::QUEUE_TYPE_GRAPHICS),
        .commandPool = cp,
        .commandBuffer = cb,
        .fence = waitFence,
    };

    device->ImmediateSubmit([&](CommandBufferID commandBuffer) {
        /*
        RD::TextureBarrier barrier{
            .texture = texture,
            .srcAccess = 0,
            .dstAccess = RD::BARRIER_ACCESS_SHADER_WRITE_BIT,
            .newLayout = RD::TEXTURE_LAYOUT_GENERAL,
            .srcQueueFamily = QUEUE_FAMILY_IGNORED,
            .dstQueueFamily = QUEUE_FAMILY_IGNORED,
            .baseMipLevel = 0,
            .baseArrayLayer = 0,
            .levelCount = 1,
            .layerCount = 1,
        };

        device->PipelineBarrier(commandBuffer, RD::PIPELINE_STAGE_TOP_OF_PIPE_BIT, RD::PIPELINE_STAGE_COMPUTE_SHADER_BIT, &barrier, 1, nullptr, 0);
        device->BindPipeline(commandBuffer, clearTexturePipeline);
        device->BindUniformSet(commandBuffer, clearTexturePipeline, &clearTextureSet, 1);

        uint32_t workGroupSize = RenderingUtils::GetWorkGroupSize(size, 8);
        device->DispatchCompute(commandBuffer, workGroupSize, workGroupSize, workGroupSize);

        barrier.srcAccess = RD::BARRIER_ACCESS_SHADER_WRITE_BIT;
        device->PipelineBarrier(commandBuffer, RD::PIPELINE_STAGE_COMPUTE_SHADER_BIT, RD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT, &barrier, 1, nullptr, 0);
        */

        DrawVoxelScene(commandBuffer, prepassPipeline, &prepassSet, 1);

        // Transfer image access to shader read
        // barrier.srcAccess = RD::BARRIER_ACCESS_SHADER_WRITE_BIT;
        // barrier.dstAccess = RD::BARRIER_ACCESS_SHADER_READ_BIT;

        // device->PipelineBarrier(commandBuffer, RD::PIPELINE_STAGE_COMPUTE_SHADER_BIT, RD::PIPELINE_STAGE_ALL_COMMANDS_BIT, &barrier, 1, nullptr, 0);
    },
                            &submitInfo);

    device->WaitForFence(&waitFence, 1, UINT64_MAX);
    device->ResetFences(&waitFence, 1);

    voxelCount = countBufferPtr[0];
    LOG("Total voxels: " + std::to_string(voxelCount));
}

void SceneVoxelizer::ExecuteMainPass(CommandPoolID cp, CommandBufferID cb, FenceID waitFence) {
    RD::ImmediateSubmitInfo submitInfo = {
        .queue = device->GetDeviceQueue(RD::QUEUE_TYPE_GRAPHICS),
        .commandPool = cp,
        .commandBuffer = cb,
        .fence = waitFence,
    };

    device->ImmediateSubmit([&](CommandBufferID commandBuffer) {
        DrawVoxelScene(commandBuffer, mainPipeline, &mainSet, 1);
    },
                            &submitInfo);

    device->WaitForFence(&waitFence, 1, UINT64_MAX);
    LOG("Voxelization Write Pass Finished ..." + std::to_string(countBufferPtr[1]));
}

void SceneVoxelizer::Voxelize(CommandPoolID cp, CommandBufferID cb) {

    FenceID waitFence = device->CreateFence("TempFence");

    ExecuteVoxelPrepass(cp, cb, waitFence);

    if (voxelCount > 0) {
        // Allocate Buffer
        uint32_t bufferSize = static_cast<uint32_t>(sizeof(uint64_t)) * voxelCount;
        voxelFragmentBuffer = device->CreateBuffer(bufferSize, RD::BUFFER_USAGE_STORAGE_BUFFER_BIT, RD::MEMORY_ALLOCATION_TYPE_GPU, "VoxelFragmentList Buffer");
        RD::BoundUniform boundedUniform[] = {
            {RD::BINDING_TYPE_STORAGE_BUFFER, 1, scene->vertexBuffer},
            {RD::BINDING_TYPE_STORAGE_BUFFER, 2, scene->drawCommandBuffer},
            {RD::BINDING_TYPE_STORAGE_BUFFER, 3, scene->transformBuffer},
            {RD::BINDING_TYPE_STORAGE_BUFFER, 4, scene->materialBuffer},
            {RD::BINDING_TYPE_STORAGE_BUFFER, 5, voxelCountBuffer},
            {RD::BINDING_TYPE_STORAGE_BUFFER, 6, voxelFragmentBuffer},
            /*{RD::BINDING_TYPE_IMAGE, 7, texture}*/};
        mainSet = device->CreateUniformSet(mainPipeline, boundedUniform, static_cast<uint32_t>(std::size(boundedUniform)), 0, "Main SceneVoxelizer Binding");

        ExecuteMainPass(cp, cb, waitFence);
    } else
        LOGE("Voxelization Prepass return zero voxelCount");

    device->Destroy(waitFence);
    device->ResetCommandPool(cp);
}
/*
void SceneVoxelizer::RayMarch(CommandBufferID commandBuffer, std::shared_ptr<gfx::Camera> camera) {

    raymarchConstants.uInvP = camera->GetInvProjectionMatrix();
    raymarchConstants.uInvV = camera->GetInvViewMatrix();
    raymarchConstants.uCamPos = glm::vec4(camera->GetPosition(), size);

    device->BindPipeline(commandBuffer, raymarchPipeline);
    device->BindUniformSet(commandBuffer, raymarchPipeline, &raymarchSet, 1);
    device->BindPushConstants(commandBuffer, raymarchPipeline, RD::SHADER_STAGE_FRAGMENT, &raymarchConstants, 0, sizeof(RayMarchPushConstant));

    device->Draw(commandBuffer, 6, 1, 0, 0);
}
*/

void SceneVoxelizer::Shutdown() {
    /*
    device->Destroy(clearTexturePipeline);
    device->Destroy(clearTextureSet);

    device->Destroy(raymarchPipeline);
    device->Destroy(raymarchSet);
    device->Destroy(texture);
    */
    device->Destroy(mainPipeline);
    device->Destroy(mainSet);
    device->Destroy(prepassSet);
    device->Destroy(prepassPipeline);
    device->Destroy(voxelCountBuffer);
    device->Destroy(voxelFragmentBuffer);
}
