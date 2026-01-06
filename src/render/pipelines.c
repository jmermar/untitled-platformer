#include "pipelines.h"
#include "../files.h"
#include "render_context.h"
#include "wgpu.h"
#include <stdlib.h>

WGPUShaderModule compileShaderModule(const char* src) {
    WGPUShaderModuleDescriptor desc = {0};

    WGPUShaderSourceWGSL shaderCodeDesc = {0};
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderCodeDesc.code = WGPU_STR(src);
    desc.nextInChain = &shaderCodeDesc.chain;

    return wgpuDeviceCreateShaderModule(renderContext.device, &desc);
}

WGPUShaderModule createShaderModule(const char* filepath) {
    char* src = readTextFile(filepath);

    if (src == 0) {
        return 0;
    }

    WGPUShaderModule module = compileShaderModule(src);
    free(src);

    return module;
}