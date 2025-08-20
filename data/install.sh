#!/bin/bash

# Importar variables de configuración
source "$(dirname "$0")/variables.sh"



# Verificar privilegios de root y ejecutar con sudo su si es necesario
if [ "$EUID" -ne 0 ]; then
    echo -e "\033[1;33mEste script requiere privilegios de root.\033[0m"
    echo -e "\033[0;36mEjecutando con sudo su...\033[0m"
    echo ""
    exec sudo su -c "bash '$0'"
fi

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

# Configuración inicial del LiveCD
echo -e "${GREEN}| Configurando LiveCD |${NC}"
echo ""

# Configuración de zona horaria
timedatectl set-timezone $TIMEZONE
hwclock -w
hwclock --hctosys
#hwclock --systohc

# Configuración de locale
echo "$LOCALE.UTF-8 UTF-8" > /etc/locale.gen
locale-gen
export LANG=$LOCALE.UTF-8

sleep 2
timedatectl status
echo ""
date +' %A, %B %d, %Y - %r'
sleep 5
clear

# Actualización de keys
echo -e "${GREEN}| Actualizando lista de Keys en LiveCD |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
pacman -Sy archlinux-keyring --noconfirm
sleep 2
clear

# Instalación de herramientas necesarias
sleep 3
pacman -Sy reflector --noconfirm
pacman -Sy python3 --noconfirm
pacman -Sy rsync --noconfirm
clear

# Actualización de mirrorlist
echo -e "${GREEN}| Actualizando mejores listas de Mirrors |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist
sleep 3
clear
cat /etc/pacman.d/mirrorlist
sleep 3
clear

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
echo -e "${GREEN}| Firmware detectado: $FIRMWARE_TYPE |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
sleep 2
clear

# Función para particionado automático ext4
partition_auto() {
    echo -e "${GREEN}| Particionando automáticamente disco: $SELECTED_DISK (EXT4) |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI
        echo -e "${GREEN}| Configurando particiones para UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script --align optimal mklabel gpt

        # Crear partición EFI (512MB)
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 513MiB 100%

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkfs.ext4 -F ${SELECTED_DISK}2
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}2 /mnt
        mkdir -p /mnt/boot
        mount ${SELECTED_DISK}1 /mnt/boot

    else
        # Configuración para BIOS Legacy
        echo -e "${GREEN}| Configurando particiones para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición root (todo el disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 1MiB 100%
        parted $SELECTED_DISK --script set 1 boot on

        # Formatear partición
        echo -e "${GREEN}| Formateando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.ext4 -F ${SELECTED_DISK}1
        sleep 2

        # Montar partición
        echo -e "${GREEN}| Montando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}1 /mnt
    fi
}

# Función para particionado automático btrfs
partition_auto_btrfs() {
    echo -e "${GREEN}| Particionando automáticamente disco: $SELECTED_DISK (BTRFS) |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI
        echo -e "${GREEN}| Configurando particiones BTRFS para UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script --align optimal mklabel gpt

        # Crear partición EFI (512MB)
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 513MiB 100%

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BTRFS UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkfs.btrfs -f ${SELECTED_DISK}2
        sleep 2

        # Montar y crear subvolúmenes BTRFS
        echo -e "${GREEN}| Creando subvolúmenes BTRFS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}2 /mnt
        btrfs subvolume create /mnt/@
        btrfs subvolume create /mnt/@home
        btrfs subvolume create /mnt/@var
        btrfs subvolume create /mnt/@tmp
        umount /mnt

        # Montar subvolúmenes
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ ${SELECTED_DISK}2 /mnt
        mkdir -p /mnt/{boot,home,var,tmp}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}2 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}2 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}2 /mnt/tmp
        mount ${SELECTED_DISK}1 /mnt/boot

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs

    else
        # Configuración para BIOS Legacy
        echo -e "${GREEN}| Configurando particiones BTRFS para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición root (todo el disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 1MiB 100%
        parted $SELECTED_DISK --script set 1 boot on

        # Formatear partición
        echo -e "${GREEN}| Formateando particiones BTRFS BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.btrfs -f ${SELECTED_DISK}1
        sleep 2

        # Montar y crear subvolúmenes BTRFS
        echo -e "${GREEN}| Creando subvolúmenes BTRFS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}1 /mnt
        btrfs subvolume create /mnt/@
        btrfs subvolume create /mnt/@home
        btrfs subvolume create /mnt/@var
        btrfs subvolume create /mnt/@tmp
        umount /mnt

        # Montar subvolúmenes
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ ${SELECTED_DISK}1 /mnt
        mkdir -p /mnt/{home,var,tmp}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}1 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}1 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}1 /mnt/tmp

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs
    fi
}

