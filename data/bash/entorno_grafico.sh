# Instalación de entorno de escritorio/gestor de ventanas
echo -e "${GREEN}| Configurando entorno gráfico: $INSTALLATION_TYPE |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$INSTALLATION_TYPE" in
    "TERMINAL")
        echo -e "${CYAN}Instalación solo terminal - No se instalará entorno gráfico${NC}"
        ;;
    "DESKTOP")
        echo -e "${GREEN}Instalando entorno de escritorio: $DESKTOP_ENVIRONMENT${NC}"

        # Instalar X.org como base para todos los escritorios
        echo -e "${CYAN}Instalando servidor X.org...${NC}"
        install_pacman_chroot_with_retry "xorg-server"
        install_pacman_chroot_with_retry "xorg-server-common"
        install_pacman_chroot_with_retry "xorg-xinit"
        install_pacman_chroot_with_retry "xorg-xauth"
        install_pacman_chroot_with_retry "xorg-xsetroot"
        install_pacman_chroot_with_retry "xorg-xrandr"
        install_pacman_chroot_with_retry "xorg-setxkbmap"
        install_pacman_chroot_with_retry "xorg-xrdb"
        install_pacman_chroot_with_retry "xorg-apps"
        install_pacman_chroot_with_retry "xterm"
        install_pacman_chroot_with_retry "wayland"            # Protocolo Wayland
        install_pacman_chroot_with_retry "xorg-xwayland"      # Compatibilidad con apps X11
        install_pacman_chroot_with_retry "ffmpegthumbs"
        install_pacman_chroot_with_retry "ffmpegthumbnailer"
        install_pacman_chroot_with_retry "poppler"
        install_pacman_chroot_with_retry "mediainfo"
        install_pacman_chroot_with_retry "freetype2"
        install_pacman_chroot_with_retry "libgsf"
        install_pacman_chroot_with_retry "libnotify"
        install_pacman_chroot_with_retry "tumbler"
        install_pacman_chroot_with_retry "gdk-pixbuf2"
        install_pacman_chroot_with_retry "fontconfig"
        install_pacman_chroot_with_retry "gvfs"

        case "$DESKTOP_ENVIRONMENT" in
            "GNOME")
                echo -e "${CYAN}Instalando GNOME Desktop...${NC}"
                install_pacman_chroot_with_retry "gdm"
                install_pacman_chroot_with_retry "gnome-session"
                install_pacman_chroot_with_retry "gnome-settings-daemon"
                install_pacman_chroot_with_retry "gnome-shell"
                install_pacman_chroot_with_retry "gnome-control-center"
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "nautilus"
                install_pacman_chroot_with_retry "gvfs"
                install_pacman_chroot_with_retry "gvfs-goa"
                install_pacman_chroot_with_retry "gnome-console"
                install_pacman_chroot_with_retry "gnome-text-editor"
                install_pacman_chroot_with_retry "gnome-calculator"
                install_pacman_chroot_with_retry "gnome-system-monitor"
                install_pacman_chroot_with_retry "gnome-disk-utility"
                install_pacman_chroot_with_retry "baobab"
                install_pacman_chroot_with_retry "dconf-editor"
                install_pacman_chroot_with_retry "gnome-themes-extra"
                install_pacman_chroot_with_retry "gnome-tweaks"
                install_pacman_chroot_with_retry "gnome-backgrounds"
                install_pacman_chroot_with_retry "gnome-user-docs"
                install_pacman_chroot_with_retry "gnome-software"
                install_pacman_chroot_with_retry "xdg-desktop-portal-gnome"
                install_pacman_chroot_with_retry "gnome-shell-extensions"
                install_pacman_chroot_with_retry "gnome-browser-connector"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "sushi"
                echo "Installing extension-manager..."
                install_pacman_chroot_with_retry "extension-manager"
                chroot /mnt /bin/bash -c "systemctl enable gdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

                ;;
            "BUDGIE")
                echo -e "${CYAN}Instalando Budgie Desktop...${NC}"
                install_pacman_chroot_with_retry "budgie-desktop"
                install_pacman_chroot_with_retry "budgie-extras"
                install_pacman_chroot_with_retry "budgie-desktop-view"
                install_pacman_chroot_with_retry "budgie-backgrounds"
                install_pacman_chroot_with_retry "network-manager-applet"
                install_pacman_chroot_with_retry "materia-gtk-theme"
                install_pacman_chroot_with_retry "papirus-icon-theme"
                install_pacman_chroot_with_retry "nautilus"
                install_pacman_chroot_with_retry "gvfs-goa"
                install_pacman_chroot_with_retry "gnome-console"
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-slick-greeter"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "CINNAMON")
                echo -e "${CYAN}Instalando Cinnamon Desktop...${NC}"
                install_pacman_chroot_with_retry "cinnamon"
                install_pacman_chroot_with_retry "cinnamon-translations"
                install_pacman_chroot_with_retry "engrampa"
                install_pacman_chroot_with_retry "gvfs-smb"
                install_yay_chroot_with_retry "bibata-cursor-theme"
                install_pacman_chroot_with_retry "hicolor-icon-theme"
                install_yay_chroot_with_retry "mint-backgrounds"
                install_yay_chroot_with_retry "mint-themes"
                install_yay_chroot_with_retry "mint-x-icons"
                install_yay_chroot_with_retry "mint-y-icons"
                install_yay_chroot_with_retry "mintlocale"
                install_pacman_chroot_with_retry "cinnamon-control-center"
                install_pacman_chroot_with_retry "xed"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "gnome-console"
                install_pacman_chroot_with_retry "gnome-screenshot"
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-slick-greeter"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "COSMIC")
                echo -e "${CYAN}Instalando COSMIC Desktop...${NC}"
                install_pacman_chroot_with_retry "cosmic"
                install_pacman_chroot_with_retry "power-profiles-daemon"
                install_pacman_chroot_with_retry "cosmic-icon-theme"
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "cosmic-greeter"
                chroot /mnt /bin/bash -c "systemctl enable cosmic-greeter.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "CUTEFISH")
                echo -e "${CYAN}Instalando CUTEFISH Desktop...${NC}"
                install_pacman_chroot_with_retry "cutefish"
                install_pacman_chroot_with_retry "polkit-kde-agent"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "sddm"
                install_pacman_chroot_with_retry "sddm-kcm"
                chroot /mnt /bin/bash -c "systemctl enable sddm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "UKUI")
                echo -e "${CYAN}Instalando UKUI Desktop...${NC}"
                install_pacman_chroot_with_retry "ukui"
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-slick-greeter"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "PANTHEON")
                echo -e "${CYAN}Instalando PANTHEON Desktop...${NC}"
                install_pacman_chroot_with_retry "pantheon"
                install_pacman_chroot_with_retry "udisks2"               # Montaje automático de discos
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "gnome-console"
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-pantheon-greeter"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                chroot /mnt /bin/bash -c "pacman -Q orca >/dev/null 2>&1 && pacman -Rdd orca --noconfirm; exit 0"
                chroot /mnt /bin/bash -c "pacman -Q onboard >/dev/null 2>&1 && pacman -Rdd onboard --noconfirm; exit 0"
                sed -i '$d' /mnt/etc/lightdm/Xsession
                sed -i '$a io.elementary.wingpanel &\nplank &\nexec gala' /mnt/etc/lightdm/Xsession
                ;;
            "ENLIGHTENMENT")
                echo -e "${CYAN}Instalando Enlightenment Desktop...${NC}"
                install_pacman_chroot_with_retry "enlightenment"
                install_pacman_chroot_with_retry "terminology"
                install_pacman_chroot_with_retry "evisum"
                install_pacman_chroot_with_retry "network-manager-applet"  # Si prefieres NetworkManager
                install_pacman_chroot_with_retry "udisks2"               # Montaje automático de discos
                install_pacman_chroot_with_retry "lightdm"           # Display Manager
                install_pacman_chroot_with_retry "lightdm-slick-greeter"  # Greeter moderno
                install_pacman_chroot_with_retry "lightdm-gtk-greeter-settings"  # Configurar greeter
                install_pacman_chroot_with_retry "ephoto"            # Visor de imágenes EFL
                install_pacman_chroot_with_retry "rage"              # Reproductor de video EFL (opcional)
                install_pacman_chroot_with_retry "polkit-gnome"      # Autenticación gráfica
                install_pacman_chroot_with_retry "gnome-keyring"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "KDE")
                echo -e "${CYAN}Instalando KDE Plasma Desktop...${NC}"
                # Base Xorg/Wayland
                install_pacman_chroot_with_retry "xorg-server"           # Para sesión X11
                install_pacman_chroot_with_retry "wayland"               # Para sesión Wayland

                # Plasma Core Mínimo
                install_pacman_chroot_with_retry "plasma-desktop"   # Escritorio base
                install_pacman_chroot_with_retry "plasma-workspace" # Workspace esencial
                install_pacman_chroot_with_retry "kwin"            # Compositor (Wayland + X11)

                # Configuración y sistema
                install_pacman_chroot_with_retry "systemsettings"        # Configuración del sistema
                install_pacman_chroot_with_retry "kinfocenter"          # Información del sistema
                install_pacman_chroot_with_retry "kscreen"              # Gestión de pantallas

                # Display Manager
                install_pacman_chroot_with_retry "sddm"                  # Display Manager
                install_pacman_chroot_with_retry "sddm-kcm"             # Configurar SDDM desde Plasma

                # Hardware y Red (Esencial)
                install_pacman_chroot_with_retry "plasma-nm"       # NetworkManager
                install_pacman_chroot_with_retry "powerdevil"      # Gestión de energía
                install_pacman_chroot_with_retry "plasma-pa"       # Control de audio
                install_pacman_chroot_with_retry "bluedevil"        # Bluetooth

                # Autenticación y seguridad
                install_pacman_chroot_with_retry "polkit-kde-agent"      # Autenticación
                install_pacman_chroot_with_retry "kwallet"               # Gestor de contraseñas
                install_pacman_chroot_with_retry "kwalletmanager"        # GUI para kwallet

                # Portales XDG
                install_pacman_chroot_with_retry "xdg-desktop-portal-kde"  # Portal KDE
                install_pacman_chroot_with_retry "xdg-desktop-portal"    # Base de portales

                # Tema y apariencia
                install_pacman_chroot_with_retry "breeze"                # Tema Plasma
                install_pacman_chroot_with_retry "breeze-gtk"            # Tema GTK
                install_pacman_chroot_with_retry "kde-gtk-config"        # Configurar apps GTK
                install_pacman_chroot_with_retry "kdeplasma-addons"      # Widgets adicionales

                # Aplicaciones KDE básicas
                install_pacman_chroot_with_retry "konsole"               # Terminal
                install_pacman_chroot_with_retry "dolphin"               # Gestor de archivos
                install_pacman_chroot_with_retry "kate"                  # Editor de texto
                install_pacman_chroot_with_retry "spectacle"             # Capturas de pantalla
                install_pacman_chroot_with_retry "ark"                   # Compresor
                install_pacman_chroot_with_retry "kcalc"                 # Calculadora
                install_pacman_chroot_with_retry "gwenview"              # Visor de imágenes
                install_pacman_chroot_with_retry "okular"                # Visor de PDFs
                install_pacman_chroot_with_retry "kdeconnect"            # Integración con móvil

                # Sistema de archivos y multimedia
                install_pacman_chroot_with_retry "kdegraphics-thumbnailers"  # Miniaturas
                install_pacman_chroot_with_retry "ffmpegthumbs"          # Miniaturas de video
                install_pacman_chroot_with_retry "kimageformats"         # Formatos de imagen adicionales
                install_pacman_chroot_with_retry "qt6-imageformats"      # Más formatos de imagen

                # Herramientas del sistema
                install_pacman_chroot_with_retry "plasma-systemmonitor"  # Monitor de sistema
                install_pacman_chroot_with_retry "partitionmanager"      # Gestor de particiones

                # Gestor de software
                install_pacman_chroot_with_retry "discover"              # Centro de software
                install_pacman_chroot_with_retry "flatpak"               # Soporte Flatpak

                # Extras útiles
                install_pacman_chroot_with_retry "plasma-browser-integration"  # Integración con navegadores
                install_pacman_chroot_with_retry "plasma-firewall"       # Configurar firewall
                install_pacman_chroot_with_retry "kgamma"                # Calibración de gamma

                chroot /mnt /bin/bash -c "systemctl enable sddm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "LXDE")
                echo -e "${CYAN}Instalando LXDE Desktop...${NC}"
                install_pacman_chroot_with_retry "lxde"
                install_yay_chroot_with_retry "lightdm"
                install_yay_chroot_with_retry "lightdm-slick-greeter"
                # Extras útiles
                install_pacman_chroot_with_retry "udisks2"               # Montaje automático de discos
                # install_pacman_chroot_with_retry "leafpad"               # Editor de texto simple
                # Sistema
                install_pacman_chroot_with_retry "network-manager-applet"  # Applet de red
                install_pacman_chroot_with_retry "pavucontrol"           # Control de volumen

                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "LXQT")
                echo -e "${CYAN}Instalando LXQt Desktop...${NC}"
                # Soporte Xorg
                install_pacman_chroot_with_retry "xorg-server"
                install_pacman_chroot_with_retry "xorg-xinit"
                install_pacman_chroot_with_retry "xorg-xauth"
                install_pacman_chroot_with_retry "xf86-input-libinput"
                # Soporte Wayland
                install_pacman_chroot_with_retry "wayland"
                install_pacman_chroot_with_retry "labwc"
                install_pacman_chroot_with_retry "xdg-desktop-portal"
                install_pacman_chroot_with_retry "xdg-desktop-portal-wlr"
                install_pacman_chroot_with_retry "layer-shell-qt"
                install_pacman_chroot_with_retry "qtxdg-tools"
                # LXQt grupo completo (paquetes oficiales)
                install_pacman_chroot_with_retry "lxqt"
                install_pacman_chroot_with_retry "lxqt-wayland-session"
                install_pacman_chroot_with_retry "lxqt-menu-data"
                install_pacman_chroot_with_retry "breeze-icons"
                # Utilidades del sistema
                install_pacman_chroot_with_retry "xss-lock"              # Activador de bloqueo
                install_pacman_chroot_with_retry "slock"                 # Bloqueador
                install_yay_chroot_with_retry "nm-tray"
                # Display manager
                install_pacman_chroot_with_retry "sddm"
                chroot /mnt /bin/bash -c "systemctl enable sddm"
                ;;
            "MATE")
                echo -e "${CYAN}Instalando MATE Desktop...${NC}"
                # Soporte Xorg
                install_pacman_chroot_with_retry "xorg-server"
                install_pacman_chroot_with_retry "xorg-xinit"
                install_pacman_chroot_with_retry "xorg-xauth"
                install_pacman_chroot_with_retry "xf86-input-libinput"
                # Grupos MATE oficiales
                install_pacman_chroot_with_retry "mate"
                install_pacman_chroot_with_retry "mate-extra"
                # Aplicaciones esenciales MATE oficiales
                install_pacman_chroot_with_retry "pluma"
                install_pacman_chroot_with_retry "atril"
                install_pacman_chroot_with_retry "engrampa"
                install_pacman_chroot_with_retry "eom"
                # Componentes del sistema oficiales
                install_pacman_chroot_with_retry "network-manager-applet"
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-slick-greeter"
                install_pacman_chroot_with_retry "accountsservice"
                # Paquetes de AUR (solo los que realmente están en AUR)
                install_pacman_chroot_with_retry "mate-applet-dock"
                install_yay_chroot_with_retry "mate-tweak"
                install_yay_chroot_with_retry "brisk-menu"
                install_pacman_chroot_with_retry "mugshot"
                # Configuración de LightDM
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "XFCE4")
                echo -e "${CYAN}Instalando XFCE4 Desktop...${NC}"
                # Soporte Xorg
                install_pacman_chroot_with_retry "xorg-server"
                install_pacman_chroot_with_retry "xorg-xinit"
                install_pacman_chroot_with_retry "xorg-xauth"
                install_pacman_chroot_with_retry "xf86-input-libinput"
                install_pacman_chroot_with_retry "xfce4"
                install_pacman_chroot_with_retry "xfce4-goodies"
                install_pacman_chroot_with_retry "network-manager-applet"
                install_pacman_chroot_with_retry "loupe"
                install_pacman_chroot_with_retry "showtime"
                install_pacman_chroot_with_retry "papers"
                install_pacman_chroot_with_retry "pavucontrol"
                install_pacman_chroot_with_retry "polkit-gnome"          # Autenticación
                install_pacman_chroot_with_retry "gnome-keyring"
                install_pacman_chroot_with_retry "light-locker"
                install_pacman_chroot_with_retry "xfce4-screensaver"
                # Soporte Wayland
                install_pacman_chroot_with_retry "wayland"
                install_pacman_chroot_with_retry "labwc"
                install_pacman_chroot_with_retry "xdg-desktop-portal"
                install_pacman_chroot_with_retry "xdg-desktop-portal-wlr"
                # lightdm
                install_pacman_chroot_with_retry "lightdm"
                install_pacman_chroot_with_retry "lightdm-slick-greeter"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                install_pacman_chroot_with_retry "accountsservice"
                install_pacman_chroot_with_retry "mugshot"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            *)
                echo -e "${YELLOW}Entorno de escritorio no reconocido: $DESKTOP_ENVIRONMENT${NC}"
                ;;
        esac
        ;;
    "WINDOW_MANAGER")
        echo -e "${GREEN}Instalando gestor de ventanas: $WINDOW_MANAGER${NC}"

        # Instalar X.org y dependencias base para gestores de ventanas
        echo -e "${CYAN}Instalando servidor X.org y dependencias base...${NC}"
        install_pacman_chroot_with_retry "xorg-server"
        install_pacman_chroot_with_retry "xorg-apps"
        install_pacman_chroot_with_retry "xorg-xinit"
        install_pacman_chroot_with_retry "pcmanfm"
        install_pacman_chroot_with_retry "gvfs"
        install_pacman_chroot_with_retry "lm_sensors"
        install_pacman_chroot_with_retry "tumbler"
        install_pacman_chroot_with_retry "ffmpegthumbs"
        install_pacman_chroot_with_retry "ffmpegthumbnailer"
        install_pacman_chroot_with_retry "freetype2"
        install_pacman_chroot_with_retry "libgsf"
        install_pacman_chroot_with_retry "gdk-pixbuf2"
        install_pacman_chroot_with_retry "fontconfig"
        install_pacman_chroot_with_retry "gnome-keyring"
        # Instalar Ly display manager
        echo -e "${CYAN}Instalando Ly display manager...${NC}"
        install_yay_chroot_with_retry "ly"
        install_pacman_chroot_with_retry "xorg-xauth"
        install_pacman_chroot_with_retry "brightnessctl"
        chroot /mnt /bin/bash -c "systemctl enable ly@tty1.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        case "$WINDOW_MANAGER" in
            "I3WM"|"I3")
                echo -e "${CYAN}Instalando Extras de i3 Window Manager...${NC}"
                install_pacman_chroot_with_retry "xorg-server" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xinit" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xauth" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xrandr" #Configurar pantallas en tiempo real en el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xterm" #Terminal para el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "network-manager-applet" #Para gestionar conexiones de red desde la bandeja del sistema.
                install_pacman_chroot_with_retry "rofi" #Lanzadores de aplicaciones. Rofi es más moderno y configurable.
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen
                install_pacman_chroot_with_retry "pavucontrol" #Control de volumen gráfico para PulseAudio/PipeWire.
                install_pacman_chroot_with_retry "dunst" #Notificaciones en pantalla.
                install_pacman_chroot_with_retry "lxappearance" #Para configurar temas GTK.
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla.
                install_pacman_chroot_with_retry "maim" #Captura de pantalla.
                install_pacman_chroot_with_retry "xclip" #Copiar y pegar texto entre aplicaciones.
                install_pacman_chroot_with_retry "arandr" #Configuración de monitores.
                install_pacman_chroot_with_retry "polkit-gnome" #Para gestionar contraseñas de administración.
                install_pacman_chroot_with_retry "unclutter" #Oculta el cursor tras inactividad.
                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "alacritty" #Emulador de terminal acelerado por GPU
                echo -e "${CYAN}Instalando i3 Window Manager...${NC}"
                install_pacman_chroot_with_retry "i3-wm"
                install_pacman_chroot_with_retry "i3status"
                install_pacman_chroot_with_retry "i3lock"
                install_pacman_chroot_with_retry "i3blocks"
                # Crear configuración básica de i3
                mkdir -p /mnt/home/$USER/.config/i3
                chroot /mnt /bin/bash -c "install -Dm644 /etc/i3/config /home/$USER/.config/i3/config"
                mkdir -p /mnt/home/$USER/.config/i3status
                chroot /mnt /bin/bash -c "install -Dm644 /etc/i3status.conf /home/$USER/.config/i3status/config"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "AWESOME")
                echo -e "${CYAN}Instalando Extras de Awesome Window Manager...${NC}"
                install_pacman_chroot_with_retry "xorg-server" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xinit" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xauth" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xrandr" #Configurar pantallas en tiempo real en el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xterm" #Terminal para el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "network-manager-applet" #Para gestionar conexiones de red desde la bandeja del sistema.
                install_pacman_chroot_with_retry "rofi" #Lanzadores de aplicaciones. Rofi es más moderno y configurable.
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen
                install_pacman_chroot_with_retry "pavucontrol" #Control de volumen gráfico para PulseAudio/PipeWire.
                install_pacman_chroot_with_retry "dunst" #Notificaciones en pantalla.
                install_pacman_chroot_with_retry "lxappearance" #Para configurar temas GTK.
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "maim" #Captura de pantalla.
                install_pacman_chroot_with_retry "xclip" #Copiar y pegar texto entre aplicaciones.
                install_pacman_chroot_with_retry "arandr" #Configuración de monitores.
                install_pacman_chroot_with_retry "polkit-gnome" #Para gestionar contraseñas de administración.
                install_pacman_chroot_with_retry "unclutter" #Oculta el cursor tras inactividad.
                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "alacritty" #Emulador de terminal acelerado por GPU
                echo -e "${CYAN}Instalando Awesome Window Manager...${NC}"
                install_pacman_chroot_with_retry "awesome"
                install_pacman_chroot_with_retry "vicious"
                # Crear configuración básica de awesome
                mkdir -p /mnt/home/$USER/.config/awesome
                chroot /mnt /bin/bash -c "install -Dm755 /etc/xdg/awesome/rc.lua /home/$USER/.config/awesome/rc.lua"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "BSPWM")
                echo -e "${CYAN}Instalando BSPWM Window Manager...${NC}"
                install_pacman_chroot_with_retry "xorg-server" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xinit" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xauth" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xrandr" #Configurar pantallas en tiempo real en el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xterm" #Terminal para el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "network-manager-applet" #Para gestionar conexiones de red desde la bandeja del sistema.
                install_pacman_chroot_with_retry "rofi" #Lanzadores de aplicaciones. Rofi es más moderno y configurable.
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen
                install_pacman_chroot_with_retry "pavucontrol" #Control de volumen gráfico para PulseAudio/PipeWire.
                install_pacman_chroot_with_retry "dunst" #Notificaciones en pantalla.
                install_pacman_chroot_with_retry "lxappearance" #Para configurar temas GTK.
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "maim" #Captura de pantalla.
                install_pacman_chroot_with_retry "xclip" #Copiar y pegar texto entre aplicaciones.
                install_pacman_chroot_with_retry "arandr" #Configuración de monitores.
                install_pacman_chroot_with_retry "polkit-gnome" #Para gestionar contraseñas de administración.
                install_pacman_chroot_with_retry "unclutter" #Oculta el cursor tras inactividad.
                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "alacritty" #Emulador de terminal acelerado por GPU
                echo -e "${CYAN}Instalando BSPWM Window Manager...${NC}"
                install_pacman_chroot_with_retry "bspwm"
                install_pacman_chroot_with_retry "sxhkd"
                install_pacman_chroot_with_retry "slock"
                install_pacman_chroot_with_retry "polybar"
                # Crear configuración básica de bspwm
                mkdir -p /mnt/home/$USER/.config/bspwm
                mkdir -p /mnt/home/$USER/.config/sxhkd
                mkdir -p /mnt/home/$USER/.config/polybar/
                chroot /mnt /bin/bash -c "install -Dm755 /usr/share/doc/bspwm/examples/bspwmrc /home/$USER/.config/bspwm/bspwmrc"
                chroot /mnt /bin/bash -c "install -Dm644 /usr/share/doc/bspwm/examples/sxhkdrc /home/$USER/.config/sxhkd/sxhkdrc"
                chroot /mnt /bin/bash -c "install -Dm644 /etc/polybar/config.ini /home/$USER/.config/polybar/config.ini"
                chroot /mnt /bin/bash -c "echo polybar >> /home/$USER/.config/bspwm/bspwmrc"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "DWM")
                echo -e "${CYAN}Instalando Extras Window Manager...${NC}"
                install_pacman_chroot_with_retry "xorg-server" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xinit" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xauth" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xrandr" #Configurar pantallas en tiempo real en el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xterm" #Terminal para el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "network-manager-applet" #Para gestionar conexiones de red desde la bandeja del sistema.
                install_pacman_chroot_with_retry "rofi" #Lanzadores de aplicaciones. Rofi es más moderno y configurable.
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen
                install_pacman_chroot_with_retry "pavucontrol" #Control de volumen gráfico para PulseAudio/PipeWire.
                install_pacman_chroot_with_retry "dunst" #Notificaciones en pantalla.
                install_pacman_chroot_with_retry "lxappearance" #Para configurar temas GTK.
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "maim" #Captura de pantalla.
                install_pacman_chroot_with_retry "xclip" #Copiar y pegar texto entre aplicaciones.
                install_pacman_chroot_with_retry "arandr" #Configuración de monitores.
                install_pacman_chroot_with_retry "polkit-gnome" #Para gestionar contraseñas de administración.
                install_pacman_chroot_with_retry "unclutter" #Oculta el cursor tras inactividad.
                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "alacritty" #Emulador de terminal acelerado por GPU
                echo -e "${CYAN}Instalando DWM Window Manager...${NC}"
                install_yay_chroot_with_retry "dwm"
                install_yay_chroot_with_retry "st"
                install_yay_chroot_with_retry "slock"
                ;;
            "DWL")
                echo -e "${YELLOW}Instalando dependencias de DWL...${NC}"
                install_pacman_chroot_with_retry "wayland"                      # Protocolo de servidor de display moderno (reemplazo de X11)
                install_pacman_chroot_with_retry "wlr-randr"                    # Gestor de pantallas para Wayland
                install_pacman_chroot_with_retry "xorg-xwayland"                # Compatibilidad con apps X11
                install_pacman_chroot_with_retry "wayland-protocols"
                install_pacman_chroot_with_retry "xdg-desktop-portal-wlr"
                install_pacman_chroot_with_retry "wlroots0.19"
                install_pacman_chroot_with_retry "foot"
                install_pacman_chroot_with_retry "dunst"
                install_pacman_chroot_with_retry "swaylock"
                install_pacman_chroot_with_retry "swayidle"
                install_pacman_chroot_with_retry "brightnessctl"
                install_pacman_chroot_with_retry "polkit-gnome"
                install_pacman_chroot_with_retry "wmenu"
                install_pacman_chroot_with_retry "wl-clipboard"
                install_pacman_chroot_with_retry "grim"
                install_pacman_chroot_with_retry "slurp"
                install_pacman_chroot_with_retry "swaybg"
                install_pacman_chroot_with_retry "ttf-jetbrains-mono-nerd"
                install_pacman_chroot_with_retry "tllist"
                install_pacman_chroot_with_retry "mako"
                install_pacman_chroot_with_retry "jq"
                install_pacman_chroot_with_retry "pixman"
                install_pacman_chroot_with_retry "libxkbcommon-x11"
                install_pacman_chroot_with_retry "libxkbcommon"
                install_pacman_chroot_with_retry "wofi"
                install_pacman_chroot_with_retry "fuzzel"
                install_pacman_chroot_with_retry "libinput"
                install_pacman_chroot_with_retry "pkg-config"
                install_pacman_chroot_with_retry "fcft"
                install_pacman_chroot_with_retry "kitty"
                install_yay_chroot_with_retry "wdisplays"                       # Gestor gráfico de resolución y monitores Wayland
                echo -e "${CYAN}Instalando DWL Wayland Compositor...${NC}"
                install_yay_chroot_with_retry "dwl"

                # Crear directorio temporal para compilación
                chroot /mnt /bin/bash -c "mkdir -p /home/$USER/.config/src && chown $USER:$USER /home/$USER/.config/src"

                # Compilar e instalar dwl
                # https://github.com/yukiisen/waydots
                chroot /mnt /bin/bash -c "cd /home/$USER/.config/src && sudo -u $USER git clone https://github.com/CodigoCristo/dwl"
                chroot /mnt /bin/bash -c "cd /home/$USER/.config/src/dwl && sudo -u $USER make clean && sudo make install"

                # Compilar e instalar slstatus
                chroot /mnt /bin/bash -c "cd /home/$USER/.config/src && sudo -u $USER git clone https://git.suckless.org/slstatus"
                chroot /mnt /bin/bash -c "cd /home/$USER/.config/src/slstatus && sudo -u $USER make clean && sudo make install"

                # Mantener directorio src para futuras compilaciones y configuraciones personalizadas
                # chroot /mnt /bin/bash -c "rm -rf /home/$USER/.config/src"

                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpg

                # Crear script start_dwl.sh en el home del usuario
                echo -e "${YELLOW}Creando script de inicio start_dwl.sh...${NC}"
                cat > /mnt/home/$USER/start_dwl.sh << EOF
