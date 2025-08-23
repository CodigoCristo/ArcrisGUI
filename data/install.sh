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
echo "█CRISTO VIVE3333█║  ██║██║  ██║╚██████╗██║  ██║██║███████║";
echo "╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝╚══════╝";
echo -e "${NC}"
echo ""

# Configuración inicial del LiveCD
echo -e "${GREEN}| Configurando LiveCD |${NC}"
echo ""

# Configuración de zona horaria
timedatectl set-timezone $TIMEZONE
sudo hwclock -w
sudo hwclock --systohc --rtc=/dev/rtc0

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
barra_progreso
sudo reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist
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

        # Crear partición swap (8GB)
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 513MiB 8705MiB

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 8705MiB 100%

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkswap ${SELECTED_DISK}2
        mkfs.ext4 -F ${SELECTED_DISK}3
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}3 /mnt
        swapon ${SELECTED_DISK}2
        mkdir -p /mnt/boot/efi
        mount ${SELECTED_DISK}1 /mnt/boot/efi

    else
        # Configuración para BIOS Legacy
        echo -e "${GREEN}| Configurando particiones para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición swap (8GB)
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 1MiB 8193MiB

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 8193MiB 100%
        parted $SELECTED_DISK --script set 2 boot on

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkswap ${SELECTED_DISK}1
        mkfs.ext4 -F ${SELECTED_DISK}2
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}2 /mnt
        swapon ${SELECTED_DISK}1
        mkdir -p /mnt/boot
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

        # Crear partición swap (8GB)
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 513MiB 8705MiB

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 8705MiB 100%

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BTRFS UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkswap ${SELECTED_DISK}2
        mkfs.btrfs -f ${SELECTED_DISK}3
        sleep 2

        # Montar y crear subvolúmenes BTRFS
        echo -e "${GREEN}| Creando subvolúmenes BTRFS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}3 /mnt
        btrfs subvolume create /mnt/@
        btrfs subvolume create /mnt/@home
        btrfs subvolume create /mnt/@var
        btrfs subvolume create /mnt/@tmp
        umount /mnt

        # Montar subvolúmenes
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ ${SELECTED_DISK}3 /mnt
        swapon ${SELECTED_DISK}2
        mkdir -p /mnt/{boot/efi,home,var,tmp}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}3 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}3 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}3 /mnt/tmp
        mount ${SELECTED_DISK}1 /mnt/boot/efi

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs

    else
        # Configuración para BIOS Legacy
        echo -e "${GREEN}| Configurando particiones BTRFS para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición swap (8GB)
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 1MiB 8193MiB

        # Crear partición root (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 8193MiB 100%
        parted $SELECTED_DISK --script set 2 boot on

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BTRFS BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkswap ${SELECTED_DISK}1
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
        swapon ${SELECTED_DISK}1
        mkdir -p /mnt/{boot,home,var,tmp}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}2 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}2 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}2 /mnt/tmp

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs
    fi
}

