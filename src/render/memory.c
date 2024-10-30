#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "../util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>

#define MEMORY_MANAGER_ENTRIES VK_MAX_MEMORY_TYPES


// this is why I don't like writing a memory manager.
// once you are doing this, you start trying to solve every case, instead of doing optimal solutions for your
// specific case
// I feel like it all stems from this getAllocationForImage idea
// that API surface is so general it has to support a ton of different cases as best as it can
// why can't I narrow things down more here?
//
//
// new requirement: either work directly on this or don't think about it. too much mental energy on nothing

#define MM_ALLOCATION_SIZE (256 * 1024 * 1024)

typedef struct Allocation {
    VkDeviceMemory allocation;
    VkDeviceSize offset;
} AllocatedInfo;
typedef struct MemoryManagerEntry {
    VkDeviceMemory allocation;
    VkDeviceSize currentOffset;
} MemoryManagerEntry;
typedef struct MemoryManager {
    MemoryManagerEntry entries[MEMORY_MANAGER_ENTRIES];
    VkPhysicalDevice physicalDevice;
    VkDevice device; // memory manager is device-specific so might as well include it here
    VkPhysicalDeviceMemoryProperties properties;
} MemoryManager; 

MemoryManager rc_mm_init(VkPhysicalDevice physicalDevice, VkDevice device) {
    MemoryManager mm = {
        .physicalDevice = physicalDevice,
        .device = device,
    };
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mm.properties);
    for (uint32_t i = 0; i < MEMORY_MANAGER_ENTRIES; ++i) {
        mm.entries[i] = (MemoryManagerEntry) {
            .allocation = VK_NULL_HANDLE,
            .currentOffset = 0,
        };
    }
    return mm;
}

// gets a memory allocation for an image
AllocatedInfo rc_mm_getAllocationForImage(MemoryManager* mm, VkImage image) {
    VkMemoryRequirements imageMemoryRequirements = { 0 };
    vkGetImageMemoryRequirements(mm->device, image, &imageMemoryRequirements);

    // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    // then allocate VkDeviceMemory from that memory type
    VkDeviceSize requiredSize = 256 * 1024 * 1024;
    uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    for (int i = 0; i < mm->properties.memoryTypeCount; ++i) {
        if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) != 0) {
            uint32_t flags = mm->properties.memoryTypes[i].propertyFlags;
            if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
                uint32_t heapIndex = mm->properties.memoryTypes[i].heapIndex;
                VkDeviceSize heapSize = mm->properties.memoryHeaps[heapIndex].size; 
                if (heapSize >= requiredSize) {
                    chosenMemoryTypeIndex = i;
                    printf("chosen type #%d heapIndex: %u, flags: %x\n", i,
                            mm->properties.memoryTypes[i].heapIndex, mm->properties.memoryTypes[i].propertyFlags);
                    break;
                }
            }
        }
    }
    assert(chosenMemoryTypeIndex != UINT32_MAX);
    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = requiredSize,
        .memoryTypeIndex = chosenMemoryTypeIndex,
    };
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    check(vkAllocateMemory(mm->device, &allocateInfo, NULL, &deviceMemory));

    return (AllocatedInfo) {
        .allocation = deviceMemory,
        .offset = 0,
    };
}

// typedef struct MemoryAllocator {
// 
// } MemoryAllocator;
// 
// MemoryAllocator rcma_init() {
//     return (MemoryAllocator) {
//     };
// }
// 
// void rcma_allocate(MemoryAllocator* rcma) {
// }
