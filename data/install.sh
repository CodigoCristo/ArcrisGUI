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

# Función para verificar disponibilidad de dispositivos LVM
verify_lvm_devices() {
    echo -e "${CYAN}Verificando disponibilidad de dispositivos LVM...${NC}"

    # Mostrar información de debugging inicial
    echo -e "${CYAN}Estado actual del sistema:${NC}"
    echo "• Dispositivos de mapeo:"
    ls -la /dev/mapper/ 2>/dev/null || echo "  No hay dispositivos en /dev/mapper/"
    echo "• Información de cryptsetup:"
    cryptsetup status cryptlvm 2>/dev/null || echo "  cryptlvm no está activo"

    # Esperar a que el sistema detecte los dispositivos
    sleep 5

    # Verificar que cryptlvm esté disponible
    if [ ! -b "/dev/mapper/cryptlvm" ]; then
        echo -e "${RED}ERROR: /dev/mapper/cryptlvm no está disponible${NC}"
        echo -e "${YELLOW}Información de debugging:${NC}"
        echo "• Dispositivos en /dev/mapper/:"
        ls -la /dev/mapper/ 2>/dev/null
        return 1
    fi

    # Activar volume groups
    echo -e "${CYAN}Activando volume groups...${NC}"
    if ! vgchange -ay vg0; then
        echo -e "${RED}ERROR: No se pudieron activar los volúmenes LVM${NC}"
        echo -e "${YELLOW}Información de debugging:${NC}"
        echo "• Volume Groups disponibles:"
        vgs 2>/dev/null || echo "  No hay volume groups"
        echo "• Physical Volumes:"
        pvs 2>/dev/null || echo "  No hay physical volumes"
        return 1
    fi

    # Esperar un poco más para que los dispositivos estén disponibles
    sleep 3

    # Verificar que los dispositivos LVM existan
    local max_attempts=15
    local attempt=1

    while [ $attempt -le $max_attempts ]; do
        # Forzar actualización de dispositivos
        udevadm settle
        vgchange -ay vg0 2>/dev/null || true

        if [ -b "/dev/vg0/root" ] && [ -b "/dev/vg0/swap" ]; then
            echo -e "${GREEN}✓ Dispositivos LVM verificados correctamente${NC}"
            echo -e "${CYAN}Información final:${NC}"
            echo "• Volume Groups:"
            vgs 2>/dev/null
            echo "• Logical Volumes:"
            lvs 2>/dev/null
            echo "• Estructura de bloques:"
            lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT 2>/dev/null | head -20
            return 0
        fi

        echo -e "${YELLOW}Intento $attempt/$max_attempts: Esperando dispositivos LVM...${NC}"
        if [ $attempt -eq 5 ]; then
            echo -e "${YELLOW}Información intermedia de debugging:${NC}"
            echo "• Logical Volumes disponibles:"
            lvs 2>/dev/null || echo "  No hay logical volumes"
            echo "• Dispositivos en /dev/vg0/:"
            ls -la /dev/vg0/ 2>/dev/null || echo "  Directorio /dev/vg0/ no existe"
        fi

        if [ $attempt -eq 10 ]; then
            echo -e "${YELLOW}Intentando reactivar volume groups...${NC}"
            vgchange -an vg0 2>/dev/null || true
            sleep 2
            vgchange -ay vg0 2>/dev/null || true
        fi

        sleep 3
        attempt=$((attempt + 1))
    done

    echo -e "${RED}ERROR: Los dispositivos LVM no están disponibles después de $max_attempts intentos${NC}"
    echo -e "${RED}Información completa de debugging:${NC}"
    echo -e "${RED}  • /dev/vg0/root existe: $([ -b '/dev/vg0/root' ] && echo 'SÍ' || echo 'NO')${NC}"
    echo -e "${RED}  • /dev/vg0/swap existe: $([ -b '/dev/vg0/swap' ] && echo 'SÍ' || echo 'NO')${NC}"
    echo -e "${RED}  • Volume Groups:${NC}"
    vgs 2>/dev/null || echo "    No hay volume groups disponibles"
    echo -e "${RED}  • Logical Volumes:${NC}"
    lvs 2>/dev/null || echo "    No hay logical volumes disponibles"
    echo -e "${RED}  • Physical Volumes:${NC}"
    pvs 2>/dev/null || echo "    No hay physical volumes disponibles"
    echo -e "${RED}  • Dispositivos de mapeo:${NC}"
    ls -la /dev/mapper/ 2>/dev/null || echo "    No hay dispositivos de mapeo"
    echo -e "${RED}  • Estructura actual de bloques:${NC}"
    lsblk 2>/dev/null | head -20 || echo "    No se puede mostrar lsblk"
    return 1
}

# Configuración inicial del LiveCD
echo -e "${GREEN}| Configurando LiveCD |${NC}"
echo ""

# Configuración de zona horaria
sudo timedatectl set-timezone $TIMEZONE
sudo hwclock -w
sudo hwclock --systohc --rtc=/dev/rtc0

# Configuración de locale
echo "$LOCALE.UTF-8 UTF-8" > /etc/locale.gen
sudo locale-gen
export LANG=$LOCALE.UTF-8

