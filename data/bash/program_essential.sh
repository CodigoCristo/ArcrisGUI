# Instalación de aplicaciones adicionales basadas en configuración
echo -e "${GREEN}| Instalando aplicaciones adicionales |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Verificar si ESSENTIAL_APPS está habilitado
if [ "${ESSENTIAL_APPS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando shell del sistema: ${SYSTEM_SHELL:-bash}${NC}"

    case "${SYSTEM_SHELL:-bash}" in
        "bash")
            install_pacman_chroot_with_retry "bash"
            install_pacman_chroot_with_retry "bash-completion"
            chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
        "dash")
            install_pacman_chroot_with_retry "dash"
            chroot /mnt /bin/bash -c "chsh -s /bin/dash $USER"
            ;;
        "ksh")
            install_pacman_chroot_with_retry "ksh"
            chroot /mnt /bin/bash -c "chsh -s /usr/bin/ksh $USER"
            ;;
        "fish")
            install_pacman_chroot_with_retry "fish"
            chroot /mnt /bin/bash -c "chsh -s /usr/bin/fish $USER"
            ;;
        "zsh")
            install_pacman_chroot_with_retry "zsh"
            install_pacman_chroot_with_retry "zsh-completions"
            install_pacman_chroot_with_retry "zsh-syntax-highlighting"
            install_pacman_chroot_with_retry "zsh-autosuggestions"
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/home/$USER/.zshrc
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/root/.zshrc
            chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.zshrc"
            chroot /mnt /bin/bash -c "chsh -s /bin/zsh $USER"
            ;;
        *)
            echo -e "${YELLOW}Shell no reconocida: ${SYSTEM_SHELL}, usando bash${NC}"
            install_pacman_chroot_with_retry "bash"
            install_pacman_chroot_with_retry "bash-completion"
            chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
    esac
    echo -e "${GREEN}✓ Shell del sistema configurada${NC}"
fi

