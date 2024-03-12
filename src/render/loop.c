#include <stdbool.h>
#include "context.h"
#include "render_functions.h"
#include <math.h>
#include <vulkan/vulkan_core.h>

void rc_init_loop(RenderContext* context) {
    if (context->device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating synchronization primitives\n");
    }
    VkDevice device = context->device;

    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        VkFence renderFence = VK_NULL_HANDLE;
        VkSemaphore swapchainSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderSemaphore = VK_NULL_HANDLE;

        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        check(vkCreateFence(device, &fenceCreateInfo, context->allocationCallbacks, &renderFence));

        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
        };
        check(vkCreateSemaphore(device, &semaphoreCreateInfo, context->allocationCallbacks, &swapchainSemaphore));
        check(vkCreateSemaphore(device, &semaphoreCreateInfo, context->allocationCallbacks, &renderSemaphore));

        context->frames[i].renderFence = renderFence;
        context->frames[i].swapchainSemaphore = swapchainSemaphore;
        context->frames[i].renderSemaphore = renderSemaphore;
    }
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

void rc_draw(RenderContext* context) {
    VkDevice device = context->device;
    VkSwapchainKHR swapchain = context->swapchain;
    FrameData* frame = &context->frames[context->frameNumber % RC_SWAPCHAIN_LENGTH];
    uint64_t frameNumber = context->frameNumber;
    VkQueue graphicsQueue = context->graphicsQueue;

    check(vkWaitForFences(device, 1, &frame->renderFence, true, 1000000000));
    check(vkResetFences(device, 1, &frame->renderFence));

    uint32_t swapchainImageIndex;
    check(vkAcquireNextImageKHR(device, swapchain, 1000000000, frame->swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex));
    SwapchainImageData* image = &context->images[swapchainImageIndex];

    VkCommandBuffer cmd = frame->mainCommandBuffer;
    check(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .pInheritanceInfo = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    check(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    rc_transition_image(cmd, image->swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    // make a clear-color from frame number. This will flash with a 120 frame period.
	float flash = fabs(sin(frameNumber / 120.f));
    VkClearColorValue clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = basic_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, image->swapchainImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    rc_transition_image(cmd, image->swapchainImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    check(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = command_buffer_submit_info(cmd);
    VkSemaphoreSubmitInfo waitInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame->swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame->renderSemaphore);
    VkSubmitInfo2 submit = submit_info(&cmdInfo, &signalInfo, &waitInfo);
    check(vkQueueSubmit2(graphicsQueue, 1, &submit, frame->renderFence));

    // present
    // it puts the image we just rendered on the screen
    // we need to wait on the renderSemaphore for it to be done since drawing commands must be
    // complete before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .pSwapchains = &swapchain,
        .swapchainCount = 1,

        .pWaitSemaphores = &frame->renderSemaphore,
        .waitSemaphoreCount = 1,

        .pImageIndices = &swapchainImageIndex,
    };
    check(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    context->frameNumber++;
}
