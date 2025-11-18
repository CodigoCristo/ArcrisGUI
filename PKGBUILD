# Maintainer: Cristo

pkgname=arcrisgui
pkgver=0.1
pkgrel=1
pkgdesc="Instalador Gráfico para instalar fácil ArchLinux - Modern GTK4 installer"
arch=('x86_64')
license=('GPL3')
depends=('gtk4' 'libadwaita' 'vte4' 'glib2' 'curl' 'wget' 'udisks2')
makedepends=('meson' 'ninja' 'git')
provides=("${pkgname}")
conflicts=("${pkgname}")


build() {
  cd "${srcdir}/../"
  meson setup build --buildtype=release --prefix=/usr
  ninja -C build
}

package() {

  cd "${srcdir}/../"
  DESTDIR="${pkgdir}" meson install -C build

  # Create proper directory structure
  install -dm755 "${pkgdir}/usr/lib/${pkgname}"
  install -dm755 "${pkgdir}/usr/share/${pkgname}/data"
  install -dm755 "${pkgdir}/usr/share/${pkgname}/data/config"
  install -dm755 "${pkgdir}/usr/bin"

  # Move binary from /usr/bin to /usr/lib/arcrisgui/
  if [ -f "${pkgdir}/usr/bin/arcris" ]; then
    mv "${pkgdir}/usr/bin/arcris" "${pkgdir}/usr/lib/${pkgname}/"
  fi

  # Install data files that meson doesn't install
  install -m644 data/variables.sh "${pkgdir}/usr/share/${pkgname}/data/"
  install -m644 data/locale.gen "${pkgdir}/usr/share/${pkgname}/data/"
  install -m644 data/reserved_usernames "${pkgdir}/usr/share/${pkgname}/data/"
  install -m755 data/install.sh "${pkgdir}/usr/share/${pkgname}/data/"
  
  # Install config files
  install -m644 data/config/bashrc "${pkgdir}/usr/share/${pkgname}/data/config/"
  install -m644 data/config/bashrc-root "${pkgdir}/usr/share/${pkgname}/data/config/"
  install -m644 data/config/pacman.conf "${pkgdir}/usr/share/${pkgname}/data/config/"
  install -m644 data/config/pacman-chroot.conf "${pkgdir}/usr/share/${pkgname}/data/config/"
  install -m644 data/config/zshrc "${pkgdir}/usr/share/${pkgname}/data/config/"
  install -m644 data/config/sudoers "${pkgdir}/usr/share/${pkgname}/data/config/"

  # Add fallback with full icon path for better compatibility
  # (.desktop file is automatically installed by Meson to /usr/share/applications/)
  sed -i 's|Icon=org.gtk.arcris|Icon=/usr/share/icons/hicolor/scalable/apps/org.gtk.arcris.svg|' "${pkgdir}/usr/share/applications/org.gtk.arcris.desktop"

  # Icons and desktop files are already installed in the correct location with --prefix=/usr

  # Create wrapper script that uses user-writable directory
  cat > "${pkgdir}/usr/bin/arcris" << 'EOF'
#!/bin/bash

# Create user data directory
USER_DATA_DIR="$HOME/.local/share/arcrisgui"
SYSTEM_DATA_DIR="/usr/share/arcrisgui"

mkdir -p "$USER_DATA_DIR/data/config"

# Copy system data files to user directory if they don't exist
for file in variables.sh locale.gen reserved_usernames install.sh; do
    if [ ! -f "$USER_DATA_DIR/data/$file" ] && [ -f "$SYSTEM_DATA_DIR/data/$file" ]; then
        cp "$SYSTEM_DATA_DIR/data/$file" "$USER_DATA_DIR/data/$file"
    fi
done

# Copy config files to user directory if they don't exist
for file in bashrc bashrc-root pacman.conf pacman-chroot.conf zshrc sudoers; do
    if [ ! -f "$USER_DATA_DIR/data/config/$file" ] && [ -f "$SYSTEM_DATA_DIR/data/config/$file" ]; then
        cp "$SYSTEM_DATA_DIR/data/config/$file" "$USER_DATA_DIR/data/config/$file"
    fi
done

# Change to user directory and execute
cd "$USER_DATA_DIR"
exec /usr/lib/arcrisgui/arcris "$@"
EOF
  chmod 755 "${pkgdir}/usr/bin/arcris"
}

# Post-install hooks
post_install() {
  echo "==> Arcris GUI Installer has been installed"
  echo "==> You can launch it from the applications menu or run 'arcris' in terminal"
  echo "==> Configuration files will be created in ~/.local/share/arcrisgui/ on first run"

  # Update icon cache
  if command -v gtk-update-icon-cache &> /dev/null; then
    echo "==> Updating icon cache..."
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
  fi

  # Update desktop database
  if command -v update-desktop-database &> /dev/null; then
    echo "==> Updating desktop database..."
    update-desktop-database -q /usr/share/applications
  fi
}

post_upgrade() {
  post_install
}

post_remove() {
  # Update icon cache
  if command -v gtk-update-icon-cache &> /dev/null; then
    echo "==> Updating icon cache..."
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
  fi

  # Update desktop database
  if command -v update-desktop-database &> /dev/null; then
    echo "==> Updating desktop database..."
    update-desktop-database -q /usr/share/applications
  fi
}
