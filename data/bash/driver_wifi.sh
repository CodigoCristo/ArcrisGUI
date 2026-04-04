# Instalación de drivers de WiFi
echo -e "${GREEN}| Instalando drivers de WiFi: $DRIVER_WIFI |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_WIFI" in
    "Ninguno")
        echo "Sin drivers de WiFi"
        ;;
    "Open Source")
        install_pacman_chroot_with_retry "wpa_supplicant"
        install_pacman_chroot_with_retry "wireless_tools"
        install_pacman_chroot_with_retry "iw"
        ;;
    "broadcom-wl")
        install_pacman_chroot_with_retry "wpa_supplicant"
        install_pacman_chroot_with_retry "wireless_tools"
        install_pacman_chroot_with_retry "iw"
        install_pacman_chroot_with_retry "broadcom-wl"
        ;;
    "Realtek")
        install_pacman_chroot_with_retry "wpa_supplicant"
        install_pacman_chroot_with_retry "wireless_tools"
        install_pacman_chroot_with_retry "iw"
        install_yay_chroot_with_retry "rtl8821cu-dkms-git"
        install_yay_chroot_with_retry "rtl8821ce-dkms-git"
        install_yay_chroot_with_retry "rtw88-dkms-git"
        ;;
esac
