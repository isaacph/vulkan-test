#include "util/backtrace.h"
#include "util/memory.h"
#include "render/context.h"
#include <assert.h>
#include <stdio.h>
#include <util/backtrace.h>
#include <math.h>
#include "shaders_generated.h"

typedef struct RcImmediateConstructs {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
} RcImmediateConstructs;
typedef struct CleanupRcImmediate {
    VkDevice device;
    RcImmediateConstructs constructs;
} CleanupRcImmediate;
static void cleanup_immediate(void* user_ptr, sc_t id) {
    CleanupRcImmediate* ptr = (CleanupRcImmediate*) user_ptr;
    vkDestroyFence(ptr->device, ptr->constructs.fence, NULL);
    vkFreeCommandBuffers(ptr->device, ptr->constructs.commandPool, 1, &ptr->constructs.commandBuffer);
    vkDestroyCommandPool(ptr->device, ptr->constructs.commandPool, NULL);
}
RcImmediateConstructs rc_init_immediate(VkDevice device, uint32_t graphicsQueueFamily, StaticCache* cleanup) {
    VkCommandPool immediateCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer immediateCommandBuffer = VK_NULL_HANDLE;
    VkFence immediateFence = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueFamily,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .pNext = NULL,
    };
    check(vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &immediateCommandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = immediateCommandPool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
    check(vkAllocateCommandBuffers(device, &cmdAllocInfo, &immediateCommandBuffer));

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    check(vkCreateFence(device, &fenceCreateInfo, NULL, &immediateFence));

    RcImmediateConstructs constructs = {
        .commandPool = immediateCommandPool,
        .commandBuffer = immediateCommandBuffer,
        .fence = immediateFence,
    };
    CleanupRcImmediate* cleanupObj = checkMalloc(malloc(sizeof(CleanupRcImmediate)));
    *cleanupObj = (CleanupRcImmediate) {
        .device = device,
        .constructs = constructs,
    };
    StaticCache_add(cleanup, cleanup_immediate, cleanupObj);

    return constructs;
}
VkCommandBuffer rc_begin_immediate(VkDevice device, VkQueue graphicsQueue, RcImmediateConstructs immediate) {
    check(vkResetFences(device, 1, &immediate.fence));
    check(vkResetCommandBuffer(immediate.commandBuffer, 0));
    VkCommandBuffer cmd = immediate.commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .pInheritanceInfo = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    check(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    return cmd;
}
void rc_end_immediate(VkDevice device, VkQueue graphicsQueue, RcImmediateConstructs immediate) {
    VkCommandBuffer cmd = immediate.commandBuffer;
    check(vkEndCommandBuffer(cmd));
    VkCommandBufferSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = NULL,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
    VkSubmitInfo2 submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = NULL,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &submitInfo,
    };
    check(vkQueueSubmit2(graphicsQueue, 1, &submit, immediate.fence));
    check(vkWaitForFences(device, 1, &immediate.fence, true, 1000000000));
}

