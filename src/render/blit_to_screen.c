#include "blit_to_screen.h"
#include "render_imp.h"
#include "resources.h"
typedef struct {
  WGPUPipelineLayout pipelineLayout;
  WGPURenderPipeline pipeline;
  WGPUBindGroup group;
  WGPUBindGroupLayout bindgroupLayout;
  WGPUSampler sampler;
} BlitToScreenContext;

static BlitToScreenContext context;

static int createPipelineLayout() {
  WGPUBindGroupLayoutEntry entry[3] = {0};
  entry[0].binding = 0;
  entry[0].visibility = WGPUShaderStage_Fragment;
  entry[0].texture.sampleType = WGPUTextureSampleType_Float;
  entry[0].texture.viewDimension = WGPUTextureViewDimension_2DArray;

  entry[1].binding = 1;
  entry[1].visibility = WGPUShaderStage_Fragment;
  entry[1].sampler.type = WGPUSamplerBindingType_Filtering;

  WGPUBindGroupLayoutDescriptor desc = {0};
  desc.entries = entry;
  desc.entryCount = 2;
  desc.label = WGPU_STR("Blit screen Layout Group");

  if ((context.bindgroupLayout =
           wgpuDeviceCreateBindGroupLayout(renderContext.device, &desc)) == 0) {
    return -1;
  }

  WGPUPipelineLayoutDescriptor layoutDesc = {0};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = &context.bindgroupLayout;
  layoutDesc.label = WGPU_STR("Blit screen Pipeline Layout");

  if ((context.pipelineLayout = wgpuDeviceCreatePipelineLayout(
           renderContext.device, &layoutDesc)) == 0) {
    return -1;
  }

  return 0;
}

static int createPipeline() {
  WGPURenderPipelineDescriptor desc = {0};

  WGPUShaderModule module;
  if ((module = createShaderModule("res/shaders/blit.wgsl")) == 0) {
    return -1;
  }

  desc.vertex.module = module;
  desc.vertex.entryPoint = WGPU_STR("vs_main");
  desc.vertex.buffers = 0;
  desc.vertex.bufferCount = 0;

  desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  desc.primitive.frontFace = WGPUFrontFace_CCW;
  desc.primitive.cullMode = WGPUCullMode_None;

  WGPUFragmentState fragment = {0};

  fragment.module = module;
  fragment.entryPoint = WGPU_STR("fs_main");

  desc.fragment = &fragment;

  desc.depthStencil = 0;

  WGPUBlendState blendState = {0};
  WGPUColorTargetState colorTarget = {0};
  colorTarget.format = renderContext.format;
  colorTarget.blend = 0;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  desc.multisample.count = 1;
  desc.multisample.mask = ~0u;
  desc.multisample.alphaToCoverageEnabled = 0;
  desc.layout = context.pipelineLayout;

  context.pipeline =
      wgpuDeviceCreateRenderPipeline(renderContext.device, &desc);

  wgpuShaderModuleRelease(module);

  if (context.pipeline == 0)
    return -1;
  return 0;
}

int createBindGroup() {

  {
    WGPUSamplerDescriptor desc = {0};
    desc.addressModeU = WGPUAddressMode_ClampToEdge;
    desc.addressModeV = WGPUAddressMode_ClampToEdge;
    desc.addressModeW = WGPUAddressMode_ClampToEdge;

    desc.magFilter = WGPUFilterMode_Nearest;
    desc.minFilter = WGPUFilterMode_Nearest;
    desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    desc.lodMaxClamp = 1.f;
    desc.maxAnisotropy = 1;
    desc.compare = WGPUCompareFunction_Undefined;
    if ((context.sampler =
             wgpuDeviceCreateSampler(renderContext.device, &desc)) == 0) {
      return -1;
    }
  }
  {
    WGPUBindGroupEntry entries[2] = {0};
    entries[0].binding = 0;
    entries[0].textureView = renderContext.backbuffer.view;
    entries[1].binding = 1;
    entries[1].sampler = context.sampler;
    WGPUBindGroupDescriptor desc = {0};
    desc.entryCount = 2;
    desc.entries = entries;
    desc.layout = context.bindgroupLayout;

    if ((context.group =
             wgpuDeviceCreateBindGroup(renderContext.device, &desc)) == 0) {
      return -1;
    }
  }

  return 0;
}

int blitToScreenInit() {
  context = (BlitToScreenContext){0};
  if (createPipelineLayout()) {
    blitToScreenFinish();
    return -1;
  }
  if (createPipeline()) {
    blitToScreenFinish();
    return -1;
  }
  if (createBindGroup()) {
    blitToScreenFinish();
    return -1;
  }
  return 0;
}

void blitToScreenFinish() {
  if (context.group) {
    wgpuBindGroupRelease(context.group);
  }
  if (context.pipeline) {
    wgpuRenderPipelineRelease(context.pipeline);
  }
  if (context.pipelineLayout) {
    wgpuPipelineLayoutRelease(context.pipelineLayout);
  }
  if (context.bindgroupLayout) {
    wgpuBindGroupLayoutRelease(context.bindgroupLayout);
  }
  if (context.sampler) {
    wgpuSamplerRelease(context.sampler);
  }
  context = (BlitToScreenContext){0};
}

void blitBackbufferToScreen() {
  WGPURenderPassEncoder renderPass;
  {
    WGPURenderPassColorAttachment renderPassColorAttachment = {0};
    renderPassColorAttachment.view = renderContext.frameData.screenSurface.view;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    WGPURenderPassDescriptor desc = {0};
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &renderPassColorAttachment;
    desc.label = WGPU_STR("Blit backbuffer to screen");
    renderPass = wgpuCommandEncoderBeginRenderPass(
        renderContext.frameData.encoder, &desc);
  }

  uint32_t w = renderContext.backbuffer.size.width;
  uint32_t h = renderContext.backbuffer.size.height;

  wgpuRenderPassEncoderSetBindGroup(renderPass, 0, context.group, 0, 0);

  wgpuRenderPassEncoderSetPipeline(renderPass, context.pipeline);

  wgpuRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);

  wgpuRenderPassEncoderEnd(renderPass);
}