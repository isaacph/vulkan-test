#ifndef RENDER_UTIL_H_INCLUDED
#define RENDER_UTIL_H_INCLUDED

#include "../util.h"
#include <stdint.h>
#include "functions.h"
#include <stdbool.h>

// returns true if the string is valid unicode
bool validate_unicode(const char* unicode);

void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len);
void print_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions);
void print_VkLayerProperties(uint32_t count, VkLayerProperties* layers);
void check(VkResult res);

// void interpret_VK_API_VERSION(uint32_t version, char* out_memory, uint64_t memory_length);
// void interpret_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions, char* out_memory, uint64_t memory_length);
// void interpret_VkLayerProperties(uint32_t count, VkLayerProperties* layers, char* out_memory, uint64_t memory_length);


#endif
