project('vulkan-test', 'c')
add_global_arguments('-std=c17', language: 'c')
add_global_arguments('-fsanitize=address', language: 'c')
cc = meson.get_compiler('c')

# asan_dep_1 = meson.get_compiler('c').find_library('libclang_rt.asan-x86_64.a')

cmake = import('cmake')

vulkan_loader_opts = cmake.subproject_options()
vulkan_loader_opts.add_cmake_defines({'UPDATE_DEPS': 'On'})
vulkan_loader_opts.set_override_option('A', 'x64')
vulkan_loader = cmake.subproject('Vulkan-Loader', options: vulkan_loader_opts)
vulkan = vulkan_loader.dependency('loader_common_options')

# volk_proj = subproject('volk')
# volk_dep = volk_proj.get_variable('volk_dep')
# volk_inc = volk_proj.get_variable('volk_inc')


dependencies = [
  dependency('vulkan'),
  volk_dep,
  ]

sources = [
  'src/backtrace.c',
  'src/main.c',
  'src/render.c',
  'src/render_util.c',
  ]

link = ['-fsanitize=address', '-shared-libasan']
if host_machine.system() == 'windows'
  link += ['-ldbghelp']
endif


exec = executable('demo',
  sources,
  dependencies: dependencies,
  link_args: link,
  include_directories: [volk_inc]
  )

# if host_machine.system() == 'windows'
#   custom_target('pdb file',
#             depends : exec,
#             input : exec,
#             output : 'demo.pdb',
#             command : ['cv2pdb', 'demo_gdb.exe', 'demo.exe', 'demo.pdb'],
#             build_by_default : true)
# endif
#
# script_name = 'debug.sh'
# custom_target('copy debug script',
#   input : script_name,
#   output :  script_name,
#   command : ['cp', '@INPUT@', '@OUTPUT@'],
#   install : false,
#   build_by_default : true)

