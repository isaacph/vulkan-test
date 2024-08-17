# Vulkan Test

Trying a Vulkan test with MSVC and Vulkan loader, no other runtime dependencies

### Backlog

* Finish utf8/utf16 converter
* Finish making Windows WSI have a platform-agnostic interface (using the utf8/utf16 converter once complete)
* Implement WSI for Wayland instead of using GLFW (I'm that masochistic)
* Continue implementing the Vulkan rendering pipeline

### Weird issues

* For some reason, if you build Vulkan-ValidationLayers with Visual Studio, but you try to build this project with Ninja, then the layer will show up with vkEnumerateInstanceLayerProperties, but it won't work for vkCreateInstance, which is a really frustrating thing to try to debug

### Build dependencies

* glslang
* Vulkan-Headers
* Vulkan-Loader
* Vulkan-ValidationLayers

### Runtime dependencies

* vulkan-1.dll

### Notes

https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html

