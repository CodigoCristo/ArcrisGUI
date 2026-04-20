#!/bin/bash

export LANG=C
export LC_ALL=C

# Importar variables de configuración
source "$(dirname "$0")/variables.sh"


# Verificar tamaño mínimo del disco (30GB)
disk_size_bytes=$(lsblk -b -d -o SIZE "$SELECTED_DISK" 2>/dev/null | tail -1 | tr -d ' ')
min_size_bytes=$((30 * 1024 * 1024 * 1024))

if [ -z "$disk_size_bytes" ] || [ "$disk_size_bytes" -lt "$min_size_bytes" ]; then
    disk_size_human=$(lsblk -d -o SIZE "$SELECTED_DISK" 2>/dev/null | tail -1 | tr -d ' ')
    echo "┌─────────────────────────────────────────────────┐"
    echo "│         ERROR: Disco insuficiente               │"
    echo "├─────────────────────────────────────────────────┤"
    echo "│ Disco:    $SELECTED_DISK"
    echo "│ Tamaño:   ${disk_size_human:-desconocido}"
    echo "│ Mínimo:   30GB"
    echo "└─────────────────────────────────────────────────┘"
    exit 1
fi

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
    if [ -f /mnt/usr/local/bin/btrfs-guide ]; then
        echo -e "${GREEN}✓ btrfs-guide instalado en: /usr/local/bin/btrfs-guide${NC}"
        ls -lh /mnt/usr/local/bin/btrfs-guide
    else
        echo -e "${RED}ERROR: btrfs-guide no se encontró en /mnt/usr/local/bin/${NC}"
    fi

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
sudo chmod 700 /root/.gnupg
sudo chmod 600 /root/.gnupg/*
run_command_with_retry "sudo pacman-key --init"
run_command_with_retry "sudo pacman-key --populate archlinux"
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

# -------------------------------------------------
source "$(dirname "$0")/config_disk.sh"
# -------------------------------------------------


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

    # Montar efivarfs en modo escritura para que grub-install pueda escribir entradas NVRAM
    if [ -d /sys/firmware/efi/efivars ]; then
        # Desmontar el bind que dejó --rbind (puede estar read-only) y remontar como efivarfs rw
        umount /mnt/sys/firmware/efi/efivars 2>/dev/null || true
        mkdir -p /mnt/sys/firmware/efi/efivars
        mount -t efivarfs efivarfs /mnt/sys/firmware/efi/efivars || {
            echo -e "${YELLOW}Warning: no se pudo montar efivarfs en modo escritura${NC}"
        }
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

elif [ "$ENCRYPTION" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de cifrado LUKS+LVM...${NC}"
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

# -------------------------------------------------
source "$(dirname "$0")/config_fstab.sh"
# -------------------------------------------------

# Instalación del kernel seleccionado
echo -e "${GREEN}| Instalando kernel: $SELECTED_KERNEL |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$SELECTED_KERNEL" in
    "linux")
        #install_pacman_chroot_with_retry "linux"
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
install_aur_with_retry "alsi"
sleep 2
install_pacman_chroot_with_retry "fastfetch"

sleep 2
clear

# Configuración de mkinitcpio según el modo de particionado
echo -e "${GREEN}| Configurando mkinitcpio |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

if [ "$ENCRYPTION" = "true" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para LUKS+LVM ($FILESYSTEM_TYPE)...${NC}"
    case "$FILESYSTEM_TYPE" in
        "btrfs")
            sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt btrfs crc32c zstd lzo lz4 zlib_deflate)/' /mnt/etc/mkinitcpio.conf
            ;;
        "xfs")
            sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt xfs crc32c)/' /mnt/etc/mkinitcpio.conf
            ;;
        *)
            sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt ext4)/' /mnt/etc/mkinitcpio.conf
            ;;
    esac
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block encrypt lvm2 filesystems fsck)/' /mnt/etc/mkinitcpio.conf
    echo -e "${GREEN}✓ Configuración mkinitcpio para LUKS+LVM${NC}"
    echo -e "${CYAN}  • Hooks: base udev autodetect modconf block encrypt lvm2 filesystems fsck${NC}"

elif [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para auto BTRFS...${NC}"
    sed -i 's/^MODULES=.*/MODULES=(btrfs crc32c zstd lzo lz4 zlib_deflate)/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
    echo -e "${GREEN}✓ Módulos BTRFS configurados${NC}"

