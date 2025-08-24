#!/bin/bash
# Variables de configuración generadas por Arcris
# Archivo generado automáticamente - No editar manualmente

KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="America/Lima"
LOCALE="es_PE.UTF-8"

# Variables de configuración del usuario
export USER="qwe"
export PASSWORD_USER="133"
export HOSTNAME="arcris"
# La contraseña del usuario también será la contraseña de root
export PASSWORD_ROOT="133"

# Tipo de instalación seleccionado
INSTALLATION_TYPE="TERMINAL"

# Kernel seleccionado
SELECTED_KERNEL="linux"

# Driver de Video
DRIVER_VIDEO="nvidia-dkms"

# Driver de Audio
DRIVER_AUDIO="Alsa Audio"

# Driver de WiFi
DRIVER_WIFI="Ninguno"

# Driver de Bluetooth
DRIVER_BLUETOOTH="Ninguno"

# Configuración de aplicaciones - Página 6
ESSENTIAL_APPS_ENABLED="false"
UTILITIES_ENABLED="false"
PROGRAM_EXTRA="false"
SELECTED_DISK="/dev/sdc"
PARTITION_MODE="manual"
PARTITIONS=(
    "/dev/sdc1 mkfs.fat32 /boot/EFI"
    "/dev/sdc2 mkswap swap"
    "/dev/sdc3 none /"
)
