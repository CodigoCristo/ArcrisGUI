#!/bin/bash
# Variables de configuración generadas por Arcris
# Archivo generado automáticamente - No editar manualmente

KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="America/Lima"
LOCALE="es_PE.UTF-8"
ENCRYPTION_ENABLED="true"
ENCRYPTION_PASSWORD="123e"
SELECTED_DISK="/dev/nvme0n1"
PARTITION_MODE="manual"
PARTITIONS=(
    "/dev/nvme0n1p2 mkfs.ext4 /"
    "/dev/nvme0n1p1 mkfs.ext4 /boot"
    "/dev/nvme0n1p3 mkswap swap"
    "/dev/nvme0n1p4 mkfs.ext4 /home"
)

# Variables de configuración del usuario
export USER="qwe"
export PASSWORD_USER="123"
export HOSTNAME="arcris"
# La contraseña del usuario también será la contraseña de root
export PASSWORD_ROOT="123"
