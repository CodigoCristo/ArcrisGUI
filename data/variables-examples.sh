#!/bin/bash
# Archivo de ejemplos de configuración para Arcris Linux
# Copia y modifica las variables según tu configuración deseada

# =====================================================================
# EJEMPLO 1: INSTALACIÓN AUTOMÁTICA BÁSICA (EXT4)
# =====================================================================
# Configuración más simple para usuarios principiantes

# Configuración regional
KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="America/Lima"
LOCALE="es_PE.UTF-8"

# Configuración de particionado
PARTITION_MODE="auto"  # Particionado automático con EXT4
SELECTED_DISK="/dev/sda"  # Cambia por tu disco

# Configuración de cifrado (no usado en modo auto)
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD=""

# Configuración de usuario
USER="usuario"
PASSWORD_USER="micontraseña123"
HOSTNAME="mi-pc"
PASSWORD_ROOT="rootpassword123"

# Kernel seleccionado
SELECTED_KERNEL="linux"  # Kernel estándar

# Drivers básicos
DRIVER_VIDEO="Open Source"
DRIVER_AUDIO="Alsa Audio"
DRIVER_WIFI="Open Source"
DRIVER_BLUETOOTH="Ninguno"

# =====================================================================
# EJEMPLO 2: INSTALACIÓN AUTOMÁTICA CON BTRFS
# =====================================================================
# Para usuarios que quieren aprovechar las ventajas de BTRFS

# Configuración regional
KEYBOARD_LAYOUT="us"
KEYMAP_TTY="us"
TIMEZONE="Europe/Madrid"
LOCALE="es_ES.UTF-8"

# Configuración de particionado
PARTITION_MODE="auto_btrfs"  # Particionado automático con BTRFS
SELECTED_DISK="/dev/nvme0n1"

# Configuración de cifrado (no usado en modo auto_btrfs)
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD=""

# Configuración de usuario
USER="desarrollador"
PASSWORD_USER="dev123456"
HOSTNAME="workstation-dev"
PASSWORD_ROOT="root123456"

# Kernel optimizado
SELECTED_KERNEL="linux-zen"  # Kernel optimizado para rendimiento

# Drivers para desarrollo
DRIVER_VIDEO="Open Source"
DRIVER_AUDIO="pipewire"  # Audio moderno
DRIVER_WIFI="Open Source"
DRIVER_BLUETOOTH="blueman (Graphical)"

# =====================================================================
# EJEMPLO 3: INSTALACIÓN CON CIFRADO COMPLETO (LUKS + LVM)
# =====================================================================
# Para usuarios que requieren máxima seguridad

# Configuración regional
KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="America/Mexico_City"
LOCALE="es_MX.UTF-8"

# Configuración de particionado
PARTITION_MODE="cifrado"  # Particionado con cifrado LUKS
SELECTED_DISK="/dev/sdb"

# Configuración de cifrado ¡IMPORTANTE!
ENCRYPTION_ENABLED="true"
ENCRYPTION_PASSWORD="MiClaveSupersegura2024!"  # Usa una clave fuerte

# Configuración de usuario
USER="usuario_seguro"
PASSWORD_USER="password_muy_seguro_123"
HOSTNAME="pc-seguro"
PASSWORD_ROOT="root_password_seguro_456"

# Kernel endurecido para seguridad
SELECTED_KERNEL="linux-hardened"

# Drivers con foco en seguridad
DRIVER_VIDEO="Open Source"
DRIVER_AUDIO="Alsa Audio"
DRIVER_WIFI="Open Source"
DRIVER_BLUETOOTH="Ninguno"  # Bluetooth deshabilitado por seguridad

# =====================================================================
# EJEMPLO 4: INSTALACIÓN MANUAL CON PARTICIONES PERSONALIZADAS
# =====================================================================
# Para usuarios avanzados que quieren control total

# Configuración regional
KEYBOARD_LAYOUT="us"
KEYMAP_TTY="us"
TIMEZONE="America/New_York"
LOCALE="en_US.UTF-8"

# Configuración de particionado
PARTITION_MODE="manual"  # Particionado manual
SELECTED_DISK="/dev/nvme0n1"  # Solo informativo en modo manual

# Array de configuraciones de particiones
# Formato: "dispositivo formato punto_de_montaje"
PARTITIONS=(
    "/dev/nvme0n1p1 mkfs.fat32 /boot"     # Partición EFI/Boot
    "/dev/nvme0n1p2 mkswap swap"          # Partición swap
    "/dev/nvme0n1p3 mkfs.ext4 /"          # Partición root
    "/dev/nvme0n1p4 mkfs.ext4 /home"      # Partición home separada
    "/dev/nvme0n1p5 mkfs.ext4 /var"       # Partición var separada
)

# Configuración de cifrado (no usado en modo manual)
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD=""

# Configuración de usuario
USER="admin"
PASSWORD_USER="admin123"
HOSTNAME="servidor-custom"
PASSWORD_ROOT="rootadmin123"

# Kernel LTS para estabilidad
SELECTED_KERNEL="linux-lts"

# Drivers para servidor
DRIVER_VIDEO="Open Source"
DRIVER_AUDIO="Alsa Audio"
DRIVER_WIFI="Ninguno"  # Sin WiFi en servidor
DRIVER_BLUETOOTH="Ninguno"