sleep 2
timedatectl status
echo ""
date +' %A, %B %d, %Y - %r'
sleep 5
clear
# 12. Aplicar configuración de teclado inmediatamente en el LiveCD actual
echo -e "${CYAN}12. Aplicando configuración al sistema actual...${NC}"
sudo localectl set-keymap $KEYMAP_TTY 2>/dev/null || true
sudo localectl set-x11-keymap $KEYBOARD_LAYOUT pc105 "" "" 2>/dev/null || true
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

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

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

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

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

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

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

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

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
    echo -e "${CYAN}  • Solo las particiones EFI y boot quedan sin cifrar (necesario para el bootloader)${NC}"
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

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 4
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

        # Crear tabla de particiones GPT
        parted $SELECTED_DISK --script --align optimal mklabel gpt

        # Crear partición EFI (512MB)
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 esp on

        # Crear partición boot sin cifrar (1GB)
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 513MiB 1537MiB

        # Crear partición principal cifrada (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 1537MiB 100%

        # Formatear particiones
        mkfs.fat -F32 ${SELECTED_DISK}1
        mkfs.ext4 -F ${SELECTED_DISK}2

        # Sincronizar y esperar reconocimiento de particiones
        echo -e "${CYAN}Sincronizando sistema de archivos...${NC}"
        sync
        partprobe $SELECTED_DISK
        sleep 4

        # Configurar LUKS en la partición principal
        # Aplicar cifrado LUKS
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Limpiar firmas de sistemas de archivos existentes
        echo -e "${CYAN}Limpiando firmas de sistemas de archivos...${NC}"
        wipefs -af ${SELECTED_DISK}3 2>/dev/null || true
        dd if=/dev/zero of=${SELECTED_DISK}3 bs=1M count=10 2>/dev/null || true

        echo -e "${CYAN}Aplicando cifrado LUKS a ${SELECTED_DISK}3...${NC}"
        echo -e "${YELLOW}IMPORTANTE: Esto puede tomar unos minutos dependiendo del tamaño del disco${NC}"
        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat --batch-mode --verify-passphrase ${SELECTED_DISK}3 -; then
            echo -e "${RED}ERROR: Falló el cifrado LUKS de la partición${NC}"
            exit 1
        fi

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open --batch-mode ${SELECTED_DISK}3 cryptlvm -; then
            echo -e "${RED}ERROR: No se pudo abrir el dispositivo cifrado${NC}"
            exit 1
        fi

        # Verificar que el dispositivo cifrado esté disponible
        if [ ! -b "/dev/mapper/cryptlvm" ]; then
            echo -e "${RED}ERROR: El dispositivo /dev/mapper/cryptlvm no está disponible${NC}"
            exit 1
        fi

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup ${SELECTED_DISK}3 --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS
        echo -e "${GREEN}| Configurando LVM sobre LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -e "${CYAN}Creando Physical Volume sobre dispositivo cifrado...${NC}"
        if ! pvcreate /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Physical Volume${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Volume Group 'vg0'...${NC}"
        if ! vgcreate vg0 /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Volume Group vg0${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'swap' de 8GB...${NC}"
        if ! lvcreate -L 8G vg0 -n swap; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume swap${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'root' con el espacio restante...${NC}"
        if ! lvcreate -l 100%FREE vg0 -n root; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume root${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Configuración LVM completada:${NC}"
        echo -e "${GREEN}  • Volume Group: vg0${NC}"
        echo -e "${GREEN}  • Swap: 8GB (/dev/vg0/swap)${NC}"
        echo -e "${GREEN}  • Root: Resto del espacio (/dev/vg0/root)${NC}"

        # Sincronizar antes de verificar LVM
        echo -e "${CYAN}Sincronizando dispositivos del sistema...${NC}"
        sync
        udevadm settle
        sleep 2

        # Verificar que los volúmenes LVM estén disponibles
        if ! verify_lvm_devices; then
            echo -e "${RED}FALLO CRÍTICO: No se pudieron verificar los dispositivos LVM${NC}"
            exit 1
        fi

        # Formatear volúmenes LVM
        echo -e "${CYAN}Formateando volúmenes LVM...${NC}"
        if ! mkfs.ext4 -F /dev/vg0/root; then
            echo -e "${RED}ERROR: No se pudo formatear /dev/vg0/root${NC}"
            exit 1
        fi

        if ! mkswap /dev/vg0/swap; then
            echo -e "${RED}ERROR: No se pudo formatear /dev/vg0/swap${NC}"
            exit 1
        fi

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

        # Verificar que las particiones existan antes de montar
        echo -e "${CYAN}Verificando particiones antes del montaje...${NC}"
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición EFI ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi
        if [ ! -b "${SELECTED_DISK}2" ]; then
            echo -e "${RED}ERROR: Partición boot ${SELECTED_DISK}2 no existe${NC}"
            exit 1
        fi

        # Esperar que las particiones estén completamente listas
        sleep 2
        sync

        echo -e "${CYAN}Creando directorio de montaje boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount ${SELECTED_DISK}2 /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición boot${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando directorio EFI dentro de boot...${NC}"
        mkdir -p /mnt/boot/efi

        echo -e "${CYAN}Montando partición EFI...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot/efi; then
            echo -e "${RED}ERROR: Falló el montaje de la partición EFI${NC}"
            exit 1
        fi

        # Verificar que los montajes sean exitosos (en orden correcto)
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi
        if ! mountpoint -q /mnt/boot/efi; then
            echo -e "${RED}ERROR: /mnt/boot/efi no está montado correctamente${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Todas las particiones montadas correctamente${NC}"
        echo -e "${GREEN}✓ Esquema LUKS+LVM configurado:${NC}"
        echo -e "${GREEN}  • UEFI: EFI (512MB) + boot (1GB) sin cifrar, resto cifrado${NC}"

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2

    else
        # Configuración para BIOS Legacy con cifrado (siguiendo mejores prácticas)
        echo -e "${GREEN}| Configurando particiones cifradas para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Borrar completamente el disco
        echo -e "${CYAN}Limpiando disco completamente...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

        # Crear tabla de particiones MBR
        parted $SELECTED_DISK --script --align optimal mklabel msdos

        # Crear partición de boot sin cifrar (512MB) - mínima necesaria
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 1MiB 513MiB
        parted $SELECTED_DISK --script set 1 boot on

        # Crear partición cifrada (resto del disco)
        parted $SELECTED_DISK --script --align optimal mkpart primary 513MiB 100%

        # Formatear partición boot
        mkfs.ext4 -F ${SELECTED_DISK}2

        # Sincronizar y esperar reconocimiento de particiones
        echo -e "${CYAN}Sincronizando sistema de archivos...${NC}"
        sync
        partprobe $SELECTED_DISK
        sleep 3

        # Configurar LUKS en la partición principal
        # Configurar cifrado LUKS
        echo -e "${GREEN}| Configurando cifrado LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Limpiar firmas de sistemas de archivos existentes
        echo -e "${CYAN}Limpiando firmas de sistemas de archivos...${NC}"
        wipefs -af ${SELECTED_DISK}2 2>/dev/null || true
        dd if=/dev/zero of=${SELECTED_DISK}2 bs=1M count=10 2>/dev/null || true

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat --batch-mode --verify-passphrase ${SELECTED_DISK}2 -; then
            echo -e "${RED}ERROR: Falló el cifrado LUKS de la partición${NC}"
            exit 1
        fi

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open --batch-mode ${SELECTED_DISK}2 cryptlvm -; then
            echo -e "${RED}ERROR: No se pudo abrir el dispositivo cifrado${NC}"
            exit 1
        fi

        # Verificar que el dispositivo cifrado esté disponible
        if [ ! -b "/dev/mapper/cryptlvm" ]; then
            echo -e "${RED}ERROR: El dispositivo /dev/mapper/cryptlvm no está disponible${NC}"
            exit 1
        fi

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
        if ! pvcreate /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Physical Volume${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Volume Group 'vg0'...${NC}"
        if ! vgcreate vg0 /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Volume Group vg0${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'swap' de 8GB...${NC}"
        if ! lvcreate -L 8G vg0 -n swap; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume swap${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'root' con el espacio restante...${NC}"
        if ! lvcreate -l 100%FREE vg0 -n root; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume root${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Configuración LVM completada:${NC}"
        echo -e "${GREEN}  • Volume Group: vg0${NC}"
        echo -e "${GREEN}  • Swap: 8GB (/dev/vg0/swap)${NC}"
        echo -e "${GREEN}  • Root: Resto del espacio (/dev/vg0/root)${NC}"

        # Sincronizar antes de verificar LVM
        echo -e "${CYAN}Sincronizando dispositivos del sistema...${NC}"
        sync
        udevadm settle
        sleep 2

        # Verificar que los volúmenes LVM estén disponibles
        if ! verify_lvm_devices; then
            echo -e "${RED}FALLO CRÍTICO: No se pudieron verificar los dispositivos LVM${NC}"
            exit 1
        fi

        # Formatear volúmenes LVM
        echo -e "${CYAN}Formateando volúmenes LVM...${NC}"
        if ! mkfs.ext4 -F /dev/vg0/root; then
            echo -e "${RED}ERROR: No se pudo formatear /dev/vg0/root${NC}"
            exit 1
        fi

        if ! mkswap /dev/vg0/swap; then
            echo -e "${RED}ERROR: No se pudo formatear /dev/vg0/swap${NC}"
            exit 1
        fi

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

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
        echo -e "${GREEN}✓ Esquema LUKS+LVM configurado:${NC}"
        echo -e "${GREEN}  • BIOS Legacy: boot (512MB) sin cifrar, resto cifrado${NC}"

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2
    fi
}

# Función para particionado manual
partition_manual() {
    echo -e "${GREEN}| Particionado manual detectado |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Primera pasada: Formatear todas las particiones
    echo -e "${CYAN}=== FASE 1: Formateo de particiones ===${NC}"
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        echo -e "${GREEN}| Formateando: $device -> $format |${NC}"

        # Formatear según el tipo especificado
        case $format in
            "none")
                echo -e "${CYAN}Sin formatear: $device${NC}"
                ;;
            "mkfs.ext4")
                mkfs.ext4 -F $device
                ;;
            "mkfs.ext3")
                mkfs.ext3 -F $device
                ;;
            "mkfs.ext2")
                mkfs.ext2 -F $device
                ;;
            "mkfs.btrfs")
                mkfs.btrfs -f $device
                ;;
            "mkfs.xfs")
                mkfs.xfs -f $device
                ;;
            "mkfs.f2fs")
                mkfs.f2fs -f $device
                ;;
            "mkfs.fat32")
                mkfs.fat -F32 -v $device
                ;;
            "mkfs.fat16")
                mkfs.fat -F16 -v $device
                ;;
            "mkfs.ntfs")
                mkfs.ntfs -f $device
                ;;
            "mkfs.reiserfs")
                mkfs.reiserfs -f $device
                ;;
            "mkfs.jfs")
                mkfs.jfs -f $device
                ;;
            "mkswap")
                mkswap $device
                swapon $device
                ;;
            *)
                echo -e "${RED}| Formato no reconocido: $format |${NC}"
                ;;
        esac
    done

    # Validaciones antes del montaje
    echo -e "${CYAN}=== VALIDACIONES ===${NC}"

    # Verificar que existe partición raíz
    ROOT_FOUND=false
    EFI_FOUND=false
    BOOT_FOUND=false

    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        if [ "$mountpoint" = "/" ]; then
            ROOT_FOUND=true
        elif [ "$mountpoint" = "/boot/EFI" ]; then
            EFI_FOUND=true
            # Verificar que la partición EFI use formato FAT
            if [ "$format" != "mkfs.fat32" ] && [ "$format" != "mkfs.fat16" ]; then
                echo -e "${YELLOW}ADVERTENCIA: Partición EFI ($device) debería usar formato FAT32 o FAT16${NC}"
                echo -e "${YELLOW}Formato actual: $format${NC}"
            fi
        elif [ "$mountpoint" = "/boot" ]; then
            BOOT_FOUND=true
        fi
    done

    # Validar configuración
    if [ "$ROOT_FOUND" = false ]; then
        echo -e "${RED}ERROR: No se encontró partición raíz (/) configurada${NC}"
        echo -e "${RED}Debe configurar al menos una partición con punto de montaje '/'${NC}"
        exit 1
    fi

    if [ "$EFI_FOUND" = true ] && [ "$BOOT_FOUND" = true ]; then
        echo -e "${GREEN}✓ Configuración detectada: /boot separado + /boot/EFI${NC}"
    elif [ "$EFI_FOUND" = true ]; then
        echo -e "${GREEN}✓ Configuración detectada: /boot/EFI (sin /boot separado)${NC}"
    fi

    echo -e "${GREEN}✓ Validaciones completadas${NC}"

    # Segunda pasada: Montaje en orden correcto
    echo -e "${CYAN}=== FASE 2: Montaje de particiones ===${NC}"

    # 1. Montar partición raíz primero
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/" ]; then
            echo -e "${GREEN}| Montando raíz: $device -> /mnt |${NC}"
            mount $device /mnt
            break
        fi
    done

    # 2. Montar /boot si existe
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/boot" ]; then
            echo -e "${GREEN}| Montando /boot: $device -> /mnt/boot |${NC}"
            mkdir -p /mnt/boot
            mount $device /mnt/boot
            break
        fi
    done

    # 3. Montar /boot/EFI (debe ir después de /boot para evitar conflictos)
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/boot/EFI" ]; then
            echo -e "${GREEN}| Montando EFI: $device -> /mnt/boot/efi |${NC}"
            mkdir -p /mnt/boot/efi
            mount $device /mnt/boot/efi
            echo -e "${CYAN}Partición EFI montada en /mnt/boot/efi${NC}"
            break
        fi
    done

    # 4. Montar todas las demás particiones
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        # Saltar las ya montadas y swap
        if [ "$mountpoint" = "/" ] || [ "$mountpoint" = "/boot" ] || [ "$mountpoint" = "/boot/EFI" ] || [ "$mountpoint" = "swap" ]; then
            continue
        fi

        echo -e "${GREEN}| Montando: $device -> /mnt$mountpoint |${NC}"
        mkdir -p /mnt$mountpoint
        mount $device /mnt$mountpoint
    done

    lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT
    sleep 3
}

# Desmontar particiones existentes del disco seleccionado
unmount_selected_disk_partitions() {
    echo -e "${CYAN}Desmontando particiones existentes del disco: $SELECTED_DISK${NC}"
    sleep 3
    # Obtener el dispositivo donde está montada la ISO (sistema live)
    LIVE_DEVICE=$(findmnt -n -o SOURCE / 2>/dev/null | head -1)
    if [ -z "$LIVE_DEVICE" ]; then
        # Buscar dispositivos con sistema de archivos de solo lectura (típico de ISO live)
        LIVE_DEVICE=$(findmnt -n -o SOURCE -t squashfs,iso9660 2>/dev/null | head -1)
    fi

    echo -e "${YELLOW}Sistema live detectado en: ${LIVE_DEVICE:-"no detectado"}${NC}"
    sleep 3

    # Verificar si hay particiones montadas del disco seleccionado
    echo -e "${CYAN}Verificando particiones montadas en: $SELECTED_DISK${NC}"
    MOUNTED_PARTITIONS_CHECK=$(findmnt -rn -o TARGET,SOURCE | grep "$SELECTED_DISK" | while read -r mountpoint source; do
        # Excluir puntos de montaje del sistema live
        if [[ "$source" != *"$LIVE_DEVICE"* ]] && [[ "$mountpoint" != "/" ]] && [[ "$mountpoint" != "/run/archiso"* ]] && [[ "$mountpoint" != "/boot"* ]] && [[ "$source" == "$SELECTED_DISK"* ]]; then
            echo "$mountpoint"
        fi
    done)

    if [ -z "$MOUNTED_PARTITIONS_CHECK" ]; then
        echo -e "${GREEN}✓ No se encontraron particiones montadas en: $SELECTED_DISK${NC}"
        echo -e "${YELLOW}Continuando con el script sin necesidad de desmontar particiones...${NC}"
        echo ""
        return 0
    fi

    echo -e "${YELLOW}Se encontraron particiones montadas. Procediendo con el desmontaje...${NC}"
    sleep 3

    # Desactivar swap del disco seleccionado
    echo -e "${CYAN}Desactivando swap del disco seleccionado...${NC}"
    for swap_device in $(swapon --show=NAME --noheadings 2>/dev/null | grep "^$SELECTED_DISK"); do
        echo -e "${YELLOW}Desactivando swap: $swap_device${NC}"
        swapoff "$swap_device" 2>/dev/null || true
    done
    sleep 3
    # Obtener todas las particiones montadas del disco seleccionado
    echo -e "${CYAN}Desmontando particiones montadas del disco seleccionado...${NC}"
    sleep 3
    # Listar particiones del disco seleccionado que están montadas (en orden inverso para desmontar correctamente)
    MOUNTED_PARTITIONS=$(findmnt -rn -o TARGET,SOURCE | grep "$SELECTED_DISK" | sort -r | while read -r mountpoint source; do
        # Excluir puntos de montaje del sistema live
        if [[ "$source" != *"$LIVE_DEVICE"* ]] && [[ "$mountpoint" != "/" ]] && [[ "$mountpoint" != "/run/archiso"* ]] && [[ "$mountpoint" != "/boot"* ]] && [[ "$source" == "$SELECTED_DISK"* ]]; then
            echo "$mountpoint"
        fi
    done)
    sleep 3
    # Desmontar cada partición encontrada
    echo "$MOUNTED_PARTITIONS" | while IFS= read -r mountpoint; do
        if [ -n "$mountpoint" ]; then
            echo -e "${YELLOW}Desmontando: $mountpoint${NC}"
            umount "$mountpoint" 2>/dev/null || umount -l "$mountpoint" 2>/dev/null || true
        fi
    done
    sleep 3
    # Cerrar dispositivos LVM/LUKS relacionados con el disco seleccionado
    echo -e "${CYAN}Cerrando dispositivos cifrados/LVM relacionados...${NC}"
    sleep 3
    # Cerrar dispositivos LUKS que usen particiones del disco seleccionado
    if command -v cryptsetup >/dev/null 2>&1; then
        for luks_device in $(ls /dev/mapper/ 2>/dev/null | grep -E "(crypt|luks)"); do
            if cryptsetup status "$luks_device" 2>/dev/null | grep -q "$SELECTED_DISK"; then
                echo -e "${YELLOW}Cerrando dispositivo LUKS: $luks_device${NC}"
                cryptsetup close "$luks_device" 2>/dev/null || true
            fi
        done
    fi
    sleep 3
    # Desactivar grupos de volúmenes LVM relacionados
    if command -v vgchange >/dev/null 2>&1; then
        for vg in $(vgs --noheadings -o vg_name 2>/dev/null); do
            if pvs --noheadings -o pv_name,vg_name 2>/dev/null | grep "$SELECTED_DISK" | grep -q "$vg"; then
                echo -e "${YELLOW}Desactivando grupo de volúmenes LVM: $vg${NC}"
                vgchange -an "$vg" 2>/dev/null || true
            fi
        done
    fi

    # Esperar un momento para que el sistema procese los cambios
    sleep 3

    echo -e "${GREEN}✓ Limpieza de particiones completada para: $SELECTED_DISK${NC}"
    echo ""
}

