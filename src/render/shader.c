#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "../util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct CleanupShaderModule {
    VkDevice device;
    VkShaderModule shaderModule;
} CleanupShaderModule;
static void cleanup_shader_module(void* user_ptr, sc_t id) {
    CleanupShaderModule* ptr = (CleanupShaderModule*) user_ptr;
    vkDestroyShaderModule(ptr->device, ptr->shaderModule, NULL);
    free(ptr);
}
void rc_load_shader_module(VkDevice device,
    unsigned char* file, unsigned int file_len,
    VkShaderModule* outShaderModule,
    StaticCache* cleanup)
{
    if (file_len == 0) {
        exception_msg("Tried to load shader of length 0");
    }

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
    };

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = file_len * sizeof(unsigned char);
    createInfo.pCode = (uint32_t*) file;

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        exception_msg("Failed to load shader module");
    }
    *outShaderModule = shaderModule;

    CleanupShaderModule* cleanupObj = malloc(sizeof(CleanupShaderModule));
    *cleanupObj = (CleanupShaderModule) {
        .device = device,
        .shaderModule = shaderModule,
    };
    StaticCache_add(cleanup, cleanup_shader_module, cleanupObj);
}
