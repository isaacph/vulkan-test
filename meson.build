project('vulkan-test', 'c')
add_global_arguments('-std=c17', language: 'c')
add_global_arguments('-fsanitize=address', language: 'c')

# asan_dep_1 = meson.get_compiler('c').find_library('libclang_rt.asan-x86_64.a')

dependencies = [
  dependency('glfw3'),
  dependency('vulkan'),
  dependency('libbacktrace'),
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

exec = executable('demo', sources, dependencies: dependencies, link_args: link)

if host_machine.system() == 'windows'
  custom_target('pdb file',
            depends : exec,
            input : exec,
            output : 'demo.pdb',
            command : ['cv2pdb', 'demo.exe', 'demo_pdb.exe', 'demo.pdb'],
            build_by_default : true)
endif

script_name = 'debug.sh'
custom_target('copy debug script',
  input : script_name,
  output :  script_name,
  command : ['cp', '@INPUT@', '@OUTPUT@'],
  install : false,
  build_by_default : true)