# Ejecutar limpieza de particiones
unmount_selected_disk_partitions

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
pacstrap /mnt curl
pacstrap /mnt wget
pacstrap /mnt git


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
if [ "$PARTITION_MODE" = "manual" ]; then
    echo -e "${GREEN}| Generando fstab para particionado manual |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Crear fstab base
    echo "# <file system> <mount point> <type> <options> <dump> <pass>" > /mnt/etc/fstab

    # Procesar configuraciones de particiones para fstab
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        # Omitir particiones swap (se manejan separadamente)
        if [ "$mountpoint" = "swap" ]; then
            continue
        fi

        # Para particiones no formateadas (none), detectar el sistema de archivos existente
        if [ "$format" = "none" ]; then
            DETECTED_FS=$(blkid -s TYPE -o value $device)
            if [ -z "$DETECTED_FS" ]; then
                echo -e "${YELLOW}ADVERTENCIA: No se pudo detectar sistema de archivos en $device, omitiendo del fstab${NC}"
                continue
            fi
            echo -e "${CYAN}Detectado sistema de archivos existente en $device: $DETECTED_FS${NC}"
            format_for_fstab="$DETECTED_FS"
        else
            # Para particiones formateadas, usar el formato especificado
            format_for_fstab="$format"
        fi

        # Obtener UUID de la partición
        PART_UUID=$(blkid -s UUID -o value $device)
        if [ -n "$PART_UUID" ]; then
            # Determinar el tipo de sistema de archivos
            case $format_for_fstab in
                "mkfs.fat32"|"mkfs.fat16"|"vfat")
                    FS_TYPE="vfat"
                    if [ "$mountpoint" = "/boot/EFI" ]; then
                        echo "UUID=$PART_UUID /boot/efi vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
                "mkfs.ext4"|"mkfs.ext3"|"mkfs.ext2"|"ext4"|"ext3"|"ext2")
                    if [[ "$format_for_fstab" =~ ^mkfs\. ]]; then
                        FS_TYPE="${format_for_fstab#mkfs.}"
                    else
                        FS_TYPE="$format_for_fstab"
                    fi
                    if [ "$mountpoint" = "/" ]; then
                        echo "UUID=$PART_UUID / $FS_TYPE rw,relatime 0 1" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint $FS_TYPE rw,relatime 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
                "mkfs.btrfs"|"btrfs")
                    echo "UUID=$PART_UUID $mountpoint btrfs rw,noatime,compress=zstd,space_cache=v2 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.xfs"|"xfs")
                    echo "UUID=$PART_UUID $mountpoint xfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.f2fs"|"f2fs")
                    echo "UUID=$PART_UUID $mountpoint f2fs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.ntfs"|"ntfs")
                    echo "UUID=$PART_UUID $mountpoint ntfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.reiserfs"|"reiserfs")
                    echo "UUID=$PART_UUID $mountpoint reiserfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.jfs"|"jfs")
                    echo "UUID=$PART_UUID $mountpoint jfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                *)
                    echo -e "${YELLOW}ADVERTENCIA: Sistema de archivos no reconocido ($format_for_fstab) para $device${NC}"
                    echo -e "${YELLOW}Usando opciones genéricas en fstab${NC}"
                    if [ "$mountpoint" = "/" ]; then
                        echo "UUID=$PART_UUID / $format_for_fstab rw,relatime 0 1" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint $format_for_fstab rw,relatime 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
            esac
        fi
    done

    # Agregar particiones swap
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        if [ "$mountpoint" = "swap" ]; then
            SWAP_UUID=$(blkid -s UUID -o value $device)
            if [ -n "$SWAP_UUID" ]; then
                echo "UUID=$SWAP_UUID none swap defaults 0 0" >> /mnt/etc/fstab
            fi
        fi
    done

    echo -e "${GREEN}✓ fstab generado para particionado manual${NC}"
else
    # Usar genfstab para modos automáticos
    genfstab -U /mnt > /mnt/etc/fstab
fi

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
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
    "linux-hardened")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-hardened --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
    "linux-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-lts --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
    "linux-rt-lts")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-rt-lts --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
    "linux-zen")
        arch-chroot /mnt /bin/bash -c "pacman -S linux-zen --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
    *)
        arch-chroot /mnt /bin/bash -c "pacman -S linux --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S linux-firmware --noconfirm"
        ;;
esac

sleep 3
clear
echo -e "${GREEN}✓ Instalanado extras${NC}"
arch-chroot /mnt pacman -S yay-bin --noconfirm
arch-chroot /mnt pacman -S alsi --noconfirm
clear

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

# Configuración temporal NOPASSWD para instalaciones
echo -e "${GREEN}| Configurando permisos sudo temporales |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Detectar usuarios existentes en el sistema
USUARIOS_EXISTENTES=$(awk -F':' '$3 >= 1000 && $3 != 65534 {print $1}' /mnt/etc/passwd 2>/dev/null)

if [[ -n "$USUARIOS_EXISTENTES" ]]; then
    echo "✓ Usuarios detectados en el sistema:"
    echo "$USUARIOS_EXISTENTES" | while read -r usuario; do
        echo "  - $usuario"
    done
    echo ""

    # Configurar sudo para todos los usuarios encontrados
    {
        echo "# Configuración temporal para instalaciones"
        echo "$USUARIOS_EXISTENTES" | while read -r usuario_encontrado; do
            echo "$usuario_encontrado ALL=(ALL:ALL) NOPASSWD: ALL"
        done
        echo "%wheel ALL=(ALL:ALL) NOPASSWD: ALL"
    } > /mnt/etc/sudoers.d/temp-install

    echo "✓ Configuración sudo aplicada para usuarios existentes y grupo wheel"
else
    echo "⚠️  No se encontraron usuarios existentes en el sistema"
    echo "   Usando variable \$USER: $USER"

    # Usar la variable USER proporcionada
    {
        echo "# Configuración temporal para instalaciones"
        echo "$USER ALL=(ALL:ALL) NOPASSWD: ALL"
    } > /mnt/etc/sudoers.d/temp-install

    echo "✓ Configuración sudo aplicada para usuario: $USER"
fi

# Establecer permisos correctos para el archivo sudoers
chmod 440 /mnt/etc/sudoers.d/temp-install




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
# Instalar bootloader para todos los modos (incluyendo manual)
if true; then
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

        sleep 4
        # Limpiar entradas UEFI previas que puedan causar conflictos
        # echo -e "${CYAN}Limpiando entradas UEFI previas...${NC}"
        # efibootmgr | awk '/grub/i {gsub(/Boot|\*.*/, ""); system("efibootmgr -b " $1 " -B 2>/dev/null")}'
        efibootmgr | grep -i grub | cut -d'*' -f1 | sed 's/Boot//' | xargs -I {} efibootmgr -b {} -B 2>/dev/null || true
        sleep 4

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

            CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada ${SELECTED_DISK}3${NC}"
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

            # Configurar GRUB_CMDLINE_LINUX_DEFAULT sin 'quiet' para mejor debugging en sistemas cifrados
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub

            echo -e "${GREEN}✓ Configuración GRUB para cifrado:${NC}"
            echo -e "${CYAN}  • cryptdevice=UUID=${CRYPT_UUID}:cryptlvm${NC}"
            echo -e "${CYAN}  • root=/dev/vg0/root${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y (permite a GRUB leer discos cifrados)${NC}"
            echo -e "${CYAN}  • Sin 'quiet' para mejor debugging del arranque cifrado${NC}"
        elif [ "$PARTITION_MODE" = "btrfs" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="rootflags=subvol=@ loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos btrfs\"" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos\"" >> /mnt/etc/default/grub
        fi

        echo -e "${CYAN}Instalando GRUB en partición EFI...${NC}"
        arch-chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot/efi --removable" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI (modo removible)${NC}"
            echo -e "${YELLOW}Información adicional:${NC}"
            echo "- Estado de /boot:"
            ls -la /mnt/boot/
            echo "- Estado de /boot/efi:"
            ls -la /mnt/boot/efi/
            echo "- Espacio disponible en /boot:"
            df -h /mnt/boot
            echo "- Espacio disponible en /boot/efi:"
            df -h /mnt/boot/efi
            exit 1
        }
        sleep 5
        echo -e "${GREEN}✓ GRUB instalado en modo removible (/EFI/BOOT/bootx64.efi)${NC}"

        echo -e "${CYAN}Instalando GRUB con entrada NVRAM...${NC}"
        arch-chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI (entrada NVRAM)${NC}"
            echo -e "${YELLOW}Información adicional:${NC}"
            echo "- Estado de /boot/efi/EFI/:"
            ls -la /mnt/boot/efi/EFI/ 2>/dev/null || echo "  Directorio EFI no existe"
            exit 1
        }

        echo -e "${GREEN}✓ GRUB instalado con entrada NVRAM (/EFI/GRUB/grubx64.efi)${NC}"

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
            # Configurar GRUB_CMDLINE_LINUX_DEFAULT sin 'quiet' para mejor debugging en sistemas cifrados
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub

            echo -e "${GREEN}✓ Configuración GRUB para cifrado BIOS Legacy:${NC}"
            echo -e "${CYAN}  • cryptdevice=UUID=${CRYPT_UUID}:cryptlvm${NC}"
            echo -e "${CYAN}  • root=/dev/vg0/root${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y (permite a GRUB leer discos cifrados)${NC}"
            echo -e "${CYAN}  • Sin 'quiet' para mejor debugging del arranque cifrado${NC}"
            echo -e "${CYAN}  • Módulos MBR: part_msdos lvm luks${NC}"
        elif [ "$PARTITION_MODE" = "btrfs" ]; then
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="rootflags=subvol=@ loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos btrfs\"" >> /mnt/etc/default/grub
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos\"" >> /mnt/etc/default/grub
        fi

        sleep 4

        echo -e "${CYAN}Instalando GRUB en disco...${NC}"
        if ! arch-chroot /mnt /bin/bash -c "grub-install --target=i386-pc $SELECTED_DISK --recheck"; then
            echo -e "${RED}ERROR: Falló la instalación de GRUB BIOS${NC}"
            exit 1
        fi

        sleep 4

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
fi