typedef struct SecondSwapchainImageCleanup {
    VkDevice device;
    VkImage image;
    VkImageView imageView;
} SecondSwapchainImageCleanup;
static void cleanup_second_swapchain_image(void* user_ptr, sc_t id) {
    SecondSwapchainImageCleanup* ptr = (SecondSwapchainImageCleanup*) user_ptr;
    vkDestroyImageView(ptr->device, ptr->imageView, NULL);
    vkDestroyImage(ptr->device, ptr->image, NULL);
    free(ptr);
}
typedef struct SecondSwapchainImageInit {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkImage drawImage; // current version of this image, or VK_NULL_HANDLE
    VkImageView drawImageView; // ignored if image is VK_NULL_HANDLE
    sc_t cleanupHandle; // current cleanup handle, or SC_ID_NONE
    VkDeviceMemory drawImageMemory; // ignored if image is VK_NULL_HANDLE
    VkDeviceSize drawImageMemoryPosition; // ignored if image is VK_NULL_HANDLE
    VkExtent2D windowSize;
    Allocations* allocations;
} SecondSwapchainImageInit;
typedef struct SecondSwapchainImage {
    VkImage drawImage; // new version of this image
    VkImageView drawImageView;
    VkFormat drawImageFormat;
    sc_t cleanupHandle; // new version of cleanup handle
    VkDeviceMemory drawImageMemory;
    VkDeviceSize drawImageMemoryPosition;
} SecondSwapchainImage;
SecondSwapchainImage rc_init_second_swapchain_image(SecondSwapchainImageInit params, StaticCache* cleanup) {
    bool newImage = params.drawImage == VK_NULL_HANDLE;
    printf("Initializing second swapchain image. New: %d\n", newImage);
    VkDeviceSize drawImageMemoryPosition = params.drawImageMemoryPosition;
    VkDeviceMemory drawImageMemory = params.drawImageMemory;

    VkImageUsageFlags drawImageUsages = 0;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkExtent3D extent = {
        .width = params.windowSize.width,
        .height = params.windowSize.height,
        .depth = 1,
    };
    VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkImageCreateInfo createInfo = rc_image_create_info(imageFormat, drawImageUsages, extent);

    SecondSwapchainImageCleanup* drawImageCleanup = checkMalloc(malloc(sizeof(SecondSwapchainImageCleanup)));
    drawImageCleanup->device = params.device;

    VkImage drawImage = params.drawImage;
    check(vkCreateImage(params.device, &createInfo, NULL, &drawImage));
    drawImageCleanup->image = drawImage;
    VkMemoryRequirements imageMemoryRequirements = { 0 };
    vkGetImageMemoryRequirements(params.device, drawImage, &imageMemoryRequirements);

    if (newImage) {
        const VkDeviceSize maxPossibleSize = 131 * 1024 * 1024;
        imageMemoryRequirements.size = max(imageMemoryRequirements.size, maxPossibleSize);
        uint32_t requiredFlags[] = {
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        Allocation allocation = rc_allocate_memory(
                params.physicalDevice, params.device, params.allocations, imageMemoryRequirements,
                requiredFlags, sizeof(requiredFlags) / sizeof(uint32_t)
        );
        drawImageMemory = allocation.memory;
        drawImageMemoryPosition = allocation.address;
        check(vkBindImageMemory(params.device, drawImage, drawImageMemory, 0));
    } else {
        // how do we reallocate if the size could be different?????
        // best idea right now: think of biggest possible screen size and just reserve that much lol
        // lmfao if we take 4x the biggest screen size we get 7680 * 4320 * 4 = 132710400 ~= 132 MB
        // lets just give a whole memory block or something?
        assert(drawImageMemory != VK_NULL_HANDLE);
        check(vkBindImageMemory(params.device, drawImage, drawImageMemory, 0));
    }

    if (newImage) {
        vkDestroyImageView(params.device, params.drawImageView, NULL);
    }
    printf("main.c:rc_init_second_swapchain_image Image about to be used for image view ID: %p\n", drawImage);
    VkImageViewCreateInfo imageViewInfo = rc_imageview_create_info(imageFormat, drawImage, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageView drawImageView = VK_NULL_HANDLE;
    check(vkCreateImageView(params.device, &imageViewInfo, NULL, &drawImageView));
    drawImageCleanup->imageView = drawImageView;

    // this will delete the old image if it exists
    sc_t cleanupHandle = StaticCache_put(cleanup, cleanup_second_swapchain_image, (void*) drawImageCleanup, params.cleanupHandle);

    return (SecondSwapchainImage) {
        .drawImage = drawImage,
        .drawImageView = drawImageView,
        .cleanupHandle = cleanupHandle,
        .drawImageMemory = drawImageMemory,
        .drawImageMemoryPosition = drawImageMemoryPosition,
        .drawImageFormat = imageFormat,
    };
}

typedef struct RcCreateImageParameters {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    Allocations* allocations;
    VkExtent2D size;
    VkFormat format;
    VkImageAspectFlags imageAspectFlags;
} RcCreateImageParameters;
typedef struct RcImage {
    VkImage image;
    VkImageView view;
    VkFormat format;
    VkExtent3D size;
    sc_t cleanupHandle;
} RcImage;
typedef struct CleanupRcImage {
    VkDevice device;
    VkImage image;
    VkImageView imageView;
} CleanupRcImage;
static void cleanup_rc_image(void* user_ptr, sc_t id) {
    CleanupRcImage* ptr = (CleanupRcImage*) user_ptr;
    vkDestroyImageView(ptr->device, ptr->imageView, NULL);
    vkDestroyImage(ptr->device, ptr->image, NULL);
    free(ptr);
}
RcImage rc_create_image(RcCreateImageParameters params, StaticCache* cleanup) {
    VkImageUsageFlags drawImageUsages = 0;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;
    VkExtent3D extent = {
        .width = params.size.width,
        .height = params.size.height,
        .depth = 1,
    };
    VkImageCreateInfo createInfo = rc_image_create_info(params.format, drawImageUsages, extent);
    CleanupRcImage* drawImageCleanup = checkMalloc(malloc(sizeof(CleanupRcImage)));
    drawImageCleanup->device = params.device;

    VkImage drawImage = VK_NULL_HANDLE;
    check(vkCreateImage(params.device, &createInfo, NULL, &drawImage));
    drawImageCleanup->image = drawImage;

    VkMemoryRequirements imageMemoryRequirements = { 0 };
    vkGetImageMemoryRequirements(params.device, drawImage, &imageMemoryRequirements);
    uint32_t requiredFlags[] = {
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    Allocation allocation = rc_allocate_memory(params.physicalDevice, params.device, params.allocations, imageMemoryRequirements,
            requiredFlags, sizeof(requiredFlags) / sizeof(uint32_t));
    // todo add cleanup

    check(vkBindImageMemory(params.device, drawImage, allocation.memory, 0));

    // maybe use VK_IMAGE_ASPECT_COLOR_BIT
    printf("main.c:rc_create_image Image about to be used for image view ID: %p\n", drawImage);
    VkImageViewCreateInfo imageViewInfo = rc_imageview_create_info(params.format, drawImage, params.imageAspectFlags);
    VkImageView drawImageView = VK_NULL_HANDLE;
    check(vkCreateImageView(params.device, &imageViewInfo, NULL, &drawImageView));
    drawImageCleanup->imageView = drawImageView;

    sc_t cleanupHandle = StaticCache_add(cleanup, cleanup_rc_image, (void*) drawImageCleanup);

    return (RcImage) {
        .image = drawImage,
        .view = drawImageView,
        .cleanupHandle = cleanupHandle,
        .format = params.format,
        .size = extent,
    };
}
void rc_write_image(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, Allocations* allocations,
        RcImmediateConstructs immediate, RcImage image, void* data, size_t length) {
    // allocate temp buffer
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = length,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    VkBuffer buffer = VK_NULL_HANDLE;
    VkMemoryRequirements requirements = { 0 };
    check(vkCreateBuffer(device, &bufferInfo, NULL, &buffer));
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    uint32_t flagPreferredLevels[] = {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    };
    Allocation allocation = rc_allocate_memory(physicalDevice, device, allocations, requirements,
            flagPreferredLevels, sizeof(flagPreferredLevels) / sizeof(uint32_t));
    check(vkBindBufferMemory(device, buffer, allocation.memory, allocation.address));

    // write data to buffer
    void* mappedMemory = NULL;
    printf("memory: %p, address: %lld, size: %lld\n", allocation.memory, allocation.address, allocation.size);
    check(vkMapMemory(device, allocation.memory, allocation.address, allocation.size, 0, &mappedMemory));
    memcpy(mappedMemory, data, length);
    if ((allocation.flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 0) {
        printf("Flushing mapped memory ranges, size: %lld\n", allocation.size);
        VkMappedMemoryRange range = {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = NULL,
            .memory = allocation.memory,
            .offset = allocation.address,
            .size = allocation.size,
        };
        check(vkFlushMappedMemoryRanges(device, 1, &range));
    }

    // write buffer to image
    VkCommandBuffer cmd = rc_begin_immediate(device, graphicsQueue, immediate);
    rc_transition_image(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy copyRegion = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = image.size,
    };
    vkCmdCopyBufferToImage(cmd, buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    rc_transition_image(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    rc_end_immediate(device, graphicsQueue, immediate);
}

typedef struct DescriptorPoolsCleanup {
    VkDevice device;
    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
} DescriptorPoolsCleanup;
static void cleanup_descriptor_pools(void* user_ptr, sc_t id) {
    DescriptorPoolsCleanup* cleanup = (DescriptorPoolsCleanup*) user_ptr;
    // vkFreeDescriptorSets(cleanup->device, cleanup->pool, 1, &cleanup->set);
    vkDestroyDescriptorSetLayout(cleanup->device, cleanup->layout, NULL);
    vkDestroyDescriptorPool(cleanup->device, cleanup->pool, NULL);
    free(cleanup);
}
typedef struct InitDescriptors {
    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
} InitDescriptors;
InitDescriptors rc_init_descriptors(VkDevice device, VkImageView drawImageView, StaticCache* cleanup) {
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;

    // descriptor pool
    uint32_t maxSets = 10;
    VkDescriptorPoolSize poolSizes[] = {
        (VkDescriptorPoolSize) {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = (uint32_t) (1 * maxSets), // ratio x maxSets
        },
    };
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize),
        .pPoolSizes = poolSizes,
    };
    check(vkCreateDescriptorPool(device, &info, NULL, &pool));

    // descriptor set layout
    VkDescriptorSetLayoutBinding bindings[] = {
        (VkDescriptorSetLayoutBinding) {
            .binding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pBindings = bindings,
        .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
        .flags = 0,
    };
    check(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, NULL, &layout));

    // descriptor set
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    check(vkAllocateDescriptorSets(device, &allocInfo, &set));

    // now we have to point the descriptor set to be able to write to drawImage
    VkDescriptorImageInfo imgInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageView = drawImageView,
    };
    VkWriteDescriptorSet drawImageWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = NULL,
        .dstBinding = 0,
        .dstSet = set,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &imgInfo,
    };
    vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, NULL);

    DescriptorPoolsCleanup* cleanupObj = malloc(sizeof(DescriptorPoolsCleanup));
    *cleanupObj = (DescriptorPoolsCleanup) {
        .device = device,
        .pool = pool,
        .layout = layout,
    };
    StaticCache_add(cleanup, cleanup_descriptor_pools, cleanupObj);
    return (InitDescriptors) {
        .pool = pool,
        .layout = layout,
        .set = set,
    };
}

typedef struct CleanupPipelines {
    VkDevice device;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
} CleanupPipelines;
static void cleanup_pipelines(void* user_ptr, sc_t id) {
    CleanupPipelines* ptr = (CleanupPipelines*) user_ptr;
    vkDestroyPipelineLayout(ptr->device, ptr->pipelineLayout, NULL);
    vkDestroyPipeline(ptr->device, ptr->pipeline, NULL);
    free(ptr);
}
typedef struct InitPipelines {
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
} InitPipelines;
InitPipelines rc_init_compute_pipelines(VkDevice device, VkDescriptorSetLayout layout, StaticCache* cleanup) {
    VkPipelineLayout gradientPipelineLayout = VK_NULL_HANDLE;
    VkPipeline gradientPipeline = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo computeLayout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = &layout,
        .setLayoutCount = 1,
    };
    check(vkCreatePipelineLayout(device, &computeLayout, NULL, &gradientPipelineLayout));
    VkShaderModule computeDrawShader = VK_NULL_HANDLE;
    rc_load_shader_module(device, SHADER_gradient_comp, SHADER_gradient_comp_len, &computeDrawShader, cleanup);

    VkPipelineShaderStageCreateInfo stageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = computeDrawShader,
        .pName = "main",
    };

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .layout = gradientPipelineLayout,
        .stage = stageInfo,
    };
    check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, NULL, &gradientPipeline));

    CleanupPipelines* cleanupObj = malloc(sizeof(CleanupPipelines));
    *cleanupObj = (CleanupPipelines) {
        .device = device,
        .pipelineLayout = gradientPipelineLayout,
        .pipeline = gradientPipeline,
    };
    StaticCache_add(cleanup, cleanup_pipelines, cleanupObj);

    return (InitPipelines) {
        .pipeline = gradientPipeline,
        .pipelineLayout = gradientPipelineLayout,
    };
}