# Función para particionado con cifrado LUKS
partition_cifrado() {
    echo -e "${GREEN}| Particionando disco con cifrado LUKS: $SELECTED_DISK |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    echo -e "${CYAN}NOTA IMPORTANTE: Esta configuración implementa LUKS+LVM siguiendo mejores prácticas:${NC}"
    echo -e "${CYAN}  • Solo la partición EFI/boot queda sin cifrar (necesario para el bootloader)${NC}"
    echo -e "${CYAN}  • LUKS cifra toda la partición principal${NC}"
    echo -e "${CYAN}  • LVM se ejecuta sobre LUKS para flexibilidad en particiones${NC}"
    echo -e "${CYAN}  • CRITICAL: Guarda bien tu contraseña LUKS - sin ella perderás todos los datos${NC}"
    echo ""
    sleep 3

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI con cifrado (siguiendo mejores prácticas)
        echo -e "${GREEN}| Configurando particiones cifradas para UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script --align optimal mklabel gpt

        # Crear partición EFI (512MB) - única partición sin cifrar
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición principal cifrada (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 513MiB 100%

        # Formatear partición EFI
        mkfs.fat -F32 -v ${SELECTED_DISK}1

        # Configurar LUKS en la partición principal
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -e "${CYAN}Aplicando cifrado LUKS a ${SELECTED_DISK}2...${NC}"
        echo -e "${YELLOW}IMPORTANTE: Esto puede tomar unos minutos dependiendo del tamaño del disco${NC}"
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat ${SELECTED_DISK}2 -
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open ${SELECTED_DISK}2 cryptlvm -

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup ${SELECTED_DISK}2 --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS
        echo -e "${GREEN}| Configurando LVM sobre LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -e "${CYAN}Creando Physical Volume sobre dispositivo cifrado...${NC}"
        pvcreate /dev/mapper/cryptlvm
        echo -e "${CYAN}Creando Volume Group 'vg0'...${NC}"
        vgcreate vg0 /dev/mapper/cryptlvm
        echo -e "${CYAN}Creando Logical Volume 'swap' de 8GB...${NC}"
        lvcreate -L 8G vg0 -n swap
        echo -e "${CYAN}Creando Logical Volume 'root' con el espacio restante...${NC}"
        lvcreate -l 100%FREE vg0 -n root

        echo -e "${GREEN}✓ Configuración LVM completada:${NC}"
        echo -e "${GREEN}  • Volume Group: vg0${NC}"
        echo -e "${GREEN}  • Swap: 8GB (/dev/vg0/swap)${NC}"
        echo -e "${GREEN}  • Root: Resto del espacio (/dev/vg0/root)${NC}"

        # Verificar que el volumen LVM esté disponible
        sleep 2
        vgchange -ay vg0

        # Formatear volúmenes LVM
        echo -e "${CYAN}Formateando volúmenes LVM...${NC}"
        mkfs.ext4 -F /dev/vg0/root
        mkswap /dev/vg0/swap

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        mount /dev/vg0/root /mnt
        swapon /dev/vg0/swap

        # Verificar que la partición EFI exista
        echo -e "${CYAN}Verificando partición EFI antes del montaje...${NC}"
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición EFI ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi

        # Esperar que las particiones estén completamente listas
        sleep 2
        sync

        # Montar partición EFI directamente en /boot
        echo -e "${CYAN}Creando directorio /boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición EFI en /boot...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición EFI${NC}"
            exit 1
        fi

        # Verificar que el montaje sea exitoso
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Todas las particiones montadas correctamente${NC}"
        echo -e "${GREEN}✓ Esquema LUKS+LVM configurado: Solo EFI sin cifrar, resto cifrado${NC}"

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2

    else
        # Configuración para BIOS Legacy con cifrado (siguiendo mejores prácticas)
        echo -e "${GREEN}| Configurando particiones cifradas para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición de boot sin cifrar (512MB) - mínima necesaria
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 boot on

        # Crear partición cifrada (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 513MiB 100%

        # Formatear partición boot
        mkfs.ext4 -F ${SELECTED_DISK}1

        # Configurar LUKS en la partición principal
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat ${SELECTED_DISK}2 -
        echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open ${SELECTED_DISK}2 cryptlvm -

        # Configurar LVM sobre LUKS
        echo -e "${GREEN}| Configurando LVM sobre LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        pvcreate /dev/mapper/cryptlvm
        vgcreate vg0 /dev/mapper/cryptlvm
        lvcreate -L 8G vg0 -n swap
        lvcreate -l 100%FREE vg0 -n root

        # Verificar que el volumen LVM esté disponible
        sleep 2
        vgchange -ay vg0

        # Formatear volúmenes LVM
        echo -e "${CYAN}Formateando volúmenes LVM...${NC}"
        mkfs.ext4 -F /dev/vg0/root
        mkswap /dev/vg0/swap

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        mount /dev/vg0/root /mnt
        swapon /dev/vg0/swap

        # Verificar que la partición boot exista
        echo -e "${CYAN}Verificando partición boot antes del montaje...${NC}"
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición boot ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi

        # Esperar que la partición esté completamente lista
        sleep 2
        sync

        # Montar partición boot
        echo -e "${CYAN}Creando directorio /boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición boot${NC}"
            exit 1
        fi

        # Verificar que el montaje sea exitoso
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Partición boot montada correctamente${NC}"

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
        arch-chroot /mnt /bin/bash -c "pacman -S linux --noconfirm"
        ;;
    "linux-hardened")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-hardened --noconfirm"
        ;;
    "linux-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-lts --noconfirm"
        ;;
    "linux-rt-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-rt-lts --noconfirm"
        ;;
    "linux-zen")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-zen --noconfirm"
        ;;
    *)
        arch-chroot /mnt /bin/bash -c "pacman -S linux --noconfirm"
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

