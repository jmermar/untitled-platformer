#include "pipelines.h"
#include "../files.h"
#include "render_context.h"
#include "wgpu.h"
#include <stdlib.h>

WGPUShaderModule compileShaderModule(const char *src) {
  WGPUShaderModuleDescriptor desc = {0};

  WGPUShaderSourceWGSL shaderCodeDesc = {0};
  shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
  shaderCodeDesc.code = WGPU_STR(src);
  desc.nextInChain = &shaderCodeDesc.chain;

  return wgpuDeviceCreateShaderModule(renderContext.device, &desc);
}

WGPUShaderModule createShaderModule(const char *filepath) {
  char *src = readTextFile(filepath);

  if (src == 0) {
    return 0;
  }

  WGPUShaderModule module = compileShaderModule(src);
  free(src);

  return module;
}

WGPUVertexBufferLayout getSpriteBufferLayout(WGPUVertexAttribute *attr) {
  WGPUVertexBufferLayout layout = {0};

  attr[0].format = WGPUVertexFormat_Float32x3;
  attr[0].offset = 0;
  attr[0].shaderLocation = 0;

  attr[1].format = WGPUVertexFormat_Float32x2;
  attr[1].offset = 3 * sizeof(float);
  attr[1].shaderLocation = 1;

  attr[2].format = WGPUVertexFormat_Uint32;
  attr[2].offset = 5 * sizeof(uint32_t);
  attr[2].shaderLocation = 2;

  layout.arrayStride = 5 * sizeof(float) + sizeof(uint32_t);
  layout.attributeCount = 3;
  layout.attributes = attr;
  layout.stepMode = WGPUVertexStepMode_Vertex;

  return layout;
}