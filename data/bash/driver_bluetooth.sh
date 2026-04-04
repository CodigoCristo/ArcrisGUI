# Instalación de drivers de Bluetooth
echo -e "${GREEN}| Instalando drivers de Bluetooth: $DRIVER_BLUETOOTH |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_BLUETOOTH" in
    "Ninguno")
        echo "Sin soporte Bluetooth"
        ;;
    "bluetoothctl (terminal)")
        install_pacman_chroot_with_retry "bluez"
        install_pacman_chroot_with_retry "bluez-utils"
        chroot /mnt /bin/bash -c "systemctl enable bluetooth" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
        ;;
    "blueman (Graphical)")
        install_pacman_chroot_with_retry "bluez"
        install_pacman_chroot_with_retry "bluez-utils"
        install_pacman_chroot_with_retry "blueman"
        chroot /mnt /bin/bash -c "systemctl enable bluetooth" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
        ;;
esac
sleep 2
clear
