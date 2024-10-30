# Vulkan Test

Trying a Vulkan test with MSVC and Vulkan loader, no other runtime dependencies

### Backlog

* Continue implementing the Vulkan rendering pipeline
* Finish vkguide.dev-equivalent implementation
  * For this, I plan to rewrite a vulkan memory allocator. I figured out I really do need basically the whole thing because ...
* Eventually make tests/ work in some sensible way. Right now just ignoring that exists anymore.

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

