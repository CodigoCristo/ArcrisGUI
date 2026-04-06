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

# =============================================================================
# FUNCIONES HELPER PARA partition_auto
# =============================================================================

# Calcula el tamaño de swap en MiB según SWAP_TYPE y la RAM del sistema.
# Exporta SWAP_SIZE_MIB=0 si es zram (sin partición en disco).
_auto_calc_swap() {
    local ram_mib
    ram_mib=$(awk '/MemTotal/ {printf "%d", $2/1024}' /proc/meminfo)

    # Redondear al GB superior si fracción >= 512MB y RAM total >= 3584MB (3.5G)
    local ram_remainder=$(( ram_mib % 1024 ))
    local ram_gb=$(( ram_mib / 1024 ))
    if [ $ram_mib -ge 3584 ] && [ $ram_remainder -ge 512 ]; then
        ram_mib=$(( (ram_gb + 1) * 1024 ))
    fi

    case "$SWAP_TYPE" in
        "zram")
            SWAP_SIZE_MIB=0
            echo -e "${CYAN}  • Swap: zram (sin partición en disco, RAM=${ram_mib}MiB)${NC}"
            ;;
        "half")
            SWAP_SIZE_MIB=$(( ram_mib / 2 ))
            echo -e "${CYAN}  • Swap: mitad de RAM = ${SWAP_SIZE_MIB}MiB${NC}"
            ;;
        "equal")
            SWAP_SIZE_MIB=$ram_mib
            echo -e "${CYAN}  • Swap: igual a RAM = ${SWAP_SIZE_MIB}MiB${NC}"
            ;;
        "custom")
            SWAP_SIZE_MIB=$(( SWAP_CUSTOM_SIZE * 1024 ))
            echo -e "${CYAN}  • Swap: custom = ${SWAP_SIZE_MIB}MiB${NC}"
            ;;
        *)
            SWAP_SIZE_MIB=0
            echo -e "${YELLOW}  • Swap: tipo desconocido '$SWAP_TYPE', usando zram${NC}"
            ;;
    esac
}

# Limpia el disco completamente antes de particionar.
_auto_wipe_disk() {
    echo -e "${CYAN}Limpiando disco completamente...${NC}"
    sgdisk --zap-all "$SELECTED_DISK"
    sleep 2
    partprobe "$SELECTED_DISK"
    wipefs -af "$SELECTED_DISK"
    sync
    sleep 2
    udevadm settle --timeout=10
}

# Formatea la partición root según FILESYSTEM_TYPE.
# $1 = dispositivo (ej: /dev/sda2)
_auto_format_root() {
    local dev="$1"
    case "$FILESYSTEM_TYPE" in
        "btrfs")
            echo -e "${CYAN}Formateando root como BTRFS...${NC}"
            mkfs.btrfs -f "$dev"
            ;;
        "xfs")
            echo -e "${CYAN}Formateando root como XFS...${NC}"
            mkfs.xfs -f "$dev"
            ;;
        *)
            echo -e "${CYAN}Formateando root como EXT4...${NC}"
            mkfs.ext4 -F "$dev"
            ;;
    esac
    sleep 2
}

# Monta root, home y crea subvolúmenes BTRFS si aplica.
# $1=root_dev  $2=home_dev (vacío si no hay partición home separada)
_auto_mount_root_and_home() {
    local root_dev="$1"
    local home_dev="$2"

    case "$FILESYSTEM_TYPE" in
        "btrfs")
            # Montar temporalmente para crear subvolúmenes
            mount "$root_dev" /mnt
            echo -e "${CYAN}Creando subvolúmenes BTRFS...${NC}"
            btrfs subvolume create /mnt/@
            btrfs subvolume create /mnt/@var_log
            # Crear @home solo si no hay partición separada
            if [ "$HOME_PARTITION" = "subvolume" ]; then
                btrfs subvolume create /mnt/@home
            fi
            umount /mnt

            # Montar subvolumen root
            mount -o noatime,compress=zstd,space_cache=v2,subvol=@ "$root_dev" /mnt
            mkdir -p /mnt/var/log
            mount -o noatime,compress=zstd,space_cache=v2,subvol=@var_log "$root_dev" /mnt/var/log

            # Montar home
            if [ "$HOME_PARTITION" = "subvolume" ]; then
                mkdir -p /mnt/home
                mount -o noatime,compress=zstd,space_cache=v2,subvol=@home "$root_dev" /mnt/home
            elif [ "$HOME_PARTITION" = "partition" ] && [ -n "$home_dev" ]; then
                mkfs.btrfs -f "$home_dev"
                sleep 2
                mkdir -p /mnt/home
                mount -o noatime,compress=zstd,space_cache=v2 "$home_dev" /mnt/home
            fi
            ;;
        "xfs")
            mount -t xfs "$root_dev" /mnt
            if [ "$HOME_PARTITION" = "partition" ] && [ -n "$home_dev" ]; then
                mkfs.xfs -f "$home_dev"
                sleep 2
                mkdir -p /mnt/home
                mount -t xfs -o noatime "$home_dev" /mnt/home
            fi
            ;;
        *)
            mount "$root_dev" /mnt
            if [ "$HOME_PARTITION" = "partition" ] && [ -n "$home_dev" ]; then
                mkfs.ext4 -F "$home_dev"
                sleep 2
                mkdir -p /mnt/home
                mount "$home_dev" /mnt/home
            fi
            ;;
    esac
}

