#ifndef RENDER_UTIL_H_INCLUDED
#define RENDER_UTIL_H_INCLUDED

#include "render_functions.h"
#include <stdint.h>
#include <vulkan/vulkan_core.h>

// caller must free return value
const char* interpret_VK_API_VERSION(uint32_t version);
// caller must free return value
const char* interpret_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions);
// caller must free return value
const char* interpret_VkLayerProperties(uint32_t count, VkLayerProperties* layers);

// void interpret_VK_API_VERSION(uint32_t version, char* out_memory, uint64_t memory_length);
// void interpret_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions, char* out_memory, uint64_t memory_length);
// void interpret_VkLayerProperties(uint32_t count, VkLayerProperties* layers, char* out_memory, uint64_t memory_length);

#endif
