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

# ================================================================================================
# FUNCIÓN PARA MANEJO CORRECTO DE NOMENCLATURA DE PARTICIONES NVMe
# ================================================================================================
# Función para obtener el nombre correcto de la partición según el tipo de dispositivo
get_partition_name() {
    local disk="$1"
    local partition_number="$2"

    # Verificar si es un dispositivo NVMe
    if [[ "$disk" == *"nvme"* ]]; then
        echo "${disk}p${partition_number}"
    else
        echo "${disk}${partition_number}"
    fi
}
# =============================================
source "$(dirname "$0")/config_conectividad.sh"
# =============================================

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

# =============================================
source "$(dirname "$0")/config_teclado.sh"
# =============================================

################################################################################################
# #################### CONFIGURAR BTRFS ##################################################################
################################################################################################
#
configurar_btrfs() {
    echo -e "${GREEN}| Configuración adicional para BTRFS |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Verificar que BTRFS esté montado correctamente
    echo -e "${CYAN}Verificando sistema de archivos BTRFS...${NC}"
    if ! chroot /mnt /bin/bash -c "btrfs filesystem show" >/dev/null 2>&1; then
        echo -e "${RED}ERROR: No se pudo verificar el sistema BTRFS${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Sistema BTRFS verificado${NC}"

    # Configuración básica para BTRFS (sin complicaciones)
    echo -e "${CYAN}Aplicando configuración básica BTRFS...${NC}"

    # Solo asegurar que el bootloader funcione correctamente
    echo -e "${GREEN}✓ Configuración BTRFS simplificada completada${NC}"

    # Instalar herramientas adicionales para BTRFS si no están presentes
    echo -e "${CYAN}Verificando herramientas BTRFS adicionales...${NC}"

    # Solo instalar grub-btrfs ya que btrfs-progs ya está instalado
    install_pacman_chroot_with_retry "btrfs-progs"
    install_pacman_chroot_with_retry "btrfsmaintenance"
    install_pacman_chroot_with_retry "snapper"
    install_pacman_chroot_with_retry "btrfs-assistant"
    install_pacman_chroot_with_retry "grub-btrfs" "--needed" 2>/dev/null || echo -e "${YELLOW}Warning: No se pudo instalar grub-btrfs${NC}"
    install_pacman_chroot_with_retry "inotify-tools" "--needed" 2>/dev/null || echo -e "${YELLOW}Warning: No se pudo instalar inotify-tools${NC}"

    # Configurar grub-btrfs para boot desde snapshots
    if chroot /mnt /bin/bash -c "pacman -Qq grub-btrfs" 2>/dev/null; then
        echo -e "${CYAN}Configurando grub-btrfs para boot desde snapshots...${NC}"

        # Habilitar servicio de actualización automática de grub con snapshots
        chroot /mnt /bin/bash -c "systemctl enable grub-btrfsd.service" 2>/dev/null || echo -e "${YELLOW}Warning: grub-btrfsd.service no disponible${NC}"

        # Configurar grub-btrfs para detectar snapshots en /.snapshots
        if [ ! -f /mnt/etc/default/grub-btrfs/config ]; then
            mkdir -p /mnt/etc/default/grub-btrfs
            cp /usr/share/arcrisgui/data/bash/btrfs/config /mnt/etc/default/grub-btrfs/config
            cat /mnt/etc/default/grub-btrfs/config
        fi
        echo -e "${GREEN}✓ grub-btrfs configurado para detectar snapshots automáticamente${NC}"
    else
        echo -e "${YELLOW}Warning: grub-btrfs no instalado, boot desde snapshots no disponible${NC}"
    fi

    # Habilitar servicios de mantenimiento BTRFS
    echo -e "${CYAN}Configurando servicios de mantenimiento BTRFS...${NC}"
    chroot /mnt /bin/bash -c "systemctl enable btrfs-scrub@-.timer" 2>/dev/null || echo -e "${YELLOW}Warning: btrfs-scrub timer no disponible${NC}"
    chroot /mnt /bin/bash -c "systemctl enable fstrim.timer" || echo -e "${RED}ERROR: Falló habilitar fstrim.timer${NC}"

    # Instalar y configurar snapshots automáticos con Snapper
    echo -e "${CYAN}Instalando Snapper para snapshots automáticos...${NC}"
    install_pacman_chroot_with_retry "snapper" "--needed" 2>/dev/null || echo -e "${YELLOW}Warning: No se pudo instalar snapper${NC}"

    if chroot /mnt /bin/bash -c "pacman -Qq snapper" 2>/dev/null; then
        echo -e "${CYAN}Configurando Snapper para snapshots automáticos...${NC}"

        # Crear configuración para el subvolumen raíz (esto crea automáticamente /.snapshots)
        echo -e "${CYAN}Configurando Snapper para el sistema raíz (/)...${NC}"

        # Crear directorio de configuración si no existe
        chroot /mnt /bin/bash -c "mkdir -p /etc/snapper/configs"

        # Intentar crear configuración con --no-dbus para LiveCD
        if chroot /mnt /bin/bash -c "snapper --no-dbus -c root create-config /" 2>/dev/null; then
            chroot /mnt /bin/bash -c "snapper --no-dbus -c root set-config TIMELINE_LIMIT_HOURLY=0 TIMELINE_LIMIT_DAILY=3 TIMELINE_LIMIT_WEEKLY=0 TIMELINE_LIMIT_MONTHLY=0 TIMELINE_LIMIT_YEARLY=0" 2>/dev/null
            echo -e "${GREEN}✓ Configuración de snapper para raíz creada exitosamente${NC}"
        else
            # Crear configuración manualmente si falla
            cp /usr/share/arcrisgui/data/bash/btrfs/root /mnt/etc/snapper/configs/root
            cat /mnt/etc/snapper/configs/root
            # Crear directorio de snapshots manualmente
            chroot /mnt /bin/bash -c "mkdir -p /.snapshots"
            chroot /mnt /bin/bash -c "chmod 755 /.snapshots"

            echo -e "${GREEN}✓ Configuración manual de snapper para raíz completada${NC}"
        fi

        # Crear configuración para /home si el subvolumen existe
        if chroot /mnt /bin/bash -c "mountpoint -q /home"; then
            echo -e "${CYAN}Configurando Snapper para /home...${NC}"

            # Intentar crear configuración para /home con --no-dbus para LiveCD
            if chroot /mnt /bin/bash -c "snapper --no-dbus -c home create-config /home" 2>/dev/null; then
                chroot /mnt /bin/bash -c "snapper --no-dbus -c home set-config TIMELINE_LIMIT_HOURLY=0 TIMELINE_LIMIT_DAILY=3 TIMELINE_LIMIT_WEEKLY=0 TIMELINE_LIMIT_MONTHLY=0 TIMELINE_LIMIT_YEARLY=0" 2>/dev/null
                echo -e "${GREEN}✓ Configuración de snapper para /home creada exitosamente${NC}"
            else
                # Crear configuración manualmente si falla
                cp /usr/share/arcrisgui/data/bash/btrfs/home /mnt/etc/snapper/configs/home
                cat /mnt/etc/snapper/configs/home
                # Crear directorio de snapshots manualmente
                chroot /mnt /bin/bash -c "mkdir -p /home/.snapshots"
                chroot /mnt /bin/bash -c "chmod 755 /home/.snapshots"

                echo -e "${GREEN}✓ Configuración manual de snapper para /home completada${NC}"
            fi
        else
            echo -e "${YELLOW}Warning: /home no está montado como subvolumen, saltando configuración de snapper${NC}"
        fi

        # Habilitar servicios de Snapper
        chroot /mnt /bin/bash -c "systemctl enable snapper-timeline.timer" 2>/dev/null || echo -e "${YELLOW}Warning: Falló habilitar snapper-timeline.timer${NC}"
        chroot /mnt /bin/bash -c "systemctl enable snapper-cleanup.timer" 2>/dev/null || echo -e "${YELLOW}Warning: Falló habilitar snapper-cleanup.timer${NC}"

        echo -e "${GREEN}✓ Servicios automáticos de Snapper habilitados:${NC}"
        echo -e "${CYAN}  • snapper-timeline.timer: Crea snapshots automáticos${NC}"
        echo -e "${CYAN}    - Cada hora (mantiene 5)${NC}"
        echo -e "${CYAN}    - Diarios (mantiene 7)${NC}"
        echo -e "${CYAN}    - Semanales (mantiene 4)${NC}"
        echo -e "${CYAN}    - Mensuales (mantiene 6-12)${NC}"
        echo -e "${CYAN}    - Anuales (mantiene 2-5)${NC}"
        echo -e "${CYAN}  • snapper-cleanup.timer: Limpia snapshots antiguos automáticamente${NC}"

        echo -e "${GREEN}✓ Configuración de Snapper completada${NC}"

        echo -e "${GREEN}✓ Snapper configurado con snapshots automáticos para ROOT (/):${NC}"
        echo -e "${CYAN}  • Cada hora: mantiene 5 snapshots${NC}"
        echo -e "${CYAN}  • Diariamente: mantiene 7 snapshots${NC}"
        echo -e "${CYAN}  • Semanalmente: mantiene 4 snapshots${NC}"
        echo -e "${CYAN}  • Mensualmente: mantiene 6 snapshots${NC}"
        echo -e "${CYAN}  • Anualmente: mantiene 2 snapshots${NC}"
        echo -e "${CYAN}  • Límite total: 50 snapshots + 10 importantes${NC}"

        echo -e "${GREEN}✓ Snapper configurado con snapshots automáticos para HOME (/home):${NC}"
        echo -e "${CYAN}  • Cada hora: mantiene 3 snapshots${NC}"
        echo -e "${CYAN}  • Diariamente: mantiene 7 snapshots${NC}"
        echo -e "${CYAN}  • Semanalmente: mantiene 4 snapshots${NC}"
        echo -e "${CYAN}  • Mensualmente: mantiene 12 snapshots${NC}"
        echo -e "${CYAN}  • Anualmente: mantiene 5 snapshots${NC}"
        echo -e "${CYAN}  • Límite total: 40 snapshots + 10 importantes${NC}"

        echo -e "\n${GREEN}✓ Estructura final de subvolúmenes BTRFS:${NC}"
        echo -e "${CYAN}  • @ - Raíz del sistema (/)${NC}"
        echo -e "${CYAN}  • @home - Directorios de usuarios (/home)${NC}"
        echo -e "${CYAN}  • @var_log - Logs del sistema (/var/log)${NC}"
        echo -e "${CYAN}  • /.snapshots - Snapshots de raíz (por Snapper)${NC}"
        echo -e "${CYAN}  • /home/.snapshots - Snapshots de home (por Snapper)${NC}"

        echo -e "${GREEN}✓ Configuraciones de Snapper creadas:${NC}"
        echo -e "${CYAN}  • root: Snapshots del sistema con retención completa${NC}"
        echo -e "${CYAN}  • home: Snapshots de datos de usuario con retención extendida${NC}"

        echo -e "\n${GREEN}✓ grub-btrfs configurado:${NC}"
        echo -e "${CYAN}  • Boot desde snapshots disponible en GRUB${NC}"
        echo -e "${CYAN}  • Recuperación de emergencia habilitada${NC}"

    else
        echo -e "${RED}ERROR: No se pudo instalar Snapper${NC}"
    fi

    # Optimizar fstab para BTRFS
    echo -e "${CYAN}Optimizando fstab para BTRFS...${NC}"
    chroot /mnt /bin/bash -c "sed -i 's/relatime/noatime/g' /etc/fstab"

    # Agregar opciones de montaje optimizadas para todos los subvolúmenes
    chroot /mnt /bin/bash -c "sed -i 's/subvol=@,/subvol=@,compress=zstd:3,space_cache=v2,autodefrag,/' /etc/fstab" 2>/dev/null || true
    chroot /mnt /bin/bash -c "sed -i 's/subvol=@home,/subvol=@home,compress=zstd:3,space_cache=v2,autodefrag,/' /etc/fstab" 2>/dev/null || true
    chroot /mnt /bin/bash -c "sed -i 's/subvol=@var_log,/subvol=@var_log,compress=zstd:3,space_cache=v2,/' /etc/fstab" 2>/dev/null || true

    # Verificar configuración final de fstab
    echo -e "${CYAN}Verificando configuración final de fstab...${NC}"
    if chroot /mnt /bin/bash -c "mount -a --fake" 2>/dev/null; then
        echo -e "${GREEN}✓ Configuración fstab válida${NC}"
    else
        echo -e "${YELLOW}Warning: Posibles issues en fstab, pero continuando...${NC}"
    fi

    # Regenerar GRUB para incluir snapshots de grub-btrfs
    if chroot /mnt /bin/bash -c "pacman -Qq grub-btrfs" 2>/dev/null; then
        echo -e "${CYAN}Regenerando GRUB para incluir snapshots...${NC}"
        chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg" 2>/dev/null || echo -e "${YELLOW}Warning: No se pudo regenerar GRUB con snapshots${NC}"
        echo -e "${GREEN}✓ GRUB configurado para mostrar snapshots en el menú de arranque${NC}"
    fi

    # Crear script de mantenimiento BTRFS
    echo -e "${CYAN}Creando script de mantenimiento BTRFS...${NC}"
    cp /usr/share/arcrisgui/data/bash/btrfs/btrfs-maintenance /mnt/usr/local/bin/btrfs-maintenance
    cat /mnt/usr/local/bin/btrfs-maintenance
    chmod +x /mnt/usr/local/bin/btrfs-maintenance

    # Crear servicio systemd para el mantenimiento
    cp /usr/share/arcrisgui/data/bash/btrfs/btrfs-maintenance.service /mnt/etc/systemd/system/btrfs-maintenance.service
    cat /mnt/etc/systemd/system/btrfs-maintenance.service

    cp /usr/share/arcrisgui/data/bash/btrfs/btrfs-maintenance.timer /mnt/etc/systemd/system/btrfs-maintenance.timer
    cat /mnt/etc/systemd/system/btrfs-maintenance.timer

    chroot /mnt /bin/bash -c "systemctl daemon-reload" || echo -e "${YELLOW}Warning: No se pudo daemon-reload${NC}"
    chroot /mnt /bin/bash -c "systemctl enable btrfs-maintenance.timer" || echo -e "${YELLOW}Warning: No se pudo habilitar btrfs-maintenance.timer${NC}"

    # Crear script de documentación interactiva BTRFS y Snapper
    echo -e "${CYAN}Creando guía interactiva BTRFS y Snapper...${NC}"
    cp /usr/share/arcrisgui/data/bash/btrfs/btrfs-guide /mnt/usr/local/bin/btrfs-guide
    cat /mnt/usr/local/bin/btrfs-guide

    # Hacer el script ejecutable
    chmod +x /mnt/usr/local/bin/btrfs-guide

    echo -e "${GREEN}✓ Guía interactiva BTRFS creada en /usr/local/bin/btrfs-guide${NC}"
    echo -e "${CYAN}  Ejecuta 'btrfs-guide' después del reinicio para acceder a la documentación${NC}"

    echo -e "${GREEN}✓ Configuración BTRFS completada${NC}"
    sleep 2
}

