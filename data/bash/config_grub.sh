# Detectar tipo de firmware
FIRMWARE_TYPE=$(detect_firmware)
echo -e "${GREEN}| Firmware detectado: $FIRMWARE_TYPE |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
sleep 2
clear
# Instalación de bootloader
# Instalar bootloader para todos los modos (incluyendo manual)
if true; then
    echo -e "${GREEN}| Instalando bootloader |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Verificar que la partición EFI esté montada con debug adicional
        echo -e "${CYAN}Verificando montaje de partición EFI...${NC}"
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: Partición EFI no está montada en /mnt/boot${NC}"
            echo -e "${YELLOW}Información de debug:${NC}"
            echo "- Contenido de /mnt/boot:"
            ls -la /mnt/boot/ 2>/dev/null || echo "  Directorio /mnt/boot no accesible"
            echo "- Contenido de /mnt/boot:"
            ls -la /mnt/boot/ 2>/dev/null || echo "  Directorio /mnt/boot no accesible"
            echo "- Montajes actuales:"
            mount | grep "/mnt"
            echo "- Particiones disponibles:"
            lsblk ${SELECTED_DISK}
            exit 1
        fi
        echo -e "${GREEN}✓ Partición EFI montada correctamente en /mnt/boot${NC}"

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
        ############################################################################################################
        # efibootmgr | grep -i grub | cut -d'*' -f1 | sed 's/Boot//' | xargs -I {} efibootmgr -b {} -B 2>/dev/null || true
        # Limpieza UEFI optimizada para el instalador de Arch
        echo -e "${CYAN}Limpiando entradas UEFI previas de GRUB...${NC}"

        # Verificar que efibootmgr esté disponible (siempre lo está en Arch live)
        if ! command -v efibootmgr >/dev/null 2>&1; then
            echo -e "${YELLOW}efibootmgr no disponible, omitiendo limpieza UEFI${NC}"
        else
            # Mostrar entradas actuales para debug (útil durante instalación)
            echo -e "${YELLOW}Entradas UEFI actuales con GRUB:${NC}"
            GRUB_ENTRIES=$(efibootmgr | grep -i grub | wc -l)

            if [ "$GRUB_ENTRIES" -gt 0 ]; then
                efibootmgr | grep -i grub
                echo -e "${CYAN}Eliminando $GRUB_ENTRIES entradas GRUB previas...${NC}"

                # Opción más simple y robusta para el instalador
                efibootmgr | grep -i grub | while read -r line; do
                    BOOT_NUM=$(echo "$line" | cut -d'*' -f1 | sed 's/Boot//')
                    if [ -n "$BOOT_NUM" ] && [ "$BOOT_NUM" != "Boot" ]; then
                        echo "  Eliminando entrada: $BOOT_NUM"
                        efibootmgr -b "$BOOT_NUM" -B >/dev/null 2>&1 || true
                    fi
                done

                echo -e "${GREEN}✓ Entradas GRUB previas eliminadas${NC}"
            else
                echo -e "${GREEN}✓ No se encontraron entradas GRUB previas${NC}"
            fi
        fi

        sleep 4

        # Limpiar directorio EFI previo si existe
        #if [ -d "/mnt/boot/EFI/GRUB" ]; then
        #    rm -rf /mnt/boot/EFI/GRUB
        #fi

        # Crear directorio EFI si no existe
        #mkdir -p /mnt/boot/EFI

        echo -e "${CYAN}Instalando paquetes GRUB para UEFI...${NC}"
        install_pacman_chroot_with_retry "grub"
        install_pacman_chroot_with_retry "efibootmgr"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$ENCRYPTION" = "true" ]; then
            if [ -z "$CRYPT_LUKS_UUID" ]; then
                echo -e "${RED}ERROR: CRYPT_LUKS_UUID no disponible${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID partición LUKS: ${CRYPT_LUKS_UUID}${NC}"
            _grub_cmdline="cryptdevice=UUID=${CRYPT_LUKS_UUID}:cryptlvm root=/dev/vg0/root"
            [ "$FILESYSTEM_TYPE" = "btrfs" ] && _grub_cmdline="$_grub_cmdline rootflags=subvol=@"
            [ "${SWAP_SIZE_MIB:-0}" -gt 0 ] && _grub_cmdline="$_grub_cmdline resume=/dev/vg0/swap"
            _grub_cmdline="$_grub_cmdline splash loglevel=3"
            sed -i "s|GRUB_CMDLINE_LINUX=\"\"|GRUB_CMDLINE_LINUX=\"${_grub_cmdline}\"|" /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo -e "${GREEN}✓ Configuración GRUB UEFI para LUKS+LVM:${NC}"
            echo -e "${CYAN}  • ${_grub_cmdline}${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y${NC}"
        elif [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; then
            sed -i 's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=\"rootflags=subvol=@ loglevel=3\"/' /mnt/etc/default/grub
            sed -i 's/^GRUB_PRELOAD_MODULES=.*/GRUB_PRELOAD_MODULES=\"part_gpt part_msdos btrfs\"/' /mnt/etc/default/grub
            echo -e "${GREEN}✓ Configuración GRUB UEFI simplificada para BTRFS${NC}"
        elif [ "$PARTITION_MODE" = "manual" ]; then
            echo -e "${CYAN}Configurando GRUB para particionado manual...${NC}"

            # Detectar módulos necesarios según sistemas de archivos utilizados
            GRUB_MODULES_LIST="part_gpt part_msdos"
            ROOTFLAGS=""

            # Verificar qué sistemas de archivos se están usando
            for partition_config in "${PARTITIONS[@]}"; do
                IFS=' ' read -r device format mountpoint <<< "$partition_config"
                case "$format" in
                    "mkfs.btrfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST btrfs"
                        if [ "$mountpoint" = "/" ]; then
                            ROOTFLAGS="rootflags=subvol=@"
                        fi
                        echo -e "${CYAN}  • Detectado BTRFS: agregando módulo btrfs${NC}"
                        ;;
                    "mkfs.xfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST xfs"
                        echo -e "${CYAN}  • Detectado XFS: agregando módulo xfs${NC}"
                        ;;
                    "mkfs.f2fs")
                        # F2FS no tiene módulo específico en GRUB, usar genérico
                        echo -e "${CYAN}  • Detectado F2FS: usando módulos estándar${NC}"
                        ;;
                    "mkfs.ntfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST ntfs"
                        echo -e "${CYAN}  • Detectado NTFS: agregando módulo ntfs${NC}"
                        ;;
                    "mkfs.jfs")
                        # JFS no tiene módulo específico en GRUB moderno
                        echo -e "${CYAN}  • Detectado JFS: usando módulos estándar${NC}"
                        ;;
                    "mkfs.reiserfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST reiserfs"
                        echo -e "${CYAN}  • Detectado ReiserFS: agregando módulo reiserfs${NC}"
                        ;;
                    "mkfs.fat32"|"mkfs.fat16")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST fat"
                        echo -e "${CYAN}  • Detectado FAT: agregando módulo fat${NC}"
                        ;;
                esac
            done

            # Eliminar duplicados en la lista de módulos
            GRUB_MODULES_LIST=$(echo "$GRUB_MODULES_LIST" | tr ' ' '\n' | sort -u | tr '\n' ' ' | sed 's/ $//')

            # Configurar GRUB_PRELOAD_MODULES
            echo "GRUB_PRELOAD_MODULES=\"$GRUB_MODULES_LIST\"" >> /mnt/etc/default/grub

            # Configurar GRUB_CMDLINE_LINUX_DEFAULT con rootflags si es necesario
            if [ -n "$ROOTFLAGS" ]; then
                sed -i "s/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=\"$ROOTFLAGS loglevel=3\"/" /mnt/etc/default/grub
                echo -e "${GREEN}✓ Configuración GRUB manual con rootflags: ${ROOTFLAGS}${NC}"
            else
                sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
                echo -e "${GREEN}✓ Configuración GRUB manual estándar${NC}"
            fi

            echo -e "${CYAN}  • Módulos GRUB configurados: ${GRUB_MODULES_LIST}${NC}"
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos\"" >> /mnt/etc/default/grub
        fi

        sleep 2
        clear

        echo -e "${CYAN}Instalando GRUB en partición EFI...${NC}"

        # Instalar GRUB en modo removible (crea /EFI/BOOT/bootx64.efi)
        echo -e "${CYAN}Instalando GRUB en modo removible...${NC}"
        chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot --removable --force --recheck" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI (modo removible)${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ GRUB instalado en modo removible (/EFI/BOOT/bootx64.efi)${NC}"

        # Instalar GRUB con entrada NVRAM (crea /EFI/GRUB/grubx64.efi)
        echo -e "${CYAN}Instalando GRUB...${NC}"
        chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot --bootloader-id=GRUB --force --recheck" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ GRUB instalado con entrada NVRAM (/EFI/GRUB/grubx64.efi)${NC}"

        # Verificar que ambos bootloaders se hayan creado
        if [ ! -f "/mnt/boot/EFI/BOOT/bootx64.efi" ]; then
            echo -e "${RED}ERROR: No se creó bootx64.efi${NC}"
            exit 1
        fi

        if [ ! -f "/mnt/boot/EFI/GRUB/grubx64.efi" ]; then
            echo -e "${RED}ERROR: No se creó grubx64.efi${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Ambos bootloaders creados exitosamente${NC}"

        echo -e "${CYAN}Generando configuración de GRUB...${NC}"
        if ! chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"; then
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
        install_pacman_chroot_with_retry "grub"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$ENCRYPTION" = "true" ]; then
            if [ -z "$CRYPT_LUKS_UUID" ]; then
                echo -e "${RED}ERROR: CRYPT_LUKS_UUID no disponible${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID partición LUKS: ${CRYPT_LUKS_UUID}${NC}"
            _grub_cmdline="cryptdevice=UUID=${CRYPT_LUKS_UUID}:cryptlvm root=/dev/vg0/root"
            [ "$FILESYSTEM_TYPE" = "btrfs" ] && _grub_cmdline="$_grub_cmdline rootflags=subvol=@"
            [ "${SWAP_SIZE_MIB:-0}" -gt 0 ] && _grub_cmdline="$_grub_cmdline resume=/dev/vg0/swap"
            _grub_cmdline="$_grub_cmdline splash loglevel=3"
            sed -i "s|GRUB_CMDLINE_LINUX=\"\"|GRUB_CMDLINE_LINUX=\"${_grub_cmdline}\"|" /mnt/etc/default/grub
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub
            echo -e "${GREEN}✓ Configuración GRUB BIOS Legacy para LUKS+LVM:${NC}"
            echo -e "${CYAN}  • ${_grub_cmdline}${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y${NC}"

        elif [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; then
            sed -i 's/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=\"rootflags=subvol=@ loglevel=3\"/' /mnt/etc/default/grub
            sed -i 's/^GRUB_PRELOAD_MODULES=.*/GRUB_PRELOAD_MODULES=\"part_msdos btrfs\"/' /mnt/etc/default/grub
            echo -e "${GREEN}✓ Configuración GRUB BIOS Legacy simplificada para BTRFS${NC}"
        elif [ "$PARTITION_MODE" = "manual" ]; then
            echo -e "${CYAN}Configurando GRUB BIOS para particionado manual...${NC}"

            # Detectar módulos necesarios según sistemas de archivos utilizados
            GRUB_MODULES_LIST="part_msdos"
            ROOTFLAGS=""

            # Verificar qué sistemas de archivos se están usando
            for partition_config in "${PARTITIONS[@]}"; do
                IFS=' ' read -r device format mountpoint <<< "$partition_config"
                case "$format" in
                    "mkfs.btrfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST btrfs"
                        if [ "$mountpoint" = "/" ]; then
                            ROOTFLAGS="rootflags=subvol=@"
                        fi
                        echo -e "${CYAN}  • Detectado BTRFS: agregando módulo btrfs${NC}"
                        ;;
                    "mkfs.xfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST xfs"
                        echo -e "${CYAN}  • Detectado XFS: agregando módulo xfs${NC}"
                        ;;
                    "mkfs.f2fs")
                        # F2FS no tiene módulo específico en GRUB, usar genérico
                        echo -e "${CYAN}  • Detectado F2FS: usando módulos estándar${NC}"
                        ;;
                    "mkfs.ntfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST ntfs"
                        echo -e "${CYAN}  • Detectado NTFS: agregando módulo ntfs${NC}"
                        ;;
                    "mkfs.jfs")
                        # JFS no tiene módulo específico en GRUB moderno
                        echo -e "${CYAN}  • Detectado JFS: usando módulos estándar${NC}"
                        ;;
                    "mkfs.reiserfs")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST reiserfs"
                        echo -e "${CYAN}  • Detectado ReiserFS: agregando módulo reiserfs${NC}"
                        ;;
                    "mkfs.fat32"|"mkfs.fat16")
                        GRUB_MODULES_LIST="$GRUB_MODULES_LIST fat"
                        echo -e "${CYAN}  • Detectado FAT: agregando módulo fat${NC}"
                        ;;
                esac
            done

            # Eliminar duplicados en la lista de módulos
            GRUB_MODULES_LIST=$(echo "$GRUB_MODULES_LIST" | tr ' ' '\n' | sort -u | tr '\n' ' ' | sed 's/ $//')

            # Configurar GRUB_PRELOAD_MODULES
            echo "GRUB_PRELOAD_MODULES=\"$GRUB_MODULES_LIST\"" >> /mnt/etc/default/grub

            # Configurar GRUB_CMDLINE_LINUX_DEFAULT con rootflags si es necesario
            if [ -n "$ROOTFLAGS" ]; then
                sed -i "s/^GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT=\"$ROOTFLAGS loglevel=3\"/" /mnt/etc/default/grub
                echo -e "${GREEN}✓ Configuración GRUB BIOS manual con rootflags: ${ROOTFLAGS}${NC}"
            else
                sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
                echo -e "${GREEN}✓ Configuración GRUB BIOS manual estándar${NC}"
            fi

            echo -e "${CYAN}  • Módulos GRUB BIOS configurados: ${GRUB_MODULES_LIST}${NC}"
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos\"" >> /mnt/etc/default/grub
        fi

        sleep 4

        echo -e "${CYAN}Instalando GRUB en disco...${NC}"
        if ! chroot /mnt /bin/bash -c "grub-install --target=i386-pc $SELECTED_DISK"; then
            echo -e "${RED}ERROR: Falló la instalación de GRUB BIOS${NC}"
            exit 1
        fi

        sleep 4

        echo -e "${CYAN}Generando configuración de GRUB...${NC}"
        if ! chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"; then
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
        if [ -f "/mnt/boot/EFI/GRUB/grubx64.efi" ] && [ -f "/mnt/boot/EFI/BOOT/bootx64.efi" ] && [ -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${GREEN}✓ Bootloader UEFI verificado correctamente${NC}"
            echo -e "${GREEN}✓ Modo NVRAM: /EFI/GRUB/grubx64.efi${NC}"
            echo -e "${GREEN}✓ Modo removible: /EFI/BOOT/bootx64.efi${NC}"
        else
            echo -e "${RED}⚠ Problema con la instalación del bootloader UEFI${NC}"
            echo -e "${YELLOW}Archivos verificados:${NC}"
            echo "  - /mnt/boot/EFI/GRUB/grubx64.efi: $([ -f "/mnt/boot/EFI/GRUB/grubx64.efi" ] && echo "✓" || echo "✗")"
            echo "  - /mnt/boot/EFI/BOOT/bootx64.efi: $([ -f "/mnt/boot/EFI/BOOT/bootx64.efi" ] && echo "✓" || echo "✗")"
            echo "  - /mnt/boot/grub/grub.cfg: $([ -f "/mnt/boot/grub/grub.cfg" ] && echo "✓" || echo "✗")"
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
install_pacman_chroot_with_retry "os-prober"
install_pacman_chroot_with_retry "ntfs-3g"
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
    if [ "$WINDOWS_PARTITIONS" -gt 0 ]; then
        echo -e "${CYAN}  • Particiones Windows (NTFS) detectadas: $WINDOWS_PARTITIONS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Método 3: Detectar otras particiones Linux (ext4, ext3, btrfs, xfs)
    EXT4_PARTITIONS=$(blkid -t TYPE=ext4 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    EXT3_PARTITIONS=$(blkid -t TYPE=ext3 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    BTRFS_PARTITIONS=$(blkid -t TYPE=btrfs 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    XFS_PARTITIONS=$(blkid -t TYPE=xfs 2>/dev/null | grep -v "$(findmnt -n -o SOURCE /)" | wc -l || echo "0")
    LINUX_PARTITIONS=$((EXT4_PARTITIONS + EXT3_PARTITIONS + BTRFS_PARTITIONS + XFS_PARTITIONS))

    if [ "$LINUX_PARTITIONS" -gt 0 ]; then
        echo -e "${CYAN}  • Otras particiones Linux detectadas: $LINUX_PARTITIONS${NC}"
        [ "$EXT4_PARTITIONS" -gt 0 ] && echo -e "${CYAN}    - ext4: $EXT4_PARTITIONS${NC}"
        [ "$EXT3_PARTITIONS" -gt 0 ] && echo -e "${CYAN}    - ext3: $EXT3_PARTITIONS${NC}"
        [ "$BTRFS_PARTITIONS" -gt 0 ] && echo -e "${CYAN}    - btrfs: $BTRFS_PARTITIONS${NC}"
        [ "$XFS_PARTITIONS" -gt 0 ] && echo -e "${CYAN}    - xfs: $XFS_PARTITIONS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Método 4: Buscar particiones con indicadores de SO
    OTHER_OS=$(blkid 2>/dev/null | grep -E "LABEL.*Windows|LABEL.*Microsoft|TYPE.*fat32" | wc -l || echo "0")
    if [ "$OTHER_OS" -gt 0 ]; then
        echo -e "${CYAN}  • Otras particiones de SO detectadas: $OTHER_OS${NC}"
        OS_COUNT=$((OS_COUNT + 1))
    fi

    # Considerar múltiples sistemas si hay más indicadores de OS o más de 1 partición bootable
    if [ "$OS_COUNT" -gt 0 ] || [ "$BOOTABLE_PARTITIONS" -gt 1 ]; then
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
    DETECTED_SYSTEMS=$(chroot /mnt /bin/bash -c "os-prober" 2>/dev/null || true)

    if [ -n "$DETECTED_SYSTEMS" ]; then
        echo -e "${GREEN}✓ Sistemas detectados:${NC}"
        echo "$DETECTED_SYSTEMS" | while IFS= read -r line; do
            if [ -n "$line" ]; then
                echo -e "${CYAN}  • $line${NC}"
            fi
        done

        # Regenerar configuración de GRUB con los sistemas detectados
        echo -e "${CYAN}Regenerando configuración de GRUB con sistemas detectados...${NC}"
        chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"

        # Verificar que se agregaron entradas
        GRUB_ENTRIES=$(chroot /mnt /bin/bash -c "grep -c 'menuentry' /boot/grub/grub.cfg" 2>/dev/null || echo "0")
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
