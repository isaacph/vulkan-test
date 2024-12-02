#include "util/backtrace.h"
#include "util/memory.h"
#include "render/context.h"
#include <assert.h>
#include <stdio.h>
#include <util/backtrace.h>
#include <math.h>
#include "shaders_generated.h"

typedef struct AllocationCleanup {
    VkDevice device;
    VkDeviceMemory allocation;
} AllocationCleanup;
void cleanup_allocation(void* user_ptr, sc_t id) {
    AllocationCleanup* ptr = (AllocationCleanup*) user_ptr;
    vkFreeMemory(ptr->device, ptr->allocation, NULL);
    free(ptr);
}

// I see this as something we can build out little by little.
// 1. One allocation per memory type, with reasonable size for allocation. No partial deallocation.
// 2. Multiple allocations per memory type, with reasonable size for each allocation. No partial deallocation.
// 3. Multiple allocations. Simple linear search allocation + simple deallocation tracking.
// 4. Multiple allocations. At this point we need a more advanced strategy for choosing allocation position. Possibly can fragment based on
//    how frequently we need to reallocate some specific memory allocation. Longer-term, larger allocations can get a space, and shorter-term
//    small allocations can get another space which uses a basic allocation strategy. Possibly could design to over-deallocate so that
//    frequent allocation is as stream-lined as possible for these smaller or shorter-term allocations. Could implement tracking of performance
//    too for specific applications so that we can make informed decisions on more complex designs.
typedef struct Allocations {
    VkDeviceMemory allocation[VK_MAX_MEMORY_TYPES];
    VkDeviceSize allocationOffset[VK_MAX_MEMORY_TYPES];
    VkDeviceMemory* toDeallocate; // ptr to array of length VK_MAX_MEMORY_TYPES. Make sure to add all allocations to this array too for cleanup
} Allocations;
typedef struct AllocationsCleanUp {
    VkDevice device;
    VkDeviceMemory* toDeallocate; // ptr to array of length VK_MAX_MEMORY_TYPES
} AllocationsCleanup;
// Another consideration is handling the lifetime of this thing. Every other Vulkan resource we can just assume we can allocate
// and deallocate like it's a stack. But I think this one works a bit differently. There are other Vulkan resources depending on this
// structure, so we need to make sure to deallocate after all of them deallocate. Is there anything that needs to be deallocated before
// this one though? Probably the device + instance at least. Going off that just making a universal memory deallocation to happen
// when device is destroyed
void cleanup_allocations(void* user_ptr, sc_t id) {
    AllocationsCleanup* ptr = (AllocationsCleanup*) user_ptr;
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        if (ptr->toDeallocate[i] != VK_NULL_HANDLE) {
            vkFreeMemory(ptr->device, ptr->toDeallocate[i], NULL);
        }
    }
    free(ptr);
}

