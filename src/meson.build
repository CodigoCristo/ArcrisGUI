
gtkdep = dependency('gtk4')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
adwaita = dependency('libadwaita-1', version: '>= 1.5')
libsoup = dependency('libsoup-3.0', required: true)
udisks2 = dependency('udisks2', required: true)


# Incluir `arcris_resources_dep` directamente desde el ámbito global
executable('arcris',
  [
    'main.c',
    'window.c',
    'carousel.c',
    'internet.c',
    'startbutton.c',
    'close.c',
    'about.c',
    'page1.c',
    'page2.c',
    'page3.c',
    'page4.c',
    'page5.c',
    'page6.c',
    'partitionmanual.c',


    'config.c',
    'disk_manager.c',
    'partition_manager.c'
  ],
  dependencies : [gtkdep, glib_dep, gio_dep, adwaita, libsoup, udisks2, arcris_resources_dep],
  install : true,
  install_dir : get_option('bindir')
)