# Activa swap en disco si SWAP_SIZE_MIB > 0.
# $1 = dispositivo swap
_auto_activate_swap() {
    local swap_dev="$1"
    if [ "$SWAP_SIZE_MIB" -eq 0 ]; then
        return 0
    fi

    echo -e "${CYAN}Verificando partición swap antes de activar...${NC}"
    sleep 3
    udevadm settle --timeout=10
    partprobe "$SELECTED_DISK"

    if ! blkid "$swap_dev" | grep -q "TYPE=\"swap\""; then
        echo -e "${YELLOW}Warning: swap no detectada, reformateando...${NC}"
        mkswap "$swap_dev" || { echo -e "${RED}ERROR: No se pudo reformatear swap${NC}"; exit 1; }
        sleep 2
    fi
    swapon "$swap_dev"
    echo -e "${GREEN}✓ Swap activada: $swap_dev (${SWAP_SIZE_MIB}MiB)${NC}"
}

# =============================================================================
# FUNCIÓN PRINCIPAL: partition_auto
# Soporta FILESYSTEM_TYPE=ext4|btrfs|xfs
#           HOME_PARTITION=no|subvolume|partition
#           ROOT_SIZE=<GB>
#           SWAP_TYPE=zram|half|equal|custom  SWAP_CUSTOM_SIZE=<GB>
# =============================================================================
partition_auto() {
    echo -e "${GREEN}| Particionado automático: $SELECTED_DISK ($FILESYSTEM_TYPE) |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    echo -e "${CYAN}  • Filesystem: $FILESYSTEM_TYPE${NC}"
    echo -e "${CYAN}  • Home: $HOME_PARTITION${NC}"
    echo -e "${CYAN}  • Root size: ${ROOT_SIZE}GB${NC}"
    sleep 2

    # Calcular tamaño de swap
    _auto_calc_swap

    local swap_has_partition=false
    [ "$SWAP_SIZE_MIB" -gt 0 ] && swap_has_partition=true

    # Calcular offsets de particiones (en MiB)
    local efi_start=1 efi_end=513          # 512MiB EFI (UEFI) o inicio BIOS
    local swap_start swap_end
    local root_start root_end
    local home_start="100%"

    # ROOT_SIZE en GB → MiB
    local root_size_mib=$(( ROOT_SIZE * 1024 ))

    _auto_wipe_disk

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        echo -e "${GREEN}| Configurando particiones UEFI |${NC}"
        parted "$SELECTED_DISK" --script --align optimal mklabel gpt
        parted "$SELECTED_DISK" --script --align optimal mkpart ESP fat32 1MiB 513MiB
        parted "$SELECTED_DISK" --script set 1 esp on

        local next_offset=513
        local part_num=2
        local swap_part="" root_part="" home_part=""

        # Partición swap (opcional)
        if [ "$swap_has_partition" = true ]; then
            local swap_end_mib=$(( next_offset + SWAP_SIZE_MIB ))
            parted "$SELECTED_DISK" --script --align optimal mkpart primary linux-swap "${next_offset}MiB" "${swap_end_mib}MiB"
            swap_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
            next_offset=$swap_end_mib
            (( part_num++ ))
        fi

        # Partición root
        if [ "$HOME_PARTITION" = "partition" ]; then
            local root_end_mib=$(( next_offset + root_size_mib ))
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "${root_end_mib}MiB"
            root_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
            next_offset=$root_end_mib
            (( part_num++ ))
            # Partición home (resto del disco)
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "100%"
            home_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
        else
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "100%"
            root_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
        fi

        partprobe "$SELECTED_DISK"
        sleep 3
        udevadm settle --timeout=10

        # Formatear EFI
        mkfs.fat -F32 -v "$(get_partition_name "$SELECTED_DISK" "1")"

        # Formatear swap
        if [ "$swap_has_partition" = true ]; then
            mkswap "$swap_part"
        fi

        # Formatear y montar root (y home si aplica)
        _auto_format_root "$root_part"
        _auto_activate_swap "$swap_part"
        _auto_mount_root_and_home "$root_part" "$home_part"

        mkdir -p /mnt/boot
        mount "$(get_partition_name "$SELECTED_DISK" "1")" /mnt/boot

    else
        echo -e "${GREEN}| Configurando particiones BIOS Legacy |${NC}"
        parted "$SELECTED_DISK" --script --align optimal mklabel msdos

        local next_offset=1
        local part_num=1
        local swap_part="" root_part="" home_part=""

        # Partición swap (opcional)
        if [ "$swap_has_partition" = true ]; then
            local swap_end_mib=$(( next_offset + SWAP_SIZE_MIB ))
            parted "$SELECTED_DISK" --script --align optimal mkpart primary linux-swap "${next_offset}MiB" "${swap_end_mib}MiB"
            swap_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
            next_offset=$swap_end_mib
            (( part_num++ ))
        fi

        # Partición root
        if [ "$HOME_PARTITION" = "partition" ]; then
            local root_end_mib=$(( next_offset + root_size_mib ))
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "${root_end_mib}MiB"
            root_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
            # Marcar boot si es la primera partición (sin swap) o la segunda
            parted "$SELECTED_DISK" --script set "$part_num" boot on
            next_offset=$root_end_mib
            (( part_num++ ))
            # Partición home
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "100%"
            home_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
        else
            parted "$SELECTED_DISK" --script --align optimal mkpart primary "$FILESYSTEM_TYPE" "${next_offset}MiB" "100%"
            root_part=$(get_partition_name "$SELECTED_DISK" "$part_num")
            parted "$SELECTED_DISK" --script set "$part_num" boot on
        fi

        partprobe "$SELECTED_DISK"
        sleep 3
        udevadm settle --timeout=10

        # Formatear swap
        if [ "$swap_has_partition" = true ]; then
            mkswap "$swap_part"
        fi

        # Formatear y montar root (y home si aplica)
        _auto_format_root "$root_part"
        _auto_activate_swap "$swap_part"
        _auto_mount_root_and_home "$root_part" "$home_part"

        mkdir -p /mnt/boot
    fi

    # Instalar herramientas según filesystem
    case "$FILESYSTEM_TYPE" in
        "btrfs")
            install_pacstrap_with_retry "btrfs-progs"
            install_pacstrap_with_retry "btrfsmaintenance"
            install_pacstrap_with_retry "snapper"
            install_pacstrap_with_retry "btrfs-assistant"
            ;;
        "xfs")
            install_pacstrap_with_retry "xfsprogs"
            ;;
    esac

    echo -e "${GREEN}✓ Particionado automático completado${NC}"
    lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT "$SELECTED_DISK"
    sleep 3
}

