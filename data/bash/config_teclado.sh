################################################################################################
# #################### teclado##################################################################
################################################################################################

configurar_teclado() {
# Verificar que las variables necesarias estén definidas
if [[ -z "$KEYBOARD_LAYOUT" ]]; then
    echo -e "${RED}Error: KEYBOARD_LAYOUT no está definido${NC}"
    return 1
fi
if [[ -z "$KEYMAP_TTY" ]]; then
    echo -e "${RED}Error: KEYMAP_TTY no está definido${NC}"
    return 1
fi
if [[ -z "$USER" ]]; then
    echo -e "${RED}Error: USER no está definido${NC}"
    return 1
fi

# Configuración completa del layout de teclado para Xorg y Wayland
echo -e "${GREEN}| Configurando layout de teclado: $KEYBOARD_LAYOUT |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# 1. Configuración con localectl (método universal y permanente)
echo -e "${CYAN}1. Configurando con localectl (permanente para ambos Xorg y Wayland)...${NC}"
if chroot /mnt localectl set-keymap "$KEYBOARD_LAYOUT" 2>/dev/null; then
    echo -e "${GREEN}  ✓ Keymap configurado correctamente${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudo configurar keymap con localectl${NC}"
fi

if chroot /mnt localectl set-x11-keymap "$KEYBOARD_LAYOUT" pc105 "" "" 2>/dev/null; then
    echo -e "${GREEN}  ✓ X11 keymap configurado correctamente${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudo configurar X11 keymap con localectl${NC}"
fi

# También ejecutar como usuario para configuración por usuario
# echo -e "${CYAN}1.1. Configurando localectl como usuario...${NC}"
# chroot /mnt /bin/bash -c "sudo -u $USER localectl set-keymap $KEYBOARD_LAYOUT" || echo "Warning: No se pudo configurar keymap para usuario $USER"
# chroot /mnt /bin/bash -c "sudo -u $USER localectl set-x11-keymap $KEYBOARD_LAYOUT pc105 \"\" \"\"" || echo "Warning: No se pudo configurar X11 keymap para usuario $USER"

# 2. Configuración para Xorg (X11)
echo -e "${CYAN}2. Configurando teclado para Xorg (X11)...${NC}"
if mkdir -p /mnt/etc/X11/xorg.conf.d 2>/dev/null; then
    cat > /mnt/etc/X11/xorg.conf.d/00-keyboard.conf << EOF
Section "InputClass"
        Identifier "system-keyboard"
        MatchIsKeyboard "on"
        Option "XkbLayout" "$KEYBOARD_LAYOUT"
        Option "XkbModel" "pc105"
        Option "XkbVariant" ""
        Option "XkbOptions" "grp:alt_shift_toggle"
EndSection
EOF
    if [[ -f /mnt/etc/X11/xorg.conf.d/00-keyboard.conf ]]; then
        echo -e "${GREEN}  ✓ Configuración Xorg creada correctamente${NC}"
    else
        echo -e "${RED}  ✗ Error al crear configuración Xorg${NC}"
    fi
else
    echo -e "${RED}  ✗ Error al crear directorio Xorg${NC}"
fi

# 3. Configuración para Wayland
echo -e "${CYAN}3. Configurando teclado para Wayland...${NC}"
if mkdir -p /mnt/etc/xdg/wlroots 2>/dev/null; then
    cat > /mnt/etc/xdg/wlroots/wlr.conf << EOF
[keyboard]
layout=$KEYBOARD_LAYOUT
model=pc105
variant=
options=grp:alt_shift_toggle

[input]
kb_layout=$KEYBOARD_LAYOUT
kb_model=pc105
kb_variant=
kb_options=grp:alt_shift_toggle
EOF
    if [[ -f /mnt/etc/xdg/wlroots/wlr.conf ]]; then
        echo -e "${GREEN}  ✓ Configuración Wayland creada correctamente${NC}"
    else
        echo -e "${RED}  ✗ Error al crear configuración Wayland${NC}"
    fi
else
    echo -e "${RED}  ✗ Error al crear directorio Wayland${NC}"
fi

# 4. Configuración persistente del archivo /etc/default/keyboard
echo -e "${CYAN}4. Configurando archivo /etc/default/keyboard...${NC}"
cat > /mnt/etc/default/keyboard << EOF
XKBMODEL="pc105"
XKBLAYOUT="$KEYBOARD_LAYOUT"
XKBVARIANT=""
XKBOPTIONS="grp:alt_shift_toggle"
EOF
if [[ -f /mnt/etc/default/keyboard ]]; then
    echo -e "${GREEN}  ✓ Archivo /etc/default/keyboard creado correctamente${NC}"
else
    echo -e "${RED}  ✗ Error al crear /etc/default/keyboard${NC}"
fi

# 5. Configuración de la consola virtual (vconsole.conf)
echo -e "${CYAN}5. Configurando consola virtual...${NC}"
{
    echo "KEYMAP=$KEYMAP_TTY"
    echo "FONT=lat0-16"
} > /mnt/etc/vconsole.conf
if [[ -f /mnt/etc/vconsole.conf ]]; then
    echo -e "${GREEN}  ✓ Configuración vconsole creada correctamente${NC}"
else
    echo -e "${RED}  ✗ Error al crear vconsole.conf${NC}"
fi

# 6. Configuración para GNOME (si se usa)
echo -e "${CYAN}6. Configurando para GNOME...${NC}"
if mkdir -p /mnt/etc/dconf/db/local.d 2>/dev/null; then
    cat > /mnt/etc/dconf/db/local.d/00-keyboard << EOF
[org/gnome/desktop/input-sources]
sources=[('xkb', '$KEYBOARD_LAYOUT')]
EOF
    if [[ -f /mnt/etc/dconf/db/local.d/00-keyboard ]]; then
        echo -e "${GREEN}  ✓ Configuración GNOME creada correctamente${NC}"
    else
        echo -e "${RED}  ✗ Error al crear configuración GNOME${NC}"
    fi
else
    echo -e "${RED}  ✗ Error al crear directorio dconf${NC}"
fi

# 7. Configuración adicional para el usuario
echo -e "${CYAN}7. Configurando variables de entorno para el usuario...${NC}"
# Verificar que el directorio home del usuario exista
if [[ ! -d "/mnt/home/$USER" ]]; then
    echo -e "${YELLOW}  ⚠ Creando directorio home para usuario $USER${NC}"
    mkdir -p "/mnt/home/$USER"
    chroot /mnt chown "$USER:$USER" "/home/$USER" 2>/dev/null || true
fi

cat >> /mnt/home/$USER/.profile << EOF

# Configuración de teclado
export XKB_DEFAULT_LAYOUT=$KEYBOARD_LAYOUT
export XKB_DEFAULT_MODEL=pc105
export XKB_DEFAULT_OPTIONS=grp:alt_shift_toggle
EOF
if [[ -f /mnt/home/$USER/.profile ]]; then
    echo -e "${GREEN}  ✓ Variables de entorno añadidas a .profile${NC}"
else
    echo -e "${RED}  ✗ Error al modificar .profile${NC}"
fi

# 8. Script de configuración automática para el arranque
# 8. Script de configuración automática mejorado
echo -e "${CYAN}8. Creando script de configuración universal de teclado...${NC}"
if mkdir -p /mnt/usr/local/bin 2>/dev/null; then
    cat > /mnt/usr/local/bin/setup-keyboard.sh << EOF
#!/bin/bash
# Script de configuración universal del teclado
# Compatible con X11 y múltiples compositores Wayland

KEYBOARD_LAYOUT="$KEYBOARD_LAYOUT"

# Configurar variables de entorno XKB (universales para Wayland)
export XKB_DEFAULT_LAYOUT="\$KEYBOARD_LAYOUT"
export XKB_DEFAULT_OPTIONS="grp:alt_shift_toggle"

# Importar variables al entorno de usuario systemd
if command -v systemctl >/dev/null 2>&1; then
    systemctl --user import-environment XKB_DEFAULT_LAYOUT XKB_DEFAULT_OPTIONS 2>/dev/null
fi

# Para X11: usar setxkbmap si está disponible
if [ -n "\$DISPLAY" ] && command -v setxkbmap >/dev/null 2>&1; then
    setxkbmap "\$KEYBOARD_LAYOUT" -option grp:alt_shift_toggle 2>/dev/null
fi

# Para Wayland: las variables XKB_DEFAULT_* serán leídas por el compositor
# Funciona con: KDE/kwin_wayland, GNOME/mutter, wlroots-based compositors, etc.
EOF

    if chmod +x /mnt/usr/local/bin/setup-keyboard.sh 2>/dev/null; then
        echo -e "${GREEN}  ✓ Script setup-keyboard.sh universal creado correctamente${NC}"
    else
        echo -e "${RED}  ✗ Error al crear script setup-keyboard.sh${NC}"
    fi
else
    echo -e "${RED}  ✗ Error al crear directorio /usr/local/bin${NC}"
fi

# 9. Configuración para autostart universal
echo -e "${CYAN}9. Configurando autostart universal...${NC}"
if mkdir -p /mnt/etc/xdg/autostart 2>/dev/null; then
    cat > /mnt/etc/xdg/autostart/keyboard-setup.desktop << EOF
[Desktop Entry]
Type=Application
Name=Universal Keyboard Layout Setup
Exec=/usr/local/bin/setup-keyboard.sh
Hidden=false
NoDisplay=true
StartupNotify=false
EOF
    if [[ -f /mnt/etc/xdg/autostart/keyboard-setup.desktop ]]; then
        echo -e "${GREEN}  ✓ Autostart desktop file universal creado correctamente${NC}"
    else
        echo -e "${RED}  ✗ Error al crear autostart desktop file${NC}"
    fi
else
    echo -e "${RED}  ✗ Error al crear directorio autostart${NC}"
fi

# 10. Establecer permisos correctos
echo -e "${CYAN}10. Estableciendo permisos correctos...${NC}"
if chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER" 2>/dev/null; then
    echo -e "${GREEN}  ✓ Permisos del directorio home establecidos${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudieron establecer permisos del home${NC}"
fi

if chroot /mnt chmod 755 /usr/local/bin/setup-keyboard.sh 2>/dev/null; then
    echo -e "${GREEN}  ✓ Permisos del script establecidos${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudieron establecer permisos del script${NC}"
fi

if chroot /mnt chmod 644 /etc/xdg/autostart/keyboard-setup.desktop 2>/dev/null; then
    echo -e "${GREEN}  ✓ Permisos del desktop file establecidos${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudieron establecer permisos del desktop file${NC}"
fi

# 11. Actualizar base de datos dconf si existe
echo -e "${CYAN}11. Actualizando configuraciones del sistema...${NC}"
if chroot /mnt dconf update 2>/dev/null; then
    echo -e "${GREEN}  ✓ Base de datos dconf actualizada${NC}"
else
    echo -e "${YELLOW}  ⚠ Warning: No se pudo actualizar dconf (normal si no está instalado)${NC}"
fi

echo ""
echo -e "${GREEN}✓ Configuración completa del teclado finalizada${NC}"
echo -e "${CYAN}  • Layout: $KEYBOARD_LAYOUT${NC}"
echo -e "${CYAN}  • Keymap TTY: $KEYMAP_TTY${NC}"
echo -e "${CYAN}  • Modelo: pc105${NC}"
echo -e "${CYAN}  • Cambio de layout: Alt+Shift${NC}"
echo -e "${CYAN}  • Métodos configurados: localectl, Xorg, Wayland, GNOME, vconsole${NC}"
echo -e "${YELLOW}  • La configuración será efectiva después del reinicio${NC}"

sleep 4
clear
}
