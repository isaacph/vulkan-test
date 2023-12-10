# Vulkan Test

Trying a Vulkan test with msys2's mingw-w64 target on Windows, using meson as a build system

### Backlog

* Consider modifying Volk to generate strings for error codes
* Maybe use protected memory and swapchain? Right now I have no idea what the point really is and I don't know if I should care and I can't find any posts online talking about it besides the documentation that just describes what it does, not why

### Weird issues

* For some reason, if you build Vulkan-ValidationLayers with Visual Studio, but you try to build this project with Ninja, then the layer will show up with vkEnumerateInstanceLayerProperties, but it won't work for vkCreateInstance, which is a really frustrating thing to try to debug

### Build dependencies

* glslang
* Vulkan-Headers
* Vulkan-Loader
* Vulkan-ValidationLayers

### Runtime dependencies

* vulkan-1.dll