# Función legacy (reemplazada por partition_auto con FILESYSTEM_TYPE=btrfs)
partition_auto_btrfs() {
    echo -e "${GREEN}| Particionando automáticamente disco: $SELECTED_DISK (BTRFS) |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    sleep 2

    # Limpieza agresiva del disco ANTES de cualquier particionado
    echo -e "${CYAN}Desmontando todas las particiones del disco ${SELECTED_DISK}...${NC}"

    # Desmontar todas las particiones montadas del disco seleccionado
    for partition in $(lsblk -lno NAME ${SELECTED_DISK} | grep -v "^$(basename ${SELECTED_DISK})$" | sort -r); do
        partition_path="/dev/$partition"
        if mountpoint -q "/mnt" && grep -q "$partition_path" /proc/mounts; then
            echo -e "${YELLOW}Desmontando $partition_path de /mnt...${NC}"
            umount -R /mnt 2>/dev/null || umount -l /mnt 2>/dev/null || true
        fi
        if grep -q "$partition_path" /proc/mounts; then
            echo -e "${YELLOW}Desmontando $partition_path...${NC}"
            umount -f "$partition_path" 2>/dev/null || umount -l "$partition_path" 2>/dev/null || true
        fi
    done

    # Desactivar swap si está en el disco seleccionado
    echo -e "${CYAN}Desactivando swap en ${SELECTED_DISK}...${NC}"
    for partition in $(lsblk -lno NAME ${SELECTED_DISK} | grep -v "^$(basename ${SELECTED_DISK})$"); do
        swapoff "/dev/$partition" 2>/dev/null || true
    done

    # Limpiar estructuras BTRFS existentes
    echo -e "${CYAN}Limpiando estructuras BTRFS existentes...${NC}"
    for partition in $(lsblk -lno NAME ${SELECTED_DISK} | grep -v "^$(basename ${SELECTED_DISK})$"); do
        wipefs -af "/dev/$partition" 2>/dev/null || true
    done

    # Limpiar completamente el disco - cabecera y final
    echo -e "${CYAN}Limpieza completa del disco ${SELECTED_DISK}...${NC}"
    # Limpiar los primeros 100MB (tablas de partición, etc.)
    dd if=/dev/zero of=$SELECTED_DISK bs=1M count=100 2>/dev/null || true
    # Limpiar los últimos 100MB (backup de tablas GPT)
    DISK_SIZE=$(blockdev --getsz $SELECTED_DISK)
    DISK_SIZE_MB=$((DISK_SIZE * 512 / 1024 / 1024))
    if [ $DISK_SIZE_MB -gt 200 ]; then
        dd if=/dev/zero of=$SELECTED_DISK bs=1M seek=$((DISK_SIZE_MB - 100)) count=100 2>/dev/null || true
    fi
    sync
    sleep 5

    # Forzar re-lectura de la tabla de particiones
    blockdev --rereadpt $SELECTED_DISK 2>/dev/null || true
    partprobe $SELECTED_DISK 2>/dev/null || true

    # Reinicializar kernel sobre el dispositivo
    echo -e "${CYAN}Reinicializando kernel sobre el dispositivo...${NC}"
    # Intentar rescan solo si el archivo existe y tenemos permisos
    RESCAN_FILE="/sys/block/$(basename $SELECTED_DISK)/device/rescan"
    if [ -w "$RESCAN_FILE" ]; then
        echo 1 > "$RESCAN_FILE" 2>/dev/null || true
    fi
    udevadm settle --timeout=10
    udevadm trigger --subsystem-match=block
    udevadm settle --timeout=10

    # Verificaciones adicionales
    echo -e "${CYAN}Verificando estado del disco después de la limpieza...${NC}"
    if ! [ -b "$SELECTED_DISK" ]; then
        echo -e "${RED}ERROR: El disco $SELECTED_DISK no es un dispositivo de bloque válido${NC}"
        exit 1
    fi

    # Verificar que no hay particiones activas
    if [ $(lsblk -n -o NAME $SELECTED_DISK | grep -c "├─\|└─") -gt 0 ]; then
        echo -e "${YELLOW}Warning: Aún se detectan particiones. Realizando limpieza adicional...${NC}"
        sgdisk --clear $SELECTED_DISK 2>/dev/null || true
        wipefs -af $SELECTED_DISK 2>/dev/null || true
        partprobe $SELECTED_DISK 2>/dev/null || true
        sleep 2
    fi

    echo -e "${GREEN}✓ Disco limpio y listo para particionado${NC}"
    sleep 3

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Configuración para UEFI
        echo -e "${GREEN}| Configurando particiones BTRFS para UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Borrar completamente el disco
        echo -e "${CYAN}Creando nueva tabla de particiones...${NC}"
        sgdisk --zap-all $SELECTED_DISK
        sleep 2
        partprobe $SELECTED_DISK
        wipefs -af $SELECTED_DISK

        # Crear tabla de particiones GPT
        echo -e "${CYAN}Creando tabla de particiones GPT...${NC}"
        parted $SELECTED_DISK --script --align optimal mklabel gpt || {
            echo -e "${RED}ERROR: No se pudo crear tabla GPT${NC}"
            exit 1
        }
        sleep 2
        partprobe $SELECTED_DISK

        # Crear partición EFI (512MB)
        echo -e "${CYAN}Creando partición EFI...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart ESP fat32 1MiB 513MiB || {
            echo -e "${RED}ERROR: No se pudo crear partición EFI${NC}"
            exit 1
        }
        parted $SELECTED_DISK --script set 1 esp on
        sleep 1

        # Crear partición swap (8GB)
        echo -e "${CYAN}Creando partición swap...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 513MiB 8705MiB || {
            echo -e "${RED}ERROR: No se pudo crear partición swap${NC}"
            exit 1
        }
        sleep 1

        # Crear partición root (resto del disco)
        echo -e "${CYAN}Creando partición root...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 8705MiB 100% || {
            echo -e "${RED}ERROR: No se pudo crear partición root${NC}"
            exit 1
        }

        # Verificar creación de particiones
        partprobe $SELECTED_DISK
        sleep 3
        udevadm settle --timeout=10

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BTRFS UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.fat -F32 -v $(get_partition_name "$SELECTED_DISK" "1")
        mkswap $(get_partition_name "$SELECTED_DISK" "2")

        # Verificar que el sistema reconozca la nueva swap BTRFS
        echo -e "${CYAN}Esperando reconocimiento del sistema para partición swap...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        mkfs.btrfs -f $(get_partition_name "$SELECTED_DISK" "3")
        sleep 2

        # Verificar que las particiones estén disponibles y no montadas
        echo -e "${CYAN}Verificando particiones creadas...${NC}"
        sleep 5
        partprobe $SELECTED_DISK
        sleep 2

        # Verificar que las particiones no estén montadas
        for i in 1 2 3; do
            if mountpoint -q "${SELECTED_DISK}${i}" 2>/dev/null; then
                echo -e "${YELLOW}Desmontando ${SELECTED_DISK}${i}...${NC}"
                umount -f "${SELECTED_DISK}${i}" 2>/dev/null || true
            fi
            if swapon --show=NAME --noheadings 2>/dev/null | grep -q "${SELECTED_DISK}${i}"; then
                echo -e "${YELLOW}Desactivando swap ${SELECTED_DISK}${i}...${NC}"
                swapoff "${SELECTED_DISK}${i}" 2>/dev/null || true
            fi
        done

        lsblk $SELECTED_DISK
        sleep 2

        # Montar y crear subvolúmenes BTRFS
        echo -e "${GREEN}| Creando subvolúmenes BTRFS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Verificar que la partición no esté montada antes de montar
        echo -e "${CYAN}Preparando montaje de partición BTRFS...${NC}"
        if mountpoint -q /mnt; then
            echo -e "${YELLOW}Desmontando /mnt recursivamente...${NC}"
            umount -R /mnt 2>/dev/null || umount -l /mnt 2>/dev/null || true
            sleep 2
        fi

        # Verificar específicamente la partición BTRFS
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        if mountpoint -q "$PARTITION_3" 2>/dev/null; then
            echo -e "${YELLOW}Desmontando $PARTITION_3...${NC}"
            umount -f "$PARTITION_3" 2>/dev/null || true
            sleep 2
        fi

        echo -e "${CYAN}Montando partición BTRFS $PARTITION_3 en /mnt...${NC}"
        mount "$PARTITION_3" /mnt || {
            echo -e "${RED}ERROR: No se pudo montar $PARTITION_3${NC}"
            exit 1
        }

        # Limpiar contenido existente del filesystem BTRFS
        echo -e "${CYAN}Limpiando contenido existente del filesystem BTRFS...${NC}"
        find /mnt -mindepth 1 -maxdepth 1 -not -name 'lost+found' -exec rm -rf {} + 2>/dev/null || true

        # No necesitamos eliminar subvolúmenes porque el filesystem está recién formateado

        # Crear subvolúmenes BTRFS
        echo -e "${CYAN}Creando subvolúmenes BTRFS...${NC}"
        btrfs subvolume create /mnt/@
        btrfs subvolume create /mnt/@home
        btrfs subvolume create /mnt/@var_log
        umount /mnt

        # Montar subvolúmenes
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        PARTITION_2=$(get_partition_name "$SELECTED_DISK" "2")
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ "$PARTITION_3" /mnt

        # Verificar que la partición swap esté formateada correctamente antes de activar
        echo -e "${CYAN}Verificando partición swap antes de activar...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        if ! blkid "$PARTITION_2" | grep -q "TYPE=\"swap\""; then
            echo -e "${RED}ERROR: Partición swap no está formateada correctamente${NC}"
            echo -e "${YELLOW}Intentando reformatear la partición swap...${NC}"
            mkswap "$PARTITION_2" || {
                echo -e "${RED}ERROR: No se pudo reformatear la partición swap${NC}"
                exit 1
            }
            sleep 2
        fi

        echo -e "${CYAN}Activando partición swap...${NC}"
        swapon "$PARTITION_2"
        mkdir -p /mnt/{boot/efi,home,var/log}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home "$PARTITION_3" /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var_log "$PARTITION_3" /mnt/var/log
        mount "$PARTITION_1" /mnt/boot

        # Instalar herramientas específicas para BTRFS
        install_pacstrap_with_retry "btrfs-progs"
        install_pacstrap_with_retry "btrfsmaintenance"
        install_pacstrap_with_retry "snapper"
        install_pacstrap_with_retry "btrfs-assistant"

    else
        # Configuración para BIOS Legacy
        echo -e "${GREEN}| Configurando particiones BTRFS para BIOS Legacy |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Crear tabla de particiones MBR
        echo -e "${CYAN}Creando tabla de particiones MBR...${NC}"
        parted $SELECTED_DISK --script --align optimal mklabel msdos || {
            echo -e "${RED}ERROR: No se pudo crear tabla MBR${NC}"
            exit 1
        }
        sleep 2
        partprobe $SELECTED_DISK

        # Crear partición boot (1GB) - necesaria para GRUB en BIOS Legacy con BTRFS
        echo -e "${CYAN}Creando partición boot...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart primary ext4 1MiB 1025MiB || {
            echo -e "${RED}ERROR: No se pudo crear partición boot${NC}"
            exit 1
        }
        parted $SELECTED_DISK --script set 1 boot on
        sleep 1

        # Crear partición swap (8GB)
        echo -e "${CYAN}Creando partición swap...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart primary linux-swap 1025MiB 9217MiB || {
            echo -e "${RED}ERROR: No se pudo crear partición swap${NC}"
            exit 1
        }
        sleep 1

        # Crear partición root (resto del disco)
        echo -e "${CYAN}Creando partición root...${NC}"
        parted $SELECTED_DISK --script --align optimal mkpart primary btrfs 9217MiB 100% || {
            echo -e "${RED}ERROR: No se pudo crear partición root${NC}"
            exit 1
        }

        # Verificar creación de particiones
        partprobe $SELECTED_DISK
        sleep 3
        udevadm settle --timeout=10

        # Formatear particiones
        echo -e "${GREEN}| Formateando particiones BTRFS BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mkfs.ext4 -F $(get_partition_name "$SELECTED_DISK" "1")
        mkswap $(get_partition_name "$SELECTED_DISK" "2")

        # Verificar que el sistema reconozca la nueva swap BTRFS BIOS
        echo -e "${CYAN}Esperando reconocimiento del sistema para partición swap...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        mkfs.btrfs -f $(get_partition_name "$SELECTED_DISK" "3")
        sleep 2

        # Verificar que las particiones estén disponibles y no montadas
        echo -e "${CYAN}Verificando particiones creadas...${NC}"
        sleep 5
        partprobe $SELECTED_DISK
        sleep 2

        # Verificar que las particiones no estén montadas
        for i in 1 2 3; do
            if mountpoint -q "${SELECTED_DISK}${i}" 2>/dev/null; then
                echo -e "${YELLOW}Desmontando ${SELECTED_DISK}${i}...${NC}"
                umount -f "${SELECTED_DISK}${i}" 2>/dev/null || true
            fi
            if swapon --show=NAME --noheadings 2>/dev/null | grep -q "${SELECTED_DISK}${i}"; then
                echo -e "${YELLOW}Desactivando swap ${SELECTED_DISK}${i}...${NC}"
                swapoff "${SELECTED_DISK}${i}" 2>/dev/null || true
            fi
        done

        lsblk $SELECTED_DISK
        sleep 2

        # Montar y crear subvolúmenes BTRFS
        echo -e "${GREEN}| Creando subvolúmenes BTRFS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""

        # Verificar que la partición no esté montada antes de montar
        echo -e "${CYAN}Preparando montaje de partición BTRFS...${NC}"
        if mountpoint -q /mnt; then
            echo -e "${YELLOW}Desmontando /mnt recursivamente...${NC}"
            umount -R /mnt 2>/dev/null || umount -l /mnt 2>/dev/null || true
            sleep 2
        fi

        # Verificar específicamente la partición BTRFS
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        if mountpoint -q "$PARTITION_3" 2>/dev/null; then
            echo -e "${YELLOW}Desmontando $PARTITION_3...${NC}"
            umount -f "$PARTITION_3" 2>/dev/null || true
            sleep 2
        fi

        echo -e "${CYAN}Montando partición BTRFS $PARTITION_3 en /mnt...${NC}"
        mount "$PARTITION_3" /mnt || {
            echo -e "${RED}ERROR: No se pudo montar $PARTITION_3${NC}"
            exit 1
        }

        # Limpiar contenido existente del filesystem BTRFS
        echo -e "${CYAN}Limpiando contenido existente del filesystem BTRFS...${NC}"
        find /mnt -mindepth 1 -maxdepth 1 -not -name 'lost+found' -exec rm -rf {} + 2>/dev/null || true

        # No necesitamos eliminar subvolúmenes porque el filesystem está recién formateado

        # Crear subvolúmenes BTRFS
        echo -e "${CYAN}Creando subvolúmenes BTRFS...${NC}"
        btrfs subvolume create /mnt/@
        btrfs subvolume create /mnt/@home
        btrfs subvolume create /mnt/@var_log
        umount /mnt

        # Montar subvolúmenes
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        PARTITION_2=$(get_partition_name "$SELECTED_DISK" "2")
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ "$PARTITION_3" /mnt

        # Verificar que la partición swap esté formateada correctamente antes de activar
        echo -e "${CYAN}Verificando partición swap antes de activar...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        if ! blkid "$PARTITION_2" | grep -q "TYPE=\"swap\""; then
            echo -e "${RED}ERROR: Partición swap no está formateada correctamente${NC}"
            echo -e "${YELLOW}Intentando reformatear la partición swap...${NC}"
            mkswap "$PARTITION_2" || {
                echo -e "${RED}ERROR: No se pudo reformatear la partición swap${NC}"
                exit 1
            }
            sleep 2
        fi

        echo -e "${CYAN}Activando partición swap...${NC}"
        swapon "$PARTITION_2"
        mkdir -p /mnt/{boot,home,var/log}
        mount "$PARTITION_1" /mnt/boot
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home "$PARTITION_3" /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var_log "$PARTITION_3" /mnt/var/log

        # Instalar herramientas específicas para BTRFS
        install_pacstrap_with_retry "btrfs-progs"
        install_pacstrap_with_retry "btrfsmaintenance"
        install_pacstrap_with_retry "snapper"
        install_pacstrap_with_retry "btrfs-assistant"
    fi
}