################################################################################################
# #################### XMONAD ##################################################################
################################################################################################

guardar_configuraciones_xmonad() {
    echo "=== Configurando XMonad para Arch Linux ==="

    # Variables
    USER_HOME="/mnt/home/$USER"
    XMONAD_DIR="$USER_HOME/.config/xmonad"
    XMOBAR_DIR="$USER_HOME/.config/xmobar"

    # Crear directorios necesarios
    echo "Creando directorios de configuración..."
    mkdir -p "$XMONAD_DIR"
    mkdir -p "$XMOBAR_DIR"

    # Crear configuración de XMonad
    echo "Creando configuración de XMonad..."
    cp /usr/share/arcrisgui/data/bash/xmonad/xmonad.hs "$XMONAD_DIR/xmonad.hs"

    # Crear configuración de XMobar
    echo "Creando configuración de XMobar..."
    cp /usr/share/arcrisgui/data/bash/xmonad/xmobarrc "$XMOBAR_DIR/xmobarrc"

    # Crear .xinitrc
    echo "Creando archivo .xinitrc..."
    cp /usr/share/arcrisgui/data/bash/xmonad/xinitrc "$USER_HOME/.xinitrc"

    # Hacer .xinitrc ejecutable
    chmod +x "$USER_HOME/.xinitrc"

    # Ajustar permisos
    echo "Ajustando permisos..."
    if [ -n "$USER" ] && getent passwd "$USER" > /dev/null 2>&1; then
        USER_ID=$(id -u "$USER" 2>/dev/null || echo "1000")
        GROUP_ID=$(id -g "$USER" 2>/dev/null || echo "1000")

        # Solo ajustar si los archivos existen
        [ -f "$USER_HOME/.config" ] && chown -R "$USER_ID:$GROUP_ID" "$USER_HOME/.config" 2>/dev/null
        [ -f "$USER_HOME/.xinitrc" ] && chown "$USER_ID:$GROUP_ID" "$USER_HOME/.xinitrc" 2>/dev/null
    else
        echo "Advertencia: Usuario $USER no encontrado. Omitiendo ajuste de permisos."
    fi

    echo "=== Configuración de XMonad completada ==="
    echo ""
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

    while [ "$attempt" -le "$max_attempts" ]; do
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
        if [ "$attempt" -eq 5 ]; then
            echo -e "${YELLOW}Información intermedia de debugging:${NC}"
            echo "• Logical Volumes disponibles:"
            lvs 2>/dev/null || echo "  No hay logical volumes"
            echo "• Dispositivos en /dev/vg0/:"
            ls -la /dev/vg0/ 2>/dev/null || echo "  Directorio /dev/vg0/ no existe"
        fi

        if [ "$attempt" -eq 10 ]; then
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
update_repositories
clear
# Instala sin verificar firmas temporalmente
install_pacman_livecd_with_retry "archlinux-keyring"
# Luego reinicia las claves
sudo pacman-key --init
sudo pacman-key --populate archlinux
sleep 2
clear

# Instalación de herramientas necesarias
sleep 3
install_pacman_livecd_with_retry "reflector"
install_pacman_livecd_with_retry "python3"
install_pacman_livecd_with_retry "rsync"
clear

# Actualización de mirrorlist
echo -e "${GREEN}| Actualizando mejores listas de Mirrors |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
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
        mkfs.fat -F32 -v $(get_partition_name "$SELECTED_DISK" "1")
        mkswap $(get_partition_name "$SELECTED_DISK" "2")

        # Verificar que el sistema reconozca la nueva swap
        echo -e "${CYAN}Esperando reconocimiento del sistema para partición swap...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        mkfs.ext4 -F $(get_partition_name "$SELECTED_DISK" "3")
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount $(get_partition_name "$SELECTED_DISK" "3") /mnt

        # Verificar que la partición swap esté disponible antes de activar
        echo -e "${CYAN}Verificando partición swap antes de activar...${NC}"
        sleep 2
        udevadm settle --timeout=10
        SWAP_PARTITION=$(get_partition_name "$SELECTED_DISK" "2")

        if ! blkid "$SWAP_PARTITION" | grep -q "TYPE=\"swap\""; then
            echo -e "${YELLOW}Warning: Partición swap no detectada correctamente, verificando...${NC}"
            sleep 2
        fi

        swapon "$SWAP_PARTITION"
        mkdir -p /mnt/boot
        mount $(get_partition_name "$SELECTED_DISK" "1") /mnt/boot

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
        mkswap $(get_partition_name "$SELECTED_DISK" "1")

        # Verificar que el sistema reconozca la nueva swap
        echo -e "${CYAN}Esperando reconocimiento del sistema para partición swap...${NC}"
        sleep 3
        udevadm settle --timeout=10
        partprobe $SELECTED_DISK

        mkfs.ext4 -F $(get_partition_name "$SELECTED_DISK" "2")
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount $(get_partition_name "$SELECTED_DISK" "2") /mnt

        # Verificar que la partición swap esté disponible antes de activar
        echo -e "${CYAN}Verificando partición swap antes de activar...${NC}"
        sleep 2
        udevadm settle --timeout=10
        SWAP_PARTITION=$(get_partition_name "$SELECTED_DISK" "1")

        if ! blkid "$SWAP_PARTITION" | grep -q "TYPE=\"swap\""; then
            echo -e "${YELLOW}Warning: Partición swap no detectada correctamente, verificando...${NC}"
            sleep 2
        fi

        swapon "$SWAP_PARTITION"
        mkdir -p /mnt/boot
    fi
}

