#!/bin/bash

# Importar variables de configuración
source "$(dirname "$0")/variables.sh"

# Colores
RED='\033[0;31m'
BOLD_RED='\033[1;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Función para imprimir en rojo
print_red() {
    echo -e "${BOLD_RED}$1${NC}"
}

# Función para imprimir en color
print_color() {
    echo -e "$1$2${NC}"
}

# Función para mostrar barra de progreso
barra_progreso() {
    local duration=5
    local steps=50
    local step_duration=$(echo "scale=3; $duration/$steps" | bc -l 2>/dev/null || echo "0.1")

    echo -e "\n${CYAN}${titulo_progreso:-Cargando...}${NC}"
    printf "["

    for ((i=0; i<=steps; i++)); do
        # Calcular porcentaje
        local percent=$((i * 100 / steps))

        # Mostrar barra
        printf "\r["
        for ((j=0; j<i; j++)); do
            printf "${GREEN}█${NC}"
        done
        for ((j=i; j<steps; j++)); do
            printf " "
        done
        printf "] ${YELLOW}%d%%${NC} " "$percent"

        # Esperar
        sleep $(echo "$step_duration" | bc -l 2>/dev/null || echo "0.1")
    done
    echo -e "\n${GREEN}✓ Completado!${NC}\n"
}

clear
echo ""
echo ""

# Mostrar logo ARCRIS
echo -e "${CYAN}"
echo " █████╗ ██████╗  ██████╗██████╗ ██╗███████╗";
echo "██╔══██╗██╔══██╗██╔════╝██╔══██╗██║██╔════╝";
echo "███████║██████╔╝██║     ██████╔╝██║███████╗";
echo "██╔══██║██╔══██╗██║     ██╔══██╗██║╚════██║";
echo "██║  ██║██║  ██║╚██████╗██║  ██║██║███████║";
echo "╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝╚══════╝";
echo -e "${NC}"
echo ""

# Mostrar resumen de variables
echo -e "${YELLOW}=== RESUMEN DE CONFIGURACIÓN ===${NC}"
echo ""
echo -e "${GREEN}Usuario:${NC} $USER"
echo -e "${GREEN}Hostname:${NC} $HOSTNAME"
echo -e "${GREEN}Zona Horaria:${NC} $TIMEZONE"
echo -e "${GREEN}Teclado:${NC} $KEYBOARD_LAYOUT"
echo -e "${GREEN}Locale:${NC} $LOCALE"
echo -e "${GREEN}Disco Seleccionado:${NC} $SELECTED_DISK"
echo -e "${GREEN}Kernel:${NC} $SELECTED_KERNEL"
echo -e "${GREEN}Tipo de Instalación:${NC} $INSTALLATION_TYPE"
echo ""
echo -e "${BLUE}Drivers configurados:${NC}"
echo -e "  Video: $DRIVER_VIDEO"
echo -e "  Audio: $DRIVER_AUDIO"
echo -e "  WiFi: $DRIVER_WIFI"
echo -e "  Bluetooth: $DRIVER_BLUETOOTH"
echo ""

# Barra de progreso para cargar variables (5 segundos)
titulo_progreso="| Cargando Variables de Configuración |"
barra_progreso

clear

# Mostrar logo line de Arch Linux
echo -e "${BLUE}"
echo '                             -`                        '
echo '                           .o+`                        '
echo '                           `ooo/                       '
echo '                          `+oooo:                      '
echo '                         `+oooooo:                     '
echo '                         -+oooooo+:                    '
echo '                       `/:-:++oooo+:                   '
echo '                      `/++++/+++++++:                  '
echo '                     `/++++++++++++++:                 '
echo '                    `/+++ooooooooooooo/`               '
echo '                   ./ooosssso++osssssso+`              '
echo '                  .oossssso-````/ossssss+`             '
echo '                 -osssssso.      :ssssssso.            '
echo '                :osssssss/        osssso+++.           '
echo '               /ossssssss/        +ssssooo/-           '
echo '             `/ossssso+/:-        -:/+osssso+-         '
echo '            `+sso+:-`                 `.-/+oso:        '
echo '           `++:.                           `-/+/       '
echo '           .`                                 `/       '
echo "                          _                  _         "
echo "  .--.                   / \   _ __ ___ _ __(_)___     "
echo " / _.-' .-.  .-.  .-.   / _ \ | '__/ __| '__| / __|    "
echo " \  '-. '-'  '-'  '-'  / ___ \| | | (__| |  | \__ \    "
echo "  '--'                /_/   \_\_|  \___|_|  |_|___/    "
echo -e "${NC}"
echo ""

# Configuración de zona horaria
zonahoraria="$TIMEZONE"

echo -e "\t\t\t| Configurando Zona Horaria: $zonahoraria |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

timedatectl set-timezone $zonahoraria
hwclock -w
hwclock --hctosys
hwclock --systohc

echo -e "\t\t\t| Actualizando Hora Actual en LiveCD |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo -e ""
sleep 2
timedatectl status
echo ""
date +' %A, %B %d, %Y - %r'
sleep 5
clear

