# Generar fstab usando genfstab -U
echo -e "${GREEN}| Generando fstab con genfstab -U |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

genfstab -U /mnt > /mnt/etc/fstab

# Modificar opciones de btrfs si existe partición raíz con subvolumen @
if grep -q "subvol=@" /mnt/etc/fstab; then
    echo -e "${CYAN}Optimizando opciones de btrfs en fstab...${NC}"
    sed -i 's/subvol=@[^,]*/subvol=@,compress=zstd:3,space_cache=v2,autodefrag/' /mnt/etc/fstab
    echo -e "${GREEN}✓ Opciones de btrfs optimizadas${NC}"
fi

# Modificar prioridad del swap tradicional de -2 a 10 (menor que zram que tiene 100)
if grep -q "swap" /mnt/etc/fstab; then
    echo -e "${CYAN}Configurando prioridad del swap tradicional a 10...${NC}"
    sed -i 's/\(.*swap.*defaults\)\(.*0.*0\)/\1,pri=10\2/' /mnt/etc/fstab
    echo -e "${GREEN}✓ Prioridad del swap tradicional configurada a 10${NC}"
fi

echo -e "${GREEN}✓ fstab generado correctamente${NC}"

echo ""
chroot /mnt /bin/bash -c "cat /etc/fstab"

# Verificación final de fstab antes de continuar
echo -e "${CYAN}Realizando verificación final de fstab...${NC}"
FSTAB_ERRORS=0

# Verificar que todas las particiones swap en fstab existan
while IFS= read -r line; do
    if [[ "$line" =~ ^UUID=.*[[:space:]].*[[:space:]]swap ]]; then
        SWAP_UUID=$(echo "$line" | grep -o 'UUID=[a-fA-F0-9-]*' | cut -d'=' -f2)
        if [ -n "$SWAP_UUID" ] && ! blkid | grep -q "$SWAP_UUID"; then
            echo -e "${RED}ERROR: UUID de swap $SWAP_UUID en fstab no existe en el sistema${NC}"
            FSTAB_ERRORS=1
        fi
    elif [[ "$line" =~ ^/dev/.*[[:space:]].*[[:space:]]swap ]]; then
        SWAP_DEVICE=$(echo "$line" | awk '{print $1}')
        if [ -n "$SWAP_DEVICE" ] && [ ! -b "$SWAP_DEVICE" ]; then
            echo -e "${RED}ERROR: Dispositivo swap $SWAP_DEVICE en fstab no existe${NC}"
            FSTAB_ERRORS=1
        fi
    fi
done < /mnt/etc/fstab

if [ $FSTAB_ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ Verificación de fstab completada sin errores${NC}"
else
    echo -e "${YELLOW}WARNING: Se encontraron posibles problemas en fstab${NC}"
    echo -e "${YELLOW}El sistema podría tener problemas durante el boot${NC}"
    echo -e "${CYAN}Presiona Enter para continuar o Ctrl+C para abortar...${NC}"
    read
fi


# Optimizaciones específicas para particionado manual con BTRFS
if [ "$PARTITION_MODE" = "manual" ]; then
    # Verificar si hay particiones btrfs configuradas
    HAS_BTRFS=false
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$format" = "mkfs.btrfs" ] || [ "$format" = "btrfs" ]; then
            HAS_BTRFS=true
            break
        fi
    done

    if [ "$HAS_BTRFS" = true ]; then
        # Optimizar fstab para BTRFS
        echo -e "${CYAN}Optimizando fstab para BTRFS...${NC}"
        chroot /mnt /bin/bash -c "sed -i 's/relatime/noatime/g' /etc/fstab"

        # Agregar opciones de montaje optimizadas para todos los subvolúmenes
        chroot /mnt /bin/bash -c "sed -i 's/subvol=@/subvol=@,compress=zstd:3,space_cache=v2,autodefrag,/' /etc/fstab" 2>/dev/null || true

        # Verificar configuración final de fstab
        echo -e "${CYAN}Verificando configuración final de fstab...${NC}"
        if chroot /mnt /bin/bash -c "mount -a --fake" 2>/dev/null; then
            echo -e "${GREEN}✓ Configuración fstab válida${NC}"
        else
            echo -e "${YELLOW}Warning: Posibles issues en fstab, pero continuando...${NC}"
        fi
    fi
fi

sleep 3
clear
