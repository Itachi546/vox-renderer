#pragma once

#include "core/bitfield.h"

#include <functional>
#include <string>

struct ID {
    size_t id = 0;
    inline ID() = default;
    ID(size_t _id) : id(_id) {}
};
#define DEFINE_ID(m_name)                                                                    \
    struct m_name##ID : public ID {                                                          \
        inline operator bool() const { return id != 0; }                                     \
        inline m_name##ID &operator=(m_name##ID p_other) {                                   \
            id = p_other.id;                                                                 \
            return *this;                                                                    \
        }                                                                                    \
        inline bool operator<(const m_name##ID &p_other) const { return id < p_other.id; }   \
        inline bool operator==(const m_name##ID &p_other) const { return id == p_other.id; } \
        inline bool operator!=(const m_name##ID &p_other) const { return id != p_other.id; } \
        inline m_name##ID(const m_name##ID &p_other) : ID(p_other.id) {}                     \
        inline explicit m_name##ID(uint64_t p_int) : ID(p_int) {}                            \
        inline explicit m_name##ID(void *p_ptr) : ID((size_t)p_ptr) {}                       \
        inline m_name##ID() = default;                                                       \
    };                                                                                       \
    /* Ensure type-punnable to pointer. Makes some things easier.*/                          \
    static_assert(sizeof(m_name##ID) == sizeof(void *));

DEFINE_ID(Shader)
DEFINE_ID(CommandBuffer)
DEFINE_ID(CommandPool)
DEFINE_ID(Pipeline)
DEFINE_ID(Texture)
DEFINE_ID(UniformSet)
DEFINE_ID(Buffer)
DEFINE_ID(Queue)
DEFINE_ID(Fence)

constexpr const uint64_t INVALID_ID = UINT64_MAX;
constexpr const uint32_t INVALID_TEXTURE_ID = UINT32_MAX;
constexpr const uint32_t INVALID_QUEUE_ID = UINT32_MAX;

class RenderingDevice {
  public:
    enum class DeviceType {
        DEVICE_TYPE_OTHER = 0x0,
        DEVICE_TYPE_INTEGRATED_GPU = 0x1,
        DEVICE_TYPE_DISCRETE_GPU = 0x2,
        DEVICE_TYPE_VIRTUAL_GPU = 0x3,
        DEVICE_TYPE_CPU = 0x4,
        DEVICE_TYPE_MAX = 0x5
    };

    struct Device {
        std::string name;
        uint32_t vendor;
        DeviceType deviceType;
    };

    enum ShaderStage {
        SHADER_STAGE_VERTEX = 0,
        SHADER_STAGE_FRAGMENT,
        SHADER_STAGE_COMPUTE,
        SHADER_STAGE_GEOMETRY,
        SHADER_STAGE_MAX,
    };

    enum BindingType {
        BINDING_TYPE_TEXTURE,
        BINDING_TYPE_IMAGE,
        BINDING_TYPE_STORAGE_BUFFER,
        BINDING_TYPE_UNIFORM_BUFFER,
        BINDING_TYPE_MAX
    };

    struct UniformBinding {
        BindingType bindingType;
        uint32_t set;
        uint32_t binding;
    };

    struct BoundUniform {
        BindingType bindingType;
        uint32_t binding;
        ID resourceID;
        uint64_t offset = 0;
        uint64_t range = UINT64_MAX;
    };

    struct PushConstant {
        uint32_t offset;
        uint32_t size;
    };

    struct ImmediateSubmitInfo {
        QueueID queue;
        CommandPoolID commandPool;
        CommandBufferID commandBuffer;
        FenceID fence;
    };

    struct ShaderDescription {
        UniformBinding *bindings;
        uint32_t bindingCount;

        PushConstant *pushConstants;
        uint32_t pushConstantCount;

        ShaderStage stage;
    };

    enum CullMode {
        CULL_MODE_NONE = 0,
        CULL_MODE_FRONT,
        CULL_MODE_BACK,
        CULL_MODE_FRONT_AND_BACK,
        CULL_MODE_MAX
    };

    enum FrontFace {
        FRONT_FACE_COUNTER_CLOCKWISE = 0,
        FRONT_FACE_CLOCKWISE = 1,
        FRONT_FACE_MAX
    };

    enum PolygonMode {
        POLYGON_MODE_FILL = 0,
        POLYGON_MODE_LINE,
        POLYGON_MODE_POINT,
        POLYGON_MODE_MAX
    };

    enum Topology {
        TOPOLOGY_POINT_LIST = 0,
        TOPOLOGY_LINE_LIST = 1,
        TOPOLOGY_LINE_STRIP = 2,
        TOPOLOGY_TRIANGLE_LIST = 3,
        TOPOLOGY_TRIANGLE_STRIP = 4,
        TOPOLOGY_TRIANGLE_FAN = 5,
        TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
        TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
        TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
        TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
        TOPOLOGY_PATCH_LIST = 10,
        TOPOLOGY_MAX = 11
    };

    enum Format {
        FORMAT_B8G8R8A8_UNORM = 0,
        FORMAT_R8G8B8A8_UNORM,
        FORMAT_R8G8B8A8_SRGB,
        FORMAT_R8G8B8_UNORM,
        FORMAT_R8G8_UNORM,
        FORMAT_R8_UNORM,
        FORMAT_R16_SFLOAT,
        FORMAT_R16G16_SFLOAT,
        FORMAT_R16G16B16_SFLOAT,
        FORMAT_R16G16B16A16_SFLOAT,
        FORMAT_R32G32B32A32_SFLOAT,
        FORMAT_R32G32B32_SFLOAT,
        FORMAT_R32G32_SFLOAT,
        FORMAT_D16_UNORM,
        FORMAT_D32_SFLOAT,
        FORMAT_D32_SFLOAT_S8_UINT,
        FORMAT_D24_UNORM_S8_UINT,
        FORMAT_UNDEFINED,
        FORMAT_MAX
    };

    struct RasterizationState {
        float lineWidth;
        CullMode cullMode;
        PolygonMode polygonMode;
        FrontFace frontFace;
        bool enableConservative;

        static RasterizationState Create() {
            return RasterizationState{
                .lineWidth = 1.0f,
                .cullMode = CULL_MODE_BACK,
                .polygonMode = POLYGON_MODE_FILL,
                .frontFace = FRONT_FACE_CLOCKWISE,
                .enableConservative = false,
            };
        }
    };

    enum AttachmentLoadOp {
        LOAD_OP_LOAD = 0,
        LOAD_OP_CLEAR = 1,
        LOAD_OP_DONT_CARE = 2,
    };

    enum AttachmentStoreOp {
        STORE_OP_STORE = 0,
        STORE_OP_DONT_CARE = 0
    };

    struct AttachmentInfo {
        AttachmentLoadOp loadOp;
        AttachmentStoreOp storeOp;

        float clearColor[4];
        float clearDepth;
        uint32_t clearStencil;
        TextureID attachment;
    };

    struct RenderingInfo {
        uint32_t width;
        uint32_t height;
        uint32_t layerCount;

        uint32_t colorAttachmentCount;
        AttachmentInfo *pColorAttachments;
        AttachmentInfo *pDepthStencilAttachment;
    };

    struct DepthState {
        bool enableDepthClamp;
        bool enableDepthTest;
        bool enableDepthWrite;
        float maxDepthBounds, minDepthBounds;

        static DepthState Create() {
            return DepthState{
                .enableDepthClamp = false,
                .enableDepthTest = false,
                .enableDepthWrite = false,
                .maxDepthBounds = 1.0f,
                .minDepthBounds = 0.0f,
            };
        }
    };

    struct BlendState {
        bool enable;

        static BlendState Create() {
            return BlendState{
                .enable = false,
            };
        }
    };

    enum TextureType {
        TEXTURE_TYPE_1D = 0,
        TEXTURE_TYPE_2D = 1,
        TEXTURE_TYPE_3D = 2,
        TEXTURE_TYPE_MAX = 3
    };

    enum TextureUsageBits {
        TEXTURE_USAGE_TRANSFER_SRC_BIT = (1 << 0),
        TEXTURE_USAGE_TRANSFER_DST_BIT = (1 << 1),
        TEXTURE_USAGE_SAMPLED_BIT = (1 << 2),
        TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 3),
        TEXTURE_USAGE_DEPTH_ATTACHMENT_BIT = (1 << 4),
        TEXTURE_USAGE_STENCIL_ATTACHMENT_BIT = (1 << 5),
        TEXTURE_USAGE_INPUT_ATTACHMENT_BIT = (1 << 6),
        TEXTURE_USAGE_STORAGE_BIT = (1 << 7),
    };

    enum BufferUsageBits {
        BUFFER_USAGE_TRANSFER_SRC_BIT = (1 << 0),
        BUFFER_USAGE_TRANSFER_DST_BIT = (1 << 1),
        BUFFER_USAGE_UNIFORM_BUFFER_BIT = (1 << 4),
        BUFFER_USAGE_STORAGE_BUFFER_BIT = (1 << 5),
        BUFFER_USAGE_INDEX_BUFFER_BIT = (1 << 6),
        BUFFER_USAGE_VERTEX_BUFFER_BIT = (1 << 7),
        BUFFER_USAGE_INDIRECT_BUFFER_BIT = (1 << 8)
    };

    enum PipelineStageBits {
        PIPELINE_STAGE_TOP_OF_PIPE_BIT = (1 << 0),
        PIPELINE_STAGE_DRAW_INDIRECT_BIT = (1 << 1),
        PIPELINE_STAGE_VERTEX_INPUT_BIT = (1 << 2),
        PIPELINE_STAGE_VERTEX_SHADER_BIT = (1 << 3),
        PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = (1 << 4),
        PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = (1 << 5),
        PIPELINE_STAGE_GEOMETRY_SHADER_BIT = (1 << 6),
        PIPELINE_STAGE_FRAGMENT_SHADER_BIT = (1 << 7),
        PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = (1 << 8),
        PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = (1 << 9),
        PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = (1 << 10),
        PIPELINE_STAGE_COMPUTE_SHADER_BIT = (1 << 11),
        PIPELINE_STAGE_TRANSFER_BIT = (1 << 12),
        PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = (1 << 13),
        PIPELINE_STAGE_ALL_GRAPHICS_BIT = (1 << 15),
        PIPELINE_STAGE_ALL_COMMANDS_BIT = (1 << 16),
    };

    enum BarrierAccessBits {
        BARRIER_ACCESS_NONE = 0,
        BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT = (1 << 0),
        BARRIER_ACCESS_INDEX_READ_BIT = (1 << 1),
        BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = (1 << 2),
        BARRIER_ACCESS_UNIFORM_READ_BIT = (1 << 3),
        BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT = (1 << 4),
        BARRIER_ACCESS_SHADER_READ_BIT = (1 << 5),
        BARRIER_ACCESS_SHADER_WRITE_BIT = (1 << 6),
        BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT = (1 << 7),
        BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = (1 << 8),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = (1 << 9),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = (1 << 10),
        BARRIER_ACCESS_TRANSFER_READ_BIT = (1 << 11),
        BARRIER_ACCESS_TRANSFER_WRITE_BIT = (1 << 12),
        BARRIER_ACCESS_HOST_READ_BIT = (1 << 13),
        BARRIER_ACCESS_HOST_WRITE_BIT = (1 << 14),
        BARRIER_ACCESS_MEMORY_READ_BIT = (1 << 15),
        BARRIER_ACCESS_MEMORY_WRITE_BIT = (1 << 16),
        BARRIER_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT = (1 << 23),
    };

    enum TextureLayout {
        TEXTURE_LAYOUT_UNDEFINED,
        TEXTURE_LAYOUT_GENERAL,
        TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL,
        TEXTURE_LAYOUT_PREINITIALIZED,
        TEXTURE_LAYOUT_VRS_ATTACHMENT_OPTIMAL = 1000164003,
        TEXTURE_LAYOUT_PRESENT_SRC = 1000001002
    };

    struct BufferCopyRegion {
        uint64_t srcOffset;
        uint64_t dstOffset;
        uint64_t size;
    };

    struct BufferImageCopyRegion {
        uint64_t bufferOffset;
    };

#define QUEUE_FAMILY_IGNORED QueueID(UINT32_MAX)

    struct TextureBarrier {
        TextureID texture;
        BitField<BarrierAccessBits> srcAccess;
        BitField<BarrierAccessBits> dstAccess;
        TextureLayout newLayout;
        QueueID srcQueueFamily;
        QueueID dstQueueFamily;

        uint32_t baseMipLevel, baseArrayLayer;
        uint32_t levelCount, layerCount;
    };

    struct BufferBarrier {
        BufferID buffer;
        BitField<BarrierAccessBits> srcAccess;
        BitField<BarrierAccessBits> dstAccess;
        QueueID srcQueueFamily;
        QueueID dstQueueFamily;
        uint64_t offset, size;
    };

    enum MemoryAllocationType {
        MEMORY_ALLOCATION_TYPE_CPU,
        MEMORY_ALLOCATION_TYPE_GPU
    };

    enum Filter {
        FILTER_NEAREST = 0,
        FILTER_LINEAR = 1,
    };

    enum AddressMode {
        ADDRESS_MODE_REPEAT = 0,
        ADDRESS_MODE_MIRRORED_REPEAT = 1,
        ADDRESS_MODE_CLAMP_TO_EDGE = 2,
        ADDRESS_MODE_CLAMP_TO_BORDER = 3,
        ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4
    };

    enum MipmapMode {
        MIPMAP_MODE_NEAREST = 0,
        MIPMAP_MODE_LINEAR = 1
    };

    struct SamplerDescription {
        Filter minFilter;
        Filter magFilter;
        // Same for uvw
        AddressMode addressMode;
        MipmapMode mipmapMode;

        float lodBias;
        float maxAnisotropy;
        float minLod;
        float maxLod;
        bool enableAnisotropy;

        static SamplerDescription Initialize() {
            return SamplerDescription{
                .minFilter = FILTER_LINEAR,
                .magFilter = FILTER_LINEAR,
                .addressMode = ADDRESS_MODE_REPEAT,
                .mipmapMode = MIPMAP_MODE_LINEAR,
                .lodBias = 0,
                .maxAnisotropy = 16,
                .minLod = 0,
                .maxLod = 16,
                .enableAnisotropy = false,
            };
        }
    };

    struct TextureDescription {
        Format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t arrayLayers;
        uint32_t mipMaps;

        TextureType textureType;
        uint32_t usageFlags;

        SamplerDescription *samplerDescription;

        static TextureDescription Initialize(uint32_t width, uint32_t height, uint32_t depth = 1) {
            return TextureDescription{
                .format = FORMAT_R8G8B8A8_UNORM,
                .width = width,
                .height = height,
                .depth = depth,
                .arrayLayers = 1,
                .mipMaps = 1,
                .textureType = TEXTURE_TYPE_2D,
                .usageFlags = 0,
                .samplerDescription = nullptr,
            };
        }
    };

    enum QueueType {
        QUEUE_TYPE_INVALID = 0,
        QUEUE_TYPE_GRAPHICS = 1,
        QUEUE_TYPE_COMPUTE = 2,
        QUEUE_TYPE_TRANSFER = 4
    };

    struct DrawElementsIndirectCommand {
        uint32_t count;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t baseInstance;
        uint32_t drawId;
    };

    struct WindowPlatformData {
        void *windowPtr;
    };

    virtual void Initialize(void *platformData) = 0;

    virtual void SetValidationMode(bool state) = 0;

    virtual uint64_t GetMemoryUsage() = 0;

    virtual QueueID GetDeviceQueue(QueueType queueType) = 0;

    virtual void CreateSurface() = 0;
    virtual void CreateSwapchain(bool vsync = true) = 0;
    virtual PipelineID CreateGraphicsPipeline(const ShaderID *shaders,
                                              uint32_t shaderCount,
                                              Topology topology,
                                              const RasterizationState *rs,
                                              const DepthState *ds,
                                              const Format *colorAttachmentsFormat,
                                              const BlendState *attachmentBlendStates,
                                              uint32_t colorAttachmentCount,
                                              Format depthAttachmentFormat,
                                              bool enableBindless,
                                              const std::string &name) = 0;
    virtual PipelineID CreateComputePipeline(const ShaderID shader, bool enableBindless, const std::string &name) = 0;
    virtual TextureID CreateTexture(TextureDescription *description, const std::string &name) = 0;
    virtual ShaderID CreateShader(const uint32_t *byteCode, uint32_t codeSizeInBytes, ShaderDescription *desc, const std::string &name = "shader") = 0;
    virtual CommandBufferID CreateCommandBuffer(CommandPoolID commandPool, const std::string &name = "commandBuffer") = 0;
    virtual CommandPoolID CreateCommandPool(QueueID queue, const std::string &name = "commandPool") = 0;
    virtual void ResetCommandPool(CommandPoolID commandPool) = 0;
    virtual UniformSetID CreateUniformSet(PipelineID pipeline, BoundUniform *uniforms, uint32_t uniformCount, uint32_t set, const std::string &name) = 0;

    virtual FenceID CreateFence(const std::string &name = "fence", bool signalled = false) = 0;
    virtual void WaitForFence(FenceID *fence, uint32_t fenceCount, uint64_t timeout) = 0;
    virtual void ResetFences(FenceID *fences, uint32_t fenceCount) = 0;

    virtual BufferID CreateBuffer(uint32_t size, uint32_t usageFlags, MemoryAllocationType allocationType, const std::string &name) = 0;
    virtual uint8_t *MapBuffer(BufferID buffer) = 0;
    virtual void CopyBuffer(CommandBufferID commandBuffer, BufferID src, BufferID dst, BufferCopyRegion *region) = 0;
    virtual void CopyBufferToTexture(CommandBufferID commandBuffer, BufferID src, TextureID dst, BufferImageCopyRegion *region) = 0;

    virtual void SetViewport(CommandBufferID commandBuffer, float offsetX, float offsetY, float width, float height) = 0;
    virtual void SetScissor(CommandBufferID commandBuffer, int offsetX, int offsetY, uint32_t width, uint32_t height) = 0;

    virtual void BindIndexBuffer(CommandBufferID commandBuffer, BufferID buffer) = 0;

    virtual void BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) = 0;
    virtual void BindPushConstants(CommandBufferID commandBuffer, PipelineID pipeline, ShaderStage shaderStage, void *data, uint32_t offset, uint32_t size) = 0;

    virtual void BindUniformSet(CommandBufferID commandBuffer, PipelineID pipeline, UniformSetID *uniformSet, uint32_t uniformSetCount) = 0;
    virtual void DispatchCompute(CommandBufferID commandBuffer, uint32_t workGroupX, uint32_t workGroupY, uint32_t workGroupZ) = 0;
    virtual void DispatchComputeIndirect(CommandBufferID commandBuffer, BufferID indirectBuffer, uint32_t offset) = 0;

    virtual void Submit(CommandBufferID commandBuffer, FenceID Fence) = 0;

    virtual void ImmediateSubmit(std::function<void(CommandBufferID commandBuffer)> &&function, ImmediateSubmitInfo *queueInfo) = 0;
    virtual void PipelineBarrier(CommandBufferID commandBuffer,
                                 BitField<PipelineStageBits> srcStage,
                                 BitField<PipelineStageBits> dstStage,
                                 TextureBarrier *textureBarriers,
                                 uint32_t textureBarrierCount,
                                 BufferBarrier *bufferBarriers,
                                 uint32_t bufferBarrierCount) = 0;

    virtual void PrepareSwapchain(CommandBufferID commandBuffer, TextureLayout layout) = 0;

    virtual void CopyToSwapchain(CommandBufferID commandBuffer, TextureID texture) = 0;

    virtual void BeginFrame() = 0;
    virtual void BeginCommandBuffer(CommandBufferID commandBuffer) = 0;
    virtual void EndCommandBuffer(CommandBufferID commandBuffer) = 0;

    virtual void BeginRenderPass(CommandBufferID commandBuffer, RenderingInfo *renderingInfo) = 0;
    virtual void EndRenderPass(CommandBufferID commandBuffer) = 0;

    virtual void DrawElementInstanced(CommandBufferID commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    virtual void Draw(CommandBufferID commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
    virtual void DrawIndexedIndirect(CommandBufferID commandBuffer, BufferID indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) = 0;

    virtual void Present() = 0;

    virtual void UpdateBindlessDescriptor(TextureID *bindlessTexture, uint32_t count) = 0;
    virtual void UpdateBindlessTexture(TextureID texture) = 0;
    virtual void GenerateMipmap(CommandBufferID commandBuffer, TextureID texture) = 0;

    virtual void Destroy(PipelineID pipeline) = 0;
    virtual void Destroy(ShaderID shaderModule) = 0;
    virtual void Destroy(CommandPoolID commandPool) = 0;
    virtual void Destroy(TextureID texture) = 0;
    virtual void Destroy(UniformSetID uniformSet) = 0;
    virtual void Destroy(BufferID buffer) = 0;
    virtual void Destroy(FenceID fence) = 0;

    virtual void Shutdown() = 0;

    virtual uint32_t GetDeviceCount() = 0;
    virtual Device *GetDevice(int index) = 0;

    virtual ~RenderingDevice() = default;

    static RenderingDevice *&GetInstance() {
#ifdef VULKAN_ENABLED
        static RenderingDevice *device = nullptr;
        return device;
#else
#error No Supported Device Selected
#endif
    }
}; // namespace gfx

using RD = RenderingDevice;