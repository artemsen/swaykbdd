# Rules for building with Meson

project(
  'swaykbdd',
  'c',
  default_options: [
    'c_std=c99',
    'warning_level=3',
  ],
  license: 'MIT',
  version: '1.3',
)

add_project_arguments(
  [
    '-DVERSION="' + meson.project_version() + '"',
    '-D_POSIX_C_SOURCE=200809'
  ],
  language: 'c',
)

install_man('swaykbdd.1')

executable(
  'swaykbdd',
  [
    'src/layouts.c',
    'src/main.c',
    'src/sway.c',
  ],
  dependencies: [
    dependency('json-c')
  ],
  install: true
)
