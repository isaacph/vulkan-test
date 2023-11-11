# Vulkan Test

Trying a Vulkan test with msys2's mingw-w64 target on Windows, using meson as a build system

### Backlog

* Consider modifying Volk to generate strings for error codes

### Weird issues

* For some reason, if you build Vulkan-ValidationLayers with Visual Studio, but you try to build this project with Ninja, then the layer will show up with vkEnumerateInstanceLayerProperties, but it won't work for vkCreateInstance, which is a really frustrating thing to try to debug
