# Instalación de drivers de audio
echo -e "${GREEN}| Instalando drivers de audio: $DRIVER_AUDIO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_AUDIO" in
    "Alsa Audio")
        install_pacman_chroot_with_retry "alsa-utils"
        install_pacman_chroot_with_retry "alsa-plugins"
        ;;
    "pipewire")
        chroot /mnt /bin/bash -c "pacman -Q pulseaudio >/dev/null 2>&1 && pacman -Rdd pulseaudio --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pulseaudio-alsa >/dev/null 2>&1 && pacman -Rdd pulseaudio --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q jack2 >/dev/null 2>&1 && pacman -Rdd jack2 --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q lib32-jack2 >/dev/null 2>&1 && pacman -Rdd lib32-jack2 --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q jack2-dbus >/dev/null 2>&1 && pacman -Rdd jack2-dbus --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q carla >/dev/null 2>&1 && pacman -Rdd carla --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q qjackctl >/dev/null 2>&1 && pacman -Rdd qjackctl --noconfirm; exit 0"
        install_pacman_chroot_with_retry "pipewire"
        install_pacman_chroot_with_retry "pipewire-pulse"
        install_pacman_chroot_with_retry "pipewire-alsa"
        ;;
    "pulseaudio")
        chroot /mnt /bin/bash -c "pacman -Q pipewire >/dev/null 2>&1 && pacman -Rdd pipewire --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pipewire-pulse >/dev/null 2>&1 && pacman -Rdd pipewire-pulse --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pipewire-alsa >/dev/null 2>&1 && pacman -Rdd pipewire-alsa --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q jack2 >/dev/null 2>&1 && pacman -Rdd jack2 --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q lib32-jack2 >/dev/null 2>&1 && pacman -Rdd lib32-jack2 --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q jack2-dbus >/dev/null 2>&1 && pacman -Rdd jack2-dbus --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q carla >/dev/null 2>&1 && pacman -Rdd carla --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q qjackctl >/dev/null 2>&1 && pacman -Rdd qjackctl --noconfirm; exit 0"
        install_pacman_chroot_with_retry "pulseaudio"
        install_pacman_chroot_with_retry "pulseaudio-alsa"
        install_pacman_chroot_with_retry "pavucontrol"
        ;;
    "Jack2")
        chroot /mnt /bin/bash -c "pacman -Q pipewire >/dev/null 2>&1 && pacman -Rdd pipewire --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pipewire-pulse >/dev/null 2>&1 && pacman -Rdd pipewire-pulse --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pipewire-alsa >/dev/null 2>&1 && pacman -Rdd pipewire-alsa --noconfirm; exit 0"
        chroot /mnt /bin/bash -c "pacman -Q pipewire-jack >/dev/null 2>&1 && pacman -Rdd pipewire-jack --noconfirm; exit 0"
        install_pacman_chroot_with_retry "jack2"
        install_pacman_chroot_with_retry "lib32-jack2"
        install_pacman_chroot_with_retry "jack2-dbus"
        install_pacman_chroot_with_retry "carla"
        install_pacman_chroot_with_retry "qjackctl"
        ;;
esac

clear
