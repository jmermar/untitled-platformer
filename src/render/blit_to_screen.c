#include "blit_to_screen.h"
#include <assert.h>
typedef struct {
  WGPUDevice device;
  WGPUPipelineLayout pipelineLayout;
  WGPURenderPipeline pipeline;
  WGPUBindGroup group;
  WGPUBindGroupLayout bindgroupLayout;
  WGPUSampler sampler;

  TextureView backbuffer;
} BlitToScreen;

static BlitToScreen *context;

int blitToScreenInit(WGPUDevice device, WGPUTextureFormat attachFormat,
                     TextureView *backbuffer) {
  assert(context == 0);
  context = malloc(sizeof(BlitToScreen));
  memset(context, 0, sizeof(BlitToScreen));

  context->backbuffer = *backbuffer;

  context->device = device;

  WGPUBindGroupLayoutEntry bindGroupLayoutEntry[3] = {0};
  bindGroupLayoutEntry[0].binding = 0;
  bindGroupLayoutEntry[0].visibility = WGPUShaderStage_Fragment;
  bindGroupLayoutEntry[0].texture.sampleType = WGPUTextureSampleType_Float;
  bindGroupLayoutEntry[0].texture.viewDimension =
      WGPUTextureViewDimension_2DArray;

  bindGroupLayoutEntry[1].binding = 1;
  bindGroupLayoutEntry[1].visibility = WGPUShaderStage_Fragment;
  bindGroupLayoutEntry[1].sampler.type = WGPUSamplerBindingType_Filtering;

  WGPUBindGroupLayoutDescriptor desc = {0};
  desc.entries = bindGroupLayoutEntry;
  desc.entryCount = 2;
  desc.label = WGPU_STR("Blit screen Layout Group");

  if ((context->bindgroupLayout =
           wgpuDeviceCreateBindGroupLayout(device, &desc)) == 0) {
    return -1;
  }

  WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {0};
  pipelineLayoutDesc.bindGroupLayoutCount = 1;
  pipelineLayoutDesc.bindGroupLayouts = &context->bindgroupLayout;
  pipelineLayoutDesc.label = WGPU_STR("Blit screen Pipeline Layout");

  if ((context->pipelineLayout =
           wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc)) == 0) {
    return -1;
  }

  WGPURenderPipelineDescriptor pipelineDesc = {0};

  WGPUShaderModule shaderModule;
  if ((shaderModule =
           createShaderModule(context->device, "res/shaders/blit.wgsl")) == 0) {
    return -1;
  }

  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = WGPU_STR("vs_main");
  pipelineDesc.vertex.buffers = 0;
  pipelineDesc.vertex.bufferCount = 0;

  pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
  pipelineDesc.primitive.cullMode = WGPUCullMode_None;

  WGPUFragmentState fragment = {0};

  fragment.module = shaderModule;
  fragment.entryPoint = WGPU_STR("fs_main");

  pipelineDesc.fragment = &fragment;

  pipelineDesc.depthStencil = 0;

  WGPUBlendState blendState = {0};
  WGPUColorTargetState colorTarget = {0};
  colorTarget.format = attachFormat;
  colorTarget.blend = 0;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = 0;
  pipelineDesc.layout = context->pipelineLayout;

  context->pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

  wgpuShaderModuleRelease(shaderModule);

  if (context->pipeline == 0)
    return -1;

  WGPUSamplerDescriptor samplerDescriptor = {0};
  samplerDescriptor.addressModeU = WGPUAddressMode_ClampToEdge;
  samplerDescriptor.addressModeV = WGPUAddressMode_ClampToEdge;
  samplerDescriptor.addressModeW = WGPUAddressMode_ClampToEdge;

  samplerDescriptor.magFilter = WGPUFilterMode_Nearest;
  samplerDescriptor.minFilter = WGPUFilterMode_Nearest;
  samplerDescriptor.mipmapFilter = WGPUMipmapFilterMode_Linear;
  samplerDescriptor.lodMaxClamp = 1.f;
  samplerDescriptor.maxAnisotropy = 1;
  samplerDescriptor.compare = WGPUCompareFunction_Undefined;
  if ((context->sampler =
           wgpuDeviceCreateSampler(device, &samplerDescriptor)) == 0) {
    return -1;
  }

  WGPUBindGroupEntry bindGroupEntry[2] = {0};
  bindGroupEntry[0].binding = 0;
  bindGroupEntry[0].textureView = backbuffer->view;
  bindGroupEntry[1].binding = 1;
  bindGroupEntry[1].sampler = context->sampler;
  WGPUBindGroupDescriptor bindGroupDescriptor = {0};
  bindGroupDescriptor.entryCount = 2;
  bindGroupDescriptor.entries = bindGroupEntry;
  bindGroupDescriptor.layout = context->bindgroupLayout;

  if ((context->group =
           wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor)) == 0) {
    return -1;
  }

  return 0;
}

void blitToScreenFinish() {
  if (!context)
    return;
  if (context->group) {
    wgpuBindGroupRelease(context->group);
  }
  if (context->pipeline) {
    wgpuRenderPipelineRelease(context->pipeline);
  }
  if (context->pipelineLayout) {
    wgpuPipelineLayoutRelease(context->pipelineLayout);
  }
  if (context->bindgroupLayout) {
    wgpuBindGroupLayoutRelease(context->bindgroupLayout);
  }
  if (context->sampler) {
    wgpuSamplerRelease(context->sampler);
  }
  free(context);
  context = 0;
}

void blitBufferToScreen(WGPUCommandEncoder encoder, WGPUTextureView attachView,
                        size_t attachW, size_t attachH) {
  WGPURenderPassEncoder renderPass;
  {
    WGPURenderPassColorAttachment renderPassColorAttachment = {0};
    renderPassColorAttachment.view = attachView;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    WGPURenderPassDescriptor desc = {0};
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &renderPassColorAttachment;
    desc.label = WGPU_STR("Blit backbuffer to screen");
    renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
  }

  wgpuRenderPassEncoderSetBindGroup(renderPass, 0, context->group, 0, 0);

  wgpuRenderPassEncoderSetPipeline(renderPass, context->pipeline);

  wgpuRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);

  wgpuRenderPassEncoderEnd(renderPass);
}