
i18n = import('i18n')
gnome = import('gnome')

data_files = gnome.compile_resources(
  'arcris_resources',
  'gresource.xml',
  source_dir: '.'
)

# Declarar una dependencia para que otros subdirectorios puedan usar `data_files`
arcris_resources_dep = declare_dependency(sources: data_files)

# Instalar el icono de la aplicación
install_data('img/org.gtk.arcris.png',
  install_dir: get_option('datadir') / 'icons/hicolor/256x256/apps'
)

# Instalar iconos SVG
install_data('icons/hicolor/scalable/apps/org.gtk.arcris.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
)

install_data('icons/hicolor/symbolic/apps/org.gtk.arcris-symbolic.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/symbolic/apps'
)

# Instalar iconos limpios adicionales
install_data('icons/hicolor/scalable/apps/org.gtk.arcris-clean.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
)

install_data('icons/hicolor/symbolic/apps/org.gtk.arcris-clean-symbolic.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/symbolic/apps'
)

# Instalar iconos de diferentes tamaños
install_data('icons/hicolor/16x16/apps/org.gtk.arcris.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/16x16/apps'
)

install_data('icons/hicolor/32x32/apps/org.gtk.arcris.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/32x32/apps'
)


# Instalar archivo .desktop
install_data(
  'org.gtk.arcris.desktop',
  install_dir: get_option('datadir') / 'applications'
)