# Configuración de mkinitcpio según el modo de particionado
echo -e "${GREEN}| Configurando mkinitcpio |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para cifrado LUKS+LVM...${NC}"

    # Configurar módulos específicos para LUKS+LVM (siguiendo mejores prácticas)
    echo -e "${CYAN}Configurando módulos del kernel para cifrado...${NC}"
    sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt dm_snapshot dm_mirror)/' /mnt/etc/mkinitcpio.conf

    # Configurar hooks para cifrado con LVM - orden crítico: encrypt antes de lvm2
    echo -e "${CYAN}Configurando hooks - ORDEN CRÍTICO: encrypt debe ir antes de lvm2${NC}"
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block encrypt lvm2 filesystems fsck)/' /mnt/etc/mkinitcpio.conf

    echo -e "${GREEN}✓ Configuración mkinitcpio actualizada para LUKS+LVM${NC}"
    echo -e "${CYAN}  • Módulos: dm_mod dm_crypt dm_snapshot dm_mirror${NC}"
    echo -e "${CYAN}  • Hooks: base udev autodetect keyboard keymap consolefont modconf block encrypt lvm2 filesystems fsck${NC}"
    echo -e "${YELLOW}  • IMPORTANTE: 'encrypt' DEBE ir antes de 'lvm2' para que funcione correctamente${NC}"
    echo -e "${YELLOW}  • keyboard y keymap son necesarios para introducir la contraseña en el boot${NC}"

elif [ "$PARTITION_MODE" = "btrfs" ]; then
    echo "Configurando mkinitcpio para BTRFS..."
    # Configurar módulos específicos para BTRFS
    sed -i 's/^MODULES=.*/MODULES=(btrfs crc32c-intel crc32c zstd_compress lzo_compress)/' /mnt/etc/mkinitcpio.conf

    # Configurar hooks para BTRFS
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf

else
    echo "Configurando mkinitcpio para sistema estándar..."
    # Configuración estándar para ext4
    sed -i 's/^MODULES=.*/MODULES=()/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
fi

# Regenerar initramfs
arch-chroot /mnt /bin/bash -c "mkinitcpio -P"
sleep 2

