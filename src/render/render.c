#include "render.h"
#include "wgpu.h"
#include <stdlib.h>
#include <memory.h>
#include "commands.h"
RenderContext renderContext = {0};



int renderInit(RenderInitParams* params)
{
    renderContext.width = params->width;
    renderContext.height = params->height;
    renderContext.window = params->window;
    if (initWGPU()) {
        return -1;
    }
    renderContext.frameData.cmd = createCommandBuffer(1024);
    return 0;
}

void renderFinish() {
    commandBufferClear(&renderContext.frameData.cmd);
    finishWGPU();
}

WGPUCommandEncoder wgpu_createCommandEncoder(const char* name) {
    WGPUCommandEncoderDescriptor desc = {.label = WGPU_STR(name)};

    return wgpuDeviceCreateCommandEncoder(renderContext.device, &desc);
}

void finishFrame() {

    FrameData* frameData = &renderContext.frameData;
    wgpuRenderPassEncoderEnd(frameData->renderPass);
    wgpuRenderPassEncoderRelease(frameData->renderPass);

    {
        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = 0;
        cmdBufferDescriptor.label = WGPU_STR("Command buffer");
        commandBufferAppend(&frameData->cmd, wgpuCommandEncoderFinish(frameData->encoder, &cmdBufferDescriptor));
    }
    wgpuCommandEncoderRelease(frameData->encoder);

    wgpuQueueSubmit(renderContext.queue, frameData->cmd.size, frameData->cmd.commands);
    commandBufferClear(&frameData->cmd);

    wgpuSurfacePresent(renderContext.surface);

    wgpuTextureViewRelease(frameData->screenSurface.view);
}

void renderFrame(RenderState* state) {
    FrameData* frameData = &renderContext.frameData;
    frameData->screenSurface = getNextSurfaceViewData();
    frameData->encoder = wgpu_createCommandEncoder("My encoder");
    {
        WGPURenderPassColorAttachment renderPassColorAttachment = {0};
        renderPassColorAttachment.view = frameData->screenSurface.view;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        renderPassColorAttachment.clearValue = *((WGPUColor*)((void*)state->clearColor.raw));
        WGPURenderPassDescriptor desc = {0};
        desc.colorAttachmentCount = 1;
        desc.colorAttachments = &renderPassColorAttachment;
        frameData->renderPass = wgpuCommandEncoderBeginRenderPass(frameData->encoder, &desc);
    }

    finishFrame();
}