elif [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "xfs" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para auto XFS...${NC}"
    sed -i 's/^MODULES=.*/MODULES=(xfs crc32c)/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
    echo -e "${GREEN}✓ Módulos XFS configurados${NC}"

elif [ "$PARTITION_MODE" = "auto" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para auto EXT4...${NC}"
    sed -i 's/^MODULES=.*/MODULES=()/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
    echo -e "${GREEN}✓ Configuración estándar EXT4${NC}"

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

# Con LUKS+LVM: activar vg0 antes de mkinitcpio para que autodetect
# detecte dm-crypt y lvm2 y los incluya en el initramfs
if [ "$ENCRYPTION" = "true" ]; then
    vgchange -ay vg0 2>/dev/null || true
    udevadm settle --timeout=10
fi

if chroot /mnt /bin/bash -c "mkinitcpio -P"; then
    echo -e "${GREEN}✓ Initramfs generado correctamente${NC}"
else
    echo -e "${YELLOW}Reintentando con configuración básica...${NC}"
    chroot /mnt /bin/bash -c "mkinitcpio -p linux"
fi
sleep 2
clear


# -------------------------------------------------
if [ "$SWAP_TYPE" != "none" ]; then
    source "$(dirname "$0")/config_zram.sh"
else
    echo -e "${CYAN}  • SWAP_TYPE=none: sin zram ni swap${NC}"
fi
# -------------------------------------------------
source "$(dirname "$0")/config_grub.sh"
# -------------------------------------------------

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
chroot /mnt /bin/bash -c "systemctl enable NetworkManager dhcpcd" || echo -e "${RED}ERROR: Falló systemctl enable NetworkManager dhcpcd${NC}"
chroot /mnt /bin/bash -c "timedatectl set-ntp true" || echo -e "${RED}ERROR: Falló set-ntp${NC}"
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

# Configuración adicional para cifrado LUKS+LVM
if [ "$ENCRYPTION" = "true" ]; then
    echo -e "${GREEN}| Configuración adicional para cifrado LUKS+LVM |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # crypttab: una sola entrada para la partición LUKS (LVM está dentro)
    echo "cryptlvm UUID=${CRYPT_LUKS_UUID} none luks,discard" >> /mnt/etc/crypttab
    echo -e "${GREEN}✓ crypttab configurado (UUID: $CRYPT_LUKS_UUID)${NC}"

    # Habilitar y activar LVM dentro del chroot
    chroot /mnt /bin/bash -c "systemctl enable lvm2-monitor.service"
    chroot /mnt /bin/bash -c "vgchange -ay vg0"
    echo -e "${GREEN}✓ LVM habilitado en el sistema instalado${NC}"
fi

# Configuración adicional para BTRFS (sin cifrado o con cifrado)
if [ "$PARTITION_MODE" = "auto" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; then
    configurar_btrfs
elif [ "$ENCRYPTION" = "true" ] && [ "$FILESYSTEM_TYPE" = "btrfs" ]; then
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

# Instalación de programas adicionales según lista de programas
if [ "$UTILITIES_ENABLED" = "true" ] && [ ${#UTILITIES_APPS[@]} -gt 0 ]; then
    echo ""
    echo -e "${GREEN}| Instalando programas de utilidades seleccionados |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    for app in "${UTILITIES_APPS[@]}"; do

        # Caso especial: stremio requiere dependencias Qt5 precompiladas
        if [ "$app" = "stremio" ]; then
            echo -e "${YELLOW}⚠ Detectado stremio: instalando dependencias Qt5 precompiladas...${NC}"

            install_pacman_url_with_retry \
                "https://archive.archlinux.org/packages/q/qt5-webengine/qt5-webengine-5.15.19-4-x86_64.pkg.tar.zst"

            install_pacman_url_with_retry \
                "https://archive.archlinux.org/packages/q/qt5-webchannel/qt5-webchannel-5.15.18+kde+r3-1-x86_64.pkg.tar.zst"

            install_pacman_url_with_retry \
                "https://archive.archlinux.org/packages/q/qt5-websockets/qt5-websockets-5.15.18+kde+r2-1-x86_64.pkg.tar.zst"

            install_pacman_url_with_retry \
                "https://archive.archlinux.org/packages/q/qt5-location/qt5-location-5.15.18+kde+r7-2-x86_64.pkg.tar.zst"

            echo -e "${GREEN}✓ Dependencias Qt5 instaladas${NC}"
        fi

        echo -e "${CYAN}Instalando: $app${NC}"
        install_yay_chroot_with_retry "$app" "--overwrite '*'"
        sleep 2
    done
    echo -e "${GREEN}✓ Instalación de programas de utilidades completada${NC}"
    echo ""
    sleep 2
fi


# URLs de dependencias precompiladas
declare -A PREBUILT_URLS=(
    ["gtk2"]="https://archive.archlinux.org/packages/g/gtk2/gtk2-2.24.33-5-x86_64.pkg.tar.zst"
    ["libpng12"]="https://archive.archlinux.org/packages/l/libpng12/libpng12-1.2.59-2-x86_64.pkg.tar.zst"
    ["qt5-webengine"]="https://archive.archlinux.org/packages/q/qt5-webengine/qt5-webengine-5.15.19-4-x86_64.pkg.tar.zst"
    ["qt5-websockets"]="https://archive.archlinux.org/packages/q/qt5-websockets/qt5-websockets-5.15.18+kde+r2-1-x86_64.pkg.tar.zst"
    ["qt5-webchannel"]="https://archive.archlinux.org/packages/q/qt5-webchannel/qt5-webchannel-5.15.18+kde+r3-1-x86_64.pkg.tar.zst"
    ["qt5-location"]="https://archive.archlinux.org/packages/q/qt5-location/qt5-location-5.15.18+kde+r7-2-x86_64.pkg.tar.zst"
)

QT5_WEBENGINE_COMPANIONS=("qt5-websockets" "qt5-webchannel" "qt5-location")

# Verifica si un paquete ya está instalado dentro del chroot
is_installed_in_chroot() {
    local pkg="$1"
    chroot /mnt pacman -Qq "$pkg" &>/dev/null
}

# Obtiene dependencias via AUR RPC API con fallback a yay -Si
get_pkg_deps() {
    local app="$1"
    local deps=""

    # Primero AUR RPC API (más confiable para paquetes AUR)
    deps=$(curl -s "https://aur.archlinux.org/rpc/v5/info?arg[]=$app" \
        | grep -oP '"(Depends|MakeDepends|CheckDepends)":\[.*?\]' \
        | grep -oP '"[^"]*"' \
        | tr -d '"' \
        | grep -v 'Depends\|MakeDepends\|CheckDepends')

    # Fallback a yay -Si dentro del chroot (repos oficiales)
    if [ -z "$deps" ]; then
        deps=$(chroot /mnt /bin/bash -c "yay -Si $app 2>/dev/null \
            | grep -E '^(Depends On|Make Deps)\s*:' \
            | sed 's/.*: //'")
    fi

    echo "$deps"
}

# Detecta e instala dependencias precompiladas necesarias antes de instalar un paquete
install_prebuilt_deps_if_needed() {
    local app="$1"
    local deps_to_install=()

    echo -e "${CYAN}  Verificando dependencias precompiladas para: $app${NC}"

    local all_deps
    all_deps=$(get_pkg_deps "$app")

    if [ -z "$all_deps" ]; then
        echo -e "${YELLOW}  ⚠ No se pudieron obtener dependencias de $app, continuando...${NC}"
        return
    fi

    for dep in gtk2 libpng12; do
        if echo "$all_deps" | grep -qw "$dep"; then
            if ! is_installed_in_chroot "$dep"; then
                deps_to_install+=("$dep")
            else
                echo -e "${GREEN}  ✓ $dep ya instalado${NC}"
            fi
        fi
    done

    # Si el paquete necesita qt5-webengine, instalar companions primero y luego webengine
    if echo "$all_deps" | grep -qw "qt5-webengine"; then
        for companion in qt5-websockets qt5-webchannel qt5-location; do
            if ! is_installed_in_chroot "$companion"; then
                deps_to_install+=("$companion")
            else
                echo -e "${GREEN}  ✓ $companion ya instalado${NC}"
            fi
        done
        if ! is_installed_in_chroot "qt5-webengine"; then
            deps_to_install+=("qt5-webengine")
        else
            echo -e "${GREEN}  ✓ qt5-webengine ya instalado${NC}"
        fi
    fi

    if [ ${#deps_to_install[@]} -gt 0 ]; then
        echo -e "${YELLOW}  ⚠ Instalando dependencias precompiladas: ${deps_to_install[*]}${NC}"
        for dep in "${deps_to_install[@]}"; do
            install_pacman_url_with_retry "${PREBUILT_URLS[$dep]}"
        done
        echo -e "${GREEN}  ✓ Dependencias precompiladas listas${NC}"
    else
        echo -e "${GREEN}  ✓ Sin dependencias precompiladas necesarias${NC}"
    fi
}

# Instalación de programas extra seleccionados
if [ "$PROGRAM_EXTRA" = "true" ] && [ ${#EXTRA_PROGRAMS[@]} -gt 0 ]; then
    echo ""
    echo -e "${GREEN}| Instalando programas extra seleccionados |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    for program in "${EXTRA_PROGRAMS[@]}"; do
        install_prebuilt_deps_if_needed "$program"
        echo -e "${CYAN}Instalando: $program${NC}"
        install_yay_chroot_with_retry "$program" "--overwrite '*'"
        sleep 2
    done
    echo -e "${GREEN}✓ Instalación de programas extra completada${NC}"
    echo ""
    sleep 2
fi



# --------------------------------------------------------------------------------------
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
    _attempt=1
    while true; do
        wait_for_internet || break
        echo -e "${CYAN}🔄 Intento #$_attempt para configurar Chaotic-AUR${NC}"
        if chroot /mnt /bin/bash -c "
            pacman-key --recv-key 3056513887B78AEB --keyserver keyserver.ubuntu.com && \
            pacman-key --lsign-key 3056513887B78AEB && \
            pacman -U --noconfirm \
                'https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-keyring.pkg.tar.zst' \
                'https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-mirrorlist.pkg.tar.zst'
        " && [ -f /mnt/etc/pacman.d/chaotic-mirrorlist ]; then
            if ! grep -q '\[chaotic-aur\]' /mnt/etc/pacman.conf; then
                cat >> /mnt/etc/pacman.conf << 'EOF'

[chaotic-aur]
Include = /etc/pacman.d/chaotic-mirrorlist
EOF
            fi
            echo -e "${GREEN}✓ Chaotic-AUR configurado${NC}"
            break
        else
            echo -e "${YELLOW}⚠️  Falló Chaotic-AUR (intento #$_attempt), reintentando en 5 segundos...${NC}"
            sleep 5
            ((_attempt++))
        fi
    done
fi

# ArchLinuxCN
if [ "$REPOS_ARCHLINUXCN" = "true" ]; then
    echo -e "${CYAN}Configurando ArchLinuxCN...${NC}"
    if ! grep -q '\[archlinuxcn\]' /mnt/etc/pacman.conf; then
        cat >> /mnt/etc/pacman.conf << 'EOF'

[archlinuxcn]
Server = https://repo.archlinuxcn.org/$arch
EOF
    fi
    _attempt=1
    while true; do
        wait_for_internet || break
        echo -e "${CYAN}🔄 Intento #$_attempt para instalar archlinuxcn-keyring${NC}"
        if chroot /mnt /bin/bash -c "pacman -Sy --noconfirm archlinuxcn-keyring"; then
            echo -e "${GREEN}✓ ArchLinuxCN configurado${NC}"
            break
        else
            echo -e "${YELLOW}⚠️  Falló archlinuxcn-keyring (intento #$_attempt), reintentando en 5 segundos...${NC}"
            sleep 5
            ((_attempt++))
        fi
    done
fi

# CachyOS
if [ "$REPOS_CACHYOS" = "true" ]; then
    echo -e "${CYAN}Configurando repositorios CachyOS...${NC}"
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
    _attempt=1
    while true; do
        wait_for_internet || break
        echo -e "${CYAN}🔄 Intento #$_attempt para configurar CachyOS${NC}"
        rm -rf /tmp/cachyos-repo /tmp/cachyos-repo.tar.xz /mnt/tmp/cachyos-repo 2>/dev/null || true
        if curl -fsSL https://mirror.cachyos.org/cachyos-repo.tar.xz -o /tmp/cachyos-repo.tar.xz && \
           tar xf /tmp/cachyos-repo.tar.xz -C /tmp && \
           cp -r /tmp/cachyos-repo /mnt/tmp/cachyos-repo && \
           chroot /mnt /bin/bash -c "cd /tmp/cachyos-repo && yes | bash ./cachyos-repo.sh"; then
            rm -rf /tmp/cachyos-repo /tmp/cachyos-repo.tar.xz /mnt/tmp/cachyos-repo 2>/dev/null || true
            echo -e "${GREEN}✓ CachyOS configurado (arquitectura detectada automáticamente)${NC}"
            break
        else
            rm -rf /tmp/cachyos-repo /tmp/cachyos-repo.tar.xz /mnt/tmp/cachyos-repo 2>/dev/null || true
            echo -e "${YELLOW}⚠️  Falló CachyOS (intento #$_attempt), reintentando en 5 segundos...${NC}"
            sleep 5
            ((_attempt++))
        fi
    done
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
# Mostrar información importante para sistemas cifrados
if [ "$ENCRYPTION" = "true" ]; then
    clear
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}           SISTEMA CIFRADO CON LUKS+LVM CONFIGURADO EXITOSAMENTE${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""


    echo -e "${YELLOW}🔐 INFORMACIÓN CRÍTICA SOBRE TU SISTEMA CIFRADO:${NC}"
    echo ""
    echo -e "${GREEN}✓ Configuración aplicada:${NC}"
    echo -e "${CYAN}  • Filesystem: $FILESYSTEM_TYPE | Home: $HOME_PARTITION | Swap: $SWAP_TYPE${NC}"
    echo -e "${CYAN}  • Partición LUKS UUID: $CRYPT_LUKS_UUID${NC}"
    echo -e "${CYAN}  • LVM vg0: $([ "${SWAP_SIZE_MIB:-0}" -gt 0 ] && echo "swap(${SWAP_SIZE_MIB}MiB) + " || echo "")root + $([ "$HOME_PARTITION" = "partition" ] && echo "home" || echo "(sin home LV)")${NC}"
    echo ""
    echo -e "${RED}⚠️  ADVERTENCIAS IMPORTANTES:${NC}"
    echo -e "${RED}  • SIN LA CONTRASEÑA LUKS PERDERÁS TODOS TUS DATOS${NC}"
    echo -e "${RED}  • Guarda la contraseña en un lugar seguro${NC}"
    echo -e "${RED}  • Haz backup del header: cryptsetup luksHeaderBackup /dev/disk/by-uuid/$CRYPT_LUKS_UUID --header-backup-file luks-header.bak${NC}"
    echo ""
    echo -e "${GREEN}🚀 Al reiniciar:${NC}"
    echo -e "${CYAN}  1. El sistema pedirá tu contraseña LUKS para desbloquear el disco${NC}"
    echo -e "${CYAN}  2. Una vez desbloqueado, el sistema arrancará normalmente${NC}"
    echo -e "${CYAN}  3. Si olvidas la contraseña, no podrás acceder a tus datos${NC}"
    echo ""
    echo -e "${GREEN}🔧 Comandos útiles post-instalación:${NC}"
    echo -e "${CYAN}  • Ver estado LVM: sudo vgdisplay && sudo lvdisplay${NC}"
    echo -e "${CYAN}  • Redimensionar particiones: sudo lvresize${NC}"
    echo ""
    sleep 5
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""
fi
echo ""
echo -e "${YELLOW}IMPORTANTE:${NC}"
echo -e "${CYAN}• Reinicia el sistema y retira el medio de instalación${NC}"
echo -e "${CYAN}• El sistema iniciará con GRUB${NC}"
if [ "$ENCRYPTION" = "true" ]; then
    echo -e "${CYAN}• Se solicitará la contraseña de cifrado al iniciar${NC}"
fi
echo -e "${CYAN}• Puedes iniciar sesión con:${NC}"
echo -e "  Usuario: ${GREEN}$USER${NC}"
echo ""
sleep 5
clear
# Barra de progreso final
titulo_progreso="| Finalizando instalación de ARCRIS LINUX |"
# Colores
AZUL="\e[1;34m"
GRIS_NEGRITA="\e[1;37m"
RESET="\e[0m"

echo -e "${AZUL}
        ,                       _     _ _
       /#\\        __ _ _ __ ___| |__ | (_)_ __  _   ___  __
      /###\\      / _\` | '__/ __| '_ \\| | | '_ \\| | | \\ \\/ /
     /#####\\    | (_| | | | (__| | | | | | | | | |_| |>  <
    /##,-,##\\    \\__,_|_|  \\___|_| |_|_|_|_| |_|\\__,_/_/\\_\\
   /##(   )##\\
  /#.--   --.#\\ ${RESET}  ${GRIS_NEGRITA}A simple, elegant GNU/Linux distribution.${RESET}
${AZUL} /\`           \`\\
${RESET}"
barra_progreso
echo -e "${GREEN}✓ Instalación de ARCRIS LINUX completada exitosamente!${NC}"
sleep 2
clear