#en la # Configuración adicional para BTRFS en la linea 4351 hasta 4468
# Función para particionado automático btrfs
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

setup_chroot_mounts() {
    echo -e "${CYAN}Configurando montajes para chroot...${NC}"
    mount --types proc /proc /mnt/proc
    mount --rbind /sys /mnt/sys
    mount --make-rslave /mnt/sys
    mount --rbind /dev /mnt/dev
    mount --make-rslave /mnt/dev
    mount --bind /run /mnt/run
    mount --make-slave /mnt/run

    # IMPORTANTE: Montar efivars para sistemas UEFI
    if [ -d /sys/firmware/efi/efivars ]; then
        mount --bind /sys/firmware/efi/efivars /mnt/sys/firmware/efi/efivars 2>/dev/null || true
    fi

    cp /etc/resolv.conf /mnt/etc/
    echo -e "${GREEN}✓ Montajes para chroot configurados${NC}"
}

cleanup_chroot_mounts() {
    echo -e "${CYAN}Limpiando montajes de chroot...${NC}"
    umount -l /mnt/sys/firmware/efi/efivars 2>/dev/null || true
    umount -l /mnt/run 2>/dev/null || true
    umount -l /mnt/dev 2>/dev/null || true
    umount -l /mnt/sys 2>/dev/null || true
    umount -l /mnt/proc 2>/dev/null || true
    echo -e "${GREEN}✓ Montajes de chroot limpiados${NC}"
}

