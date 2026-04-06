# -----------------------------------------------------------------------------------

# Configuración oficial de zram usando zram-generator
echo -e "${GREEN}| Configurando zram oficial con zram-generator |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Detectar RAM total del sistema
TOTAL_RAM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
TOTAL_RAM_MB=$((TOTAL_RAM_KB / 1024 + 900))
TOTAL_RAM_GB=$((TOTAL_RAM_MB / 1024))

# Calcular zram exacto: 50% de RAM, máximo 8GB, mínimo 2GB
ZRAM_SIZE_MB=$((TOTAL_RAM_MB / 2))
if [ $ZRAM_SIZE_MB -gt 8192 ]; then
    ZRAM_SIZE_MB=8192
fi
if [ $ZRAM_SIZE_MB -lt 2048 ]; then
    ZRAM_SIZE_MB=2048
fi
ZRAM_SIZE_GB=$((ZRAM_SIZE_MB / 1024))

echo -e "${CYAN}  • RAM total: ${TOTAL_RAM_GB}GB (${TOTAL_RAM_MB}MB)${NC}"
echo -e "${CYAN}  • zram calculado: ${ZRAM_SIZE_GB}GB (${ZRAM_SIZE_MB}MB)${NC}"
echo -e "${CYAN}  • SWAP_TYPE: ${SWAP_TYPE}${NC}"
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