#!/bin/sh

# Configurar teclado
export XKB_DEFAULT_LAYOUT=$KEYBOARD_LAYOUT

# Configurar pantalla con wlr-randr (ajusta según tu monitor)
# wlr-randr --output HDMI-A-1 --mode 1920x1080 --rate 60 &

# Iniciar dwl con slstatus
slstatus -s | dwl -s "sh -c 'swaybg -i /usr/share/pixmaps/backgroundarch.jpge &'"
EOF

                # Dar permisos de ejecución al script
                chmod +x /mnt/home/$USER/start_dwl.sh
                chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/start_dwl.sh"

                # Crear/modificar el archivo dwl.desktop
                echo -e "${YELLOW}Configurando dwl.desktop...${NC}"
                mkdir -p /mnt/usr/share/wayland-sessions
                cat > /mnt/usr/share/wayland-sessions/dwl.desktop << EOF
[Desktop Entry]
Name=dwl
Comment=dwl with slstatus
Exec=/home/$USER/start_dwl.sh
Type=Application
DesktopNames=dwl
EOF

                echo -e "${GREEN}DWL instalado correctamente!${NC}"
                ;;
            "HYPRLAND")
                echo -e "${CYAN}Instalando Hyprland Window Manager...${NC}"
                install_pacman_chroot_with_retry "wayland"                      # Protocolo de servidor de display moderno (reemplazo de X11)
                install_pacman_chroot_with_retry "wlr-randr"                    # Gestor de pantallas para Wayland
                install_pacman_chroot_with_retry "xorg-xwayland"                # Compatibilidad con apps X11
                install_pacman_chroot_with_retry "hyprland"                     # Compositor Wayland dinámico con animaciones y efectos
                install_pacman_chroot_with_retry "hyprpaper"                    # Gestor de wallpapers para Hyprland
                install_pacman_chroot_with_retry "hypridle"                     # Gestor de inactividad/idle para Hyprland
                install_pacman_chroot_with_retry "hyprcursor"                   # Gestor de cursores para Hyprland
                install_pacman_chroot_with_retry "hyprpolkitagent"              # Agente de autenticación PolicyKit para Hyprland
                install_pacman_chroot_with_retry "hyprsunset"                   # Filtro de luz azul/ajuste de temperatura de color
                install_pacman_chroot_with_retry "waybar"                       # Barra de estado personalizable para Wayland
                install_pacman_chroot_with_retry "wofi"                         # Launcher de aplicaciones para Wayland (estilo rofi)
                install_pacman_chroot_with_retry "nwg-displays"                 # Configurador gráfico de pantallas para Wayland
                install_pacman_chroot_with_retry "xdg-desktop-portal-hyprland"  # Portal XDG específico para Hyprland (compartir pantalla, etc.)
                install_pacman_chroot_with_retry "xdg-desktop-portal-gtk"       # Portal XDG con backend GTK (diálogos de archivos, etc.)
                install_pacman_chroot_with_retry "wl-clipboard"                 # Utilidades de portapapeles para Wayland
                install_pacman_chroot_with_retry "grim"                         # Captura de pantalla para Wayland
                install_pacman_chroot_with_retry "slurp"                        # Selector de región de pantalla (usado con grim)
                install_pacman_chroot_with_retry "qt5-wayland"                  # Soporte de Wayland para aplicaciones Qt5
                install_pacman_chroot_with_retry "qt6-wayland"                  # Soporte de Wayland para aplicaciones Qt6
                install_pacman_chroot_with_retry "kitty"                        # Emulador de terminal acelerado por GPU
                install_pacman_chroot_with_retry "dunst"                        # Demonio de notificaciones ligero y personalizable
                install_pacman_chroot_with_retry "nwg-look"                     # Configurador de temas GTK para Wayland
                install_pacman_chroot_with_retry "xdg-utils"                    # Herramientas para integración de escritorio (abrir archivos, URLs)
                install_pacman_chroot_with_retry "brightnessctl"                # Control de brillo de pantalla desde terminal
                # Crear configuración básica de hyprland
                mkdir -p /mnt/home/$USER/.config/hypr
                chroot /mnt /bin/bash -c "install -Dm644 /usr/share/hypr/hyprland.conf /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "echo exec-once = waybar >> /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "echo exec-once = systemctl --user start hyprpolkitagent >> /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER hyprctl keyword input:kb_layout $KEYBOARD_LAYOUT"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "OPENBOX")
                echo -e "${CYAN}Instalando Openbox Window Manager...${NC}"
                install_pacman_chroot_with_retry "xorg-server" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xinit" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xauth" #necesarios para correr el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xorg-xrandr" #Configurar pantallas en tiempo real en el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "xterm" #Terminal para el entorno gráfico Xorg.
                install_pacman_chroot_with_retry "network-manager-applet" #Para gestionar conexiones de red desde la bandeja del sistema.
                install_pacman_chroot_with_retry "openbox"
                install_pacman_chroot_with_retry "lxappearance"
                install_pacman_chroot_with_retry "obconf-qt"
                install_pacman_chroot_with_retry "dmenu"
                install_pacman_chroot_with_retry "xfce4-power-manager"
                install_pacman_chroot_with_retry "volumeicon"
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen
                install_pacman_chroot_with_retry "pavucontrol" #Control de volumen gráfico para PulseAudio/PipeWire.
                install_pacman_chroot_with_retry "dunst" #Notificaciones en pantalla.
                install_pacman_chroot_with_retry "lxappearance" #Para configurar temas GTK.
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "maim" #Captura de pantalla.
                install_pacman_chroot_with_retry "xclip" #Copiar y pegar texto entre aplicaciones.
                install_pacman_chroot_with_retry "arandr" #Configuración de monitores.
                install_pacman_chroot_with_retry "polkit-gnome" #Para gestionar contraseñas de administración.
                install_pacman_chroot_with_retry "unclutter" #Oculta el cursor tras inactividad.
                install_pacman_chroot_with_retry "lxinput"
                install_pacman_chroot_with_retry "tint2"
                install_yay_chroot_with_retry "obmenu-generator"
                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "alacritty" #Emulador de terminal acelerado por GPU
                # Crear configuración básica de openbox
                mkdir -p /mnt/home/$USER/.config/openbox
                chroot /mnt /bin/bash -c "obmenu-generator -i -p"
                chroot /mnt /bin/bash -c "cp -a /etc/xdg/openbox /home/$USER/.config/"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "QTITLE"|"QTILE")
                echo -e "${CYAN}Instalando Qtile Window Manager...${NC}"
                # Base Xorg
                install_pacman_chroot_with_retry "xorg-server"        # Servidor gráfico Xorg
                install_pacman_chroot_with_retry "xorg-xinit"         # Iniciar sesión X11
                install_pacman_chroot_with_retry "xorg-xauth"         # Autenticación X11
                install_pacman_chroot_with_retry "xorg-xrandr"        # Configurar pantallas

                # Qtile y dependencias Python
                install_pacman_chroot_with_retry "qtile"              # Window Manager
                install_pacman_chroot_with_retry "python-psutil"      # Widgets de sistema (CPU, RAM)
                install_pacman_chroot_with_retry "python-dbus-next"   # Notificaciones
                install_pacman_chroot_with_retry "python-iwlib"       # Widget WiFi (opcional)
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen

                # Lanzadores y utilidades
                install_pacman_chroot_with_retry "rofi"               # Lanzador de aplicaciones
                install_pacman_chroot_with_retry "dunst"              # Notificaciones
                install_pacman_chroot_with_retry "maim"               # Capturas de pantalla
                install_pacman_chroot_with_retry "xclip"              # Portapapeles

                # Apariencia y configuración
                install_pacman_chroot_with_retry "lxappearance"       # Temas GTK
                install_pacman_chroot_with_retry "arandr"             # Configuración de monitores gráfica

                # Sistema y seguridad
                install_pacman_chroot_with_retry "network-manager-applet"  # Applet de red
                install_pacman_chroot_with_retry "pavucontrol"        # Control de volumen
                install_pacman_chroot_with_retry "polkit-gnome"       # Autenticación gráfica
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "unclutter"          # Ocultar cursor inactivo

                echo -e "${CYAN}Instalando Terminales...${NC}"
                install_pacman_chroot_with_retry "xterm"              # Terminal básica
                install_pacman_chroot_with_retry "alacritty"          # Terminal moderna
                # Crear configuración básica de qtile
                mkdir -p /mnt/home/$USER/.config/qtile
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "SWAY")
                echo -e "${CYAN}Instalando Sway Window Manager...${NC}"
                # Base Wayland
                install_pacman_chroot_with_retry "wayland"                      # Protocolo de servidor de display moderno (reemplazo de X11)
                install_pacman_chroot_with_retry "wlr-randr"                    # Gestor de pantallas para Wayland
                install_pacman_chroot_with_retry "xorg-xwayland"      # Compatibilidad con apps X11

                # Sway y componentes principales
                install_pacman_chroot_with_retry "sway"               # Window Manager
                install_pacman_chroot_with_retry "swaybg"             # Fondos de pantalla
                install_pacman_chroot_with_retry "swaylock"           # Bloqueador de pantalla
                install_pacman_chroot_with_retry "swayidle"           # Gestión de inactividad

                # Portales XDG
                install_pacman_chroot_with_retry "xdg-desktop-portal-wlr"  # Portal para screensharing
                install_pacman_chroot_with_retry "xdg-desktop-portal-gtk"  # Portal GTK

                # Barra y menús
                install_pacman_chroot_with_retry "waybar"             # Barra de estado
                install_pacman_chroot_with_retry "wofi"               # Lanzador de aplicaciones (más completo que wmenu)
                install_pacman_chroot_with_retry "wmenu"              # Lanzador minimalista

                # Capturas de pantalla
                install_pacman_chroot_with_retry "grim"               # Capturas de pantalla
                install_pacman_chroot_with_retry "slurp"              # Seleccionar región de pantalla

                # Portapapeles
                install_pacman_chroot_with_retry "wl-clipboard"       # Portapapeles Wayland
                install_pacman_chroot_with_retry "cliphist"           # Historial de portapapeles

                # Notificaciones
                install_pacman_chroot_with_retry "mako"               # Notificaciones para Wayland
                install_pacman_chroot_with_retry "libnotify"          # Soporte de notificaciones

                # Sistema
                install_pacman_chroot_with_retry "pavucontrol"        # Control de volumen
                install_pacman_chroot_with_retry "brightnessctl"      # Control de brillo
                install_pacman_chroot_with_retry "polkit-gnome"       # Autenticación gráfica
                install_pacman_chroot_with_retry "network-manager-applet"  # Applet de red

                # Apariencia
                install_pacman_chroot_with_retry "nwg-look"           # Configurar temas GTK en Wayland
                install_pacman_chroot_with_retry "qt5-wayland"        # Soporte Qt5
                install_pacman_chroot_with_retry "qt6-wayland"        # Soporte Qt6

                # Utilidades
                install_yay_chroot_with_retry "wdisplays"             # Gestor gráfico de resolución y monitores Wayland
                install_pacman_chroot_with_retry "foot"               # Terminal nativa Wayland (ligera)

                # Aplicaciones básicas
                install_pacman_chroot_with_retry "kitty"              # Terminal moderna con buen soporte Wayland
                # Crear configuración básica de sway
                mkdir -p /mnt/home/$USER/.config/sway
                chroot /mnt /bin/bash -c "install -Dm644 /etc/sway/config /home/$USER/.config/sway/config"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "XMONAD")
                echo -e "${CYAN}Instalando XMonad Window Manager...${NC}"
                # Base Xorg
                install_pacman_chroot_with_retry "xorg-server"        # Servidor gráfico Xorg
                install_pacman_chroot_with_retry "xorg-xinit"         # Iniciar sesión X11
                install_pacman_chroot_with_retry "xorg-xauth"         # Autenticación X11
                install_pacman_chroot_with_retry "xorg-xrandr"        # Configurar pantallas

                # XMonad y herramientas Haskell
                install_pacman_chroot_with_retry "xmonad"             # Window Manager
                install_pacman_chroot_with_retry "xmonad-contrib"     # Extensiones y layouts adicionales
                install_pacman_chroot_with_retry "xmobar"             # Barra de estado
                install_pacman_chroot_with_retry "ghc"                # Compilador Haskell
                install_pacman_chroot_with_retry "cabal-install"      # Gestor de paquetes Haskell

                # Compositor y fondos
                install_pacman_chroot_with_retry "feh"                # Alternativa a nitrogen

                # Lanzadores y utilidades
                install_pacman_chroot_with_retry "rofi"               # Lanzador de aplicaciones
                install_pacman_chroot_with_retry "dmenu"              # Lanzador alternativo (más ligero)
                install_pacman_chroot_with_retry "dunst"              # Notificaciones
                install_pacman_chroot_with_retry "maim"               # Capturas de pantalla
                install_pacman_chroot_with_retry "xclip"              # Portapapeles
                install_pacman_chroot_with_retry "picom"
                install_pacman_chroot_with_retry "numlockx"
                install_pacman_chroot_with_retry "flameshot"


                # Apariencia y configuración
                install_pacman_chroot_with_retry "lxappearance"       # Temas GTK
                install_pacman_chroot_with_retry "arandr"             # Configuración de monitores

                # Sistema y seguridad
                install_pacman_chroot_with_retry "network-manager-applet"  # Applet de red
                install_pacman_chroot_with_retry "pavucontrol"        # Control de volumen
                install_pacman_chroot_with_retry "polkit-gnome"       # Autenticación gráfica
                install_pacman_chroot_with_retry "xss-lock"           # Activador de bloqueo automático
                install_pacman_chroot_with_retry "slock"              # Bloqueador de pantalla
                install_pacman_chroot_with_retry "unclutter"          # Ocultar cursor inactivo

                # Bandeja del sistema
                #install_pacman_chroot_with_retry "trayer"             # Bandeja del sistema (si no usas xmobar con tray)
                install_pacman_chroot_with_retry "stalonetray"        # Alternativa a trayer

                # Aplicaciones básicas
                install_pacman_chroot_with_retry "xterm"              # Terminal básica
                install_pacman_chroot_with_retry "alacritty"          # Terminal moderna
                # Crear configuración básica de xmonad
                mkdir -p /mnt/home/$USER/.config/xmonad
                guardar_configuraciones_xmonad
                chroot /mnt /bin/bash -c "sudo -u $USER xmonad --recompile /home/$USER/.config/xmonad/xmonad.hs"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            *)
                echo -e "${YELLOW}Gestor de ventanas no reconocido: $WINDOW_MANAGER${NC}"
                echo -e "${CYAN}Instalando i3 como alternativa...${NC}"
                ;;
        esac
    ;;
esac