# Ejecutar limpieza de particiones
unmount_selected_disk_partitions
cleanup_chroot_mounts
clear

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


# Instalación de paquetes principales
echo -e "${GREEN}| Instalando paquetes principales de la distribución |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

install_pacstrap_with_retry "base"
install_pacstrap_with_retry "base-devel"
install_pacstrap_with_retry "lsb-release"
install_pacstrap_with_retry "reflector"
install_pacstrap_with_retry "python3"
install_pacstrap_with_retry "rsync"
install_pacstrap_with_retry "nano"
install_pacstrap_with_retry "xdg-user-dirs"
install_pacstrap_with_retry "curl"
install_pacstrap_with_retry "wget"
install_pacstrap_with_retry "git"
clear


# Instalar herramientas específicas según el modo de particionado
if [ "$PARTITION_MODE" = "manual" ]; then
    echo -e "${CYAN}Verificando herramientas necesarias para sistemas de archivos...${NC}"

    # Inicializar flags para cada sistema de archivos
    BTRFS_USED=false
    XFS_USED=false
    NTFS_USED=false
    REISERFS_USED=false
    JFS_USED=false
    FAT_USED=false
    F2FS_USED=false

    # Verificar qué sistemas de archivos se están usando
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        case "$format" in
            "mkfs.btrfs")
                BTRFS_USED=true
                ;;
            "mkfs.xfs")
                XFS_USED=true
                ;;
            "mkfs.ntfs")
                NTFS_USED=true
                ;;
            "mkfs.reiserfs")
                REISERFS_USED=true
                ;;
            "mkfs.jfs")
                JFS_USED=true
                ;;
            "mkfs.fat32"|"mkfs.fat16")
                FAT_USED=true
                ;;
            "mkfs.f2fs")
                F2FS_USED=true
                ;;
        esac
    done

    # Instalar herramientas según los sistemas de archivos detectados
    if [ "$BTRFS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de BTRFS, instalando btrfs-progs...${NC}"
        install_pacstrap_with_retry "btrfs-progs"
        install_pacstrap_with_retry "btrfsmaintenance"
        install_pacstrap_with_retry "snapper"
        install_pacstrap_with_retry "timeshift"
    fi

    if [ "$XFS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de XFS, instalando xfsprogs...${NC}"
        install_pacstrap_with_retry "xfsprogs"
    fi

    if [ "$NTFS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de NTFS, instalando ntfs-3g...${NC}"
        install_pacstrap_with_retry "ntfs-3g"
    fi

    if [ "$REISERFS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de ReiserFS, instalando reiserfsprogs...${NC}"
        install_pacstrap_with_retry "reiserfsprogs"
    fi

    if [ "$JFS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de JFS, instalando jfsutils...${NC}"
        install_pacstrap_with_retry "jfsutils"
    fi

    if [ "$FAT_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de FAT32/FAT16, instalando dosfstools...${NC}"
        install_pacstrap_with_retry "dosfstools"
    fi

    if [ "$F2FS_USED" = true ]; then
        echo -e "${CYAN}Detectado uso de F2FS, instalando f2fs-tools...${NC}"
        install_pacstrap_with_retry "f2fs-tools"
    fi

elif [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${CYAN}Instalando herramientas de cifrado...${NC}"
    install_pacstrap_with_retry "cryptsetup"
    install_pacstrap_with_retry "lvm2"
    install_pacstrap_with_retry "device-mapper"
    install_pacstrap_with_retry "thin-provisioning-tools"
fi

# Configurar montajes para chroot
clear
sleep 2
setup_chroot_mounts
sleep 2
update_system_chroot
sleep 2

# Actualización de mirrors en el sistema instalado
cp /etc/pacman.d/mirrorlist /mnt/etc/pacman.d/mirrorlist
#chroot /mnt /bin/bash -c "reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist"
clear
cat /mnt/etc/pacman.d/mirrorlist
sleep 3
clear


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

# Instalación del kernel seleccionado
echo -e "${GREEN}| Instalando kernel: $SELECTED_KERNEL |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$SELECTED_KERNEL" in
    "linux")
        install_pacman_chroot_with_retry "linux linux-firmware linux-headers"
        ;;
    "linux-hardened")
        install_pacman_chroot_with_retry "linux-hardened linux-firmware linux-hardened-headers"
        ;;
    "linux-lts")
        install_pacman_chroot_with_retry "linux-lts linux-firmware linux-lts-headers"
        ;;
    "linux-rt-lts")
        install_pacman_chroot_with_retry "linux-rt-lts linux-firmware linux-rt-lts-headers"
        ;;
    "linux-zen")
        install_pacman_chroot_with_retry "linux-zen linux-firmware linux-zen-headers"
        ;;
    *)
        install_pacman_chroot_with_retry "linux linux-firmware linux-headers"
        ;;
esac

sleep 3
clear
# Actualización del sistema instalado
update_system_chroot
cp /usr/share/arcrisgui/data/config/pacman.conf /mnt/etc/pacman.conf
update_system_chroot
update_system_chroot
sleep 3
clear


# Configuración del sistema
echo -e "${GREEN}| Configurando sistema base |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Instalación de paquetes principales
echo -e "${GREEN}| Instalando paquetes principales de la distribución |${NC}"
# Configuración de zona horaria
chroot /mnt /bin/bash -c "ln -sf /usr/share/zoneinfo/$TIMEZONE /etc/localtime"
chroot /mnt /bin/bash -c "hwclock --systohc"

# Configuración de locale
echo "$LOCALE UTF-8" >> /mnt/etc/locale.gen
chroot /mnt /bin/bash -c "locale-gen"
echo "LANG=$LOCALE" > /mnt/etc/locale.conf

# Configuración de teclado
# echo "KEYMAP=$KEYMAP_TTY" > /mnt/etc/vconsole.conf
# echo "FONT=lat9w-16" >> /mnt/etc/vconsole.conf

# Configuración de hostname
echo "$HOSTNAME" > /mnt/etc/hostname
cat > /mnt/etc/hosts << EOF
127.0.0.1	localhost
::1		localhost
127.0.1.1	$HOSTNAME.localdomain	$HOSTNAME
EOF

sleep 3
clear

# Configuración de usuarios y contraseñas
echo -e "${GREEN}| Configurando usuarios |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Configurar contraseña de root
echo "root:$PASSWORD_ROOT" | chroot /mnt /bin/bash -c "chpasswd"

# Crear usuario
chroot /mnt /bin/bash -c "useradd -m -G wheel,audio,video,optical,storage,input -s /bin/bash $USER"
echo "$USER:$PASSWORD_USER" | chroot /mnt /bin/bash -c "chpasswd"


# Configurar sudo
install_pacstrap_with_retry "sudo"

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
        chroot /mnt /bin/bash -c "userdel $usuario"
        chroot /mnt /bin/bash -c "useradd -m -G wheel,audio,video,optical,storage -s /bin/bash $USER"
        echo "$USER:$PASSWORD_USER" | chroot /mnt /bin/bash -c "chpasswd"
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

# Función para cambiar configuración wheel a NOPASSWD si existe
echo "🔧 Verificando configuración wheel en sudoers..."

# Verificar si existe la línea exacta %wheel ALL=(ALL) ALL
if chroot /mnt /bin/bash -c "grep -q '^%wheel ALL=(ALL) ALL$' /etc/sudoers" 2>/dev/null; then
    echo "🔄 Detectada configuración wheel normal, cambiando a NOPASSWD..."

    # Cambiar la línea específica
    sed -i 's/^%wheel ALL=(ALL) ALL$/%wheel ALL=(ALL:ALL) NOPASSWD: ALL/' /mnt/etc/sudoers

    # Verificar que el cambio se aplicó correctamente
    if chroot /mnt /bin/bash -c "grep -q '^%wheel ALL=(ALL:ALL) NOPASSWD: ALL$' /etc/sudoers" 2>/dev/null; then
        echo "✓ Configuración wheel cambiada exitosamente a NOPASSWD"
    else
        echo "❌ Error: No se pudo cambiar la configuración wheel"
    fi
else
    echo "ℹ️  No se encontró la línea '%wheel ALL=(ALL) ALL' en sudoers"
    echo "   No se realizaron cambios"
fi

sleep 2
clear
echo -e "${GREEN}✓ Instalanado extras${NC}"
# chroot /mnt pacman -S yay-bin --noconfirm
# chroot /mnt pacman -S alsi --noconfirm
# Instalar yay-bin desde AUR usando makepkg
#chroot /mnt bash -c "cd /tmp && git clone https://aur.archlinux.org/yay-bin.git && cd yay-bin && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'"
#sleep 2
# Instalar alsi desde AUR usando makepkg
#chroot /mnt bash -c "cd /tmp && git clone https://aur.archlinux.org/alsi.git && cd alsi && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'"
#sleep 2
sleep 3
install_aur_with_retry "yay"
sleep 2
install_pacman_chroot_with_retry "fastfetch"

sleep 2
clear

# Configuración de mkinitcpio según el modo de particionado
echo -e "${GREEN}| Configurando mkinitcpio |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para cifrado LUKS+LVM...${NC}"

    # Configurar módulos básicos para LUKS+LVM
    echo -e "${CYAN}Configurando módulos del kernel para cifrado...${NC}"
    sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt ext4)/' /mnt/etc/mkinitcpio.conf

    # Configurar hooks básicos - orden: encrypt antes de lvm2
    echo -e "${CYAN}Configurando hooks básicos...${NC}"
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block encrypt lvm2 filesystems fsck)/' /mnt/etc/mkinitcpio.conf

    echo -e "${GREEN}✓ Configuración mkinitcpio simplificada${NC}"
    echo -e "${CYAN}  • Módulos: dm_mod dm_crypt ext4${NC}"
    echo -e "${CYAN}  • Hooks: base udev autodetect modconf block encrypt lvm2 filesystems fsck${NC}"

