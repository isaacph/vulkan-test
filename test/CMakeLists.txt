find_package(unity CONFIG)

add_executable(util_memory_c util/memory.c)
target_link_libraries(util_memory_c Main unity::framework)
