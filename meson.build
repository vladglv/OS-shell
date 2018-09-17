project('os-shell', 'cpp', default_options : ['cpp_std=c++14'], version : '0.2')

compiler = meson.get_compiler('cpp').get_id()

cc_args = []

if compiler == 'clang' or compiler == 'gcc'
  cc_args += [
    '-Wmain',
    '-W',
    '-Wall',
    '-Wextra',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-pedantic',
    '-pedantic-errors',
    '-fstack-protector-strong'
    ]
else
  warning('cpp_args is only configured for clang and gcc')
endif

cc_args_exe = cc_args + ['-fPIC']

message('buildtype=' + get_option('buildtype'))

# TODO Add Clang specific options for some builds
# -stdlib=libc++
# -Weverything -Wthread-safety
# -fsanitize=address,undefined,leak

inc_dir = include_directories('include')
lib = static_library('shell-lib', 'src/shell/shell.cpp', include_directories : inc_dir, cpp_args : cc_args)
executable('shell', 'src/shell/shell-main.cpp', include_directories : inc_dir, link_with : lib, link_args: ['-static-libstdc++'], cpp_args : cc_args_exe)

