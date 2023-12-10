#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED
#include <stdbool.h>
#include "render_functions.h"

bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

#endif
