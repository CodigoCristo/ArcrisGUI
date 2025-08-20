#!/bin/bash
# Archivo de variables de configuración de Arcris
# Generado automáticamente - No editar manualmente

# Configuración regional
KEYBOARD_LAYOUT="es"
KEYMAP_TTY="es"
TIMEZONE="America/Lima"
LOCALE="es_PE.UTF-8"

# Configuración de particionado
PARTITION_MODE="auto"  # auto, auto_btrfs, cifrado, manual
SELECTED_DISK="/dev/sda"

# Array de configuraciones de particiones (solo para modo manual)
PARTITIONS=(
    "/dev/sda1 mkfs.fat32 /boot"
    "/dev/sda2 mkfs.ext4 /"
    "/dev/sda3 mkfs.ext4 /home"
)

# Configuración de cifrado
ENCRYPTION_ENABLED="false"
ENCRYPTION_PASSWORD="123"

# Configuración de usuario
USER="arcris"
PASSWORD_USER="123456"
HOSTNAME="arcris-pc"
PASSWORD_ROOT="123456"

# Kernel seleccionado
SELECTED_KERNEL="linux"  # linux, linux-hardened, linux-lts, linux-rt-lts, linux-zen

# Drivers de video
DRIVER_VIDEO="Open Source"  # Open Source, Nvidia Private, AMD Private, Intel Private, Máquina Virtual

# Driver de audio
DRIVER_AUDIO="Alsa Audio"  # Alsa Audio, pipewire, pulseaudio, Jack2

# Driver de WiFi
DRIVER_WIFI="Ninguno"  # Ninguno, Open Source, broadcom-wl, Realtek

# Driver de Bluetooth
DRIVER_BLUETOOTH="Ninguno"  # Ninguno, bluetoothctl (terminal), blueman (Graphical)