InitPipelines rc_init_graphics_pipelines(VkDevice device, VkFormat drawImageFormat, StaticCache* cleanup) {
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkShaderModule triangleVertexShader = VK_NULL_HANDLE;
    VkShaderModule triangleFragShader = VK_NULL_HANDLE;
    rc_load_shader_module(device, SHADER_triangle_vert, SHADER_triangle_vert_len, &triangleVertexShader, cleanup);
    rc_load_shader_module(device, SHADER_triangle_frag, SHADER_triangle_frag_len, &triangleFragShader, cleanup);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = (VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = NULL,
        .setLayoutCount = 0,
    };
    check(vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout));

    VkFormat colorAttachmentFormat = drawImageFormat;
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkPipelineRenderingCreateInfo renderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorAttachmentFormat,
        .depthAttachmentFormat = depthAttachmentFormat,
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        // keep empty since we are using a "pull" vertex input model instead of fixed (which is this)
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .viewportCount = 1,
        .scissorCount = 1,
        // keep "empty/default" since we are using dynamic state for this
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
    };
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        // no sampling (1 sample per pixel)
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        // no alpha to coverage (?????) either
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = (VkStencilOpState) { 0 },
        .back = (VkStencilOpState) { 0 },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = triangleVertexShader,
            .pName = "main",
        },
        (VkPipelineShaderStageCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = triangleFragShader,
            .pName = "main",
        },
    };

    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = state,
        .dynamicStateCount = sizeof(state) / sizeof(VkDynamicState),
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderInfo,
        .stageCount = sizeof(shaderStages) / sizeof(VkPipelineShaderStageCreateInfo),
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDepthStencilState = &depthStencil,
        .pDynamicState = &dynamicInfo,
        .layout = pipelineLayout,
    };
    check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline));

    CleanupPipelines* cleanupObj = malloc(sizeof(CleanupPipelines));
    *cleanupObj = (CleanupPipelines) {
        .device = device,
        .pipelineLayout = pipelineLayout,
        .pipeline = pipeline,
    };
    StaticCache_add(cleanup, cleanup_pipelines, cleanupObj);

    return (InitPipelines) {
        .pipeline = pipeline,
        .pipelineLayout = pipelineLayout,
    };
}


