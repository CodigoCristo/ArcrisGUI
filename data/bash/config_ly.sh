# Configurar Ly para reconocer los window managers
echo -e "${CYAN}Configurando Ly display manager...${NC}"
mkdir -p /mnt/usr/share/xsessions

# Crear archivos .desktop para cada window manager
case "$WINDOW_MANAGER" in
    "I3WM"|"I3")
        cat > /mnt/usr/share/xsessions/i3.desktop << EOF
[Desktop Entry]
Name=i3
Comment=improved dynamic tiling window manager
Exec=i3
TryExec=i3
Type=Application
X-LightDM-DesktopName=i3
DesktopNames=i3
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
    "AWESOME")
        cat > /mnt/usr/share/xsessions/awesome.desktop << EOF
[Desktop Entry]
Name=awesome
Comment=Highly configurable framework window manager
Exec=awesome
TryExec=awesome
Type=Application
X-LightDM-DesktopName=awesome
DesktopNames=awesome
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
    "BSPWM")
        cat > /mnt/usr/share/xsessions/bspwm.desktop << EOF
[Desktop Entry]
Name=bspwm
Comment=Binary space partitioning window manager
Exec=bspwm
TryExec=bspwm
Type=Application
X-LightDM-DesktopName=bspwm
DesktopNames=bspwm
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
    "DWM")
        cat > /mnt/usr/share/xsessions/dwm.desktop << EOF
[Desktop Entry]
Name=dwm
Comment=Dynamic window manager
Exec=dwm
TryExec=dwm
Type=Application
X-LightDM-DesktopName=dwm
DesktopNames=dwm
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
    "HYPRLAND")
        cat > /mnt/usr/share/wayland-sessions/hyprland.desktop << EOF
[Desktop Entry]
Name=Hyprland
Comment=An intelligent dynamic tiling Wayland compositor
Exec=Hyprland
Type=Application
EOF
        ;;
    "OPENBOX")
        cat > /mnt/usr/share/xsessions/openbox.desktop << EOF
[Desktop Entry]
Name=Openbox
Comment=A highly configurable, next generation window manager
Exec=openbox-session
TryExec=openbox
Type=Application
X-LightDM-DesktopName=Openbox
DesktopNames=Openbox
Keywords=wm;windowmanager;window;manager;
EOF
        ;;
    "QTITLE"|"QTILE")
        cat > /mnt/usr/share/xsessions/qtile.desktop << EOF
[Desktop Entry]
Name=Qtile
Comment=A full-featured, hackable tiling window manager written in Python
Exec=qtile start
TryExec=qtile
Type=Application
X-LightDM-DesktopName=Qtile
DesktopNames=Qtile
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
    "SWAY")
        cat > /mnt/usr/share/wayland-sessions/sway.desktop << EOF
[Desktop Entry]
Name=Sway
Comment=An i3-compatible Wayland compositor
Exec=sway
Type=Application
EOF
        ;;
    "XMONAD")
        cat > /mnt/usr/share/xsessions/xmonad.desktop << EOF
[Desktop Entry]
Name=XMonad
Comment=Lightweight tiling window manager
Exec=xmonad
TryExec=xmonad
Type=Application
X-LightDM-DesktopName=XMonad
DesktopNames=XMonad
Keywords=tiling;wm;windowmanager;window;manager;
EOF
        ;;
esac
;;
*)
echo -e "${YELLOW}Tipo de instalación no reconocido: $INSTALLATION_TYPE${NC}"
echo -e "${CYAN}Continuando sin instalación de entorno gráfico...${NC}"
;;
esac
sleep 3
clear