# Función para particionado con cifrado LUKS (simplificada)
partition_cifrado() {
    echo -e "${GREEN}| Particionando disco con cifrado LUKS: $SELECTED_DISK |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    echo -e "${CYAN}Esta configuración implementa LUKS+LVM:${NC}"
    echo -e "${CYAN}  • Cifrado completo del sistema (excepto boot)${NC}"
    echo -e "${CYAN}  • Compatible con UEFI y BIOS Legacy${NC}"
    echo -e "${CYAN}  • ⚠️  SIN LA CONTRASEÑA PERDERÁS TODOS LOS DATOS${NC}"
    echo ""

    echo -e "${GREEN}✓ Usando contraseña de cifrado configurada${NC}"
    sleep 1

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
        mkfs.fat -F32 $(get_partition_name "$SELECTED_DISK" "1")
        mkfs.ext4 -F $(get_partition_name "$SELECTED_DISK" "2")

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
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        wipefs -af "$PARTITION_3" 2>/dev/null || true
        dd if=/dev/zero of="$PARTITION_3" bs=1M count=10 2>/dev/null || true

        # Cifrar partición principal con LUKS
        echo -e "${GREEN}| Cifrando $PARTITION_3 con LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo -e "${CYAN}Aplicando cifrado... (puede tardar unos minutos)${NC}"

        # Crear dispositivo LUKS usando archivo temporal para contraseña
        echo -n "$ENCRYPTION_PASSWORD" > /tmp/luks_pass

        if ! cryptsetup luksFormat --batch-mode --key-file /tmp/luks_pass "$PARTITION_3"; then
            rm -f /tmp/luks_pass
            echo -e "${RED}ERROR: Falló el cifrado LUKS${NC}"
            exit 1
        fi

        if ! cryptsetup open --key-file /tmp/luks_pass "$PARTITION_3" cryptlvm; then
            rm -f /tmp/luks_pass
            echo -e "${RED}ERROR: No se pudo abrir dispositivo cifrado${NC}"
            exit 1
        fi

        rm -f /tmp/luks_pass
        echo -e "${GREEN}✓ Cifrado LUKS aplicado y dispositivo abierto${NC}"

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup "$PARTITION_3" --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS (Simplificado)
        echo -e "${GREEN}| Configurando LVM sobre dispositivo cifrado |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _

        # Crear LVM sobre el dispositivo cifrado
        echo -e "${CYAN}Configurando LVM...${NC}"
        pvcreate /dev/mapper/cryptlvm
        vgcreate vg0 /dev/mapper/cryptlvm
        lvcreate -L 8G vg0 -n swap
        lvcreate -l 100%FREE vg0 -n root

        # Activar volúmenes
        vgchange -a y vg0
        sleep 2

        echo -e "${GREEN}✓ LVM configurado: vg0 con swap(8GB) y root${NC}"

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

        # Verificar que el sistema reconozca el swap LVM
        echo -e "${CYAN}Esperando reconocimiento del sistema para swap LVM...${NC}"
        sleep 3
        udevadm settle --timeout=10

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        # Verificar que el swap LVM esté disponible antes de activar
        echo -e "${CYAN}Verificando swap LVM antes de activar...${NC}"
        sleep 3
        udevadm settle --timeout=10

        if ! blkid /dev/vg0/swap | grep -q "TYPE=\"swap\""; then
            echo -e "${RED}ERROR: Swap LVM no está formateada correctamente${NC}"
            echo -e "${YELLOW}Intentando reformatear el swap LVM...${NC}"
            mkswap /dev/vg0/swap || {
                echo -e "${RED}ERROR: No se pudo reformatear el swap LVM${NC}"
                exit 1
            }
            sleep 2
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

        # Verificar que las particiones existan antes de montar
        echo -e "${CYAN}Verificando particiones antes del montaje...${NC}"
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        if [ ! -b "$PARTITION_1" ]; then
            echo -e "${RED}ERROR: Partición EFI $PARTITION_1 no existe${NC}"
            exit 1
        fi
        if [ ! -b "$PARTITION_1" ]; then
            echo -e "${RED}ERROR: Partición boot $PARTITION_1 no existe${NC}"
            exit 1
        fi

        # Esperar que las particiones estén completamente listas
        sleep 2
        sync

        echo -e "${CYAN}Creando directorio de montaje boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount "$PARTITION_1" /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición boot${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando directorio boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición EFI...${NC}"
        if ! mount "$PARTITION_1" /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición EFI${NC}"
            exit 1
        fi

        # Verificar que los montajes sean exitosos (en orden correcto)
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Todas las particiones montadas correctamente${NC}"
        echo -e "${GREEN}✓ Esquema LUKS+LVM configurado:${NC}"
        echo -e "${GREEN}  • UEFI: EFI (512MB) + boot (1GB) sin cifrar, resto cifrado${NC}"

        # Instalar herramientas específicas para cifrado
        install_pacstrap_with_retry "cryptsetup"
        install_pacstrap_with_retry "lvm2"
        install_pacstrap_with_retry "device-mapper"
        install_pacstrap_with_retry "thin-provisioning-tools"

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
        mkfs.ext4 -F $(get_partition_name "$SELECTED_DISK" "1")

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
        PARTITION_2=$(get_partition_name "$SELECTED_DISK" "2")
        wipefs -af "$PARTITION_2" 2>/dev/null || true
        # Cifrar partición principal con LUKS
        echo -e "${GREEN}| Cifrando $PARTITION_2 con LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo -e "${CYAN}Aplicando cifrado... (puede tardar unos minutos)${NC}"

        # Crear dispositivo LUKS usando archivo temporal para contraseña
        echo -n "$ENCRYPTION_PASSWORD" > /tmp/luks_pass

        if ! cryptsetup luksFormat --batch-mode --key-file /tmp/luks_pass "$PARTITION_2"; then
            rm -f /tmp/luks_pass
            echo -e "${RED}ERROR: Falló el cifrado LUKS${NC}"
            exit 1
        fi

        if ! cryptsetup open --key-file /tmp/luks_pass "$PARTITION_2" cryptlvm; then
            rm -f /tmp/luks_pass
            echo -e "${RED}ERROR: No se pudo abrir dispositivo cifrado${NC}"
            exit 1
        fi

        rm -f /tmp/luks_pass
        echo -e "${GREEN}✓ Cifrado LUKS aplicado y dispositivo abierto${NC}"

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup "$PARTITION_2" --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS (Simplificado)
        echo -e "${GREEN}| Configurando LVM sobre dispositivo cifrado |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _

        # Crear LVM sobre el dispositivo cifrado
        echo -e "${CYAN}Configurando LVM...${NC}"
        pvcreate /dev/mapper/cryptlvm
        vgcreate vg0 /dev/mapper/cryptlvm
        lvcreate -L 8G vg0 -n swap
        lvcreate -l 100%FREE vg0 -n root

        # Activar volúmenes
        vgchange -a y vg0
        sleep 2

        echo -e "${GREEN}✓ LVM configurado: vg0 con swap(8GB) y root${NC}"

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

        # Verificar que el sistema reconozca el swap LVM BIOS
        echo -e "${CYAN}Esperando reconocimiento del sistema para swap LVM...${NC}"
        sleep 3
        udevadm settle --timeout=10

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        # Verificar que el swap LVM esté disponible antes de activar
        echo -e "${CYAN}Verificando swap LVM antes de activar...${NC}"
        sleep 2

        if ! blkid /dev/vg0/swap | grep -q "TYPE=\"swap\""; then
            echo -e "${RED}ERROR: Swap LVM no está formateada correctamente${NC}"
            echo -e "${YELLOW}Intentando reformatear el swap LVM...${NC}"
            mkswap /dev/vg0/swap || {
                echo -e "${RED}ERROR: No se pudo reformatear el swap LVM${NC}"
                exit 1
            }
            sleep 2
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

        # Verificar que la partición boot exista
        echo -e "${CYAN}Verificando partición boot antes del montaje...${NC}"
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        if [ ! -b "$PARTITION_1" ]; then
            echo -e "${RED}ERROR: Partición boot $PARTITION_1 no existe${NC}"
            exit 1
        fi

        # Esperar que la partición esté completamente lista
        sleep 2
        sync

        # Montar partición boot
        echo -e "${CYAN}Creando directorio /boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount "$PARTITION_1" /mnt/boot; then
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
        install_pacstrap_with_retry "cryptsetup"
        install_pacstrap_with_retry "lvm2"
        install_pacstrap_with_retry "device-mapper"
        install_pacstrap_with_retry "thin-provisioning-tools"
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
                # Aplicar optimizaciones XFS
                xfs_admin -O bigtime=1 $device
                ;;
            "mkfs.f2fs")
                mkfs.f2fs -f $device
                ;;
            "mkfs.fat32")
                mkfs.fat -F32 -v $device
                # Si es sistema UEFI y punto de montaje /boot, marcar como EFI System
                if [ "$FIRMWARE_TYPE" = "UEFI" ] && [ "$mountpoint" = "/boot" ]; then
                    echo -e "${CYAN}Configurando partición $device como EFI System...${NC}"
                    # Obtener número de partición del device (ej: /dev/sda1 -> 1)
                    PARTITION_NUM=$(echo "$device" | grep -o '[0-9]*$')
                    DISK_DEVICE=$(echo "$device" | sed 's/[0-9]*$//')
                    parted $DISK_DEVICE --script set $PARTITION_NUM esp on
                    echo -e "${GREEN}✓ Partición $device marcada como EFI System${NC}"
                fi
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
        elif [ "$mountpoint" = "/boot" ]; then
            BOOT_FOUND=true
            # Si es /boot con fat32 en UEFI, se considera como EFI
            if [ "$FIRMWARE_TYPE" = "UEFI" ] && [ "$format" = "mkfs.fat32" ]; then
                EFI_FOUND=true
                echo -e "${CYAN}Detectado: /boot con FAT32 en UEFI - será usado como partición EFI${NC}"
            fi
        fi
    done

    # Validar configuración
    if [ "$ROOT_FOUND" = false ]; then
        echo -e "${RED}ERROR: No se encontró partición raíz (/) configurada${NC}"
        echo -e "${RED}Debe configurar al menos una partición con punto de montaje '/'${NC}"
        exit 1
    fi

    if [ "$EFI_FOUND" = true ]; then
        echo -e "${GREEN}✓ Configuración UEFI detectada correctamente${NC}"
    fi

    echo -e "${GREEN}✓ Validaciones completadas${NC}"

    # Segunda pasada: Montaje en orden correcto
    echo -e "${CYAN}=== FASE 2: Montaje de particiones ===${NC}"

    # 1. Montar partición raíz primero
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/" ]; then
            echo -e "${GREEN}| Montando raíz: $device -> /mnt |${NC}"
            # Si es btrfs, crear subvolúmenes primero
            if [ "$format" = "mkfs.btrfs" ]; then
                echo -e "${CYAN}Configurando subvolúmenes BTRFS...${NC}"

                # Crear punto de montaje temporal
                ROOT_MOUNT_POINT="/mnt/btrfs_root_temp"
                mkdir -p "$ROOT_MOUNT_POINT"

                # Montar filesystem btrfs temporalmente
                mount -t btrfs -o noatime,compress=zstd:3,space_cache=v2,autodefrag $device "$ROOT_MOUNT_POINT"

                # Crear subvolúmenes BTRFS
                echo -e "${CYAN}Creando subvolúmenes BTRFS...${NC}"
                btrfs subvolume create "$ROOT_MOUNT_POINT/@"
                btrfs subvolume create "$ROOT_MOUNT_POINT/@var_log"
                btrfs subvolume create "$ROOT_MOUNT_POINT/@var_cache"

                # Desmontar y remontar con subvolumen root
                umount "$ROOT_MOUNT_POINT"
                rmdir "$ROOT_MOUNT_POINT"

                echo -e "${CYAN}Montando subvolumen @ de btrfs con opciones optimizadas...${NC}"
                mount -t btrfs -o noatime,subvol=@,compress=zstd:3,space_cache=v2,autodefrag $device /mnt
            else
                mount $device /mnt
            fi
            break
        fi
    done

    # 2. Montar EFI/boot partition después de raíz
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/boot" ]; then
            echo -e "${GREEN}| Montando /boot: $device -> /mnt/boot |${NC}"
            mkdir -p /mnt/boot
            # Si es FAT32 (EFI), usar opciones específicas
            if [ "$format" = "mkfs.fat32" ]; then
                mount -t vfat -o defaults,umask=0077 $device /mnt/boot
            else
                mount $device /mnt/boot
            fi
            break
        fi
    done

    # 3. Montar HOME partition si existe
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/home" ]; then
            echo -e "${GREEN}| Montando /home: $device -> /mnt/home |${NC}"
            mkdir -p /mnt/home
            # Opciones específicas según filesystem
            if [ "$format" = "mkfs.xfs" ]; then
                mount -t xfs -o defaults,noatime $device /mnt/home
            elif [ "$format" = "mkfs.btrfs" ]; then
                mount -t btrfs -o noatime,compress=zstd:3,space_cache=v2,autodefrag $device /mnt/home
            else
                mount $device /mnt/home
            fi
            break
        fi
    done

    # 4. Montar subvolúmenes adicionales de BTRFS si la raíz es BTRFS
    ROOT_IS_BTRFS=false
    ROOT_DEVICE=""
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/" ] && [ "$format" = "mkfs.btrfs" ]; then
            ROOT_IS_BTRFS=true
            ROOT_DEVICE="$device"
            break
        fi
    done

    if [ "$ROOT_IS_BTRFS" = true ]; then
        echo -e "${CYAN}Montando subvolúmenes adicionales de BTRFS...${NC}"

        # Montar @var_log
        mkdir -p /mnt/var/log
        mount -t btrfs -o noatime,subvol=@var_log,compress=zstd:3,space_cache=v2,autodefrag "$ROOT_DEVICE" /mnt/var/log

        # Montar @var_cache
        mkdir -p /mnt/var/cache
        mount -t btrfs -o noatime,subvol=@var_cache,compress=zstd:3,space_cache=v2,autodefrag "$ROOT_DEVICE" /mnt/var/cache
    fi

    # 5. Montar todas las demás particiones (/var, /tmp, /usr, /opt, etc.)
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        # Saltar las ya montadas y swap
        if [ "$mountpoint" = "/" ] || [ "$mountpoint" = "/boot" ] || [ "$mountpoint" = "/home" ] || [ "$mountpoint" = "swap" ]; then
            continue
        fi

        echo -e "${GREEN}| Montando: $device -> /mnt$mountpoint |${NC}"
        mkdir -p /mnt$mountpoint

        # Opciones específicas según filesystem y punto de montaje
        if [ "$format" = "mkfs.xfs" ]; then
            mount -t xfs -o defaults,noatime $device /mnt$mountpoint
        elif [ "$format" = "mkfs.btrfs" ]; then
            mount -t btrfs -o noatime,compress=zstd:3,space_cache=v2,autodefrag $device /mnt$mountpoint
        else
            mount $device /mnt$mountpoint
        fi
    done

    lsblk -o NAME,FSTYPE,SIZE,MOUNTPOINT
    sleep 3
}
