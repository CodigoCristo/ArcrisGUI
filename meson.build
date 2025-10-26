
project('Arcris', 'c',
  version : '0.1')

# Agregar flags de compilación                                                                        (10-26 01:16)
add_project_arguments('-Wunused-result', language : 'c')

subdir('data')
subdir('src')


# Variables para rutas comunes
datadir = get_option('prefix') / get_option('datadir')
icondir = datadir / 'icons/hicolor'
applicationsdir = datadir / 'applications'

# Scripts para actualizar cachés después de la instalación
# Actualizar caché de iconos (esencial para iconos simbólicos)
gtk_update_icon_cache = find_program('gtk-update-icon-cache', required: false)
if gtk_update_icon_cache.found()
  meson.add_install_script('sh', '-c',
    'gtk-update-icon-cache -f -t @0@ || true'.format(icondir)
  )
endif

# Actualizar base de datos de aplicaciones
update_desktop_database = find_program('update-desktop-database', required: false)
if update_desktop_database.found()
  meson.add_install_script('sh', '-c',
    'update-desktop-database @0@ || true'.format(applicationsdir)
  )
endif

# Actualizar caché de GSettings si existe esquema
glib_compile_schemas = find_program('glib-compile-schemas', required: false)
if glib_compile_schemas.found()
  meson.add_install_script('sh', '-c',
    'glib-compile-schemas @0@/glib-2.0/schemas || true'.format(datadir)
  )
endif

# Actualizar base de datos MIME
update_mime_database = find_program('update-mime-database', required: false)
if update_mime_database.found()
  meson.add_install_script('sh', '-c',
    'update-mime-database @0@/mime || true'.format(datadir)
  )
endif