# Función para particionado con cifrado LUKS
partition_cifrado() {
    echo -e "${GREEN}| Particionando disco con cifrado LUKS: $SELECTED_DISK |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI con cifrado
        echo -e "${GREEN}| Configurando particiones cifradas para UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script --align optimal mklabel gpt

        # Crear partición EFI (512MB)
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición cifrada (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 513MiB 100%

        # Formatear partición EFI
        mkfs.fat -F32 -v ${SELECTED_DISK}1

        # Configurar LUKS
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat ${SELECTED_DISK}2 -
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open ${SELECTED_DISK}2 cryptroot -

        # Configurar LVM
        echo -e "${GREEN}| Configurando LVM |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        pvcreate /dev/mapper/cryptroot
        vgcreate vg0 /dev/mapper/cryptroot
        lvcreate -l 100%FREE vg0 -n root

        # Formatear y montar
        mkfs.ext4 /dev/vg0/root
        mount /dev/vg0/root /mnt
        mkdir -p /mnt/boot
        mount ${SELECTED_DISK}1 /mnt/boot

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2

    else
        # Configuración para BIOS Legacy con cifrado
        echo -e "${GREEN}| Configurando particiones cifradas para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición cifrada (todo el disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 1MiB 100%
        parted $SELECTED_DISK --script set 1 boot on

        # Configurar LUKS
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat ${SELECTED_DISK}1 -
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open ${SELECTED_DISK}1 cryptroot -

        # Configurar LVM
        echo -e "${GREEN}| Configurando LVM |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        pvcreate /dev/mapper/cryptroot
        vgcreate vg0 /dev/mapper/cryptroot
        lvcreate -l 100%FREE vg0 -n root

        # Formatear y montar
        mkfs.ext4 /dev/vg0/root
        mount /dev/vg0/root /mnt

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2
    fi
}

# Función para particionado manual
partition_manual() {
    echo -e "${GREEN}| Particionado manual detectado |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Procesar array de particiones
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        echo -e "${GREEN}| Procesando: $device -> $format -> $mountpoint |${NC}"

        # Formatear según el tipo especificado
        case $format in
            "mkfs.fat32")
                mkfs.fat -F32 -v $device
                ;;
            "mkfs.ext4")
                mkfs.ext4 -F $device
                ;;
            "mkfs.btrfs")
                mkfs.btrfs -f $device
                ;;
            "mkswap")
                mkswap $device
                swapon $device
                continue
                ;;
            *)
                echo -e "${RED}| Formato no reconocido: $format |${NC}"
                continue
                ;;
        esac

        # Montar en el punto especificado
        if [ "$mountpoint" = "/" ]; then
            mount $device /mnt
        else
            mkdir -p /mnt$mountpoint
            mount $device /mnt$mountpoint
        fi
    done

    lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT
    sleep 3
}

# Ejecutar particionado según el modo seleccionado
case "$PARTITION_MODE" in
    "auto")
        partition_auto
        ;;
    "auto_btrfs")
        partition_auto_btrfs
        ;;
    "cifrado")
        partition_cifrado
        ;;
    "manual")
        partition_manual
        ;;
    *)
        echo -e "${RED}| Modo de particionado no válido: $PARTITION_MODE |${NC}"
        exit 1
        ;;
esac

sleep 2

# Mostrar particiones montadas
echo -e "${GREEN}| Particiones montadas |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT | grep -E "(NAME|/mnt)"
sleep 3
clear

# Instalación de paquetes principales
echo -e "${GREEN}| Instalando paquetes principales de la distribución |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

pacstrap /mnt base
pacstrap /mnt base-devel
pacstrap /mnt reflector python3 rsync
pacstrap /mnt nano
pacstrap /mnt xdg-user-dirs
pacstrap /mnt fastfetch
clear

# Actualización de mirrors en el sistema instalado
arch-chroot /mnt /bin/bash -c "reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist"
clear
cat /mnt/etc/pacman.d/mirrorlist
sleep 3
clear

# Actualización del sistema instalado
arch-chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
cp /usr/share/arcrisgui/data/config/pacman.conf /mnt/etc/pacman.conf
arch-chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
arch-chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
sleep 5

# Generar fstab
genfstab -U /mnt > /mnt/etc/fstab
echo ""
arch-chroot /mnt /bin/bash -c "cat /etc/fstab"
sleep 3
clear