typedef struct SecondSwapchainImageCleanup {
    VkDevice device;
    VkImage image;
    VkImageView imageView;
} SecondSwapchainImageCleanup;
void cleanup_second_swapchain_image(void* user_ptr, sc_t id) {
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
    SecondSwapchainImageCleanup* drawImageCleanup; // current cleanup handle, or NULL
    uint32_t drawImageChosenMemoryType; // ignored if image is VK_NULL_HANDLE
    VkDeviceSize drawImageMemoryPosition; // ignored if image is VK_NULL_HANDLE
    VkExtent2D windowSize;
    Allocations* allocations;
    StaticCache* cleanup;
} SecondSwapchainImageInit;
typedef struct SecondSwapchainImage {
    VkImage drawImage; // new version of this image
    VkImageView drawImageView;
    VkFormat drawImageFormat;
    SecondSwapchainImageCleanup* drawImageCleanup; // new version of cleanup handle
    uint32_t drawImageChosenMemoryType;
    VkDeviceSize drawImageMemoryPosition;
} SecondSwapchainImage;
SecondSwapchainImage rc_init_second_swapchain_image(SecondSwapchainImageInit params) {
    bool newImage = params.drawImage == VK_NULL_HANDLE;
    uint32_t drawImageChosenMemoryType = params.drawImageChosenMemoryType;
    VkDeviceSize drawImageMemoryPosition = params.drawImageMemoryPosition;

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

    SecondSwapchainImageCleanup* drawImageCleanup = params.drawImageCleanup;
    if (newImage) {
        drawImageCleanup = checkMalloc(malloc(sizeof(SecondSwapchainImageCleanup)));
        drawImageCleanup->device = params.device;
    }
    VkImage drawImage = params.drawImage;
    if (newImage) {
        vkDestroyImage(params.device, drawImage, NULL);
    }
    check(vkCreateImage(params.device, &createInfo, NULL, &drawImage));
    drawImageCleanup->image = drawImage;
    VkMemoryRequirements imageMemoryRequirements = { 0 };
    vkGetImageMemoryRequirements(params.device, drawImage, &imageMemoryRequirements);

    const VkDeviceSize maxPossibleSize = 131 * 1024 * 1024;
    // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    // then allocate VkDeviceMemory from that memory type
    VkDeviceSize requiredSize = 256 * 1024 * 1024;
    uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(params.physicalDevice, &properties);
    for (int i = 0; i < properties.memoryTypeCount; ++i) {
        if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) != 0) {
            uint32_t flags = properties.memoryTypes[i].propertyFlags;
            if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
                uint32_t heapIndex = properties.memoryTypes[i].heapIndex;
                VkDeviceSize heapSize = properties.memoryHeaps[heapIndex].size; 
                if (heapSize >= requiredSize) {
                    chosenMemoryTypeIndex = i;
                    printf("chosen type #%d heapIndex: %u, flags: %x\n", i,
                            properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
                    break;
                }
            }
        }
    }
    assert(chosenMemoryTypeIndex != UINT32_MAX);
    if (newImage) {
        VkMemoryAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = requiredSize,
            .memoryTypeIndex = chosenMemoryTypeIndex,
        };
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        check(vkAllocateMemory(params.device, &allocateInfo, NULL, &deviceMemory));
        check(vkBindImageMemory(params.device, drawImage, deviceMemory, 0));
        // normally we'd check if we already had an allocation before allocating here, but for our purposes we know we are
        // the first allocation
        params.allocations->allocation[chosenMemoryTypeIndex] = deviceMemory;
        params.allocations->allocationOffset[chosenMemoryTypeIndex] = maxPossibleSize;
        params.allocations->toDeallocate[chosenMemoryTypeIndex] = deviceMemory; // sets it up for cleanup
        drawImageChosenMemoryType = chosenMemoryTypeIndex;
        drawImageMemoryPosition = 0;
    } else {
        assert(chosenMemoryTypeIndex == drawImageChosenMemoryType);
        // how do we reallocate if the size could be different?????
        // best idea right now: think of biggest possible screen size and just reserve that much lol
        // lmfao if we take 4x the biggest screen size we get 7680 * 4320 * 4 = 132710400 ~= 132 MB
        // lets just give a whole memory block or something?
        VkDeviceMemory deviceMemory = params.allocations->allocation[chosenMemoryTypeIndex];
        assert(deviceMemory != VK_NULL_HANDLE);
        check(vkBindImageMemory(params.device, drawImage, deviceMemory, 0));
    }

    if (newImage) {
        vkDestroyImageView(params.device, params.drawImageView, NULL);
    }
    VkImageViewCreateInfo imageViewInfo = rc_imageview_create_info(imageFormat, drawImage, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageView drawImageView = VK_NULL_HANDLE;
    check(vkCreateImageView(params.device, &imageViewInfo, NULL, &drawImageView));
    drawImageCleanup->imageView = drawImageView;

    if (newImage) {
        StaticCache_add(params.cleanup, cleanup_second_swapchain_image, (void*) drawImageCleanup);
    }

    return (SecondSwapchainImage) {
        .drawImage = drawImage,
        .drawImageView = drawImageView,
        .drawImageCleanup = drawImageCleanup,
        .drawImageChosenMemoryType = drawImageChosenMemoryType,
        .drawImageMemoryPosition = drawImageMemoryPosition,
        .drawImageFormat = imageFormat,
    };
}

