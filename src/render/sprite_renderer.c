#include "sprite_renderer.h"
#include "wgpu.h"
#include "pipelines.h"

struct {
    WGPURenderPipeline pipeline;
} context = {0};

int createPipeline() {
    WGPURenderPipelineDescriptor desc = {0};

    WGPUShaderModule module;
    if ((module = createShaderModule("res/shaders/sprite.wgsl")) == 0) {
        return -1;
    }

    desc.vertex.module = module;
    desc.vertex.entryPoint = WGPU_STR("vs_main");

    desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    desc.primitive.frontFace = WGPUFrontFace_CCW;
    desc.primitive.cullMode = WGPUCullMode_None;

    WGPUFragmentState fragment = {0};

    fragment.module = module;
    fragment.entryPoint = WGPU_STR("fs_main");

    desc.fragment = &fragment;

    desc.depthStencil = 0;

    WGPUBlendState blendState={0};
    WGPUColorTargetState colorTarget={0};
    colorTarget.format = renderContext.format;
    colorTarget.blend = 0;
    colorTarget.writeMask = WGPUColorWriteMask_All;
    fragment.targetCount = 1;
    fragment.targets = &colorTarget;

    desc.multisample.count = 1;
    desc.multisample.mask = ~0u;
    desc.multisample.alphaToCoverageEnabled = 0;

    context.pipeline = wgpuDeviceCreateRenderPipeline(renderContext.device, &desc);

    wgpuShaderModuleRelease(module);

    if (context.pipeline == 0) return -1;
    return 0;
}


int spriteRendererCreate()
{
    if (createPipeline()) {
        return -1;
    }
    return 0;
}

void spriteRendererFinish() {
    if (context.pipeline) {
        wgpuRenderPipelineRelease(context.pipeline);
        context.pipeline = 0;
    }
}

void renderSprite(Rect *dst, Rect *src)
{
}

void spriteRendererPass()
{
    wgpuRenderPassEncoderSetPipeline(renderContext.frameData.renderPass, context.pipeline);
    wgpuRenderPassEncoderDraw(renderContext.frameData.renderPass, 3, 1, 0, 0);
}