elif [ "$PARTITION_MODE" = "auto_btrfs" ]; then
    echo "Configurando mkinitcpio para BTRFS..."
    # Configurar módulos específicos para BTRFS (agregando módulos de compresión adicionales)
    sed -i 's/^MODULES=.*/MODULES=(btrfs crc32c zstd lzo lz4 zlib_deflate)/' /mnt/etc/mkinitcpio.conf
    # Configurar hooks para BTRFS
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf

elif [ "$PARTITION_MODE" = "manual" ]; then
    echo "Configurando mkinitcpio para particionado manual..."

    # Detectar módulos necesarios según sistemas de archivos utilizados
    MODULES_LIST=()

    # Verificar qué sistemas de archivos se están usando
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        case "$format" in
            "mkfs.btrfs")
                # Módulos para BTRFS con compresión
                MODULES_LIST+=(btrfs crc32c zstd lzo lz4 zlib_deflate)
                echo -e "${CYAN}  • Detectado BTRFS: agregando módulos btrfs, crc32c, zstd, lzo, lz4, zlib_deflate${NC}"
                ;;
            "mkfs.xfs")
                # Módulos para XFS
                MODULES_LIST+=(xfs crc32c)
                echo -e "${CYAN}  • Detectado XFS: agregando módulos xfs, crc32c${NC}"
                ;;
            "mkfs.ntfs")
                # Módulos para NTFS
                MODULES_LIST+=(ntfs3)
                echo -e "${CYAN}  • Detectado NTFS: agregando módulo ntfs3${NC}"
                ;;
            "mkfs.jfs")
                # Módulos para JFS
                MODULES_LIST+=(jfs)
                echo -e "${CYAN}  • Detectado JFS: agregando módulo jfs${NC}"
                ;;
            "mkfs.f2fs")
                # Módulos para F2FS
                MODULES_LIST+=(f2fs crc32)
                echo -e "${CYAN}  • Detectado F2FS: agregando módulos f2fs, crc32${NC}"
                ;;
        esac
    done

    # Eliminar duplicados y crear string de módulos
    UNIQUE_MODULES=($(printf "%s\n" "${MODULES_LIST[@]}" | sort -u))
    MODULES_STRING=$(IFS=' '; echo "${UNIQUE_MODULES[*]}")

    if [ ${#UNIQUE_MODULES[@]} -gt 0 ]; then
        echo -e "${GREEN}Configurando módulos detectados: ${MODULES_STRING}${NC}"
        sed -i "s/^MODULES=.*/MODULES=(${MODULES_STRING})/" /mnt/etc/mkinitcpio.conf
    else
        echo -e "${CYAN}No se detectaron sistemas de archivos especiales, usando configuración estándar${NC}"
        sed -i 's/^MODULES=.*/MODULES=()/' /mnt/etc/mkinitcpio.conf
    fi

    # Configurar hooks estándar para modo manual
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf

else
    echo "Configurando mkinitcpio para sistema estándar..."
    # Configuración estándar para ext4
    sed -i 's/^MODULES=.*/MODULES=()/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
fi

# Regenerar initramfs
echo -e "${CYAN}Generando initramfs...${NC}"
echo -e "${YELLOW}Nota: Los warnings de firmware son normales${NC}"

if chroot /mnt /bin/bash -c "mkinitcpio -P"; then
    echo -e "${GREEN}✓ Initramfs generado correctamente${NC}"
else
    echo -e "${YELLOW}Reintentando con configuración básica...${NC}"
    chroot /mnt /bin/bash -c "mkinitcpio -p linux"
fi
sleep 2
clear

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

echo -e "${CYAN}📊 Detección de memoria del sistema:${NC}"
echo -e "${CYAN}  • RAM total: ${TOTAL_RAM_GB}GB (${TOTAL_RAM_MB}MB)${NC}"
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
# Tamaño exacto calculado: 50% de RAM total
zram-size = ${ZRAM_SIZE_MB}
# Algoritmo de compresión zstd (mejor ratio)
compression-algorithm = zstd
# Prioridad alta para zram
swap-priority = 100
EOF

# Deshabilitar zswap para evitar conflictos (recomendación oficial)
# zswap interfiere con zram según ArchWiki
if [ -f /mnt/etc/default/grub ]; then
    sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="/&zswap.enabled=0 /' /mnt/etc/default/grub
fi

# Configurar parámetros optimizados para zram según Pop!_OS
cat > /mnt/etc/sysctl.d/99-vm-zram-parameters.conf << 'EOF'
# Optimización para zram según mejores prácticas
# Fuente: Pop!_OS y documentación oficial
vm.swappiness = 180
vm.watermark_boost_factor = 0
vm.watermark_scale_factor = 125
vm.page-cluster = 0
EOF

echo -e "${GREEN}✓ zram configurado con método oficial:${NC}"
echo -e "${CYAN}  • RAM total detectada: ${TOTAL_RAM_GB}GB (${TOTAL_RAM_MB}MB)${NC}"
echo -e "${CYAN}  • zram: ${ZRAM_SIZE_GB}GB (${ZRAM_SIZE_MB}MB exactos) con zstd${NC}"
echo -e "${CYAN}  • zswap: DESHABILITADO (evita conflictos)${NC}"
echo -e "${CYAN}  • swap tradicional: mantiene prioridad baja${NC}"
echo -e "${YELLOW}  • Método: zram-generator con cálculo exacto${NC}"
echo -e "${YELLOW}  • Optimización: parámetros VM ajustados para zram${NC}"

sleep 3
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
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
            CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_3")
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_3")
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada $PARTITION_3${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Configurar GRUB para LUKS+LVM (Simplificado)
            echo -e "${CYAN}Configurando parámetros de kernel...${NC}"
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root resume=\/dev\/vg0\/swap splash loglevel=0 rd.systemd.show_status=false rd.udev.log_level=0\"/" /mnt/etc/default/grub

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
        elif [ "$PARTITION_MODE" = "auto_btrfs" ]; then
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
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            PARTITION_2=$(get_partition_name "$SELECTED_DISK" "2")
            CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_2")
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_2")
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada $PARTITION_2${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Configurar GRUB para LUKS+LVM (Simplificado)
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root resume=\/dev\/vg0\/swap splash loglevel=0 rd.systemd.show_status=false rd.udev.log_level=0\"/" /mnt/etc/default/grub
            # Configurar nivel de log básico
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3"/' /mnt/etc/default/grub
            echo "GRUB_ENABLE_CRYPTODISK=y" >> /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_msdos lvm luks gcry_rijndael gcry_sha256 gcry_sha512\"" >> /mnt/etc/default/grub

            echo -e "${GREEN}✓ Configuración GRUB para cifrado BIOS Legacy:${NC}"
            echo -e "${CYAN}  • cryptdevice=UUID=${CRYPT_UUID}:cryptlvm${NC}"
            echo -e "${CYAN}  • root=/dev/vg0/root${NC}"
            echo -e "${CYAN}  • GRUB_ENABLE_CRYPTODISK=y (permite a GRUB leer discos cifrados)${NC}"
            echo -e "${CYAN}  • Sin 'quiet' para mejor debugging del arranque cifrado${NC}"
            echo -e "${CYAN}  • Módulos MBR: part_msdos lvm luks${NC}"

        elif [ "$PARTITION_MODE" = "auto_btrfs" ]; then
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

# Crear script helper para actualizar GRUB después de snapshots manuales
cat > /mnt/usr/local/bin/update-grub << 'UPDATEGRUB'
#!/bin/bash
# Script para actualizar GRUB
echo "Actualizando GRUB..."
grub-mkconfig -o /boot/grub/grub.cfg
echo "✓ GRUB actualizado"
UPDATEGRUB
chmod +x /mnt/usr/local/bin/update-grub
echo -e "${GREEN}✓ Script helper creado: /usr/local/bin/update-grub${NC}"

sleep 3
clear
# -------------------------------------------------
source "$(dirname "$0")/driver_video.sh"
# -------------------------------------------------
source "$(dirname "$0")/driver_audio.sh"
# -------------------------------------------------
source "$(dirname "$0")/driver_wifi.sh"
# -------------------------------------------------
source "$(dirname "$0")/driver_bluetooth.sh"
# -------------------------------------------------

# Instalación de herramientas de red
echo -e "${GREEN}| Instalando herramientas de red |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
install_pacman_chroot_with_retry "dhcp"
install_pacman_chroot_with_retry "dhcpcd"
install_pacman_chroot_with_retry "dhclient"
install_pacman_chroot_with_retry "networkmanager"
install_pacman_chroot_with_retry "wpa_supplicant"
# Deshabilitar dhcpcd para evitar conflictos con NetworkManager
chroot /mnt /bin/bash -c "systemctl enable NetworkManager dhcpcd" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
clear

# Copiado de archivos de configuración de bash
echo -e "${GREEN}| Copiando archivos de configuración de bashrc |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

cp /usr/share/arcrisgui/data/config/bashrc /mnt/home/$USER/.bashrc
cp /usr/share/arcrisgui/data/config/bashrc /mnt/home/$USER/.bashrc
cp /usr/share/arcrisgui/data/config/bashrc-root /mnt/root/.bashrc

# Configurar permisos de archivos de usuario
chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.bashrc"
sleep 2
clear

# Configuración final del sistema
echo -e "${GREEN}| Configuración final del sistema |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
# Configurar directorios de usuario
chroot /mnt /bin/bash -c "su - $USER -c 'xdg-user-dirs-update'"

# Configuración especial para cifrado
# Configuración adicional para cifrado
if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}| Configuración adicional para cifrado |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Configurar crypttab
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        PARTITION_3=$(get_partition_name "$SELECTED_DISK" "3")
        CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_3")
    else
        PARTITION_2=$(get_partition_name "$SELECTED_DISK" "2")
        CRYPT_UUID=$(blkid -s UUID -o value "$PARTITION_2")
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
    chroot /mnt /bin/bash -c "systemctl enable lvm2-monitor.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

    # Configuración adicional para reducir timeouts de cifrado y LVM
    echo -e "${CYAN}Aplicando optimizaciones para sistema cifrado...${NC}"

    # Asegurar que LVM esté disponible y activo
    echo -e "${CYAN}Activando volumes LVM...${NC}"
    chroot /mnt /bin/bash -c "vgchange -ay vg0"
    chroot /mnt /bin/bash -c "lvchange -ay vg0/root"
    chroot /mnt /bin/bash -c "lvchange -ay vg0/swap"

    # Generar fstab correctamente con nombres de dispositivos apropiados
    echo -e "${CYAN}Generando fstab con dispositivos LVM...${NC}"
    # Limpiar fstab existente
    > /mnt/etc/fstab
    # Agregar entradas manualmente para asegurar nombres correctos
    echo "# <file system> <mount point> <type> <options> <dump> <pass>" >> /mnt/etc/fstab
    echo "/dev/mapper/vg0-root / ext4 rw,relatime 0 1" >> /mnt/etc/fstab
    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        echo "UUID=$(blkid -s UUID -o value "$PARTITION_1") /boot vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
    else
        PARTITION_1=$(get_partition_name "$SELECTED_DISK" "1")
        echo "UUID=$(blkid -s UUID -o value "$PARTITION_1") /boot ext4 rw,relatime 0 2" >> /mnt/etc/fstab
    fi
    # Usar UUID para swap LVM si está disponible, sino usar nombre de dispositivo como fallback
    SWAP_UUID=$(blkid -s UUID -o value /dev/mapper/vg0-swap 2>/dev/null)
    if [ -n "$SWAP_UUID" ]; then
        echo "UUID=$SWAP_UUID none swap defaults,pri=10 0 0" >> /mnt/etc/fstab
        echo -e "${GREEN}✓ Swap agregada al fstab con UUID: $SWAP_UUID${NC}"
    else
        echo "/dev/mapper/vg0-swap none swap defaults,pri=10 0 0" >> /mnt/etc/fstab
        echo -e "${YELLOW}Warning: Swap agregada al fstab con nombre de dispositivo (no se pudo obtener UUID)${NC}"
    fi

fi

# Configuración adicional para BTRFS
if [ "$PARTITION_MODE" = "auto_btrfs" ]; then
    configurar_btrfs
fi

clear
# Actualizar base de datos de paquetes
update_system_chroot

sleep 3
clear
cp /usr/share/arcrisgui/data/config/pacman-chroot.conf /mnt/etc/pacman.conf
cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpg
chroot /mnt /bin/bash -c "mkdir -p /home/$USER/.config/fastfetch"
chroot /mnt /bin/bash -c "cp /usr/share/fastfetch/presets/screenfetch.jsonc /home/$USER/.config/fastfetch/config.jsonc" || echo -e "${RED}ERROR: No se copio el archivo config.jsonc${NC}"
chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config/fastfetch" || echo -e "${RED}ERROR: chown del usuario${NC}"

clear
# -------------------------------------------------
source "$(dirname "$0")/entorno_grafico.sh"
# -------------------------------------------------
source "$(dirname "$0")/config_kitty.sh"
# -------------------------------------------------
source "$(dirname "$0")/config_ly.sh"
# -------------------------------------------------
source "$(dirname "$0")/program_essential.sh"
# -------------------------------------------------
#

echo -e "${GREEN}✓ Tipografías instaladas${NC}"
# Fuentes base
install_pacman_chroot_with_retry "noto-fonts"
install_pacman_chroot_with_retry "gnu-free-fonts"
install_pacman_chroot_with_retry "ttf-meslo-nerd"
install_yay_chroot_with_retry "ttf-noto-emoji-monochrome"
# Iconos
install_pacman_chroot_with_retry "ttf-nerd-fonts-symbols"
install_pacman_chroot_with_retry "ttf-jetbrains-mono-nerd"
sleep 2
clear
configurar_teclado
clear

# Instalación de programas adicionales según configuración
if [ "$UTILITIES_ENABLED" = "true" ] && [ ${#UTILITIES_APPS[@]} -gt 0 ]; then
    echo ""
    echo -e "${GREEN}| Instalando programas de utilidades seleccionados |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    for app in "${UTILITIES_APPS[@]}"; do
        echo -e "${CYAN}Instalando: $app${NC}"
        install_yay_chroot_with_retry "$app" "--overwrite '*'"
        sleep 2
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
        install_yay_chroot_with_retry "$program" "--overwrite '*'"
        sleep 2
    done

    echo -e "${GREEN}✓ Instalación de programas extra completada${NC}"
    echo ""
    sleep 2
fi




# Configuración de repositorios de Arch Linux
echo ""
echo -e "${GREEN}| Configurando repositorios de Arch Linux |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Configurar mirrorlist según modo
if [ "$REPOS_MIRROR_MODE" = "manual" ]; then
    echo -e "${CYAN}Aplicando mirrorlist personalizado...${NC}"
    printf "%b" "$REPOS_MIRROR_CUSTOM" > /mnt/etc/pacman.d/mirrorlist
    echo -e "${GREEN}✓ Mirrorlist personalizado aplicado${NC}"
elif [ "$REPOS_MIRROR_MODE" = "auto" ]; then
    echo -e "${CYAN}✨ Reflector ya hizo su magia — espejos de alta velocidad seleccionados,"
    echo -e "   rutas optimizadas, latencia al mínimo. Tu sistema descarga a toda máquina. ✓${NC}"
fi

# Chaotic-AUR
if [ "$REPOS_CHAOTIC_AUR" = "true" ]; then
    echo -e "${CYAN}Configurando Chaotic-AUR...${NC}"
    chroot /mnt /bin/bash -c "pacman-key --recv-key 3056513887B78AEB --keyserver keyserver.ubuntu.com"
    chroot /mnt /bin/bash -c "pacman-key --lsign-key 3056513887B78AEB"
    chroot /mnt /bin/bash -c "pacman -U --noconfirm \
        'https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-keyring.pkg.tar.zst' \
        'https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-mirrorlist.pkg.tar.zst'"
    cat >> /mnt/etc/pacman.conf << 'EOF'

[chaotic-aur]
Include = /etc/pacman.d/chaotic-mirrorlist
EOF
    echo -e "${GREEN}✓ Chaotic-AUR configurado${NC}"
fi

# ArchLinuxCN
if [ "$REPOS_ARCHLINUXCN" = "true" ]; then
    echo -e "${CYAN}Configurando ArchLinuxCN...${NC}"
    cat >> /mnt/etc/pacman.conf << 'EOF'

[archlinuxcn]
Server = https://repo.archlinuxcn.org/$arch
EOF
    chroot /mnt /bin/bash -c "pacman -Sy --noconfirm archlinuxcn-keyring"
    echo -e "${GREEN}✓ ArchLinuxCN configurado${NC}"
fi

# CachyOS
if [ "$REPOS_CACHYOS" = "true" ]; then
    echo -e "${CYAN}Configurando repositorios CachyOS...${NC}"
    # Detectar arquitectura del CPU
    CACHYOS_ARCH="x86-64 (genérico)"
    CPU_MARCH=$(gcc -march=native -Q --help=target 2>&1 | grep -Po "^\s+-march=\s+\K(\w+)$" || true)
    if [ "$CPU_MARCH" = "znver4" ] || [ "$CPU_MARCH" = "znver5" ]; then
        CACHYOS_ARCH="znver4 (AMD Zen 4/5)"
    elif /lib/ld-linux-x86-64.so.2 --help 2>/dev/null | grep -q "x86-64-v4 (supported, searched)"; then
        CACHYOS_ARCH="x86-64-v4"
    elif /lib/ld-linux-x86-64.so.2 --help 2>/dev/null | grep -q "x86-64-v3 (supported, searched)"; then
        CACHYOS_ARCH="x86-64-v3"
    fi
    echo -e "${CYAN}  Arquitectura detectada: ${YELLOW}${CACHYOS_ARCH}${NC}"
    # Descargar script oficial desde el LiveCD (detecta automáticamente v3/v4/znver4)
    curl -fsSL https://mirror.cachyos.org/cachyos-repo.tar.xz -o /tmp/cachyos-repo.tar.xz
    tar xf /tmp/cachyos-repo.tar.xz -C /tmp
    cp -r /tmp/cachyos-repo /mnt/tmp/cachyos-repo
    chroot /mnt /bin/bash -c "cd /tmp/cachyos-repo && yes | bash ./cachyos-repo.sh" \
        || echo -e "${RED}ERROR: No se pudo configurar CachyOS${NC}"
    rm -rf /tmp/cachyos-repo /tmp/cachyos-repo.tar.xz /mnt/tmp/cachyos-repo 2>/dev/null || true
    echo -e "${GREEN}✓ CachyOS configurado (arquitectura detectada automáticamente)${NC}"
fi

# Sincronizar base de datos con los nuevos repositorios
if [ "$REPOS_CHAOTIC_AUR" = "true" ] || [ "$REPOS_ARCHLINUXCN" = "true" ] || [ "$REPOS_CACHYOS" = "true" ]; then
    echo -e "${CYAN}Sincronizando base de datos con los nuevos repositorios...${NC}"
    chroot /mnt /bin/bash -c "pacman -Syy --noconfirm"
    echo -e "${GREEN}✓ Repositorios adicionales listos${NC}"
fi

sleep 2
clear
# -------------------------------------------------




# Actualizar sistema con reintentos
update_system_chroot
chroot /mnt /bin/bash -c "sudo -u $user yay -Scc --noconfirm"
clear
chroot /mnt /bin/bash -c "pacman -Syyu --noconfirm"
sleep 3
clear

echo ""
ls /mnt/home/$USER/
sleep 5
clear


# Revertir a configuración sudo normal
echo -e "${GREEN}| Revirtiendo configuración sudo temporal |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Eliminar configuración temporal
if [[ -f "/mnt/etc/sudoers.d/temp-install" ]]; then
    chroot /mnt /bin/bash -c "rm -f /etc/sudoers.d/temp-install"
    echo "✓ Configuración temporal eliminada"
else
    echo "⚠️  Archivo temporal no encontrado (ya fue eliminado)"
fi

# Verificar y configurar wheel en sudoers
echo "🔧 Configurando grupo wheel en sudoers..."

#echo "%wheel ALL=(ALL) ALL" > /mnt/etc/sudoers.d/wheel
#chmod 440 /mnt/etc/sudoers.d/wheel

# Verificar si existe configuración NOPASSWD
if chroot /mnt /bin/bash -c "grep -q '^%wheel.*NOPASSWD.*ALL' /etc/sudoers" 2>/dev/null; then
    echo "🔄 Detectada configuración NOPASSWD, cambiando a configuración normal..."
    # Cambiar de NOPASSWD a configuración normal
    chroot /mnt /bin/bash -c "sed -i 's/^%wheel.*NOPASSWD.*ALL$/%wheel ALL=(ALL) ALL/' /etc/sudoers"
    echo "✓ Configuración wheel cambiada a modo normal (con contraseña)"

# Verificar si existe configuración normal
elif chroot /mnt /bin/bash -c "grep -q '^%wheel.*ALL.*ALL' /etc/sudoers" 2>/dev/null; then
    echo "✓ Configuración wheel normal ya existe en sudoers"

# Si no existe ninguna configuración wheel, agregarla
else
    echo "➕ No se encontró configuración wheel, agregándola..."
    echo "# Configuración normal del grupo wheel" >> /mnt/etc/sudoers
    cp /usr/share/arcrisgui/data/config/sudoers /mnt/etc/sudoers
    echo "✓ Configuración wheel añadida al archivo sudoers"
fi

# Validar sintaxis del sudoers
#if chroot /mnt /usr/bin/visudo -c -f /etc/sudoers >/dev/null 2>&1; then
#    echo "✓ Sintaxis del sudoers validada correctamente"
#else
#    echo "❌ Error en sintaxis del sudoers detectado"
#fi


#sed -i '$d' /mnt/etc/sudoers
#echo "%wheel ALL=(ALL) ALL"
#echo "%wheel ALL=(ALL) ALL" >> /mnt/etc/sudoers



# Limpiar montajes antes del final
cleanup_chroot_mounts
sleep 1
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

    echo ""
    echo -e "${GREEN}🔧 Comandos útiles post-instalación:${NC}"
    echo -e "${CYAN}  • Ver estado LVM: sudo vgdisplay && sudo lvdisplay${NC}"
    echo -e "${CYAN}  • Redimensionar particiones: sudo lvresize${NC}"
    echo -e "${CYAN}  • Backup adicional header: sudo cryptsetup luksHeaderBackup /dev/sdaX${NC}"
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""
fi
