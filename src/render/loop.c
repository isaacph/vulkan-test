#include <stdbool.h>
#include "context.h"
#include "functions.h"
#include <math.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "render/util.h"

typedef struct CleanupLoop {
    VkDevice device;
    FrameData frames[FRAME_OVERLAP];
} CleanupLoop;
static void cleanup_loop(void* ptr, sc_t id) {
    CleanupLoop* cleanup = (CleanupLoop*) ptr;
    vkDeviceWaitIdle(cleanup->device);
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        vkFreeCommandBuffers(cleanup->device, cleanup->frames[i].commandPool, 1, &cleanup->frames[i].mainCommandBuffer);
        vkDestroyCommandPool(cleanup->device, cleanup->frames[i].commandPool, NULL);
        vkDestroyFence(cleanup->device, cleanup->frames[i].renderFence, NULL);
        vkDestroySemaphore(cleanup->device, cleanup->frames[i].renderSemaphore, NULL);
        vkDestroySemaphore(cleanup->device, cleanup->frames[i].swapchainSemaphore, NULL);
    }
}

InitLoop rc_init_loop(InitLoopParams params, StaticCache* cleanup) {
    assert(params.device != VK_NULL_HANDLE);

    FrameData frames[FRAME_OVERLAP];

    // init command pools and buffers for each frame
    for (uint32_t index = 0; index < FRAME_OVERLAP; ++index) {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = params.graphicsQueueFamily,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .pNext = NULL,
        };
        check(vkCreateCommandPool(params.device, &commandPoolCreateInfo, NULL, &commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = commandPool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };
        check(vkAllocateCommandBuffers(params.device, &cmdAllocInfo, &commandBuffer));

        frames[index].commandPool = commandPool;
        frames[index].mainCommandBuffer = commandBuffer;
    }

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        VkFence renderFence = VK_NULL_HANDLE;
        VkSemaphore swapchainSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderSemaphore = VK_NULL_HANDLE;

        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        check(vkCreateFence(params.device, &fenceCreateInfo, NULL, &renderFence));

        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
        };
        check(vkCreateSemaphore(params.device, &semaphoreCreateInfo, NULL, &swapchainSemaphore));
        check(vkCreateSemaphore(params.device, &semaphoreCreateInfo, NULL, &renderSemaphore));

        frames[i].renderFence = renderFence;
        frames[i].swapchainSemaphore = swapchainSemaphore;
        frames[i].renderSemaphore = renderSemaphore;
    }

    InitLoop initLoop = { 0 };
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        initLoop.frames[i] = frames[i];
    }
    return initLoop;
}

static VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
    VkSemaphoreSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = NULL,
        .semaphore = semaphore,
        .stageMask = stageMask,
        .deviceIndex = 0,
        .value = 1,
    };
    return submitInfo;
}

static VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd) {
    VkCommandBufferSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = NULL,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
    return info;
}

static VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
    VkSubmitInfo2 info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = NULL,

        .waitSemaphoreInfoCount = waitSemaphoreInfo == NULL ? 0 : 1,
        .pWaitSemaphoreInfos = waitSemaphoreInfo,

        .signalSemaphoreInfoCount = signalSemaphoreInfo == NULL ? 0 : 1,
        .pSignalSemaphoreInfos = signalSemaphoreInfo,

        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,
    };
    return info;
}

void rc_draw(DrawParams params) {
    VkDevice device = params.device;
    VkSwapchainKHR swapchain = params.swapchain;
    FrameData frame = params.frame;
    VkQueue graphicsQueue = params.graphicsQueue;
    VkResult result = VK_SUCCESS;

    check(vkWaitForFences(device, 1, &frame.renderFence, true, 1000000000));
    check(vkResetFences(device, 1, &frame.renderFence));

    uint32_t swapchainImageIndex;
    result = vkAcquireNextImageKHR(device, swapchain, 1000000000, frame.swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    if (result != VK_SUCCESS) {
        printf("Non-success vkAcquireNextImageKHR result: %d, returning\n", result);
        return;
    }
    SwapchainImageData* image = &params.swapchainImages[swapchainImageIndex];

    VkCommandBuffer cmd = frame.mainCommandBuffer;
    check(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .pInheritanceInfo = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);

    // write to intermediate image
    rc_transition_image(cmd, params.drawImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // to clear the color
	// float flash = params.color;
    // VkClearColorValue clearValue = { { 0.0f, 0.0f, flash, 1.0f } };
    // VkImageSubresourceRange clearRange = rc_basic_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    // vkCmdClearColorImage(cmd, params.drawImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    // to use compute shader
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, params.gradientPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, params.gradientPipelineLayout, 0, 1, &params.drawImageDescriptorSet, 0, NULL);
    vkCmdDispatch(cmd, ceil(params.drawImageExtent.width / 16.0), ceil(params.drawImageExtent.height / 16.0), 1);

    rc_transition_image(cmd, params.drawImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // write to swapchain image
    rc_transition_image(cmd, image->swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    rc_copy_image_to_image(cmd, params.drawImage, image->swapchainImage, params.drawImageExtent, params.swapchainExtent);
    rc_transition_image(cmd, image->swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    check(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = command_buffer_submit_info(cmd);
    VkSemaphoreSubmitInfo waitInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.renderSemaphore);
    VkSubmitInfo2 submit = submit_info(&cmdInfo, &signalInfo, &waitInfo);
    check(vkQueueSubmit2(graphicsQueue, 1, &submit, frame.renderFence));

    // present
    // it puts the image we just rendered on the screen
    // we need to wait on the renderSemaphore for it to be done since drawing commands must be
    // complete before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .pSwapchains = &swapchain,
        .swapchainCount = 1,

        .pWaitSemaphores = &frame.renderSemaphore,
        .waitSemaphoreCount = 1,

        .pImageIndices = &swapchainImageIndex,
    };
    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        printf("Non-success VkQueuePresentKHR result: %d\n", result);
    }
}
