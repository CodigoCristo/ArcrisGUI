# -----------------------------------------------------------------------------------

# Configuración oficial de zram usando zram-generator
echo -e "${GREEN}| Configurando zram oficial con zram-generator |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Detectar RAM total del sistema
TOTAL_RAM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
TOTAL_RAM_MB=$((TOTAL_RAM_KB / 1024))
TOTAL_RAM_GB=$((TOTAL_RAM_MB / 1024))

# Redondear al GB superior si la fracción >= 512MB (≥ 0.5G) y la RAM total >= 3584MB (3.5G)
RAM_REMAINDER=$((TOTAL_RAM_MB % 1024))
if [ $TOTAL_RAM_MB -ge 3584 ] && [ $RAM_REMAINDER -ge 512 ]; then
    TOTAL_RAM_MB=$(( (TOTAL_RAM_GB + 1) * 1024 ))
    echo -e "${CYAN}  • RAM redondeada a $((TOTAL_RAM_MB / 1024))G (fracción >= 0.5G)${NC}"
fi

# Calcular tamaño de zram según SWAP_TYPE (misma regla que la partición en disco)
case "$SWAP_TYPE" in
    "zram")
        # Solo zram, sin disco → 50% de RAM
        ZRAM_SIZE_MB=$((TOTAL_RAM_MB / 2))
        ;;
    "half")
        # Zram + disco de mitad RAM → zram también es mitad de RAM
        ZRAM_SIZE_MB=$((TOTAL_RAM_MB / 2))
        ;;
    "equal")
        # Zram + disco igual a RAM → zram igual a RAM
        ZRAM_SIZE_MB=$TOTAL_RAM_MB
        ;;
    "custom")
        # Zram + disco de tamaño custom → zram del mismo tamaño custom
        ZRAM_SIZE_MB=$(( SWAP_CUSTOM_SIZE * 1024 ))
        ;;
    *)
        ZRAM_SIZE_MB=$((TOTAL_RAM_MB / 2))
        ;;
esac

ZRAM_SIZE_GB=$((ZRAM_SIZE_MB / 1024))

echo -e "${CYAN}  • RAM total: ${TOTAL_RAM_GB}GB (${TOTAL_RAM_MB}MB)${NC}"
echo -e "${CYAN}  • SWAP_TYPE: ${SWAP_TYPE}${NC}"
echo -e "${CYAN}  • zram calculado: ${ZRAM_SIZE_GB}GB (${ZRAM_SIZE_MB}MB)${NC}"
echo ""

# Instalar zram-generator (método oficial)
install_pacman_chroot_with_retry "zram-generator"

# Crear configuración oficial de zram-generator con valor exacto
cat > /mnt/etc/systemd/zram-generator.conf << EOF
# Configuración oficial zram-generator
# RAM detectada: ${TOTAL_RAM_GB}GB (${TOTAL_RAM_MB}MB)
# zram calculado: ${ZRAM_SIZE_GB}GB (${ZRAM_SIZE_MB}MB exactos)

[zram0]
zram-size = ${ZRAM_SIZE_MB}
compression-algorithm = zstd
swap-priority = 100
EOF

# Deshabilitar zswap para evitar conflictos con zram (ArchWiki)
if [ -f /mnt/etc/default/grub ]; then
    sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="/&zswap.enabled=0 /' /mnt/etc/default/grub
fi

# vm.swappiness:
#   zram puro (sin disco)  → 180  (el kernel puede swapear agresivamente, es RAM comprimida)
#   zram + partición disco → 60   (preferir zram pero no ignorar la partición)
if [ "$SWAP_TYPE" = "zram" ]; then
    VM_SWAPPINESS=180
else
    VM_SWAPPINESS=60
fi

cat > /mnt/etc/sysctl.d/99-vm-zram-parameters.conf << EOF
# Optimización para zram — SWAP_TYPE=${SWAP_TYPE}
vm.swappiness = ${VM_SWAPPINESS}
vm.watermark_boost_factor = 0
vm.watermark_scale_factor = 125
vm.page-cluster = 0
EOF

echo -e "${GREEN}✓ zram configurado:${NC}"
echo -e "${CYAN}  • zram: ${ZRAM_SIZE_GB}GB (${ZRAM_SIZE_MB}MB) con zstd, prioridad 100${NC}"
echo -e "${CYAN}  • zswap: DESHABILITADO${NC}"
echo -e "${CYAN}  • vm.swappiness: ${VM_SWAPPINESS}${NC}"
if [ "$SWAP_TYPE" != "zram" ]; then
    echo -e "${CYAN}  • Partición swap en disco activa con prioridad 10 (inferior a zram)${NC}"
fi
# ------------------------------------------------------------------------------------------------------------------
sleep 3
clear