# Configuración del sistema
echo -e "${GREEN}| Configurando sistema base |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Configuración de zona horaria
arch-chroot /mnt /bin/bash -c "ln -sf /usr/share/zoneinfo/$TIMEZONE /etc/localtime"
arch-chroot /mnt /bin/bash -c "hwclock --systohc"

# Configuración de locale
echo "$LOCALE UTF-8" >> /mnt/etc/locale.gen
arch-chroot /mnt /bin/bash -c "locale-gen"
echo "LANG=$LOCALE" > /mnt/etc/locale.conf

# Configuración de teclado
echo "KEYMAP=$KEYMAP_TTY" > /mnt/etc/vconsole.conf
echo "FONT=lat9w-16" >> /mnt/etc/vconsole.conf

# Configuración de hostname
echo "$HOSTNAME" > /mnt/etc/hostname
cat > /mnt/etc/hosts << EOF
127.0.0.1	localhost
::1		localhost
127.0.1.1	$HOSTNAME.localdomain	$HOSTNAME
EOF

sleep 3
clear

# Instalación del kernel seleccionado
echo -e "${GREEN}| Instalando kernel: $SELECTED_KERNEL |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$SELECTED_KERNEL" in
    "linux")
        arch-chroot /mnt /bin/bash -c "pacman -S linux linux-firmware --noconfirm"
        ;;
    "linux-hardened")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-hardened linux-firmware --noconfirm"
        ;;
    "linux-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-lts linux-firmware --noconfirm"
        ;;
    "linux-rt-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-rt-lts linux-firmware --noconfirm"
        ;;
    "linux-zen")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-zen linux-firmware --noconfirm"
        ;;
    *)
        arch-chroot /mnt /bin/bash -c "pacman -S linux linux-firmware --noconfirm"
        ;;
esac

clear

# Instalación de drivers de video
echo -e "${GREEN}| Instalando drivers de video: $DRIVER_VIDEO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_VIDEO" in
    "Open Source")
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-vesa mesa --noconfirm"
        ;;
    "Nvidia Private")
        arch-chroot /mnt /bin/bash -c "pacman -S nvidia nvidia-utils --noconfirm"
        ;;
    "AMD Private")
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-amdgpu mesa --noconfirm"
        ;;
    "Intel Private")
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-intel mesa --noconfirm"
        ;;
    "Máquina Virtual")
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-vmware mesa --noconfirm"
        ;;
esac

clear

# Instalación de drivers de audio
echo -e "${GREEN}| Instalando drivers de audio: $DRIVER_AUDIO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_AUDIO" in
    "Alsa Audio")
        arch-chroot /mnt /bin/bash -c "pacman -S alsa-utils alsa-plugins --noconfirm"
        ;;
    "pipewire")
        arch-chroot /mnt /bin/bash -c "pacman -S pipewire pipewire-pulse pipewire-alsa --noconfirm"
        ;;
    "pulseaudio")
        arch-chroot /mnt /bin/bash -c "pacman -S pulseaudio pulseaudio-alsa pavucontrol --noconfirm"
        ;;
    "Jack2")
        arch-chroot /mnt /bin/bash -c "pacman -S jack2 --noconfirm"
        ;;
esac

clear

# Instalación de drivers de WiFi
echo -e "${GREEN}| Instalando drivers de WiFi: $DRIVER_WIFI |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_WIFI" in
    "Ninguno")
        echo "Sin drivers de WiFi"
        ;;
    "Open Source")
        arch-chroot /mnt /bin/bash -c "pacman -S networkmanager wpa_supplicant --noconfirm"
        ;;
    "broadcom-wl")
        arch-chroot /mnt /bin/bash -c "pacman -S broadcom-wl networkmanager --noconfirm"
        ;;
    "Realtek")
        arch-chroot /mnt /bin/bash -c "pacman -S rtl8821cu-morrownr-dkms-git networkmanager --noconfirm"
        ;;
esac

clear

# Instalación de drivers de Bluetooth
echo -e "${GREEN}| Instalando drivers de Bluetooth: $DRIVER_BLUETOOTH |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_BLUETOOTH" in
    "Ninguno")
        echo "Sin soporte Bluetooth"
        ;;
    "bluetoothctl (terminal)")
        arch-chroot /mnt /bin/bash -c "pacman -S bluez bluez-utils --noconfirm"
        arch-chroot /mnt /bin/bash -c "systemctl enable bluetooth"
        ;;
    "blueman (Graphical)")
        arch-chroot /mnt /bin/bash -c "pacman -S bluez bluez-utils blueman --noconfirm"
        arch-chroot /mnt /bin/bash -c "systemctl enable bluetooth"
        ;;
esac

clear