# Verificación final del bootloader
# Verificar bootloader para todos los modos (incluyendo manual)
if true; then
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


sleep 2
clear

# Detección de otros sistemas operativos
echo -e "${GREEN}| Detectando otros sistemas operativos |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
# Instalar os-prober para detectar otros sistemas
echo -e "${CYAN}Instalando os-prober...${NC}"
arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S os-prober --noansweredit --noconfirm --needed"
arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ntfs-3g --noansweredit --noconfirm --needed"
echo "GRUB_DISABLE_OS_PROBER=false" >> /mnt/etc/default/grub
sleep 2
clear

# Detectar tipo de firmware y múltiples sistemas operativos
echo -e "${CYAN}Detectando tipo de firmware y sistemas operativos...${NC}"

# Detectar si es sistema UEFI o BIOS Legacy
MULTIPLE_OS_DETECTED=false
SYSTEM_TYPE=""

if [ -d "/sys/firmware/efi" ]; then
    SYSTEM_TYPE="UEFI"
    echo -e "${GREEN}✓ Sistema UEFI detectado${NC}"

    # Detectar particiones EFI System
    echo -e "${CYAN}  • Método 1: Detectando particiones EFI con lsblk...${NC}"
    readarray -t EFI_PARTITIONS < <(lsblk -no NAME,PARTTYPE | grep -i "c12a7328-f81f-11d2-ba4b-00a0c93ec93b\|EFI.*System" | awk '{print $1}' | sed 's/[├─└│ ]//g' | grep -v "^$")

    # Si no se encontraron particiones con lsblk, intentar con fdisk como respaldo
    if [ ${#EFI_PARTITIONS[@]} -eq 0 ]; then
        echo -e "${CYAN}  • Método 2: Detectando EFI con fdisk como respaldo...${NC}"

        # Obtener todos los discos disponibles
        DISKS=$(lsblk -dno NAME | grep -v "loop\|sr\|rom" | grep -E "^(sd|nvme|vd|hd)" || true)

        # Buscar particiones EFI en cada disco
        for disk in $DISKS; do
            if [ -b "/dev/$disk" ]; then
                # Buscar particiones EFI usando fdisk
                DISK_EFI=$(fdisk -l "/dev/$disk" 2>/dev/null | grep -i "EFI System\|EFI.*System" | awk '{print $1}' | sed 's|/dev/||' || true)
                if [ -n "$DISK_EFI" ]; then
                    while IFS= read -r partition; do
                        if [ -n "$partition" ]; then
                            EFI_PARTITIONS+=("$partition")
                        fi
                    done <<< "$DISK_EFI"
                fi
            fi
        done
    fi

    # Si aún no hay particiones, intentar método alternativo con blkid
    if [ ${#EFI_PARTITIONS[@]} -eq 0 ]; then
        echo -e "${CYAN}  • Método 3: Detectando EFI con blkid...${NC}"
        readarray -t EFI_PARTITIONS < <(blkid -t PARTLABEL="EFI System Partition" -o device 2>/dev/null | sed 's|/dev/||' | grep -v "^$" || true)
    fi

    # Para UEFI: múltiples sistemas si hay más de 1 partición EFI
    if [ ${#EFI_PARTITIONS[@]} -gt 1 ]; then
        MULTIPLE_OS_DETECTED=true
        echo -e "${GREEN}✓ ${#EFI_PARTITIONS[@]} particiones EFI detectadas - Múltiples sistemas UEFI${NC}"
    else
        echo -e "${YELLOW}⚠ Solo ${#EFI_PARTITIONS[@]} partición EFI detectada - Sistema UEFI único${NC}"
    fi

else
    SYSTEM_TYPE="BIOS_Legacy"
    echo -e "${GREEN}✓ Sistema BIOS Legacy detectado${NC}"

    # Para BIOS Legacy: detectar múltiples sistemas usando otros métodos
    echo -e "${CYAN}  • Detectando múltiples sistemas en BIOS Legacy...${NC}"

    OS_COUNT=0

    # Método 1: Contar particiones bootables
    BOOTABLE_PARTITIONS=$(fdisk -l 2>/dev/null | grep -c "^\*" || echo "0")
    echo -e "${CYAN}  • Particiones bootables detectadas: $BOOTABLE_PARTITIONS${NC}"

    # Método 2: Detectar particiones Windows (NTFS)
    WINDOWS_PARTITIONS=$(blkid -t TYPE=ntfs 2>/dev/null | wc -l || echo "0")
    if [ $WINDOWS_PARTITIONS -gt 0 ]; then
        echo -e "${CYAN}  • Particiones Windows (NTFS) detectadas: $WINDOWS_PARTITIONS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Método 3: Detectar otras particiones Linux (ext4, ext3, btrfs, xfs)
    EXT4_PARTITIONS=$(blkid -t TYPE=ext4 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    EXT3_PARTITIONS=$(blkid -t TYPE=ext3 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    BTRFS_PARTITIONS=$(blkid -t TYPE=btrfs 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    XFS_PARTITIONS=$(blkid -t TYPE=xfs 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    LINUX_PARTITIONS=$((EXT4_PARTITIONS + EXT3_PARTITIONS + BTRFS_PARTITIONS + XFS_PARTITIONS))

    if [ $LINUX_PARTITIONS -gt 0 ]; then
        echo -e "${CYAN}  • Otras particiones Linux detectadas: $LINUX_PARTITIONS${NC}"
        [ $EXT4_PARTITIONS -gt 0 ] && echo -e "${CYAN}    - ext4: $EXT4_PARTITIONS${NC}"
        [ $EXT3_PARTITIONS -gt 0 ] && echo -e "${CYAN}    - ext3: $EXT3_PARTITIONS${NC}"
        [ $BTRFS_PARTITIONS -gt 0 ] && echo -e "${CYAN}    - btrfs: $BTRFS_PARTITIONS${NC}"
        [ $XFS_PARTITIONS -gt 0 ] && echo -e "${CYAN}    - xfs: $XFS_PARTITIONS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Método 4: Buscar particiones con indicadores de SO
    OTHER_OS=$(blkid 2>/dev/null | grep -E "LABEL.*Windows|LABEL.*Microsoft|TYPE.*fat32" | wc -l || echo "0")
    if [ $OTHER_OS -gt 0 ]; then
        echo -e "${CYAN}  • Otras particiones de SO detectadas: $OTHER_OS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Considerar múltiples sistemas si hay más indicadores de OS o más de 1 partición bootable
    if [ $OS_COUNT -gt 0 ] || [ $BOOTABLE_PARTITIONS -gt 1 ]; then
        MULTIPLE_OS_DETECTED=true
        echo -e "${GREEN}✓ Múltiples sistemas operativos detectados en BIOS Legacy${NC}"
    else
        echo -e "${YELLOW}⚠ Solo se detectó un sistema operativo en BIOS Legacy${NC}"
    fi
fi

# Solo proceder con os-prober si se detectaron múltiples sistemas operativos
if [ "$MULTIPLE_OS_DETECTED" = true ]; then
    echo -e "${GREEN}✓ ${#EFI_PARTITIONS[@]} particiones EFI detectadas - Iniciando detección de múltiples sistemas${NC}"

    # Crear directorio base de montaje temporal
    mkdir -p /mnt/mnt 2>/dev/null || true
    MOUNT_COUNTER=1

    # Para sistemas UEFI: Montar todas las particiones EFI detectadas
    if [ "$SYSTEM_TYPE" = "UEFI" ] && [ ${#EFI_PARTITIONS[@]} -gt 0 ]; then
        for partition in "${EFI_PARTITIONS[@]}"; do
            if [ -n "$partition" ]; then
                # Agregar /dev/ si no está presente
                if [[ ! "$partition" =~ ^/dev/ ]]; then
                    partition="/dev/$partition"
                fi

                # Verificar si la partición ya está montada
                if mount | grep -q "^$partition "; then
                    EXISTING_MOUNT=$(mount | grep "^$partition " | awk '{print $3}' | head -1)
                    echo -e "${GREEN}  • $partition ya está montada en $EXISTING_MOUNT${NC}"
                else
                    echo -e "${CYAN}  • Montando $partition${NC}"

                    # Crear directorio de montaje específico
                    mount_point="/mnt/mnt/efi_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true

                    # Montar la partición EFI
                    if mount "$partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Montada en $mount_point${NC}"
                    else
                        echo -e "${YELLOW}    ⚠ No se pudo montar $partition${NC}"
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                fi

                MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
            fi
        done
    fi

    # Para sistemas BIOS Legacy: Montar particiones relevantes para detección
    if [ "$SYSTEM_TYPE" = "BIOS_Legacy" ]; then
        echo -e "${CYAN}  • Montando particiones para detección en BIOS Legacy...${NC}"

        # Montar particiones Windows (NTFS) si existen
        while IFS= read -r ntfs_partition; do
            if [ -n "$ntfs_partition" ]; then
                partition_name=$(basename "$ntfs_partition")
                if ! mount | grep -q "^$ntfs_partition "; then
                    mount_point="/mnt/mnt/windows_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true
                    if mount "$ntfs_partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Windows partition $ntfs_partition montada en $mount_point${NC}"
                    else
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                    MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
                fi
            fi
        done < <(blkid -t TYPE=ntfs -o device 2>/dev/null)

        # Montar particiones Linux (ext4) si existen
        while IFS= read -r ext4_partition; do
            if [ -n "$ext4_partition" ]; then
                partition_name=$(basename "$ext4_partition")
                # Evitar montar la partición root actual del sistema live
                if ! mount | grep -q "^$ext4_partition " && [[ "$ext4_partition" != "$(findmnt -n -o SOURCE /)" ]]; then
                    mount_point="/mnt/mnt/ext4_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true
                    if mount "$ext4_partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Linux partition (ext4) $ext4_partition montada en $mount_point${NC}"
                    else
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                    MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
                fi
            fi
        done < <(blkid -t TYPE=ext4 -o device 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)")

        # Montar particiones Linux (ext3) si existen
        while IFS= read -r ext3_partition; do
            if [ -n "$ext3_partition" ]; then
                partition_name=$(basename "$ext3_partition")
                # Evitar montar la partición root actual del sistema live
                if ! mount | grep -q "^$ext3_partition " && [[ "$ext3_partition" != "$(findmnt -n -o SOURCE /)" ]]; then
                    mount_point="/mnt/mnt/ext3_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true
                    if mount "$ext3_partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Linux partition (ext3) $ext3_partition montada en $mount_point${NC}"
                    else
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                    MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
                fi
            fi
        done < <(blkid -t TYPE=ext3 -o device 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)")

        # Montar particiones Linux (btrfs) si existen
        while IFS= read -r btrfs_partition; do
            if [ -n "$btrfs_partition" ]; then
                partition_name=$(basename "$btrfs_partition")
                # Evitar montar la partición root actual del sistema live
                if ! mount | grep -q "^$btrfs_partition " && [[ "$btrfs_partition" != "$(findmnt -n -o SOURCE /)" ]]; then
                    mount_point="/mnt/mnt/btrfs_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true
                    if mount "$btrfs_partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Linux partition (btrfs) $btrfs_partition montada en $mount_point${NC}"
                    else
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                    MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
                fi
            fi
        done < <(blkid -t TYPE=btrfs -o device 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)")

        # Montar particiones Linux (xfs) si existen
        while IFS= read -r xfs_partition; do
            if [ -n "$xfs_partition" ]; then
                partition_name=$(basename "$xfs_partition")
                # Evitar montar la partición root actual del sistema live
                if ! mount | grep -q "^$xfs_partition " && [[ "$xfs_partition" != "$(findmnt -n -o SOURCE /)" ]]; then
                    mount_point="/mnt/mnt/xfs_$MOUNT_COUNTER"
                    mkdir -p "$mount_point" 2>/dev/null || true
                    if mount "$xfs_partition" "$mount_point" 2>/dev/null; then
                        echo -e "${GREEN}    ✓ Linux partition (xfs) $xfs_partition montada en $mount_point${NC}"
                    else
                        rmdir "$mount_point" 2>/dev/null || true
                    fi
                    MOUNT_COUNTER=$((MOUNT_COUNTER + 1))
                fi
            fi
        done < <(blkid -t TYPE=xfs -o device 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)")
    fi

    # Crear directorios adicionales para otros tipos de sistemas
    mkdir -p /mnt/mnt/windows 2>/dev/null || true
    mkdir -p /mnt/mnt/other 2>/dev/null || true

    # Ejecutar os-prober para detectar otros sistemas
    echo -e "${CYAN}Ejecutando os-prober para detectar otros sistemas...${NC}"
    DETECTED_SYSTEMS=$(arch-chroot /mnt /bin/bash -c "os-prober" 2>/dev/null || true)

    if [ -n "$DETECTED_SYSTEMS" ]; then
        echo -e "${GREEN}✓ Sistemas detectados:${NC}"
        echo "$DETECTED_SYSTEMS" | while IFS= read -r line; do
            if [ -n "$line" ]; then
                echo -e "${CYAN}  • $line${NC}"
            fi
        done

        # Regenerar configuración de GRUB con los sistemas detectados
        echo -e "${CYAN}Regenerando configuración de GRUB con sistemas detectados...${NC}"
        arch-chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"

        # Verificar que se agregaron entradas
        GRUB_ENTRIES=$(arch-chroot /mnt /bin/bash -c "grep -c 'menuentry' /boot/grub/grub.cfg" 2>/dev/null || echo "0")
        echo -e "${GREEN}✓ Configuración GRUB actualizada (${GRUB_ENTRIES} entradas de menú)${NC}"
    else
        echo -e "${YELLOW}⚠ No se detectaron otros sistemas operativos${NC}"
        echo -e "${CYAN}  • Solo se encontró el sistema Arcris Linux actual${NC}"
    fi

    # Limpiar montajes y directorios temporales
    echo -e "${CYAN}Limpiando montajes temporales...${NC}"

    # Desmontar todas las particiones EFI temporales
    for mount_point in /mnt/mnt/efi_*; do
        if [ -d "$mount_point" ]; then
            if mountpoint -q "$mount_point" 2>/dev/null; then
                echo -e "${CYAN}  • Desmontando $mount_point${NC}"
                if ! umount "$mount_point" 2>/dev/null; then
                    echo -e "${YELLOW}    ⚠ Forzando desmontaje de $mount_point${NC}"
                    umount -l "$mount_point" 2>/dev/null || true
                fi
            fi
            rmdir "$mount_point" 2>/dev/null || true
        fi
    done

    # Desmontar todas las particiones Windows temporales (BIOS Legacy)
    for mount_point in /mnt/mnt/windows_*; do
        if [ -d "$mount_point" ]; then
            if mountpoint -q "$mount_point" 2>/dev/null; then
                echo -e "${CYAN}  • Desmontando $mount_point${NC}"
                if ! umount "$mount_point" 2>/dev/null; then
                    echo -e "${YELLOW}    ⚠ Forzando desmontaje de $mount_point${NC}"
                    umount -l "$mount_point" 2>/dev/null || true
                fi
            fi
            rmdir "$mount_point" 2>/dev/null || true
        fi
    done

    # Desmontar todas las particiones Linux temporales (BIOS Legacy)
    for fs_type in ext4 ext3 btrfs xfs; do
        for mount_point in /mnt/mnt/${fs_type}_*; do
            if [ -d "$mount_point" ]; then
                if mountpoint -q "$mount_point" 2>/dev/null; then
                    echo -e "${CYAN}  • Desmontando $mount_point${NC}"
                    if ! umount "$mount_point" 2>/dev/null; then
                        echo -e "${YELLOW}    ⚠ Forzando desmontaje de $mount_point${NC}"
                        umount -l "$mount_point" 2>/dev/null || true
                    fi
                fi
                rmdir "$mount_point" 2>/dev/null || true
            fi
        done
    done

    # Limpiar cualquier otro montaje temporal bajo /mnt/mnt
    if [ -d "/mnt/mnt" ]; then
        for mount_point in /mnt/mnt/*; do
            if [ -d "$mount_point" ] && [[ "$(basename "$mount_point")" != "windows" ]] && [[ "$(basename "$mount_point")" != "other" ]]; then
                if mountpoint -q "$mount_point" 2>/dev/null; then
                    echo -e "${CYAN}  • Desmontando montaje adicional $mount_point${NC}"
                    if ! umount "$mount_point" 2>/dev/null; then
                        echo -e "${YELLOW}    ⚠ Forzando desmontaje de $mount_point${NC}"
                        umount -l "$mount_point" 2>/dev/null || true
                    fi
                fi
                rmdir "$mount_point" 2>/dev/null || true
            fi
        done
    fi

    # Limpiar directorios restantes
    rmdir /mnt/mnt/windows 2>/dev/null || true
    rmdir /mnt/mnt/other 2>/dev/null || true

    # Verificar que no queden montajes en /mnt/mnt antes de eliminar el directorio
    if [ -d "/mnt/mnt" ]; then
        remaining_mounts=$(find /mnt/mnt -type d -exec mountpoint -q {} \; -print 2>/dev/null || true)
        if [ -z "$remaining_mounts" ]; then
            rmdir /mnt/mnt 2>/dev/null || true
        else
            echo -e "${YELLOW}    ⚠ Algunos montajes permanecen en /mnt/mnt${NC}"
        fi
    fi

    echo -e "${GREEN}✓ Limpieza de montajes temporales completada${NC}"
    echo -e "${GREEN}✓ Detección de múltiples sistemas operativos completada${NC}"
else
    if [ "$SYSTEM_TYPE" = "UEFI" ]; then
        echo -e "${YELLOW}⚠ Solo se detectó 1 partición EFI - Sistema UEFI único${NC}"
    else
        echo -e "${YELLOW}⚠ Solo se detectó un sistema operativo - Sistema BIOS Legacy único${NC}"
    fi
    echo -e "${CYAN}  • No es necesario instalar os-prober para un solo sistema${NC}"
fi



echo -e "${GREEN}✓ Configuración de detección de sistemas operativos completada${NC}"
echo ""



sleep 3
clear



# Instalación de drivers de video
echo -e "${GREEN}| Instalando drivers de video: $DRIVER_VIDEO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_VIDEO" in
    "Open Source")
        # Detección automática de hardware de video usando VGA controller
        VGA_LINE=$(lspci | grep -i "vga compatible controller")
        echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"

        if echo "$VGA_LINE" | grep -i nvidia > /dev/null; then
            echo "Detectado hardware NVIDIA - Instalando driver open source nouveau"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-nouveau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-nouveau lib32-vulkan-nouveau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver lib32-libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"

        elif echo "$VGA_LINE" | grep -i "amd\|radeon" > /dev/null; then
            echo "Detectado hardware AMD/Radeon - Instalando driver open source amdgpu"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-amdgpu --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-ati --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-radeon lib32-vulkan-radeon --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S radeontop --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver lib32-libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"

        elif echo "$VGA_LINE" | grep -i intel > /dev/null; then
            echo "Detectado hardware Intel - Instalando driver open source intel"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-intel lib32-vulkan-intel --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S intel-media-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-intel-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S intel-gpu-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
            arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S intel-compute-runtime --noansweredit --noconfirm --needed"
            arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S intel-hybrid-codec-driver-git --noansweredit --noconfirm --needed"
            arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S vpl-gpu-rt --noansweredit --noconfirm --needed"

        elif echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then

            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"

            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S spice-vdagent --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-qxl --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S qemu-guest-agent --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent"



        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"

            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable vboxservice"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"

            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable vboxservice"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            arch-chroot /mnt /bin/bash -c "pacman -S xorg-server --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S xorg-xinit --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-vesa --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"
        fi
        ;;
    "nvidia")
        echo "Instalando driver NVIDIA para kernel linux"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"

        ;;
    "nvidia-lts")
        echo "Instalando driver NVIDIA para kernel LTS"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-lts --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"
        ;;
    "nvidia-dkms")
        echo "Instalando driver NVIDIA DKMS"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-dkms --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"
        ;;
    "nvidia-470xx-dkms")
        echo "Instalando driver NVIDIA serie 470.xx con DKMS"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-dkms --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia-470xx --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-settings --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-470xx-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia-470xx --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S mhwd-nvidia-470xx --noansweredit --noconfirm --needed"
        ;;
    "nvidia-390xx-dkms")
        echo "Instalando driver NVIDIA serie 390.xx con DKMS (hardware antiguo)"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-dkms --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia-390xx --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-390xx-utils --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia-390xx --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-settings --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S mhwd-nvidia-390xx --noansweredit --noconfirm --needed"
        ;;
    "AMD Private")
        echo "Instalando drivers privativos de AMDGPUPRO"
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-amdgpu mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S radeontop --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S amf-amdgpu-pro --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S amdgpu-pro-oglp --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-amdgpu-pro-oglp --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S vulkan-amdgpu-pro --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-vulkan-amdgpu-pro --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-amd --noansweredit --noconfirm --needed"
        ;;
    "Intel Gen(4-9)")
        echo "Instalando drivers Modernos de Intel"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-intel --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S vulkan-intel lib32-vulkan-intel --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S intel-media-driver --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S libva-intel-driver --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S intel-gpu-tools --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
        ;;
    "Máquina Virtual")

    # Detección automática de hardware de video usando VGA controller
    VGA_LINE=$(lspci | grep -i "vga compatible controller")
    echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"

        if  echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then
            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S spice-vdagent --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S xf86-video-qxl --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S qemu-guest-agent --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent"



        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable vboxservice"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            arch-chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            arch-chroot /mnt /bin/bash -c "systemctl enable vboxservice"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-utils --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S mesa-vdpau --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"
        fi
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
        arch-chroot /mnt /bin/bash -c "pacman -S lib32-jack2 --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S jack2-dbus --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S carla --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S qjackctl --noconfirm"
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
        arch-chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        ;;
    "broadcom-wl")
        arch-chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S broadcom-wl networkmanager --noconfirm"
        ;;
    "Realtek")
        arch-chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        arch-chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtl8821cu-dkms-git --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtl8821ce-dkms-git --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtw88-dkms-git --noansweredit --noconfirm --needed"
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

sleep 2
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

# Configurar permisos de archivos de usuario
arch-chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.bashrc"


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

clear
# Actualizar base de datos de paquetes
arch-chroot /mnt /bin/bash -c "pacman -Sy"

clear

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
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-server --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xinit --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xauth --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbs --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbnailer --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S freetype2 --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S poppler-glib --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S libgsf --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S tumbler --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdk-pixbuf2 --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S fontconfig --noansweredit --noconfirm --needed"

        case "$DESKTOP_ENVIRONMENT" in
            "GNOME")
                echo -e "${CYAN}Instalando GNOME Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-extra --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable gdm"
                ;;
            "BUDGIE")
                echo -e "${CYAN}Instalando Budgie Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-desktop --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-extras --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "CINNAMON")
                echo -e "${CYAN}Instalando Cinnamon Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S cinnamon --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S cinnamon-translations --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "DEEPIN")
                echo -e "${CYAN}Instalando Deepin Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S deepin --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S deepin-extra --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "ENLIGHTENMENT")
                echo -e "${CYAN}Instalando Enlightenment Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S enlightenment --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S terminology --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "KDE")
                echo -e "${CYAN}Instalando KDE Plasma Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-wayland-session --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-x11-session --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S kde-applications --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable sddm"
                ;;
            "LXDE")
                echo -e "${CYAN}Instalando LXDE Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxde --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxde-common --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "LXQT")
                echo -e "${CYAN}Instalando LXQt Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxqt --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S breeze-icons --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable sddm"
                ;;
            "MATE")
                echo -e "${CYAN}Instalando MATE Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-extra --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            "XFCE4")
                echo -e "${CYAN}Instalando XFCE4 Desktop...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4 --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4-goodies --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter-settings --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S light-locker --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
            *)
                echo -e "${YELLOW}Entorno de escritorio no reconocido: $DESKTOP_ENVIRONMENT${NC}"
                echo -e "${CYAN}Instalando XFCE4 como alternativa...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4 --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4-goodies --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "systemctl enable lightdm"
                ;;
        esac
        ;;
    "WINDOW_MANAGER")
        echo -e "${GREEN}Instalando gestor de ventanas: $WINDOW_MANAGER${NC}"

        # Instalar X.org y dependencias base para gestores de ventanas
        echo -e "${CYAN}Instalando servidor X.org y dependencias base...${NC}"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-server --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xinit --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xauth --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xterm --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S dmenu --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S rofi --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S pcmanfm --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S dunst --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gedit  --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nano  --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S vim --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S pulseaudio  --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S pavucontrol --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S nitrogen  --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S feh --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S networkmanager  --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"

        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbs --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbnailer --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S freetype2 --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S poppler-glib --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S libgsf --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S raw-thumbnailer --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S tumbler --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdk-pixbuf2 --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S fontconfig --noansweredit --noconfirm --needed"

        # Instalar herramientas adicionales para gestores de ventanas
        echo -e "${CYAN}Instalando Terminales...${NC}"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S alacritty --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S foot --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S kitty --noansweredit --noconfirm --needed"

        # Instalar Ly display manager
        echo -e "${CYAN}Instalando Ly display manager...${NC}"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S ly --noansweredit --noconfirm --needed"
        arch-chroot /mnt /bin/bash -c "systemctl enable ly"

        case "$WINDOW_MANAGER" in
            "I3WM"|"I3")
                echo -e "${CYAN}Instalando i3 Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3-wm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3status --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3lock --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3blocks --noansweredit --noconfirm --needed"
                # Crear configuración básica de i3
                mkdir -p /mnt/home/$USER/.config/i3
                echo "# i3 config file" > /mnt/home/$USER/.config/i3/config
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "AWESOME")
                echo -e "${CYAN}Instalando Awesome Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S awesome --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S vicious --noansweredit --noconfirm --needed"
                # Crear configuración básica de awesome
                mkdir -p /mnt/home/$USER/.config/awesome
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "BSPWM")
                echo -e "${CYAN}Instalando BSPWM Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S bspwm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S sxhkd --noansweredit --noconfirm --needed"
                # Crear configuración básica de bspwm
                mkdir -p /mnt/home/$USER/.config/bspwm
                mkdir -p /mnt/home/$USER/.config/sxhkd
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "DWM")
                echo -e "${CYAN}Instalando DWM Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S git --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S base-devel --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S dwm --noansweredit --noconfirm --needed"
                ;;
            "HYPRLAND")
                echo -e "${CYAN}Instalando Hyprland Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprland --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S waybar --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S wofi --noansweredit --noconfirm --needed"
                # Crear configuración básica de hyprland
                mkdir -p /mnt/home/$USER/.config/hypr
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "OPENBOX")
                echo -e "${CYAN}Instalando Openbox Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S openbox --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S obmenu --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S obconf --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S tint2 --noansweredit --noconfirm --needed"
                # Crear configuración básica de openbox
                mkdir -p /mnt/home/$USER/.config/openbox
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "QTITLE"|"QTILE")
                echo -e "${CYAN}Instalando Qtile Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S qtile --noansweredit --noconfirm --needed"
                # Crear configuración básica de qtile
                mkdir -p /mnt/home/$USER/.config/qtile
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "SWAY")
                echo -e "${CYAN}Instalando Sway Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S sway --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S swaylock --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S swayidle --noansweredit --noconfirm --needed"
                # Crear configuración básica de sway
                mkdir -p /mnt/home/$USER/.config/sway
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "XMONAD")
                echo -e "${CYAN}Instalando XMonad Window Manager...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmonad --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmonad-contrib --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmobar --noansweredit --noconfirm --needed"
                # Crear configuración básica de xmonad
                mkdir -p /mnt/home/$USER/.config/xmonad
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            *)
                echo -e "${YELLOW}Gestor de ventanas no reconocido: $WINDOW_MANAGER${NC}"
                echo -e "${CYAN}Instalando i3 como alternativa...${NC}"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3-wm --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3status --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3lock --noansweredit --noconfirm --needed"
                arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3blocks --noansweredit --noconfirm --needed"
                mkdir -p /mnt/home/$USER/.config/i3
                echo "# i3 config file" > /mnt/home/$USER/.config/i3/config
                arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
        esac



        # Configurar terminales con configuraciones básicas
        echo -e "${CYAN}Configurando terminales...${NC}"

        # Configuración básica para Foot
        mkdir -p /mnt/home/$USER/.config/foot
        cat > /mnt/home/$USER/.config/foot/foot.ini << 'EOF'
[main]
term=xterm-256color
login-shell=no

[bell]
urgent=no
notify=no
visual=no

[scrollback]
lines=1000

[url]
launch=xdg-open ${url}
label-letters=sadfjklewcmpgh
osc8-underline=url-mode
protocols=http, https, ftp, ftps, file, gemini, gopher

[cursor]
style=block
unfocused-style=hollow

[mouse]
hide-when-typing=no

[colors]
foreground=cdd6f4
background=1e1e2e
regular0=45475a
regular1=f38ba8
regular2=a6e3a1
regular3=f9e2af
regular4=89b4fa
regular5=f5c2e7
regular6=94e2d5
regular7=bac2de
bright0=585b70
bright1=f38ba8
bright2=a6e3a1
bright3=f9e2af
bright4=89b4fa
bright5=f5c2e7
bright6=94e2d5
bright7=a6adc8

[key-bindings]
scrollback-up-page=Shift+Page_Up
scrollback-up-half-page=none
scrollback-up-line=none
scrollback-down-page=Shift+Page_Down
scrollback-down-half-page=none
scrollback-down-line=none
clipboard-copy=Control+Shift+c XF86Copy
clipboard-paste=Control+Shift+v XF86Paste
primary-paste=Shift+Insert
search-start=Control+Shift+f
font-increase=Control+plus Control+equal Control+KP_Add
font-decrease=Control+minus Control+KP_Subtract
font-reset=Control+0 Control+KP_0
spawn-terminal=Control+Shift+n
minimize=none
maximize=none
fullscreen=F11
pipe-visible=[sh -c "xurls | fuzzel | xargs -r firefox"] none
pipe-scrollback=[sh -c "xurls | fuzzel | xargs -r firefox"] none
pipe-selected=[xargs -r firefox] none
show-urls-launch=Control+Shift+u
show-urls-copy=none
show-urls-persistent=none
prompt-prev=Control+Shift+z
prompt-next=Control+Shift+x
unicode-input=Control+Shift+u
noop=none

[search-bindings]
cancel=Control+g Control+c Escape
commit=Return
find-prev=Control+r
find-next=Control+s
cursor-left=Left Control+b
cursor-left-word=Control+Left Mod1+b
cursor-right=Right Control+f
cursor-right-word=Control+Right Mod1+f
cursor-home=Home Control+a
cursor-end=End Control+e
delete-prev=BackSpace
delete-prev-word=Mod1+BackSpace Control+BackSpace
delete-next=Delete
delete-next-word=Mod1+d Control+Delete
extend-to-word-boundary=Control+w
extend-to-next-whitespace=Control+Shift+w
clipboard-paste=Control+v Control+Shift+v Control+y XF86Paste
primary-paste=Shift+Insert
unicode-input=none

[url-bindings]
cancel=Control+g Control+c Control+d Escape
toggle-url-visible=t

[text-bindings]
\x03=Mod4+c
EOF

        # Configuración básica para Kitty
        mkdir -p /mnt/home/$USER/.config/kitty
        cat > /mnt/home/$USER/.config/kitty/kitty.conf << 'EOF'
# Font settings
font_family      JetBrains Mono
bold_font        auto
italic_font      auto
bold_italic_font auto
font_size 12.0

# Cursor
cursor_shape block
cursor_beam_thickness 1.5
cursor_underline_thickness 2.0
cursor_blink_interval 0

# Scrollback
scrollback_lines 2000
scrollback_pager less --chop-long-lines --RAW-CONTROL-CHARS +INPUT_LINE_NUMBER
scrollback_pager_history_size 0
wheel_scroll_multiplier 5.0
touch_scroll_multiplier 1.0

# Mouse
mouse_hide_wait 3.0
url_color #0087bd
url_style curly
open_url_modifiers kitty_mod
open_url_with default
url_prefixes http https file ftp gemini irc gopher mailto news git
detect_urls yes

# Performance tuning
repaint_delay 10
input_delay 3
sync_to_monitor yes

# Terminal bell
enable_audio_bell no
visual_bell_duration 0.0
window_alert_on_bell yes
bell_on_tab yes
command_on_bell none

# Window layout
remember_window_size  yes
initial_window_width  640
initial_window_height 400
enabled_layouts *
window_resize_step_cells 2
window_resize_step_lines 2
window_border_width 0.5pt
draw_minimal_borders yes
window_margin_width 0
single_window_margin_width -1
window_padding_width 0
placement_strategy center
active_border_color #00ff00
inactive_border_color #cccccc
bell_border_color #ff5a00
inactive_text_alpha 1.0

# Tab bar
tab_bar_edge bottom
tab_bar_margin_width 0.0
tab_bar_style fade
tab_bar_min_tabs 2
tab_switch_strategy previous
tab_fade 0.25 0.5 0.75 1
tab_separator " ┇"
tab_title_template "{title}"
active_tab_title_template none
active_tab_foreground   #000
active_tab_background   #eee
active_tab_font_style   bold-italic
inactive_tab_foreground #444
inactive_tab_background #999
inactive_tab_font_style normal

# Color scheme (Catppuccin Mocha)
foreground              #CDD6F4
background              #1E1E2E
selection_foreground    #1E1E2E
selection_background    #F5E0DC

# Cursor colors
cursor                  #F5E0DC
cursor_text_color       #1E1E2E

# URL underline color when hovering with mouse
url_color               #F5E0DC

# Kitty window border colors
active_border_color     #B4BEFE
inactive_border_color   #6C7086
bell_border_color       #F9E2AF

# OS Window titlebar colors
wayland_titlebar_color system
macos_titlebar_color system

# Tab bar colors
active_tab_foreground   #11111B
active_tab_background   #CBA6F7
inactive_tab_foreground #CDD6F4
inactive_tab_background #181825
tab_bar_background      #11111B

# Colors for marks (marked text in the terminal)
mark1_foreground #1E1E2E
mark1_background #B4BEFE
mark2_foreground #1E1E2E
mark2_background #CBA6F7
mark3_foreground #1E1E2E
mark3_background #74C7EC

# The 16 terminal colors

# normal
color0 #45475A
color1 #F38BA8
color2 #A6E3A1
color3 #F9E2AF
color4 #89B4FA
color5 #F5C2E7
color6 #94E2D5
color7 #BAC2DE

# bright
color8  #585B70
color9  #F38BA8
color10 #A6E3A1
color11 #F9E2AF
color12 #89B4FA
color13 #F5C2E7
color14 #94E2D5
color15 #A6ADC8

# extended base16 colors
color16 #FAB387
color17 #F2CDCD

# Keyboard shortcuts
kitty_mod ctrl+shift

# Clipboard
map kitty_mod+c copy_to_clipboard
map kitty_mod+v paste_from_clipboard
map kitty_mod+s paste_from_selection
map shift+insert paste_from_selection
map kitty_mod+o pass_selection_to_program

# Scrolling
map kitty_mod+up        scroll_line_up
map kitty_mod+k         scroll_line_up
map kitty_mod+down      scroll_line_down
map kitty_mod+j         scroll_line_down
map kitty_mod+page_up   scroll_page_up
map kitty_mod+page_down scroll_page_down
map kitty_mod+home      scroll_home
map kitty_mod+end       scroll_end

# Window management
map kitty_mod+enter new_window
map kitty_mod+n new_os_window
map kitty_mod+w close_window
map kitty_mod+] next_window
map kitty_mod+[ previous_window
map kitty_mod+f move_window_forward
map kitty_mod+b move_window_backward
map kitty_mod+` move_window_to_top
map kitty_mod+r start_resizing_window
map kitty_mod+1 first_window
map kitty_mod+2 second_window
map kitty_mod+3 third_window
map kitty_mod+4 fourth_window
map kitty_mod+5 fifth_window
map kitty_mod+6 sixth_window
map kitty_mod+7 seventh_window
map kitty_mod+8 eighth_window
map kitty_mod+9 ninth_window
map kitty_mod+0 tenth_window

# Tab management
map kitty_mod+right next_tab
map kitty_mod+left  previous_tab
map kitty_mod+t     new_tab
map kitty_mod+q     close_tab
map kitty_mod+.     move_tab_forward
map kitty_mod+,     move_tab_backward
map kitty_mod+alt+t set_tab_title

# Layout management
map kitty_mod+l next_layout

# Font sizes
map kitty_mod+equal     change_font_size all +2.0
map kitty_mod+plus      change_font_size all +2.0
map kitty_mod+kp_add    change_font_size all +2.0
map kitty_mod+minus     change_font_size all -2.0
map kitty_mod+kp_subtract change_font_size all -2.0
map kitty_mod+backspace change_font_size all 0

# Select and act on visible text
map kitty_mod+e kitten hints
map kitty_mod+p>f kitten hints --type path --program -
map kitty_mod+p>shift+f kitten hints --type path
map kitty_mod+p>l kitten hints --type line --program -
map kitty_mod+p>w kitten hints --type word --program -
map kitty_mod+p>h kitten hints --type hash --program -
map kitty_mod+p>n kitten hints --type linenum

# Miscellaneous
map kitty_mod+f11    toggle_fullscreen
map kitty_mod+f10    toggle_maximized
map kitty_mod+u      kitten unicode_input
map kitty_mod+f2     edit_config_file
map kitty_mod+escape kitty_shell window

# Sending arbitrary text on key presses
map kitty_mod+alt+1 send_text all \x01
map kitty_mod+alt+2 send_text all \x02
map kitty_mod+alt+3 send_text all \x03

# You can use the special action no_op to unmap a keyboard shortcut that is
# assigned in the default configuration
map kitty_mod+space no_op

# You can combine multiple actions to be triggered by a single shortcut
map kitty_mod+e combine : clear_terminal scroll active : send_text normal,application \x0c
EOF

        # Establecer permisos correctos para las configuraciones
        arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config/foot"
        arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config/kitty"

        # Configurar Ly para reconocer los window managers
        echo -e "${CYAN}Configurando Ly display manager...${NC}"
        mkdir -p /mnt/usr/share/xsessions

        # Crear archivos .desktop para cada window manager
        case "$WINDOW_MANAGER" in
            "I3WM"|"I3")
                cat > /mnt/usr/share/xsessions/i3.desktop << EOF
[Desktop Entry]
Name=i3
Comment=improved dynamic tiling window manager
Exec=i3
TryExec=i3
Type=Application
X-LightDM-DesktopName=i3
DesktopNames=i3
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
            "AWESOME")
                cat > /mnt/usr/share/xsessions/awesome.desktop << EOF
[Desktop Entry]
Name=awesome
Comment=Highly configurable framework window manager
Exec=awesome
TryExec=awesome
Type=Application
X-LightDM-DesktopName=awesome
DesktopNames=awesome
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
            "BSPWM")
                cat > /mnt/usr/share/xsessions/bspwm.desktop << EOF
[Desktop Entry]
Name=bspwm
Comment=Binary space partitioning window manager
Exec=bspwm
TryExec=bspwm
Type=Application
X-LightDM-DesktopName=bspwm
DesktopNames=bspwm
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
            "DWM")
                cat > /mnt/usr/share/xsessions/dwm.desktop << EOF
[Desktop Entry]
Name=dwm
Comment=Dynamic window manager
Exec=dwm
TryExec=dwm
Type=Application
X-LightDM-DesktopName=dwm
DesktopNames=dwm
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
            "HYPRLAND")
                cat > /mnt/usr/share/wayland-sessions/hyprland.desktop << EOF
[Desktop Entry]
Name=Hyprland
Comment=An intelligent dynamic tiling Wayland compositor
Exec=Hyprland
Type=Application
EOF
                ;;
            "OPENBOX")
                cat > /mnt/usr/share/xsessions/openbox.desktop << EOF
[Desktop Entry]
Name=Openbox
Comment=A highly configurable, next generation window manager
Exec=openbox-session
TryExec=openbox
Type=Application
X-LightDM-DesktopName=Openbox
DesktopNames=Openbox
Keywords=wm;windowmanager;window;manager;
EOF
                ;;
            "QTITLE"|"QTILE")
                cat > /mnt/usr/share/xsessions/qtile.desktop << EOF
[Desktop Entry]
Name=Qtile
Comment=A full-featured, hackable tiling window manager written in Python
Exec=qtile start
TryExec=qtile
Type=Application
X-LightDM-DesktopName=Qtile
DesktopNames=Qtile
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
            "SWAY")
                cat > /mnt/usr/share/wayland-sessions/sway.desktop << EOF
[Desktop Entry]
Name=Sway
Comment=An i3-compatible Wayland compositor
Exec=sway
Type=Application
EOF
                ;;
            "XMONAD")
                cat > /mnt/usr/share/xsessions/xmonad.desktop << EOF
[Desktop Entry]
Name=XMonad
Comment=Lightweight tiling window manager
Exec=xmonad
TryExec=xmonad
Type=Application
X-LightDM-DesktopName=XMonad
DesktopNames=XMonad
Keywords=tiling;wm;windowmanager;window;manager;
EOF
                ;;
        esac
        ;;
    *)
        echo -e "${YELLOW}Tipo de instalación no reconocido: $INSTALLATION_TYPE${NC}"
        echo -e "${CYAN}Continuando sin instalación de entorno gráfico...${NC}"
        ;;
esac

sleep 3
clear

# Instalación de aplicaciones adicionales basadas en configuración
echo -e "${GREEN}| Instalando aplicaciones adicionales |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Verificar si ESSENTIAL_APPS está habilitado
if [ "${ESSENTIAL_APPS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando shell del sistema: ${SYSTEM_SHELL:-bash}${NC}"

    case "${SYSTEM_SHELL:-bash}" in
        "bash")
            arch-chroot /mnt /bin/bash -c "pacman -S bash bash-completion --noconfirm"
            arch-chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
        "dash")
            arch-chroot /mnt /bin/bash -c "pacman -S dash --noconfirm"
            arch-chroot /mnt /bin/bash -c "chsh -s /bin/dash $USER"
            ;;
        "ksh")
            arch-chroot /mnt /bin/bash -c "pacman -S ksh --noconfirm"
            arch-chroot /mnt /bin/bash -c "chsh -s /usr/bin/ksh $USER"
            ;;
        "fish")
            arch-chroot /mnt /bin/bash -c "pacman -S fish --noconfirm"
            arch-chroot /mnt /bin/bash -c "chsh -s /usr/bin/fish $USER"
            ;;
        "zsh")
            arch-chroot /mnt /bin/bash -c "pacman -S zsh --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S zsh-completions --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S zsh-syntax-highlighting --noconfirm"
            arch-chroot /mnt /bin/bash -c "pacman -S zsh-autosuggestions --noconfirm"
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/home/$USER/.zshrc
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/root/.zshrc
            arch-chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.zshrc"
            arch-chroot /mnt /bin/bash -c "chsh -s /bin/zsh $USER"
            ;;
        *)
            echo -e "${YELLOW}Shell no reconocida: ${SYSTEM_SHELL}, usando bash${NC}"
            arch-chroot /mnt /bin/bash -c "pacman -S bash bash-completion --noconfirm"
            arch-chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
    esac
    echo -e "${GREEN}✓ Shell del sistema configurada${NC}"
fi

# Verificar si FILESYSTEMS está habilitado
if [ "${FILESYSTEMS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de sistemas de archivos...${NC}"

    arch-chroot /mnt /bin/bash -c "pacman -S android-file-transfer --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S android-tools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S android-udev --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S msmtp --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libmtp --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libcddb --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-afc --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-smb --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-gphoto2 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-mtp --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-goa --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-nfs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gvfs-google --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-libav --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S dosfstools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S f2fs-tools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S ntfs-3g --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S udftools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S nilfs-utils --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S polkit --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gpart --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S mtools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S cifs-utils --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S jfsutils --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S btrfs-progs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S xfsprogs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S reiserfsprogs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S e2fsprogs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S exfatprogs --noconfirm"

    echo -e "${GREEN}✓ Herramientas de sistemas de archivos instaladas${NC}"
fi

# Verificar si COMPRESSION está habilitado
if [ "${COMPRESSION_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de compresión...${NC}"

    arch-chroot /mnt /bin/bash -c "pacman -S xarchiver --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S unarchiver --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S binutils --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gzip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lha --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lrzip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lzip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lz4 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S p7zip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S tar --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S xz --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S bzip2 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lbzip2 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S arj --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lzop --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S cpio --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S unrar --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S unzip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S zstd --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S zip --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S unarj --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S dpkg --noconfirm"
    echo -e "${GREEN}✓ Herramientas de compresión instaladas${NC}"
fi

# Verificar si VIDEO_CODECS está habilitado
if [ "${VIDEO_CODECS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando codecs de video...${NC}"

    arch-chroot /mnt /bin/bash -c "pacman -S ffmpeg --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S aom --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libde265 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S x265 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S x264 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libmpeg2 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S xvidcore --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libtheora --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libvpx --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S sdl --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gstreamer --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-plugins-bad --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-plugins-base --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-plugins-base-libs --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-plugins-good --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S gst-plugins-ugly --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S xine-lib --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libdvdcss --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libdvdread --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S dvd+rw-tools --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S lame --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S jasper --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libmng --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libraw --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libkdcraw --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S vcdimager --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S mpv --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S faac --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S faad2 --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S flac --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S opus --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libvorbis --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S wavpack --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libheif --noconfirm"
    arch-chroot /mnt /bin/bash -c "pacman -S libavif --noconfirm"

    echo -e "${GREEN}✓ Codecs de video instalados${NC}"
fi

sleep 2
clear

echo -e "${GREEN}✓ Tipografías instaladas${NC}"
arch-chroot /mnt pacman -S noto-fonts --noconfirm
arch-chroot /mnt pacman -S noto-fonts-emoji --noconfirm
arch-chroot /mnt pacman -S adobe-source-code-pro-fonts --noconfirm
arch-chroot /mnt pacman -S ttf-cascadia-code --noconfirm
arch-chroot /mnt pacman -S cantarell-fonts --noconfirm
arch-chroot /mnt pacman -S ttf-roboto --noconfirm
arch-chroot /mnt pacman -S ttf-ubuntu-font-family --noconfirm
arch-chroot /mnt pacman -S gnu-free-fonts --noconfirm
sleep 2
clear

# Configuración completa del layout de teclado para Xorg y Wayland
echo -e "${GREEN}| Configurando layout de teclado: $KEYBOARD_LAYOUT |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# 1. Configuración con localectl (método universal y permanente)
echo -e "${CYAN}1. Configurando con localectl (permanente para ambos Xorg y Wayland)...${NC}"
arch-chroot /mnt localectl set-keymap $KEYBOARD_LAYOUT
arch-chroot /mnt localectl set-x11-keymap $KEYBOARD_LAYOUT pc105 "" ""

# También ejecutar como usuario para configuración por usuario
# echo -e "${CYAN}1.1. Configurando localectl como usuario...${NC}"
# arch-chroot /mnt /bin/bash -c "sudo -u $USER localectl set-keymap $KEYBOARD_LAYOUT" || echo "Warning: No se pudo configurar keymap para usuario $USER"
# arch-chroot /mnt /bin/bash -c "sudo -u $USER localectl set-x11-keymap $KEYBOARD_LAYOUT pc105 \"\" \"\"" || echo "Warning: No se pudo configurar X11 keymap para usuario $USER"

# 2. Configuración para Xorg (X11)
echo -e "${CYAN}2. Configurando teclado para Xorg (X11)...${NC}"
mkdir -p /mnt/etc/X11/xorg.conf.d
cat > /mnt/etc/X11/xorg.conf.d/00-keyboard.conf << EOF
Section "InputClass"
        Identifier "system-keyboard"
        MatchIsKeyboard "on"
        Option "XkbLayout" "$KEYBOARD_LAYOUT"
        Option "XkbModel" "pc105"
        Option "XkbVariant" ""
        Option "XkbOptions" "grp:alt_shift_toggle"
EndSection
EOF

# 3. Configuración para Wayland
echo -e "${CYAN}3. Configurando teclado para Wayland...${NC}"
mkdir -p /mnt/etc/xdg/wlroots
cat > /mnt/etc/xdg/wlroots/wlr.conf << EOF
[keyboard]
layout=$KEYBOARD_LAYOUT
model=pc105
variant=
options=grp:alt_shift_toggle

[input]
kb_layout=$KEYBOARD_LAYOUT
kb_model=pc105
kb_variant=
kb_options=grp:alt_shift_toggle
EOF

# 4. Configuración persistente del archivo /etc/default/keyboard
echo -e "${CYAN}4. Configurando archivo /etc/default/keyboard...${NC}"
cat > /mnt/etc/default/keyboard << EOF
XKBMODEL="pc105"
XKBLAYOUT="$KEYBOARD_LAYOUT"
XKBVARIANT=""
XKBOPTIONS="grp:alt_shift_toggle"
EOF

# 5. Configuración de la consola virtual (vconsole.conf)
echo -e "${CYAN}5. Configurando consola virtual...${NC}"
echo "KEYMAP=$KEYMAP_TTY" > /mnt/etc/vconsole.conf
echo "FONT=lat0-16" >> /mnt/etc/vconsole.conf

# 6. Configuración para GNOME (si se usa)
echo -e "${CYAN}6. Configurando para GNOME...${NC}"
mkdir -p /mnt/etc/dconf/db/local.d
cat > /mnt/etc/dconf/db/local.d/00-keyboard << EOF
[org/gnome/desktop/input-sources]
sources=[('xkb', '$KEYBOARD_LAYOUT')]
EOF

# 7. Configuración adicional para el usuario
echo -e "${CYAN}7. Configurando variables de entorno para el usuario...${NC}"
cat >> /mnt/home/$USER/.profile << EOF

# Configuración de teclado
export XKB_DEFAULT_LAYOUT=$KEYBOARD_LAYOUT
export XKB_DEFAULT_MODEL=pc105
export XKB_DEFAULT_OPTIONS=grp:alt_shift_toggle
EOF

# 8. Script de configuración automática para el arranque
echo -e "${CYAN}8. Creando script de configuración automática...${NC}"
mkdir -p /mnt/usr/local/bin
cat > /mnt/usr/local/bin/setup-keyboard.sh << 'EOF'
#!/bin/bash
# Script de configuración automática del teclado

KEYBOARD_LAYOUT="${KEYBOARD_LAYOUT}"

# Detectar si estamos en X11 o Wayland
if [ -n "$DISPLAY" ] && command -v setxkbmap >/dev/null 2>&1; then
    # Estamos en X11
    setxkbmap $KEYBOARD_LAYOUT
elif [ -n "$WAYLAND_DISPLAY" ] && command -v gsettings >/dev/null 2>&1; then
    # Estamos en Wayland con GNOME
    gsettings set org.gnome.desktop.input-sources sources "[('xkb', '$KEYBOARD_LAYOUT')]"
fi
EOF

chmod +x /mnt/usr/local/bin/setup-keyboard.sh

# 9. Configuración para autostart en sesiones gráficas
echo -e "${CYAN}9. Configurando autostart para sesiones gráficas...${NC}"
mkdir -p /mnt/etc/xdg/autostart
cat > /mnt/etc/xdg/autostart/keyboard-setup.desktop << EOF
[Desktop Entry]
Type=Application
Name=Keyboard Layout Setup
Exec=/usr/local/bin/setup-keyboard.sh
Hidden=false
NoDisplay=true
X-GNOME-Autostart-enabled=true
EOF

# 10. Establecer permisos correctos
echo -e "${CYAN}10. Estableciendo permisos correctos...${NC}"
arch-chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.profile" 2>/dev/null || true
arch-chroot /mnt chmod 755 /usr/local/bin/setup-keyboard.sh
arch-chroot /mnt chmod 644 /etc/xdg/autostart/keyboard-setup.desktop

# 11. Actualizar base de datos dconf si existe
echo -e "${CYAN}11. Actualizando configuraciones del sistema...${NC}"
arch-chroot /mnt dconf update 2>/dev/null || true



echo -e "${GREEN}✓ Configuración completa del teclado finalizada${NC}"
echo -e "${CYAN}  • Layout: $KEYBOARD_LAYOUT${NC}"
echo -e "${CYAN}  • Keymap TTY: $KEYMAP_TTY${NC}"
echo -e "${CYAN}  • Modelo: pc105${NC}"
echo -e "${CYAN}  • Cambio de layout: Alt+Shift${NC}"
echo -e "${CYAN}  • Métodos configurados: localectl, Xorg, Wayland, GNOME, vconsole${NC}"
echo -e "${YELLOW}  • La configuración será efectiva después del reinicio${NC}"

sleep 4
clear

# Instalación de programas adicionales según configuración
if [ "$UTILITIES_ENABLED" = "true" ] && [ ${#UTILITIES_APPS[@]} -gt 0 ]; then
    echo ""
    echo -e "${GREEN}| Instalando programas de utilidades seleccionados |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    for app in "${UTILITIES_APPS[@]}"; do
        echo -e "${CYAN}Instalando: $app${NC}"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S $app --noansweredit --noconfirm --needed" || {
            echo -e "${YELLOW}⚠ No se pudo instalar $app, continuando...${NC}"
        }
    done

    echo -e "${GREEN}✓ Instalación de programas de utilidades completada${NC}"
    echo ""
    sleep 2
fi

if [ "$PROGRAM_EXTRA" = "true" ] && [ ${#EXTRA_PROGRAMS[@]} -gt 0 ]; then
    echo ""
    echo -e "${GREEN}| Instalando programas extra seleccionados |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    for program in "${EXTRA_PROGRAMS[@]}"; do
        echo -e "${CYAN}Instalando: $program${NC}"
        arch-chroot /mnt /bin/bash -c "sudo -u $USER yay -S $program --noansweredit --noconfirm --needed" || {
            echo -e "${YELLOW}⚠ No se pudo instalar $program, continuando...${NC}"
        }
    done

    echo -e "${GREEN}✓ Instalación de programas extra completada${NC}"
    echo ""
    sleep 2
fi

sleep 3
clear
cp /usr/share/arcrisgui/data/config/pacman-chroot.conf /mnt/etc/pacman.conf
arch-chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
arch-chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
sleep 3
clear

echo ""
ls /mnt/home/$USER/
sleep 5
clear
# Revertir a configuración normal

# Revertir a configuración sudo normal
echo -e "${GREEN}| Revirtiendo configuración sudo temporal |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Eliminar configuración temporal
if [[ -f "/mnt/etc/sudoers.d/temp-install" ]]; then
    arch-chroot /mnt /bin/bash -c "rm -f /etc/sudoers.d/temp-install"
    echo "✓ Configuración temporal eliminada"
else
    echo "⚠️  Archivo temporal no encontrado (ya fue eliminado)"
fi

# Verificar si ya existe la configuración wheel en sudoers
if ! arch-chroot /mnt /bin/bash -c "grep -q '^%wheel.*ALL.*ALL' /etc/sudoers" 2>/dev/null; then
    echo "# Configuración normal del grupo wheel" >> /mnt/etc/sudoers
    echo "%wheel ALL=(ALL) ALL" >> /mnt/etc/sudoers
    echo "✓ Configuración wheel añadida al archivo sudoers"
else
    echo "✓ Configuración wheel ya existe en sudoers"
fi

# Verificar configuración final
echo ""
echo "Configuración sudo actual:"
arch-chroot /mnt /bin/bash -c "grep -E '^%wheel|^[^#]*ALL.*ALL' /etc/sudoers /etc/sudoers.d/* 2>/dev/null || echo 'No se encontraron reglas sudo activas'"

clear

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
sleep 5
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
    echo -e "${CYAN}  • Solo las particiones EFI y boot quedan sin cifrar (necesario para arrancar)${NC}"
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
