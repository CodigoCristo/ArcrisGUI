
project('Arcris', 'c',
  version : '0.1')

subdir('data')
subdir('src')



# Scripts para actualizar cachés después de la instalación
if find_program('gtk-update-icon-cache', required: false).found()
  meson.add_install_script('sh', '-c',
    'gtk-update-icon-cache -f -t @0@'.format(
      get_option('prefix') / get_option('datadir') / 'icons/hicolor'
    )
  )
endif

if find_program('update-desktop-database', required: false).found()
  meson.add_install_script('sh', '-c',
    'update-desktop-database @0@'.format(
      get_option('prefix') / get_option('datadir') / 'applications'
    )
  )
endif