# Instalación de bootloader
if [ "$PARTITION_MODE" != "manual" ]; then
    echo -e "${GREEN}| Instalando bootloader |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Verificar que la partición EFI esté montada con debug adicional
        echo -e "${CYAN}Verificando montaje de partición EFI...${NC}"
        if ! mountpoint -q /mnt/boot/efi; then
            echo -e "${RED}ERROR: Partición EFI no está montada en /mnt/boot/efi${NC}"
            echo -e "${YELLOW}Información de debug:${NC}"
            echo "- Contenido de /mnt/boot:"
            ls -la /mnt/boot/ 2>/dev/null || echo "  Directorio /mnt/boot no accesible"
            echo "- Contenido de /mnt/boot/efi:"
            ls -la /mnt/boot/efi/ 2>/dev/null || echo "  Directorio /mnt/boot/efi no accesible"
            echo "- Montajes actuales:"
            mount | grep "/mnt"
            echo "- Particiones disponibles:"
            lsblk ${SELECTED_DISK}
            exit 1
        fi
        echo -e "${GREEN}✓ Partición EFI montada correctamente en /mnt/boot/efi${NC}"

        # Verificar sistema UEFI con debug
        echo -e "${CYAN}Verificando sistema UEFI...${NC}"
        if [ ! -d "/sys/firmware/efi" ]; then
            echo -e "${RED}ERROR: Sistema no está en modo UEFI${NC}"
            echo "- Directorio /sys/firmware/efi no existe"
            echo "- El sistema puede estar en modo BIOS Legacy"
            exit 1
        fi
        echo -e "${GREEN}✓ Sistema en modo UEFI confirmado${NC}"

        # Limpiar entradas UEFI previas que puedan causar conflictos
        echo -e "${CYAN}Limpiando entradas UEFI previas...${NC}"
        efibootmgr | grep -i grub | cut -d'*' -f1 | sed 's/Boot//' | xargs -I {} efibootmgr -b {} -B 2>/dev/null || true

        # Limpiar directorio EFI previo si existe
        if [ -d "/mnt/boot/efi/EFI/GRUB" ]; then
            rm -rf /mnt/boot/efi/EFI/GRUB
        fi

        # Crear directorio EFI si no existe
        mkdir -p /mnt/boot/efi/EFI

        echo -e "${CYAN}Instalando paquetes GRUB para UEFI...${NC}"
        arch-chroot /mnt /bin/bash -c "pacman -S grub efibootmgr --noconfirm"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada ${SELECTED_DISK}2${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Configurar GRUB para LUKS+LVM (siguiendo mejores prácticas de la guía)
            echo -e "${CYAN}Configurando parámetros de kernel para LUKS+LVM...${NC}"
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root\"/" /mnt/etc/default/grub

            # Habilitar soporte para discos cifrados en GRUB
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub

            # Precargar módulos necesarios para cifrado
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub

            echo -e "${GREEN}✓ Configuración GRUB para cifrado:${NC}"
            echo -e "${CYAN}  • cryptdevice=UUID=${CRYPT_UUID}:cryptlvm${NC}"
            echo -e "${CYAN}  • root=/dev/vg0/root${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y (permite a GRUB leer discos cifrados)${NC}"
        elif [ "$PARTITION_MODE" = "btrfs" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="rootflags=subvol=@ loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos btrfs\"" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos\"" >> /mnt/etc/default/grub
        fi

        echo -e "${CYAN}Instalando GRUB en partición EFI...${NC}"
        if ! arch-chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB --recheck --debug" 2>&1 | tee /tmp/grub-install.log; then
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI${NC}"
            echo -e "${YELLOW}Log de grub-install:${NC}"
            cat /tmp/grub-install.log
            echo -e "${YELLOW}Información adicional:${NC}"
            echo "- Estado de /boot:"
            ls -la /mnt/boot/
            echo "- Espacio disponible en /boot:"
            df -h /mnt/boot
            echo "- Espacio disponible en /boot/efi:"
            df -h /mnt/boot/efi
            exit 1
        fi

        # Verificar que grubx64.efi se haya creado con debug
        if [ ! -f "/mnt/boot/efi/EFI/GRUB/grubx64.efi" ]; then
            echo -e "${RED}ERROR: No se creó grubx64.efi${NC}"
            echo -e "${YELLOW}Información de debug:${NC}"
            echo "- Contenido de /mnt/boot/efi/EFI/:"
            ls -la /mnt/boot/efi/EFI/ 2>/dev/null || echo "  Directorio EFI no existe"
            echo "- Contenido de /mnt/boot/efi/EFI/GRUB/:"
            ls -la /mnt/boot/efi/EFI/GRUB/ 2>/dev/null || echo "  Directorio GRUB no existe"
            exit 1
        fi
        echo -e "${GREEN}✓ grubx64.efi creado exitosamente${NC}"

        echo -e "${CYAN}Generando configuración de GRUB...${NC}"
        if ! arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"; then
            echo -e "${RED}ERROR: Falló la generación de grub.cfg${NC}"
            exit 1
        fi

        # Verificar que grub.cfg se haya creado
        if [ ! -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${RED}ERROR: No se creó grub.cfg${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ GRUB UEFI instalado correctamente${NC}"
    else
        echo -e "${CYAN}Instalando paquetes GRUB para BIOS...${NC}"
        arch-chroot /mnt /bin/bash -c "pacman -S grub --noconfirm"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada ${SELECTED_DISK}2${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Usar GRUB_CMDLINE_LINUX en lugar de GRUB_CMDLINE_LINUX_DEFAULT para mejores prácticas
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root\"/" /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub
        elif [ "$PARTITION_MODE" = "btrfs" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="rootflags=subvol=@ loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos btrfs\"" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos\"" >> /mnt/etc/default/grub
        fi

        echo -e "${CYAN}Instalando GRUB en disco...${NC}"
        if ! arch-chroot /mnt /bin/bash -c "grub-install --target=i386-pc $SELECTED_DISK --recheck"; then
            echo -e "${RED}ERROR: Falló la instalación de GRUB BIOS${NC}"
            exit 1
        fi

        echo -e "${CYAN}Generando configuración de GRUB...${NC}"
        if ! arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"; then
            echo -e "${RED}ERROR: Falló la generación de grub.cfg${NC}"
            exit 1
        fi

        # Verificar que grub.cfg se haya creado
        if [ ! -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${RED}ERROR: No se creó grub.cfg${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ GRUB BIOS instalado correctamente${NC}"
    fi
else
    echo -e "${GREEN}| Modo manual: Bootloader debe instalarse manualmente |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    echo -e "${YELLOW}Para instalar GRUB manualmente:${NC}"
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        echo "pacman -S grub efibootmgr"
        echo "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB --recheck"
    else
        echo "pacman -S grub"
        echo "grub-install --target=i386-pc $SELECTED_DISK --recheck"
    fi
    echo "grub-mkconfig -o /boot/grub/grub.cfg"
    echo ""
    echo -e "${CYAN}Verificar después de la instalación:${NC}"
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        echo "ls /boot/efi/EFI/GRUB/grubx64.efi"
    fi
    echo "ls /boot/grub/grub.cfg"
    sleep 3
fi

# Verificación final del bootloader
if [ "$PARTITION_MODE" != "manual" ]; then
    echo -e "${GREEN}| Verificación final del bootloader |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        if [ -f "/mnt/boot/efi/EFI/GRUB/grubx64.efi" ] && [ -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${GREEN}✓ Bootloader UEFI verificado correctamente${NC}"

            # Crear entrada UEFI manualmente si no existe
            if ! efibootmgr | grep -q "GRUB"; then
                echo -e "${CYAN}Creando entrada UEFI para GRUB...${NC}"
                efibootmgr --disk $SELECTED_DISK --part 1 --create --label "GRUB" --loader '\EFI\GRUB\grubx64.efi'

                # Hacer que GRUB sea la primera opción de boot
                GRUB_NUM=$(efibootmgr | grep "GRUB" | head -1 | cut -d'*' -f1 | sed 's/Boot//')
                if [ -n "$GRUB_NUM" ]; then
                    CURRENT_ORDER=$(efibootmgr | grep BootOrder | cut -d' ' -f2)
                    NEW_ORDER="$GRUB_NUM,${CURRENT_ORDER//$GRUB_NUM,/}"
                    NEW_ORDER="${NEW_ORDER//,,/,}"
                    NEW_ORDER="${NEW_ORDER%,}"
                    efibootmgr --bootorder "$NEW_ORDER" 2>/dev/null || true
                fi
            fi
        else
            echo -e "${RED}⚠ Problema con la instalación del bootloader UEFI${NC}"
        fi
    else
        if [ -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${GREEN}✓ Bootloader BIOS verificado correctamente${NC}"
        else
            echo -e "${RED}⚠ Problema con la instalación del bootloader BIOS${NC}"
        fi
    fi
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
# Configuración adicional para cifrado
if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}| Configuración adicional para cifrado |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Configurar crypttab
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
    else
        CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
    fi
    echo "cryptlvm UUID=${CRYPT_UUID} none luks,discard" >> /mnt/etc/crypttab
    echo -e "${GREEN}✓ Configuración crypttab creada para montaje automático${NC}"

    # Crear archivo de configuración para LVM
    echo "# LVM devices for encrypted setup" > /mnt/etc/lvm/lvm.conf.local
    echo -e "${CYAN}Configuración LVM aplicada para sistema cifrado${NC}"
    echo "activation {" >> /mnt/etc/lvm/lvm.conf.local
    echo "    udev_sync = 1" >> /mnt/etc/lvm/lvm.conf.local
    echo "    udev_rules = 1" >> /mnt/etc/lvm/lvm.conf.local
    echo "}" >> /mnt/etc/lvm/lvm.conf.local

    # Verificar que los servicios LVM estén habilitados
    arch-chroot /mnt /bin/bash -c "systemctl enable lvm2-monitor.service"

    # Configuración adicional para reducir timeouts de cifrado y LVM
    echo -e "${CYAN}Aplicando optimizaciones para sistema cifrado...${NC}"

    # Asegurar que LVM esté disponible y activo
    echo -e "${CYAN}Activando volumes LVM...${NC}"
    arch-chroot /mnt /bin/bash -c "vgchange -ay vg0"
    arch-chroot /mnt /bin/bash -c "lvchange -ay vg0/root"
    arch-chroot /mnt /bin/bash -c "lvchange -ay vg0/swap"

    # Generar fstab correctamente con nombres de dispositivos apropiados
    echo -e "${CYAN}Generando fstab con dispositivos LVM...${NC}"
    # Limpiar fstab existente
    > /mnt/etc/fstab
    # Agregar entradas manualmente para asegurar nombres correctos
    echo "# <file system> <mount point> <type> <options> <dump> <pass>" >> /mnt/etc/fstab
    echo "/dev/mapper/vg0-root / ext4 rw,relatime 0 1" >> /mnt/etc/fstab
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}1) /boot/efi vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2) /boot ext4 rw,relatime 0 2" >> /mnt/etc/fstab
    else
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}1) /boot ext4 rw,relatime 0 2" >> /mnt/etc/fstab
    fi
    echo "/dev/mapper/vg0-swap none swap defaults 0 0" >> /mnt/etc/fstab

    # Regenerar initramfs después de todas las configuraciones
    echo -e "${CYAN}Regenerando initramfs con configuración LVM...${NC}"
    arch-chroot /mnt /bin/bash -c "mkinitcpio -P"

    # Regenerar configuración de GRUB con parámetros LVM
    echo -e "${CYAN}Regenerando configuración de GRUB...${NC}"
    arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"

    sleep 2
fi

# Configuración adicional para BTRFS
if [ "$PARTITION_MODE" = "btrfs" ]; then
    echo -e "${GREEN}| Configuración adicional para BTRFS |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Habilitar servicios de mantenimiento BTRFS
    arch-chroot /mnt /bin/bash -c "systemctl enable btrfs-scrub@-.timer"
    arch-chroot /mnt /bin/bash -c "systemctl enable fstrim.timer"

    # Configurar snapshots automáticos si snapper está disponible
    if arch-chroot /mnt /bin/bash -c "pacman -Qq snapper" 2>/dev/null; then
        arch-chroot /mnt /bin/bash -c "snapper -c root create-config /"
        arch-chroot /mnt /bin/bash -c "systemctl enable snapper-timeline.timer snapper-cleanup.timer"
    fi

    # Optimizar fstab para BTRFS
    sed -i 's/relatime/noatime/g' /mnt/etc/fstab

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
echo -e "${YELLOW}VERIFICACIÓN DE FIRMWARE:${NC}"
echo -e "${CYAN}• Para verificar firmware cargado después del reinicio:${NC}"
echo -e "  ${GREEN}sudo journalctl -kg 'loaded f'${NC}"
echo -e "${CYAN}• Si necesitas firmware adicional, instala manualmente:${NC}"
echo -e "  ${GREEN}sudo pacman -S linux-firmware-[vendor]${NC}"
echo -e "${CYAN}• Vendors disponibles: amdgpu, intel, nvidia, realtek,${NC}"
echo -e "${CYAN}  broadcom, atheros, mediatek, cirrus, other${NC}"

echo ""

# Barra de progreso final
titulo_progreso="| Finalizando instalación de ARCRIS LINUX |"
barra_progreso

echo -e "${GREEN}✓ Instalación de ARCRIS LINUX completada exitosamente!${NC}"

# Mostrar información importante para sistemas cifrados
if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}           SISTEMA CIFRADO CON LUKS+LVM CONFIGURADO EXITOSAMENTE${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${YELLOW}🔐 INFORMACIÓN CRÍTICA SOBRE TU SISTEMA CIFRADO:${NC}"
    echo ""
    echo -e "${GREEN}✓ Configuración aplicada:${NC}"
    echo -e "${CYAN}  • Solo la partición EFI/boot queda sin cifrar (necesario para arrancar)${NC}"
    echo -e "${CYAN}  • Toda la partición principal está cifrada con LUKS${NC}"
    echo -e "${CYAN}  • LVM gestiona las particiones sobre el cifrado${NC}"
    echo -e "${CYAN}  • Swap cifrado incluido (8GB)${NC}"
    echo ""
    echo -e "${RED}⚠️  ADVERTENCIAS IMPORTANTES:${NC}"
    echo -e "${RED}  • SIN LA CONTRASEÑA LUKS PERDERÁS TODOS TUS DATOS${NC}"
    echo -e "${RED}  • Guarda la contraseña en un lugar seguro${NC}"
    echo -e "${RED}  • Considera hacer backup del header LUKS${NC}"
    echo ""
    echo -e "${GREEN}🚀 Al reiniciar:${NC}"
    echo -e "${CYAN}  1. El sistema pedirá tu contraseña LUKS para desbloquear el disco${NC}"
    echo -e "${CYAN}  2. Una vez desbloqueado, el sistema arrancará normalmente${NC}"
    echo -e "${CYAN}  3. Si olvidas la contraseña, no podrás acceder a tus datos${NC}"
    echo ""
    echo -e "${GREEN}📁 Backup del header LUKS:${NC}"
    echo -e "${CYAN}  • Se creó un backup en /tmp/luks-header-backup${NC}"
    echo -e "${YELLOW}  • CÓPIALO A UN LUGAR SEGURO después del primer arranque${NC}"
    echo -e "${CYAN}  • Comando: cp /tmp/luks-header-backup ~/luks-backup-$(date +%Y%m%d)${NC}"
    echo ""
    echo -e "${GREEN}🔧 Comandos útiles post-instalación:${NC}"
    echo -e "${CYAN}  • Ver estado LVM: sudo vgdisplay && sudo lvdisplay${NC}"
    echo -e "${CYAN}  • Redimensionar particiones: sudo lvresize${NC}"
    echo -e "${CYAN}  • Backup adicional header: sudo cryptsetup luksHeaderBackup /dev/sdaX${NC}"
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""
fi