int main() {
    init_exceptions(false);

    StaticCache cleanup = StaticCache_init(1000);
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    WindowHandle windowHandle = { 0 };
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkExtent2D size = { 0 };
    VkSurfaceFormatKHR surfaceFormat = { 0 };
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain;
    SwapchainImageData swapchainImages[RC_SWAPCHAIN_LENGTH];
    FrameData frames[RC_SWAPCHAIN_LENGTH];
    VkSemaphore swapchainSemaphores[RC_SWAPCHAIN_LENGTH];
    sc_t swapchainCleanupHandle = SC_ID_NONE;

    RcImmediateConstructs immediate;

    VkImage drawImage = VK_NULL_HANDLE; // the image we draw directly to, copied to swapchain
    VkImageView drawImageView = VK_NULL_HANDLE;
    VkFormat drawImageFormat = 0;
    sc_t drawImageCleanupHandle = SC_ID_NONE;
    VkDeviceMemory drawImageMemory = VK_NULL_HANDLE; // we need to be able to reallocate this memory on resize so record its properties here
    VkDeviceSize drawImageMemoryPosition = 0;

    Allocations allocations = { 0 };

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;

    VkPipelineLayout gradientPipelineLayout = VK_NULL_HANDLE;
    VkPipeline gradientPipeline = VK_NULL_HANDLE;
    VkPipelineLayout trianglePipelineLayout = VK_NULL_HANDLE;
    VkPipeline trianglePipeline = VK_NULL_HANDLE;

    RcImage texture = { 0 };

    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
        assert(instance != VK_NULL_HANDLE);
    }
    {
        const char* title = "Test window! \xF0\x9F\x87\xBA\xF0\x9F\x87\xB8";
        InitSurfaceParams params = {
            .instance = instance,
            .title = title,
            .titleLength = strlen(title),
            .size = DEFAULT_SURFACE_SIZE,
            .headless = false,
        };
        InitSurface ret = rc_init_surface(params, &cleanup);
        surface = ret.surface;
        windowHandle = ret.windowHandle;
        size = ret.size;
        assert(surface != NULL);
        assert(size.width != 0);
        assert(size.height != 0);
        assert(size.width != DEFAULT_SURFACE_SIZE.width && size.height != DEFAULT_SURFACE_SIZE.height);
    }
    {
        // so we need to refactor the logic so that surface format is chosen when physical device is chosen
        InitDeviceParams params = {
            .instance = instance,
            .surface = surface,
        };
        InitDevice ret = rc_init_device(params, &cleanup);
        device = ret.device;
        surfaceFormat = ret.surfaceFormat;
        graphicsQueueFamily = ret.graphicsQueueFamily;
        physicalDevice = ret.physicalDevice;
        graphicsQueue = ret.graphicsQueue;
        assert(ret.device != NULL);
    }
    // set up allocations system
    {
        for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
            allocations.allocation[i] = VK_NULL_HANDLE;
            allocations.allocationOffset[i] = 0;
        }
        allocations.toDeallocate = calloc(VK_MAX_MEMORY_TYPES, sizeof(VkDeviceMemory));

        AllocationsCleanup* allocCleanup = checkMalloc(malloc(sizeof(AllocationsCleanup)));
        *allocCleanup = (AllocationsCleanup) {
            .device = device,
            .toDeallocate = allocations.toDeallocate,
        };
        StaticCache_add(&cleanup, cleanup_allocations, allocCleanup);

        // Memory system needs access to limits
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        allocations.deviceLimits = properties.limits;
    }
    // init "immediate submit" constructs
    {
        immediate = rc_init_immediate(device, graphicsQueueFamily, &cleanup);
    }
    // init swapchain
    {
        InitSwapchainParams params = {
            .extent = size,

            .device = device,
            .physicalDevice = physicalDevice,
            .surface = surface,
            .surfaceFormat = surfaceFormat,
            .graphicsQueueFamily = graphicsQueueFamily,

            .oldSwapchain = VK_NULL_HANDLE,
            .swapchainCleanupHandle = swapchainCleanupHandle,
        };
        InitSwapchain ret = rc_init_swapchain(params, &cleanup);
        swapchain = ret.swapchain;
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            swapchainImages[i] = ret.images[i];
        }
        swapchainCleanupHandle  = ret.swapchainCleanupHandle;
        assert(swapchain != NULL);
    }
    {
        InitLoopParams params = {
            .device = device,
            .graphicsQueueFamily = graphicsQueueFamily,
        };
        InitLoop ret = rc_init_loop(params, &cleanup);
        for (int i = 0; i < FRAME_OVERLAP; ++i) {
            assert(ret.frames[i].commandPool != VK_NULL_HANDLE);
            frames[i] = ret.frames[i];
        }
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            assert(ret.swapchainSemaphores[i] != VK_NULL_HANDLE);
            swapchainSemaphores[i] = ret.swapchainSemaphores[i];
        }
    }
    // init the image that we draw to
    {
        SecondSwapchainImageInit params = {
            .physicalDevice = physicalDevice,
            .device = device,
            .drawImage = VK_NULL_HANDLE,
            .drawImageView = VK_NULL_HANDLE,
            .cleanupHandle = SC_ID_NONE,
            .drawImageMemory = 0,
            .drawImageMemoryPosition = 0,
            .windowSize = size,
            .allocations = &allocations,
        };
        SecondSwapchainImage ret = rc_init_second_swapchain_image(params, &cleanup);
        drawImage = ret.drawImage;
        drawImageView = ret.drawImageView;
        drawImageFormat = ret.drawImageFormat;
        drawImageCleanupHandle = ret.cleanupHandle;
        drawImageMemory = ret.drawImageMemory;
        drawImageMemoryPosition = ret.drawImageMemoryPosition;
    }
    // init descriptor set
    {
        InitDescriptors ret = rc_init_descriptors(device, drawImageView, &cleanup);
        pool = ret.pool;
        layout = ret.layout;
        set = ret.set;
    }
    {
        InitPipelines ret = rc_init_compute_pipelines(device, layout, &cleanup);
        gradientPipelineLayout = ret.pipelineLayout;
        gradientPipeline = ret.pipeline;
    }
    {
        InitPipelines ret = rc_init_graphics_pipelines(device, drawImageFormat, &cleanup);
        trianglePipelineLayout = ret.pipelineLayout;
        trianglePipeline = ret.pipeline;
    }
    {
        RcCreateImageParameters params = {
            .device = device,
            .physicalDevice = physicalDevice,
            .imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
            .format = VK_FORMAT_R8_UINT,
            .size = (VkExtent2D) {
                .width = 2,
                .height = 2,
            },
            .allocations = &allocations,
        };
        uint64_t data[] = {
            0xF00FEBFF, 0x000000FF,
            0x000000FF, 0xE534EBFF,
        };
        const int length = 4 * sizeof(uint64_t);
        // TODO
        // texture = rc_create_image(params, &cleanup);
        // rc_write_image(physicalDevice, device, graphicsQueue, &allocations, immediate, texture, data, 4 * sizeof(uint64_t));
    }
    {
        DrawParams params = {
            .device = device,
            .graphicsQueue = graphicsQueue,
            .swapchain = swapchain,
            .swapchainImages = { 0 },
        };
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            params.swapchainImages[i] = swapchainImages[i];
        }

        bool running = true;
        int frameNumber = 0;
        // raise this limit to test resizing manually
        while (running) {
            WindowUpdate update = rc_window_update(&windowHandle);
            running = !update.windowClosed;
            if (update.resize) {
                size = update.newSize;
                printf("new size: %d x %d\n", size.width, size.height);
                if (size.width < 0 || size.height < 0) {
                    size = (VkExtent2D) {
                        .width = 0,
                        .height = 0,
                    };
                }
                if (size.width * size.height > 0) {
                    InitSwapchainParams swapchainParams = {
                        .extent = size,

                        .device = device,
                        .physicalDevice = physicalDevice,
                        .surface = surface,
                        .surfaceFormat = surfaceFormat,
                        .graphicsQueueFamily = graphicsQueueFamily,

                        .oldSwapchain = swapchain,
                        .swapchainCleanupHandle = swapchainCleanupHandle,
                    };
                    InitSwapchain ret = rc_init_swapchain(swapchainParams, &cleanup);
                    swapchain = ret.swapchain;
                    swapchainCleanupHandle = ret.swapchainCleanupHandle;
                    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                        swapchainImages[i] = ret.images[i];
                    }
                    // YOU MUST make sure to update swapchain in context!
                    params.swapchain = swapchain;
                    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                        params.swapchainImages[i] = ret.images[i];
                    }

                    // we also need to accept second swapchain
                    {
                        SecondSwapchainImageInit params = {
                            .physicalDevice = physicalDevice,
                            .device = device,
                            .drawImage = drawImage,
                            .drawImageView = drawImageView,
                            .cleanupHandle = drawImageCleanupHandle,
                            .drawImageMemory = drawImageMemory,
                            .drawImageMemoryPosition = drawImageMemoryPosition,
                            .windowSize = size,
                            .allocations = &allocations,
                        };
                        SecondSwapchainImage ret = rc_init_second_swapchain_image(params, &cleanup);
                        drawImage = ret.drawImage;
                        drawImageView = ret.drawImageView;
                        drawImageCleanupHandle = ret.cleanupHandle;
                        drawImageMemory = ret.drawImageMemory;
                        drawImageMemoryPosition = ret.drawImageMemoryPosition;
                    }

                    // now we have to point the descriptor set to be able to write to drawImage
                    VkDescriptorImageInfo imgInfo = {
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                        .imageView = drawImageView,
                    };
                    VkWriteDescriptorSet drawImageWrite = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = NULL,
                        .dstBinding = 0,
                        .dstSet = set,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        .pImageInfo = &imgInfo,
                    };
                    vkUpdateDescriptorSets(device, 1, &drawImageWrite, 0, NULL);
                }
            }

            if (update.shouldDraw && size.width * size.height > 0) {
                frameNumber++;
                params.swapchainExtent = size;
                params.drawImageExtent = size;
                params.drawImage = drawImage;
                params.drawImageView = drawImageView;
                params.drawImageDescriptorSet = set;
                params.gradientPipeline = gradientPipeline;
                params.gradientPipelineLayout = gradientPipelineLayout;
                params.trianglePipeline = trianglePipeline;
                params.trianglePipelineLayout = trianglePipelineLayout;
                params.frame = frames[frameNumber % FRAME_OVERLAP];
                for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                    params.swapchainSemaphore[i] = swapchainSemaphores[i];
                }

                rc_draw(params);
                // if (frameNumber >= 5) {
                //     exception_msg("temp pause");
                // }
            }
        }
    }

    vkDeviceWaitIdle(device);
    StaticCache_clean_up(&cleanup);
}