# Configuración de usuarios y contraseñas
echo -e "${GREEN}| Configurando usuarios |${NC}"
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

# Instalación de bootloader
if [ "$PARTITION_MODE" != "manual" ]; then
    echo -e "${GREEN}| Instalando bootloader |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        arch-chroot /mnt /bin/bash -c "pacman -S grub efibootmgr --noconfirm"
        arch-chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB"

        # Configuración especial para cifrado
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="cryptdevice='${SELECTED_DISK}'2:cryptroot root=\/dev\/vg0\/root loglevel=3 quiet"/' /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/' /mnt/etc/default/grub
        fi

        arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"
    else
        arch-chroot /mnt /bin/bash -c "pacman -S grub --noconfirm"
        arch-chroot /mnt /bin/bash -c "grub-install --target=i386-pc $SELECTED_DISK"

        # Configuración especial para cifrado
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="cryptdevice='${SELECTED_DISK}'1:cryptroot root=\/dev\/vg0\/root loglevel=3 quiet"/' /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/' /mnt/etc/default/grub
        fi

        arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"
    fi
else
    echo -e "${GREEN}| Modo manual: Bootloader debe instalarse manualmente |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2
fi
clear

# Instalación de herramientas de red
echo -e "${GREEN}| Instalando herramientas de red |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
arch-chroot /mnt /bin/bash -c "pacman -S dhcp dhcpcd dhclient networkmanager wpa_supplicant --noconfirm"
# Deshabilitar dhcpcd para evitar conflictos con NetworkManager
arch-chroot /mnt /bin/bash -c "systemctl disable dhcpcd"
arch-chroot /mnt /bin/bash -c "systemctl enable NetworkManager"
clear

# Copiado de archivos de configuración
echo -e "${GREEN}| Copiando archivos de configuración |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

cp /usr/share/arcrisgui/data/config/bashrc /mnt/home/$USER/.bashrc
cp /usr/share/arcrisgui/data/config/bashrc /mnt/home/$USER/.bashrc
cp /usr/share/arcrisgui/data/config/bashrc-root /mnt/root/.bashrc
cp /usr/share/arcrisgui/data/config/zshrc /mnt/home/$USER/.zshrc

# Configurar permisos de archivos de usuario
arch-chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.bashrc"
arch-chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.zshrc"

sleep 2
clear

# Configuración final del sistema
echo -e "${GREEN}| Configuración final del sistema |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Configurar directorios de usuario
arch-chroot /mnt /bin/bash -c "su - $USER -c 'xdg-user-dirs-update'"

# Configuración especial para cifrado
if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}| Configurando mkinitcpio para cifrado |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Instalar paquetes adicionales para cifrado
    arch-chroot /mnt /bin/bash -c "pacman -S cryptsetup lvm2 --noconfirm"

    # Configurar mkinitcpio para cifrado con hooks en orden correcto
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block encrypt lvm2 filesystems fsck)/' /mnt/etc/mkinitcpio.conf

    # Regenerar initramfs
    arch-chroot /mnt /bin/bash -c "mkinitcpio -P"

    # Configurar crypttab si es necesario
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        echo "cryptroot UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2) none luks" >> /mnt/etc/crypttab
    else
        echo "cryptroot UUID=$(blkid -s UUID -o value ${SELECTED_DISK}1) none luks" >> /mnt/etc/crypttab
    fi

    sleep 2
fi

# Actualizar base de datos de paquetes
arch-chroot /mnt /bin/bash -c "pacman -Sy"

ls /mnt/home/$USER/
sleep 5
clear

# Mostrar resumen final
echo -e "${GREEN}"
echo "  ╔════════════════════════════════════════╗"
echo "  ║        ✓ ARCRIS LINUX INSTALADO        ║"
echo "  ╚════════════════════════════════════════╝"
echo -e "${NC}"

echo ""
echo -e "${YELLOW}IMPORTANTE:${NC}"
echo -e "${CYAN}• Reinicia el sistema y retira el medio de instalación${NC}"
echo -e "${CYAN}• El sistema iniciará con GRUB${NC}"
if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${CYAN}• Se solicitará la contraseña de cifrado al iniciar${NC}"
fi
echo -e "${CYAN}• Puedes iniciar sesión con:${NC}"
echo -e "  Usuario: ${GREEN}$USER${NC}"
echo -e "  Contraseña: ${GREEN}$PASSWORD_USER${NC}"
echo ""

# Barra de progreso final
titulo_progreso="| Finalizando instalación de ARCRIS LINUX |"
barra_progreso

echo -e "${GREEN}✓ Instalación de ARCRIS LINUX completada exitosamente!${NC}"