# =====================================================================
# EJEMPLO 5: CONFIGURACIÓN PARA GAMING
# =====================================================================
# Optimizada para juegos con drivers privativos

# Configuración regional
KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="Europe/Madrid"
LOCALE="es_ES.UTF-8"

# Configuración de particionado
PARTITION_MODE="auto"
SELECTED_DISK="/dev/nvme0n1"

# Sin cifrado para mejor rendimiento
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD=""

# Configuración de usuario
USER="gamer"
PASSWORD_USER="gaming123"
HOSTNAME="gaming-rig"
PASSWORD_ROOT="root123"

# Kernel optimizado para gaming
SELECTED_KERNEL="linux-zen"

# Drivers para gaming
DRIVER_VIDEO="Nvidia Private"  # O "AMD Private" según tu GPU
DRIVER_AUDIO="pipewire"        # Mejor para gaming
DRIVER_WIFI="Open Source"
DRIVER_BLUETOOTH="blueman (Graphical)"  # Para controles inalámbricos

# =====================================================================
# EJEMPLO 6: CONFIGURACIÓN PARA MÁQUINA VIRTUAL
# =====================================================================
# Optimizada para entornos virtualizados

# Configuración regional
KEYBOARD_LAYOUT="us"
KEYMAP_TTY="us"
TIMEZONE="UTC"
LOCALE="en_US.UTF-8"

# Configuración de particionado
PARTITION_MODE="auto"
SELECTED_DISK="/dev/vda"  # Disco típico en VMs

# Sin cifrado en VM de prueba
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD=""

# Configuración de usuario
USER="testuser"
PASSWORD_USER="test123"
HOSTNAME="test-vm"
PASSWORD_ROOT="root123"

# Kernel estándar
SELECTED_KERNEL="linux"

# Drivers para VM
DRIVER_VIDEO="Máquina Virtual"
DRIVER_AUDIO="Alsa Audio"
DRIVER_WIFI="Ninguno"  # Las VMs suelen usar red cableada
DRIVER_BLUETOOTH="Ninguno"

# =====================================================================
# NOTAS IMPORTANTES:
# =====================================================================

# 1. PARTITION_MODE opciones:
#    - "auto": Particionado automático con EXT4
#    - "auto_btrfs": Particionado automático con BTRFS y subvolúmenes
#    - "cifrado": Particionado con cifrado LUKS + LVM
#    - "manual": Particionado manual usando array PARTITIONS

# 2. SELECTED_KERNEL opciones:
#    - "linux": Kernel estándar (recomendado para principiantes)
#    - "linux-hardened": Kernel con mejoras de seguridad
#    - "linux-lts": Kernel LTS (más estable, para servidores)
#    - "linux-rt-lts": Kernel Real-Time LTS (para audio profesional)
#    - "linux-zen": Kernel optimizado para rendimiento (gaming/desktop)

# 3. DRIVER_VIDEO opciones:
#    - "Open Source": Drivers libres (funciona con cualquier GPU)
#    - "Nvidia Private": Drivers privativos de NVIDIA
#    - "AMD Private": Drivers privativos de AMD
#    - "Intel Private": Drivers privativos de Intel
#    - "Máquina Virtual": Para entornos virtualizados

# 4. DRIVER_AUDIO opciones:
#    - "Alsa Audio": Sistema de audio básico
#    - "pipewire": Sistema de audio moderno (recomendado)
#    - "pulseaudio": Servidor de audio tradicional
#    - "Jack2": Para audio profesional

# 5. DRIVER_WIFI opciones:
#    - "Ninguno": Sin drivers de WiFi
#    - "Open Source": Drivers libres (mayoría de tarjetas)
#    - "broadcom-wl": Para tarjetas Broadcom
#    - "Realtek": Para tarjetas Realtek específicas

# 6. DRIVER_BLUETOOTH opciones:
#    - "Ninguno": Sin soporte Bluetooth
#    - "bluetoothctl (terminal)": Interfaz de línea de comandos
#    - "blueman (Graphical)": Interfaz gráfica

# 7. Para modo manual (PARTITIONS array):
#    - Formato: "dispositivo comando_formateo punto_de_montaje"
#    - Comandos válidos: mkfs.fat32, mkfs.ext4, mkfs.btrfs, mkswap
#    - Para swap no especificar punto de montaje
#    - El punto de montaje "/" es obligatorio
#    - Crear las particiones ANTES de ejecutar el script

# 8. Seguridad:
#    - Usa contraseñas fuertes
#    - Para cifrado, usa una clave de al menos 12 caracteres
#    - Considera usar linux-hardened para mayor seguridad
#    - Deshabilita servicios innecesarios (WiFi/Bluetooth si no los usas)

# 9. Discos comunes:
#    - SATA: /dev/sda, /dev/sdb, etc.
#    - NVMe: /dev/nvme0n1, /dev/nvme1n1, etc.
#    - VirtIO (VMs): /dev/vda, /dev/vdb, etc.
#    - Usa 'lsblk' para listar discos disponibles

# =====================================================================
# PARA USAR ESTOS EJEMPLOS:
# =====================================================================
# 1. Copia el ejemplo que más se ajuste a tu caso
# 2. Modifica las variables según tu configuración
# 3. Guarda como variables.sh en el mismo directorio que install.sh
# 4. Ejecuta install.sh como root