# Verificar si FILESYSTEMS está habilitado
if [ "${FILESYSTEMS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de sistemas de archivos...${NC}"

    install_pacman_chroot_with_retry "android-file-transfer"
    install_pacman_chroot_with_retry "android-tools"
    install_pacman_chroot_with_retry "android-udev"
    install_pacman_chroot_with_retry "msmtp"
    install_pacman_chroot_with_retry "libmtp"
    install_pacman_chroot_with_retry "libcddb"
    install_pacman_chroot_with_retry "gvfs"
    install_pacman_chroot_with_retry "gvfs-afc"
    install_pacman_chroot_with_retry "gvfs-smb"
    install_pacman_chroot_with_retry "gvfs-gphoto2"
    install_pacman_chroot_with_retry "gvfs-mtp"
    install_pacman_chroot_with_retry "gvfs-goa"
    install_pacman_chroot_with_retry "gvfs-nfs"
    install_pacman_chroot_with_retry "gvfs-google"
    install_pacman_chroot_with_retry "gst-libav"
    install_pacman_chroot_with_retry "dosfstools"
    install_pacman_chroot_with_retry "f2fs-tools"
    install_pacman_chroot_with_retry "ntfs-3g"
    install_pacman_chroot_with_retry "udftools"
    install_pacman_chroot_with_retry "nilfs-utils"
    install_pacman_chroot_with_retry "polkit"
    install_pacman_chroot_with_retry "gpart"
    install_pacman_chroot_with_retry "mtools"
    install_pacman_chroot_with_retry "cifs-utils"
    install_pacman_chroot_with_retry "jfsutils"
    # btrfs-progs se instala condicionalmente según el sistema de archivos
    if ! { [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; }; then
        install_pacman_chroot_with_retry "btrfs-progs"
        install_pacman_chroot_with_retry "btrfsmaintenance"
        install_pacman_chroot_with_retry "snapper"
    fi
    install_pacman_chroot_with_retry "xfsprogs"
    install_pacman_chroot_with_retry "e2fsprogs"
    install_pacman_chroot_with_retry "exfatprogs"

    echo -e "${GREEN}✓ Herramientas de sistemas de archivos instaladas${NC}"
fi

# Verificar si COMPRESSION está habilitado
if [ "${COMPRESSION_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de compresión...${NC}"

    install_pacman_chroot_with_retry "xarchiver"
    install_pacman_chroot_with_retry "unarchiver"
    install_pacman_chroot_with_retry "binutils"
    install_pacman_chroot_with_retry "gzip"
    install_pacman_chroot_with_retry "lha"
    install_pacman_chroot_with_retry "lrzip"
    install_pacman_chroot_with_retry "lzip"
    install_pacman_chroot_with_retry "lz4"
    install_pacman_chroot_with_retry "p7zip"
    install_pacman_chroot_with_retry "tar"
    install_pacman_chroot_with_retry "xz"
    install_pacman_chroot_with_retry "bzip2"
    install_pacman_chroot_with_retry "lbzip2"
    install_pacman_chroot_with_retry "arj"
    install_pacman_chroot_with_retry "lzop"
    install_pacman_chroot_with_retry "cpio"
    install_pacman_chroot_with_retry "unrar"
    install_pacman_chroot_with_retry "unzip"
    install_pacman_chroot_with_retry "zstd"
    install_pacman_chroot_with_retry "zip"
    install_pacman_chroot_with_retry "unarj"
    install_pacman_chroot_with_retry "dpkg"
    echo -e "${GREEN}✓ Herramientas de compresión instaladas${NC}"
fi

# Verificar si VIDEO_CODECS está habilitado
if [ "${VIDEO_CODECS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando codecs de video...${NC}"

    install_pacman_chroot_with_retry "ffmpeg"
    install_pacman_chroot_with_retry "aom"
    install_pacman_chroot_with_retry "libde265"
    install_pacman_chroot_with_retry "x265"
    install_pacman_chroot_with_retry "x264"
    install_pacman_chroot_with_retry "libmpeg2"
    install_pacman_chroot_with_retry "xvidcore"
    install_pacman_chroot_with_retry "libtheora"
    install_pacman_chroot_with_retry "libvpx"
    install_pacman_chroot_with_retry "sdl"
    install_pacman_chroot_with_retry "gstreamer"
    install_pacman_chroot_with_retry "gst-libav"
    install_pacman_chroot_with_retry "gst-plugins-bad"
    install_pacman_chroot_with_retry "gst-plugins-base"
    install_pacman_chroot_with_retry "gst-plugins-base-libs"
    install_pacman_chroot_with_retry "gst-plugins-good"
    install_pacman_chroot_with_retry "gst-plugins-ugly"
    install_pacman_chroot_with_retry "gst-plugin-pipewire"
    install_pacman_chroot_with_retry "gst-plugin-va"
    install_pacman_chroot_with_retry "xine-lib"
    install_pacman_chroot_with_retry "libdvdcss"
    install_pacman_chroot_with_retry "libdvdread"
    install_pacman_chroot_with_retry "dvd+rw-tools"
    install_pacman_chroot_with_retry "lame"
    install_pacman_chroot_with_retry "jasper"
    install_pacman_chroot_with_retry "libmng"
    install_pacman_chroot_with_retry "libraw"
    install_pacman_chroot_with_retry "libkdcraw"
    install_pacman_chroot_with_retry "vcdimager"
    install_pacman_chroot_with_retry "mpv"
    install_pacman_chroot_with_retry "faac"
    install_pacman_chroot_with_retry "faad2"
    install_pacman_chroot_with_retry "flac"
    install_pacman_chroot_with_retry "opus"
    install_pacman_chroot_with_retry "libvorbis"
    install_pacman_chroot_with_retry "wavpack"
    install_pacman_chroot_with_retry "libheif"
    install_pacman_chroot_with_retry "libavif"

    echo -e "${GREEN}✓ Codecs de video instalados${NC}"
fi
