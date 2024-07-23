#pragma once

#include "rendering/rendering-device.h"

#include <thread>
#include "core/thread-safe-queue.h"

struct TextureLoadRequest {
    std::string path;
    TextureID textureId;
};

struct BufferUploadRequest {
    void *data;
    union {
        TextureID textureId;
        BufferID bufferId;
    } resource;
};

class AsyncLoader {
  public:
    void Initialize();

    void Start();

    void RequestTextureLoad(std::string filename, TextureID textureId) {
        textureLoadQueue.push(TextureLoadRequest{filename, textureId});
    }

    void Shutdown();

  private:
    bool execute;
    std::thread _thread;

    ThreadSafeQueue<TextureLoadRequest> textureLoadQueue;

    BufferID stagingBuffer;
    uint8_t *stagingBufferPtr;
    RD::SubmitQueueInfo submitQueueInfo;
    RD *device;
    void ProcessQueue(RD *device);
};