echo ""
echo -e ""
echo -e "\t\t\t| Actualizando lista de Keys en LiveCD |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo -e ""
sleep 2
pacman -Sy archlinux-keyring --noconfirm
sleep 2
clear

echo ""
echo -e ""
echo -e "\t\t\t| Actualizando MirrorList en LiveCD |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo -e ""
sleep 3
pacman -Sy reflector --noconfirm
pacman -Sy python3 --noconfirm
pacman -Sy rsync --noconfirm
clear

echo -e "\t\t\t| Actualizando mejores listas de Mirrors |"
echo ""
reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist
sleep 3
clear
cat /etc/pacman.d/mirrorlist
sleep 3
clear

titulo_progreso="| Instalando: Base y Base-devel |"
barra_progreso
pacstrap /mnt base
pacstrap /mnt base-devel
pacstrap /mnt reflector python3 rsync
pacstrap /mnt nano
pacstrap /mnt xdg-user-dirs
clear

titulo_progreso="| Actualizando mejores listas de Mirrors del sistema instalado |"
barra_progreso
arch-chroot /mnt /bin/bash -c "reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist"
clear
cat /mnt/etc/pacman.d/mirrorlist
sleep 3
clear
echo ""

# Función para detectar tipo de firmware
detect_firmware() {
    if [ -d /sys/firmware/efi ]; then
        echo "UEFI"
    else
        echo "BIOS"
    fi
}

# Detectar tipo de firmware
FIRMWARE_TYPE=$(detect_firmware)
echo -e "\t\t\t| Firmware detectado: $FIRMWARE_TYPE |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
sleep 2
clear

# Verificar modo de particionado
if [ "$PARTITION_MODE" = "auto" ]; then
    # Particionado automático del disco
    echo -e "\t\t\t| Particionando automáticamente disco: $SELECTED_DISK |"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI
        echo -e "\t\t\t| Configurando particiones para UEFI |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script mklabel gpt

        # Crear partición EFI (512MB)
        parted $SELECTED_DISK --script mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script mkpart primary ext4 513MiB 100%

        # Formatear particiones
        echo -e "\t\t\t| Formateando particiones UEFI |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 ${SELECTED_DISK}1
        mkfs.ext4 ${SELECTED_DISK}2
        sleep 2

        # Montar particiones
        echo -e "\t\t\t| Montando particiones UEFI |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}2 /mnt
        mkdir -p /mnt/boot
        mount ${SELECTED_DISK}1 /mnt/boot

    else
        # Configuración para BIOS Legacy
        echo -e "\t\t\t| Configurando particiones para BIOS Legacy |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script mklabel msdos

        # Crear partición root (todo el disco)
        parted $SELECTED_DISK --script mkpart primary ext4 1MiB 100%
        parted $SELECTED_DISK --script set 1 boot on

        # Formatear partición
        echo -e "\t\t\t| Formateando particiones BIOS |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.ext4 ${SELECTED_DISK}1
        sleep 2

        # Montar partición
        echo -e "\t\t\t| Montando particiones BIOS |"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}1 /mnt
    fi

    sleep 2
    clear
else
    echo -e "\t\t\t| Modo de particionado manual detectado |"
    echo -e "\t\t\t| Asumiendo que las particiones ya están montadas en /mnt |"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 3
    clear
fi

# Generar fstab
echo -e "\t\t\t| Generando fstab |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
genfstab -U /mnt >> /mnt/etc/fstab
sleep 2
clear

# Configuración del sistema instalado
echo -e "\t\t\t| Configurando zona horaria: $TIMEZONE |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
arch-chroot /mnt /bin/bash -c "ln -sf /usr/share/zoneinfo/$TIMEZONE /etc/localtime"
arch-chroot /mnt /bin/bash -c "hwclock --systohc"
sleep 2
clear

# Configuración de locale
echo -e "\t\t\t| Configurando locale: $LOCALE |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
echo "$LOCALE UTF-8" >> /mnt/etc/locale.gen
arch-chroot /mnt /bin/bash -c "locale-gen"
echo "LANG=$LOCALE" > /mnt/etc/locale.conf
sleep 2
clear

# Configuración de teclado
echo -e "\t\t\t| Configurando teclado: $KEYBOARD_LAYOUT |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
echo "KEYMAP=$KEYMAP_TTY" > /mnt/etc/vconsole.conf
echo "FONT=lat9w-16" >> /mnt/etc/vconsole.conf
sleep 2
clear

# Configuración de hostname
echo -e "\t\t\t| Configurando hostname: $HOSTNAME |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
echo "$HOSTNAME" > /mnt/etc/hostname
cat > /mnt/etc/hosts << EOF
127.0.0.1	localhost
::1		localhost
127.0.1.1	$HOSTNAME.localdomain	$HOSTNAME
EOF
sleep 2
clear

