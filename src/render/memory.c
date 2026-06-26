#include <assert.h>
#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "../util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>

#define MEMORY_MANAGER_ENTRIES VK_MAX_MEMORY_TYPES

static const VkDeviceSize DEVICE_LOCAL_ALLOCATION_BLOCK_SIZE = 256 * 1024 * 1024;
static const VkDeviceSize DEFAULT_ALLOCATION_BLOCK_SIZE = 80 * 1024 * 1024;

// I see this as something we can build out little by little.
// 1. One allocation per memory type, with reasonable size for allocation. No partial deallocation.
// 2. Multiple allocations per memory type, with reasonable size for each allocation. No partial deallocation.
// 3. Multiple allocations. Simple linear search allocation + simple deallocation tracking.
// 4. Multiple allocations. At this point we need a more advanced strategy for choosing allocation position. Possibly can fragment based on
//    how frequently we need to reallocate some specific memory allocation. Longer-term, larger allocations can get a space, and shorter-term
//    small allocations can get another space which uses a basic allocation strategy. Possibly could design to over-deallocate so that
//    frequent allocation is as stream-lined as possible for these smaller or shorter-term allocations. Could implement tracking of performance
//    too for specific applications so that we can make informed decisions on more complex designs.

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

// requiredFlags is specifically required for the first or else we signal
// after that it's a list of preferences. requiredFlagsLevelsCount must be >= 1
Allocation rc_allocate_memory(VkPhysicalDevice physicalDevice, VkDevice device, Allocations* allocations, VkMemoryRequirements requirements,
        uint32_t* requiredFlagsLevels, int requiredFlagsLevelsCount) {
    printf("allocate req: %lld\n", requirements.size);
    VkDeviceSize size = requirements.size;
    {
        VkDeviceSize dividedSize = (requirements.size / allocations->deviceLimits.nonCoherentAtomSize);
        VkDeviceSize multipliedSize = dividedSize * allocations->deviceLimits.nonCoherentAtomSize;
        if (multipliedSize != size) {
            size = (dividedSize + 1) * allocations->deviceLimits.nonCoherentAtomSize;
            printf("rounded allocation size up to: %lld (because nonCoherentAtomSize = %lld)\n", size, allocations->deviceLimits.nonCoherentAtomSize);
        }
    }
    assert(requiredFlagsLevelsCount >= 1);
    // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    // then allocate VkDeviceMemory from that memory type
    uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    uint32_t chosenMemoryTypeIndexFlags = 0;
    VkPhysicalDeviceMemoryProperties properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    uint32_t memoryOptions[32] = { 0 };
    uint32_t memoryOptionsFlags[32] = { 0 };
    int index = 0;
    // printf("selecting for memory with required flags: %x\n", requiredFlagsLevels[0]);
    for (int i = 0; i < properties.memoryTypeCount; ++i) {
        if ((requirements.memoryTypeBits & (1 << i)) != 0) {
            uint32_t flags = properties.memoryTypes[i].propertyFlags;
            if ((requiredFlagsLevels[0] & flags) != 0) {
                uint32_t heapIndex = properties.memoryTypes[i].heapIndex;
                VkDeviceSize heapSize = properties.memoryHeaps[heapIndex].size; 
                bool deviceLocal = (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0;
                if (deviceLocal && heapSize >= DEVICE_LOCAL_ALLOCATION_BLOCK_SIZE ||
                    !deviceLocal && heapSize >= DEFAULT_ALLOCATION_BLOCK_SIZE) {
                    memoryOptionsFlags[index] = flags;
                    memoryOptions[index] = i;
                    ++index;
                    printf("option type #%d heapIndex: %u, flags: %x\n", i,
                            properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
                    break;
                }
            }
        }
    }
    assert(index > 0); // assert we found at least one possible memory to map
    int bestLevel = 0;
    int bestLevelMemoryTypeIndex = memoryOptions[0]; // if we don't meet any other levels we choose the first
    int bestLevelMemoryTypeIndexFlags = memoryOptionsFlags[0];
    int i = 0;
    for (; i < index; ++i) {
        int aggregatedLevel = 0;
        for (int level = 1; level < requiredFlagsLevelsCount; ++level) {
            // printf("level %d %x\n", level, requiredFlagsLevels[level]);
            if ((memoryOptionsFlags[i] & requiredFlagsLevels[level]) != 0) {
                // printf("#%d=%x added level %d for having flags %x\n",
                //         memoryOptionsFlags[i], memoryOptions[i], level, requiredFlagsLevels[level]);
                aggregatedLevel |= (1 << (requiredFlagsLevelsCount - (level - 1)));
                if (aggregatedLevel < bestLevel) {
                    // printf("#%d, agg dropped: %x, best: %x\n", memoryOptions[i], aggregatedLevel, bestLevel);
                    // since the lower levels are more important, we can already break
                    break;
                }
            } else {
                // printf("#%d=%x did not add level %d for not having flags %x\n",
                //         memoryOptionsFlags[i], memoryOptions[i], level, requiredFlagsLevels[level]);
            }
        }
        // printf("#%d, agg: %x, best: %x\n", memoryOptions[i], aggregatedLevel, bestLevel);
        if (aggregatedLevel > bestLevel) {
            // printf("#%d overrides best\n", memoryOptions[i]);
            bestLevel = aggregatedLevel;
            bestLevelMemoryTypeIndex = memoryOptions[i];
            bestLevelMemoryTypeIndexFlags = memoryOptionsFlags[i];
        }
    }
    chosenMemoryTypeIndex = bestLevelMemoryTypeIndex;
    chosenMemoryTypeIndexFlags = bestLevelMemoryTypeIndexFlags;
    printf("final chosen type #%d flags %x\n", chosenMemoryTypeIndex, chosenMemoryTypeIndexFlags);
    bool chosenMemoryTypeIndexDeviceLocal = (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & chosenMemoryTypeIndexFlags) != 0;
    assert(chosenMemoryTypeIndex != UINT32_MAX);
    assert(chosenMemoryTypeIndexFlags != 0); // 0-flags means the memory would do nothing - sanity test

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize address = 0;
    VkDeviceSize allocationSize = chosenMemoryTypeIndexDeviceLocal
        ? DEVICE_LOCAL_ALLOCATION_BLOCK_SIZE
        : DEFAULT_ALLOCATION_BLOCK_SIZE;

    // for now there is one allocation per memory type so just allocate it or look in it or give up
    if (allocations->allocation[chosenMemoryTypeIndex] == VK_NULL_HANDLE) {
        VkMemoryAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = allocationSize,
            .memoryTypeIndex = chosenMemoryTypeIndex,
        };
        check(vkAllocateMemory(device, &allocateInfo, NULL, &memory));
        allocations->allocation[chosenMemoryTypeIndex] = memory;
        allocations->allocationOffset[chosenMemoryTypeIndex] = size;
        allocations->toDeallocate[chosenMemoryTypeIndex] = memory; // sets it up for cleanup
    } else {
        // find start of next aligned block
        VkDeviceSize nextAlignedPosition =
            (allocations->allocationOffset[chosenMemoryTypeIndex] / requirements.alignment + 1) * requirements.alignment;
        if (nextAlignedPosition + size >= allocationSize) {
            exception_msg("Failed to allocate memory: currently we only have one "
                    "allocation per memory type and that allocation is out of memory");
        }
        allocations->allocationOffset[chosenMemoryTypeIndex] = nextAlignedPosition + size;
        memory = allocations->allocation[chosenMemoryTypeIndex];
        address = nextAlignedPosition;
    }

    return (Allocation) {
        .memory = memory,
        .address = address,
        .memoryTypeIndex = chosenMemoryTypeIndex,
        .size = size,
        .flags = chosenMemoryTypeIndexFlags,
    };
}

void rc_deallocate_temp_memory(Allocations* allocations, Allocation allocation) {
    // this allocation must be the most recent allocation:
    assert(allocations->allocationOffset[allocation.memoryTypeIndex] - allocation.size == allocation.address);

    // remove this allocation
    allocations->allocationOffset[allocation.memoryTypeIndex] = allocation.address;
}