/*
 * Next plan. I want to make a movable square with PVM. Then I want it textured. Then I want the textures to be font text.
 * I was debating on how far to go with optimization here. What I want is something that will work for simple 2D projects.
 * I think that going lowest possible effort will still work for simple 2D projects, and it would be nice to be proven wrong
 * on that if that ends up being the case. This means that I will construct this exactly like I've constructed previous projects
 * in OpenGL. Basically, one pipeline per rendering mode, rerun the pipeline per square. So basically:
 * rc_transition_image(... LAYOUT_GENERAL)
 * for each square (including textured squares)
 *   vkBeginRendering()
 *   ...
 *   vkEndRendering()
 * rc_transition_image(... TRANSFER_DST_OPTIMAL)
 *
 * ok time to do research because I am wondering if we can do a bit better
 * rc_transition_image(... LAYOUT_GENERAL)
 * vkCmdBeginRendering()
 * vkCmdBindPipeline()
 * vk something scissor
 * vk something viewport
 * for each square (including textured squares)
 *   -- somehow change push constants
 *   vkCmdDraw
 * vkEndRendering()
 * rc_transition_image(... TRANSFER_DST_OPTIMAL)
 *
 * so how to do push constants
 * looks like there is a vkCmdPushConstants
 * ez enough
 * we also need something to change the texture probably with another vkCmd
 * and hopefully it doesn't involve a different descriptor set for every texture? else actually we need
 * to change that with a vkCmd
 */