typedef struct DescriptorPoolsCleanup {
    VkDevice device;
    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
} DescriptorPoolsCleanup;
void cleanup_descriptor_pools(void* user_ptr, sc_t id) {
    DescriptorPoolsCleanup* cleanup = (DescriptorPoolsCleanup*) user_ptr;
    vkFreeDescriptorSets(cleanup->device, cleanup->pool, 1, &cleanup->set);
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
    FrameData frames[FRAME_OVERLAP];
    sc_t swapchainCleanupHandle = SC_ID_NONE;

    VkImage drawImage = VK_NULL_HANDLE; // the image we draw directly to, copied to swapchain
    VkImageView drawImageView = VK_NULL_HANDLE;
    VkFormat drawImageFormat = 0;
    SecondSwapchainImageCleanup* drawImageCleanup = NULL;
    uint32_t drawImageChosenMemoryType = 0; // we need to be able to reallocate this memory on resize so record its properties here
    VkDeviceSize drawImageMemoryPosition = 0;

    Allocations allocations = { 0 };
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        allocations.allocation[i] = VK_NULL_HANDLE;
        allocations.allocationOffset[i] = 0;
    }
    allocations.toDeallocate = calloc(VK_MAX_MEMORY_TYPES, sizeof(VkDeviceMemory));

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;

    VkPipelineLayout gradientPipelineLayout = VK_NULL_HANDLE;
    VkPipeline gradientPipeline = VK_NULL_HANDLE;
    VkPipelineLayout trianglePipelineLayout = VK_NULL_HANDLE;
    VkPipeline trianglePipeline = VK_NULL_HANDLE;

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
        AllocationsCleanup* allocCleanup = checkMalloc(malloc(sizeof(AllocationsCleanup)));
        *allocCleanup = (AllocationsCleanup) {
            .device = ret.device,
            .toDeallocate = allocations.toDeallocate,
        };
        StaticCache_add(&cleanup, cleanup_allocations, allocCleanup);
    }
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
    }
    // init device memory allocation
    // {
    //     // is it possible to allocate some of each memory requirement first?
    //     // so it seems this simple:
    //     // I want to be able to allocate memory in advance.
    //     // The implementation wants to require some specific sets of memory types that we can even put images in though
    //     // the options are to either find out what types those are in advance by making sets of image types that we expect to use
    //     // that we can use to anticipate required types so that the allocations are already there once we need them
    //     // or we can just wait on allocation until a specific type is required. then we would do the allocation
    //     //
    //     // waiting on the allocation until a type is required is better because 1. if the types change over program lifetime no
    //     // extra handling might be needed
    //     //
    //     // the question is if that 1. is a valid reason. checking the documentation for "can vkGetImageMemoryRequirements2 return
    //     // different results with the same inputs"
    //     //
    //     //
    //     // so based on the docs, as long as I'm not using features that are too esoteric, types will not change over program lifetime
    //     //
    //     // so why might i want to wait on allocation until a type is required, if we assume we can figure that out at the start?
    //     // well not waiting means I have to know in advance what image formats I'm going to need, and allocate for each of those.
    //     // (and buffer formats). keeping track of this sounds like a lot of work I'm trying to minimize until absolutely necessary
    //     //
    //     // what's bad about waiting? well if I wait then I have to basically have an array of an allocation for each memoryType
    //     // it may even need to be a 2d array if we want long term scalability
    //     // it means i have to do some framework writing here right away.
    //     // normally the point of frameworks is to align to changing programmer side requirements. we find this to be overkill a lot of the time,
    //     // hence we are trying this type of coding we're doing here where we are trying to minimize frameworking
    //     // but. now we have to align to changing runtime device requirements.
    //     //
    //     // can we just go implement a really naive solution for now and make it better later?

    //     // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    //     // then allocate VkDeviceMemory from that memory type
    //     VkDeviceSize requiredSize = 256 * 1024 * 1024;
    //     uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    //     VkPhysicalDeviceMemoryProperties properties = { 0 };
    //     vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    //     for (int i = 0; i < properties.memoryTypeCount; ++i) {
    //         if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) != 0) {
    //             uint32_t flags = properties.memoryTypes[i].propertyFlags;
    //             if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
    //                 uint32_t heapIndex = properties.memoryTypes[i].heapIndex;
    //                 VkDeviceSize heapSize = properties.memoryHeaps[heapIndex].size; 
    //                 if (heapSize >= requiredSize) {
    //                     chosenMemoryTypeIndex = i;
    //                     printf("chosen type #%d heapIndex: %u, flags: %x\n", i,
    //                             properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
    //                     break;
    //                 }
    //             }
    //         }
    //     }
    //     assert(chosenMemoryTypeIndex != UINT32_MAX);
    //     VkMemoryAllocateInfo allocateInfo = {
    //         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //         .pNext = NULL,
    //         .allocationSize = requiredSize,
    //         .memoryTypeIndex = chosenMemoryTypeIndex,
    //     };
    //     check(vkAllocateMemory(device, &allocateInfo, NULL, &allocation));
    //     check(vkBindImageMemory(device, image, allocation, 0));
    //     cleanupObject->memory = allocation;
    // }
    // init descriptor set
    // init the image that we draw to
    {
        SecondSwapchainImageInit params = {
            .physicalDevice = physicalDevice,
            .device = device,
            .drawImage = VK_NULL_HANDLE,
            .drawImageView = VK_NULL_HANDLE,
            .drawImageCleanup = NULL,
            .drawImageChosenMemoryType = 0,
            .drawImageMemoryPosition = 0,
            .windowSize = size,
            .allocations = &allocations,
            .cleanup = &cleanup,
        };
        SecondSwapchainImage ret = rc_init_second_swapchain_image(params);
        drawImage = ret.drawImage;
        drawImageView = ret.drawImageView;
        drawImageFormat = ret.drawImageFormat;
        drawImageCleanup = ret.drawImageCleanup;
        drawImageChosenMemoryType = ret.drawImageChosenMemoryType;
        drawImageMemoryPosition = ret.drawImageMemoryPosition;
    }
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
                            .drawImageCleanup = drawImageCleanup,
                            .drawImageChosenMemoryType = drawImageChosenMemoryType,
                            .drawImageMemoryPosition = drawImageMemoryPosition,
                            .windowSize = size,
                            .allocations = &allocations,
                            .cleanup = &cleanup,
                        };
                        SecondSwapchainImage ret = rc_init_second_swapchain_image(params);
                        drawImage = ret.drawImage;
                        drawImageView = ret.drawImageView;
                        drawImageCleanup = ret.drawImageCleanup;
                        drawImageChosenMemoryType = ret.drawImageChosenMemoryType;
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
                params.frame = frames[frameNumber % 2];
                params.color = fabs(sin(frameNumber / 120.f));
                params.swapchainExtent = size;
                params.drawImageExtent = size;
                params.drawImage = drawImage;
                params.drawImageView = drawImageView;
                params.drawImageDescriptorSet = set;
                params.gradientPipeline = gradientPipeline;
                params.gradientPipelineLayout = gradientPipelineLayout;
                params.trianglePipeline = trianglePipeline;
                params.trianglePipelineLayout = trianglePipelineLayout;
                rc_draw(params);
            }
        }
    }

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
