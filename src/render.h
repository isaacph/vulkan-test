#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

#include <vulkan/vulkan_core.h>

struct RenderContext {
    uint32_t instanceVersion;
    uint32_t extensionsCount;
    VkExtensionProperties* extensions;
    uint32_t layersCount;
    VkLayerProperties* layers;
};

struct RenderContext init_render();
void free_render(struct RenderContext* renderContext);

#endif
