
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

# Instalar iconos simbólicos (disponibles para todo el sistema)
symbolic_icons = [
  'org.gtk.arcris-symbolic.svg',
  'org.gtk.arcris-clean-symbolic.svg',
  'music-note-symbolic.svg',
  'applications-graphics-symbolic.svg',
  'applications-multimedia-symbolic.svg',
  'web-browser-symbolic.svg',
  'code-symbolic.svg',
  'computer-symbolic.svg',
  'harddisk-inverted-symbolic.svg',
  'mail-unread-symbolic.svg',
  'rich-text-symbolic.svg',
  'org.gnome.DiskUtility-symbolic.svg',
  'keyboard-symbolic.svg',
  'month-symbolic.svg',
  'view-refresh-symbolic.svg',
  'go-next-symbolic.svg',
  'go-previous-symbolic.svg',
  'dialog-password-symbolic.svg',
  'list-add-symbolic.svg',
  'drive-removable-media-symbolic.svg',
  'drive-harddisk-symbolic.svg',
  'document-edit-symbolic.svg',
  'tab-new-symbolic.svg',
  'adw-external-link-symbolic.svg',
  'open-menu-symbolic.svg',
  'xbox-controller-symbolic.svg',
  'check-plain-symbolic.svg'
]

foreach icon : symbolic_icons
  install_data('icons/hicolor/symbolic/apps/' + icon,
    install_dir: get_option('datadir') / 'icons/hicolor/symbolic/apps'
  )
endforeach

# Instalar iconos escalables adicionales
install_data('icons/hicolor/scalable/apps/org.gtk.arcris-clean.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps'
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