# Instalación de kernel y paquetes adicionales
titulo_progreso="| Instalando kernel: $SELECTED_KERNEL |"
barra_progreso
arch-chroot /mnt /bin/bash -c "pacman -S $SELECTED_KERNEL linux-firmware --noconfirm"
clear

# Instalación de bootloader según tipo de firmware (solo en modo automático)
if [ "$PARTITION_MODE" = "auto" ]; then
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        titulo_progreso="| Instalando bootloader GRUB para UEFI |"
        barra_progreso
        arch-chroot /mnt /bin/bash -c "pacman -S grub efibootmgr --noconfirm"
        arch-chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB"
        arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"
    else
        titulo_progreso="| Instalando bootloader GRUB para BIOS Legacy |"
        barra_progreso
        arch-chroot /mnt /bin/bash -c "pacman -S grub --noconfirm"
        arch-chroot /mnt /bin/bash -c "grub-install --target=i386-pc $SELECTED_DISK"
        arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"
    fi
else
    echo -e "\t\t\t| Modo manual: Bootloader debe instalarse manualmente |"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2
fi
clear

# Configuración de usuarios y contraseñas
echo -e "\t\t\t| Configurando usuarios |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Configurar contraseña de root
echo "root:$PASSWORD_ROOT" | arch-chroot /mnt /bin/bash -c "chpasswd"

# Crear usuario
arch-chroot /mnt /bin/bash -c "useradd -m -G wheel,audio,video,optical,storage -s /bin/bash $USER"
echo "$USER:$PASSWORD_USER" | arch-chroot /mnt /bin/bash -c "chpasswd"

# Configurar sudo
arch-chroot /mnt /bin/bash -c "pacman -S sudo --noconfirm"
echo "%wheel ALL=(ALL) ALL" >> /mnt/etc/sudoers

sleep 2
clear

# Instalación de drivers según configuración
if [ "$DRIVER_AUDIO" = "Alsa Audio" ]; then
    titulo_progreso="| Instalando drivers de audio ALSA |"
    barra_progreso
    arch-chroot /mnt /bin/bash -c "pacman -S alsa-utils alsa-plugins --noconfirm"
    clear
fi

if [ "$DRIVER_VIDEO" = "Open Source" ]; then
    titulo_progreso="| Instalando drivers de video Open Source |"
    barra_progreso
    arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-vesa mesa --noconfirm"
    clear
fi

# Instalación de herramientas de red
titulo_progreso="| Instalando herramientas de red |"
barra_progreso
arch-chroot /mnt /bin/bash -c "pacman -S dhcpcd networkmanager --noconfirm"
arch-chroot /mnt /bin/bash -c "systemctl enable dhcpcd"
arch-chroot /mnt /bin/bash -c "systemctl enable NetworkManager"
clear

# Configuración final del sistema
echo -e "\t\t\t| Configuración final del sistema |"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Habilitar multilib si es necesario
sed -i '/\[multilib\]/,/Include/s/^#//' /mnt/etc/pacman.conf

# Actualizar base de datos de paquetes
arch-chroot /mnt /bin/bash -c "pacman -Sy"

# Configurar directorios de usuario
arch-chroot /mnt /bin/bash -c "su - $USER -c 'xdg-user-dirs-update'"

sleep 3
clear

# Mostrar resumen final
echo -e "${GREEN}"
echo "  ╔════════════════════════════════════════╗"
echo "  ║                                        ║"
echo "  ║    ✓ ARCH LINUX INSTALADO              ║"
echo "  ║                                        ║"
echo "  ║    Configuración completada:           ║"
echo "  ║    • Usuario: $USER                   ║"
echo "  ║    • Hostname: $HOSTNAME              ║"
echo "  ║    • Zona horaria: $TIMEZONE          ║"
echo "  ║    • Locale: $LOCALE                  ║"
echo "  ║    • Teclado: $KEYBOARD_LAYOUT        ║"
echo "  ║    • Disco: $SELECTED_DISK            ║"
echo "  ║    • Kernel: $SELECTED_KERNEL         ║"
echo "  ║    • Firmware: $FIRMWARE_TYPE         ║"
echo "  ║    • Particionado: $PARTITION_MODE    ║"
echo "  ║                                        ║"
echo "  ║    El sistema está listo para usar     ║"
echo "  ║                                        ║"
echo "  ╚════════════════════════════════════════╝"
echo -e "${NC}"

echo ""
echo -e "${YELLOW}IMPORTANTE:${NC}"
echo -e "${CYAN}• Reinicia el sistema y retira el medio de instalación${NC}"
echo -e "${CYAN}• El sistema iniciará con GRUB${NC}"
echo -e "${CYAN}• Puedes iniciar sesión con:${NC}"
echo -e "  Usuario: ${GREEN}$USER${NC}"
echo -e "  Contraseña: ${GREEN}$PASSWORD_USER${NC}"
echo ""

# Barra de progreso final
titulo_progreso="| Finalizando instalación de ARCRIS LINUX |"
barra_progreso

echo -e "${GREEN}✓ Instalación de ARCRIS LINUX completada exitosamente!${NC}"
