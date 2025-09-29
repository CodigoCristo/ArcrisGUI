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
    cat > "$XMONAD_DIR/xmonad.hs" << 'EOF'
-- xmonad.hs
-- xmonad example config file.
--
-- A template showing all available configuration hooks,
-- and how to override the defaults in your own xmonad.hs conf file.
--
-- Normally, you'd only override those defaults you care about.
--

import XMonad
import Data.Monoid
import System.Exit
import Graphics.X11.ExtraTypes.XF86
import XMonad.Hooks.DynamicLog


import qualified XMonad.StackSet as W
import qualified Data.Map        as M

-- The preferred terminal program, which is used in a binding below and by
-- certain contrib modules.
--
myTerminal      = "alacritty"

-- Whether focus follows the mouse pointer.
myFocusFollowsMouse :: Bool
myFocusFollowsMouse = True

-- Whether clicking on a window to focus also passes the click to the window
myClickJustFocuses :: Bool
myClickJustFocuses = False

-- Width of the window border in pixels.
--
myBorderWidth   = 1

-- modMask lets you specify which modkey you want to use. The default
-- is mod1Mask ("left alt").  You may also consider using mod3Mask
-- ("right alt"), which does not conflict with emacs keybindings. The
-- "windows key" is usually mod4Mask.
--
myModMask       = mod4Mask

-- The default number of workspaces (virtual screens) and their names.
-- By default we use numeric strings, but any string may be used as a
-- workspace name. The number of workspaces is determined by the length
-- of this list.
--
-- A tagging example:
--
-- > workspaces = ["web", "irc", "code" ] ++ map show [4..9]
--
myWorkspaces    = ["1","2","3","4","5","6","7","8","9"]

-- Border colors for unfocused and focused windows, respectively.
--
myNormalBorderColor  = "#dddddd"
myFocusedBorderColor = "#ff0000"

------------------------------------------------------------------------
-- Key bindings. Add, modify or remove key bindings here.
--
myKeys conf@(XConfig {XMonad.modMask = modm}) = M.fromList $

    -- launch a terminal
    [ ((modm .|. shiftMask, xK_Return), spawn $ XMonad.terminal conf)

    -- volume keys
    , ((0, xF86XK_AudioMute), spawn "pactl set-sink-mute @DEFAULT_SINK@ toggle")
    , ((0, xF86XK_AudioLowerVolume), spawn "pactl set-sink-volume @DEFAULT_SINK@ -10%")
    , ((0, xF86XK_AudioRaiseVolume), spawn "pactl set-sink-volume @DEFAULT_SINK@ +10%")

    -- launch dmenu
    , ((modm,               xK_p     ), spawn "dmenu_run")

    -- launch gmrun
    , ((modm .|. shiftMask, xK_p     ), spawn "gmrun")

    -- close focused window
    , ((modm .|. shiftMask, xK_c     ), kill)

        -- Rotate through the available layout algorithms
    , ((modm,               xK_space ), sendMessage NextLayout)

    --  Reset the layouts on the current workspace to default
    , ((modm .|. shiftMask, xK_space ), setLayout $ XMonad.layoutHook conf)

    -- Resize viewed windows to the correct size
    , ((modm,               xK_n     ), refresh)

    -- Move focus to the next window
    , ((modm,               xK_Tab   ), windows W.focusDown)

    -- Move focus to the next window
    , ((modm,               xK_j     ), windows W.focusDown)

    -- Move focus to the previous window
    , ((modm,               xK_k     ), windows W.focusUp  )

    -- Move focus to the master window
    , ((modm,               xK_m     ), windows W.focusMaster  )

    -- Swap the focused window and the master window
    , ((modm,               xK_Return), windows W.swapMaster)

    -- Swap the focused window with the next window
    , ((modm .|. shiftMask, xK_j     ), windows W.swapDown  )

    -- Swap the focused window with the previous window
    , ((modm .|. shiftMask, xK_k     ), windows W.swapUp    )

    -- Shrink the master area
    , ((modm,               xK_h     ), sendMessage Shrink)

    -- Expand the master area
    , ((modm,               xK_l     ), sendMessage Expand)

    -- Push window back into tiling
    , ((modm,               xK_t     ), withFocused $ windows . W.sink)

    -- Increment the number of windows in the master area
    , ((modm              , xK_comma ), sendMessage (IncMasterN 1))

    -- Deincrement the number of windows in the master area
    , ((modm              , xK_period), sendMessage (IncMasterN (-1)))

    -- Toggle the status bar gap
    -- Use this binding with avoidStruts from Hooks.ManageDocks.
    -- See also the statusBar function from Hooks.DynamicLog.
    --
    -- , ((modm              , xK_b     ), sendMessage ToggleStruts)

    -- Quit xmonad
    , ((modm .|. shiftMask, xK_q     ), io (exitWith ExitSuccess))

    -- Restart xmonad
    , ((modm              , xK_q     ), spawn "xmonad --recompile; xmonad --restart")

    -- Run xmessage with a summary of the default keybindings (useful for beginners)
    , ((modm .|. shiftMask, xK_slash ), spawn ("echo \"" ++ help ++ "\" | xmessage -file -"))
    ]
    ++

    --
    -- mod-[1..9], Switch to workspace N
    -- mod-shift-[1..9], Move client to workspace N
    --
    [((m .|. modm, k), windows $ f i)
        | (i, k) <- zip (XMonad.workspaces conf) [xK_1 .. xK_9]
        , (f, m) <- [(W.greedyView, 0), (W.shift, shiftMask)]]
    ++

    --
    -- mod-{w,e,r}, Switch to physical/Xinerama screens 1, 2, or 3
    -- mod-shift-{w,e,r}, Move client to screen 1, 2, or 3
    --
    [((m .|. modm, key), screenWorkspace sc >>= flip whenJust (windows . f))
        | (key, sc) <- zip [xK_w, xK_e, xK_r] [0..]
        , (f, m) <- [(W.view, 0), (W.shift, shiftMask)]]


------------------------------------------------------------------------
-- Mouse bindings: default actions bound to mouse events
--
myMouseBindings (XConfig {XMonad.modMask = modm}) = M.fromList $

    -- mod-button1, Set the window to floating mode and move by dragging
    [ ((modm, button1), (\w -> focus w >> mouseMoveWindow w
                                        >> windows W.shiftMaster))

    -- mod-button2, Raise the window to the top of the stack
    , ((modm, button2), (\w -> focus w >> windows W.shiftMaster))

    -- mod-button3, Set the window to floating mode and resize by dragging
    , ((modm, button3), (\w -> focus w >> mouseResizeWindow w
                                        >> windows W.shiftMaster))

    -- you may also bind events to the mouse scroll wheel (button4 and button5)
    ]

------------------------------------------------------------------------
-- Layouts:

-- You can specify and transform your layouts by modifying these values.
-- If you change layout bindings be sure to use 'mod-shift-space' after
-- restarting (with 'mod-q') to reset your layout state to the new
-- defaults, as xmonad preserves your old layout settings by default.
--
-- The available layouts.  Note that each layout is separated by |||,
-- which denotes layout choice.
--
myLayout = tiled ||| Mirror tiled ||| Full
    where
        -- default tiling algorithm partitions the screen into two panes
        tiled   = Tall nmaster delta ratio

        -- The default number of windows in the master pane
        nmaster = 1

        -- Default proportion of screen occupied by master pane
        ratio   = 1/2

        -- Percent of screen to increment by when resizing panes
        delta   = 3/100

------------------------------------------------------------------------
-- Window rules:

-- Execute arbitrary actions and WindowSet manipulations when managing
-- a new window. You can use this to, for example, always float a
-- particular program, or have a client always appear on a particular
-- workspace.
--
-- To find the property name associated with a program, use
-- > xprop | grep WM_CLASS
-- and click on the client you're interested in.
--
-- To match on the WM_NAME, you can use 'title' in the same way that
-- 'className' and 'resource' are used below.
--
myManageHook = composeAll
    [ className =? "MPlayer"        --> doFloat
    , className =? "Gimp"           --> doFloat
    , resource  =? "desktop_window" --> doIgnore
    , resource  =? "kdesktop"       --> doIgnore ]

------------------------------------------------------------------------
-- Event handling

-- * EwmhDesktops users should change this to ewmhDesktopsEventHook
--
-- Defines a custom handler function for X Events. The function should
-- return (All True) if the default handler is to be run afterwards. To
-- combine event hooks use mappend or mconcat from Data.Monoid.
--
myEventHook = mempty

------------------------------------------------------------------------
-- Status bars and logging

-- Perform an arbitrary action on each internal state change or X event.
-- See the 'XMonad.Hooks.DynamicLog' extension for examples.
--
myLogHook = return ()

------------------------------------------------------------------------
-- Startup hook

-- Perform an arbitrary action each time xmonad starts or is restarted
-- with mod-q.  Used by, e.g., XMonad.Layout.PerWorkspace to initialize
-- per-workspace layout choices.
--
-- By default, do nothing.
myStartupHook = return ()

------------------------------------------------------------------------
-- Command to launch the bar.
myBar = "xmobar"

-- Custom PP, configure it as you like. It determines what is being written to the bar.
myPP = xmobarPP { ppCurrent = xmobarColor "#2986cc" "" . wrap "[" "]"
                , ppTitle   = xmobarColor "#2986cc" "" . shorten 60
                }

-- Key binding to toggle the gap for the bar.
toggleStrutsKey XConfig {XMonad.modMask = modMask} = (modMask, xK_b)

------------------------------------------------------------------------
-- Now run xmonad with all the defaults we set up.

-- Run xmonad with the settings you specify. No need to modify this.
--
main = xmonad =<< statusBar myBar myPP toggleStrutsKey defaults

-- A structure containing your configuration settings, overriding
-- fields in the default config. Any you don't override, will
-- use the defaults defined in xmonad/XMonad/Config.hs
--
-- No need to modify this.
--
defaults = def {
        -- simple stuff
        terminal           = myTerminal,
        focusFollowsMouse  = myFocusFollowsMouse,
        clickJustFocuses   = myClickJustFocuses,
        borderWidth        = myBorderWidth,
        modMask            = myModMask,
        workspaces         = myWorkspaces,
        normalBorderColor  = myNormalBorderColor,
        focusedBorderColor = myFocusedBorderColor,

        -- key bindings
        keys               = myKeys,
        mouseBindings      = myMouseBindings,

        -- hooks, layouts
        layoutHook         = myLayout,
        manageHook         = myManageHook,
        handleEventHook    = myEventHook,
        logHook            = myLogHook,
        startupHook        = myStartupHook
    }

-- | Finally, a copy of the default bindings in simple textual tabular format.
help :: String
help = unlines ["The default modifier key is 'alt'. Default keybindings:",
    "",
    "-- launching and killing programs",
    "mod-Shift-Enter  Launch xterminal",
    "mod-p            Launch dmenu",
    "mod-Shift-p      Launch gmrun",
    "mod-Shift-c      Close/kill the focused window",
    "mod-Space        Rotate through the available layout algorithms",
    "mod-Shift-Space  Reset the layouts on the current workSpace to default",
    "mod-n            Resize/refresh viewed windows to the correct size",
    "",
    "-- move focus up or down the window stack",
    "mod-Tab        Move focus to the next window",
    "mod-Shift-Tab  Move focus to the previous window",
    "mod-j          Move focus to the next window",
    "mod-k          Move focus to the previous window",
    "mod-m          Move focus to the master window",
    "",
    "-- modifying the window order",
    "mod-Return   Swap the focused window and the master window",
    "mod-Shift-j  Swap the focused window with the next window",
    "mod-Shift-k  Swap the focused window with the previous window",
    "",
    "-- resizing the master/slave ratio",
    "mod-h  Shrink the master area",
    "mod-l  Expand the master area",
    "",
    "-- floating layer support",
    "mod-t  Push window back into tiling; unfloat and re-tile it",
    "",
    "-- increase or decrease number of windows in the master area",
    "mod-comma  (mod-,)   Increment the number of windows in the master area",
    "mod-period (mod-.)   Deincrement the number of windows in the master area",
    "",
    "-- quit, or restart",
    "mod-Shift-q  Quit xmonad",
    "mod-q        Restart xmonad",
    "mod-[1..9]   Switch to workSpace N",
    "",
    "-- Workspaces & screens",
    "mod-Shift-[1..9]   Move client to workspace N",
    "mod-{w,e,r}        Switch to physical/Xinerama screens 1, 2, or 3",
    "mod-Shift-{w,e,r}  Move client to screen 1, 2, or 3",
    "",
    "-- Mouse bindings: default actions bound to mouse events",
    "mod-button1  Set the window to floating mode and move by dragging",
    "mod-button2  Raise the window to the top of the stack",
    "mod-button3  Set the window to floating mode and resize by dragging"]
EOF

    # Crear configuración de XMobar
    echo "Creando configuración de XMobar..."
    cat > "$XMOBAR_DIR/xmobarrc" << 'EOF'
Config {

    -- appearance
        font =         "xft:Bitstream Vera Sans Mono:size=9:bold:antialias=true"
    , bgColor =      "black"
    , fgColor =      "#ABABAB"
    , position =     Top
    , border =       BottomB
    , borderColor =  "#646464"

    -- layout
    , sepChar =  "%"   -- delineator between plugin names and straight text
    , alignSep = "}{"  -- separator between left-right alignment
    , template = "<fc=#ABABAB>%StdinReader%</fc>  }{ | %multicpu% | %memory% | <fn=1>💾</fn> %disku% | <fn=1>🔊</fn> %alsa:default:Master% | <fn=1>📦</fn> %pacman% | %date% || %kbd% "

    -- general behavior
    , lowerOnStart =     True    -- send to bottom of window stack on start
    , hideOnStart =      False   -- start with window unmapped (hidden)
    , allDesktops =      True    -- show on all desktops
    , overrideRedirect = True    -- set the Override Redirect flag (Xlib)
    , pickBroadest =     False   -- choose widest display (multi-monitor)
    , persistent =       True    -- enable/disable hiding (True = disabled)

    -- plugins
    --   Numbers can be automatically colored according to their value. xmobar
    --   decides color based on a three-tier/two-cutoff system, controlled by
    --   command options:
    --     --Low sets the low cutoff
    --     --High sets the high cutoff
    --
    --     --low sets the color below --Low cutoff
    --     --normal sets the color between --Low and --High cutoffs
    --     --High sets the color above --High cutoff
    --
    --   The --template option controls how the plugin is displayed. Text
    --   color can be set by enclosing in <fc></fc> tags. For more details
    --   see http://projects.haskell.org/xmobar/#system-monitor-plugins.
    , commands =

        -- weather monitor
        [ Run Weather "RJTT" [ "--template", "<skyCondition> | <fc=#4682B4><tempC></fc>°C | <fc=#4682B4><rh></fc>% | <fc=#4682B4><pressure></fc>hPa"
                                ] 36000

, Run StdinReader

        -- network activity monitor (dynamic interface resolution)
        , Run DynNetwork     [ "--template" , "<dev>: <tx>kB/s|<rx>kB/s"
                                , "--Low"      , "1000"       -- units: B/s
                                , "--High"     , "5000"       -- units: B/s
                                , "--low"      , "darkgreen"
                                , "--normal"   , "darkorange"
                                , "--high"     , "darkred"
                                ] 10

        -- cpu activity monitor
        , Run MultiCpu       [ "--template" , "CPU: <total0>%|<total1>%"
                                , "--Low"      , "50"         -- units: %
                                , "--High"     , "85"         -- units: %
                                , "--low"      , "#a6e3a1"
                                , "--normal"   , "darkorange"
                                , "--high"     , "darkred"
                                ] 10

        -- cpu core temperature monitor
        , Run CoreTemp       [ "--template" , "Temp: <core0>°C|<core1>°C"
                                , "--Low"      , "70"        -- units: °C
                                , "--High"     , "80"        -- units: °C
                                , "--low"      , "darkgreen"
                                , "--normal"   , "darkorange"
                                , "--high"     , "darkred"
                                ] 50

        -- Espacio libre en disco (raíz)
        , Run DiskU         [("/", "<free> (<freep><fc=#a6e3a1>%</fc>)")]
                            [ "--Low"      , "20"        -- límite bajo (%)
                            , "--High"     , "50"        -- límite alto (%)
                            , "--low"      , "#f38ba8"   -- rojo (poco espacio)
                            , "--normal"   , "#f9e2af"   -- amarillo
                            , "--high"     , "#a6e3a1"   -- verde (mucho espacio)
                            ] 20

        -- Volumen del sistema
        , Run Alsa "default" "Master"
                            [ "--template", "<fc=#ABABAB><volume></fc> <status>"
                            , "--suffix"  , "True"
                            , "--"
                                    , "--on", ""
                                    , "--off", "<fc=#f38ba8>MUTE</fc>"
                                    , "--onc", "#a6e3a1"
                                    , "--offc", "#f38ba8"
                            ]

-- Updates de Pacman disponibles
        , Run Com "sh" ["-c", "updates=$(checkupdates 2>/dev/null | wc -l); if [ $updates -eq 0 ]; then echo \"<fc=#a6e3a1>✓ 0</fc>\"; elif [ $updates -le 5 ]; then echo \"<fc=#f9e2af>$updates</fc>\"; else echo \"<fc=#f38ba8>$updates</fc>\"; fi"] "pacman" 300


        -- memory usage monitor
        , Run Memory         [ "--template" ,"RAM: <usedratio>%"
                                , "--Low"      , "20"        -- units: %
                                , "--High"     , "90"        -- units: %
                                , "--low"      , "#a6e3a1"
                                , "--normal"   , "darkorange"
                                , "--high"     , "darkred"
                                ] 10

        -- battery monitor
        , Run Battery        [ "--template" , "Batt: <acstatus>"
                                , "--Low"      , "10"        -- units: %
                                , "--High"     , "80"        -- units: %
                                , "--low"      , "darkred"
                                , "--normal"   , "darkorange"
                                , "--high"     , "darkgreen"

                                , "--" -- battery specific options
                                        -- discharging status
                                        , "-o"	, "<left>% (<timeleft>)"
                                        -- AC "on" status
                                        , "-O"	, "<fc=#dAA520>Charging</fc>"
                                        -- charged status
                                        , "-i"	, "<fc=#006000>Charged</fc>"
                                ] 50

        -- time and date indicator
        --   (%F = y-m-d date, %a = day of week, %T = h:m:s time)
        , Run Date          "<fc=#ABABAB>%a %d/%m/%Y 🕐 %H:%M:%S</fc>" "date" 10

        -- keyboard layout indicator
        , Run Kbd            [ ("us(dvorak)" , "<fc=#00008B>DV</fc>")
                                , ("us"         , "<fc=#8B0000>US</fc>")
                                ]
        ]
    }
EOF

    # Crear .xinitrc
    echo "Creando archivo .xinitrc..."
    cat > "$USER_HOME/.xinitrc" << 'EOF'
#!/bin/sh

# Cargar recursos de X
userresources=$HOME/.Xresources
usermodmap=$HOME/.Xmodmap
sysresources=/etc/X11/xinit/.Xresources
sysmodmap=/etc/X11/xinit/.Xmodmap

# merge in defaults and keymaps
if [ -f $sysresources ]; then
    xrdb -merge $sysresources
fi

if [ -f $sysmodmap ]; then
    xmodmap $sysmodmap
fi

if [ -f "$userresources" ]; then
    xrdb -merge "$userresources"
fi

if [ -f "$usermodmap" ]; then
    xmodmap "$usermodmap"
fi

# Iniciar XMonad
exec xmonad
EOF

    # Hacer .xinitrc ejecutable
    chmod +x "$USER_HOME/.xinitrc"


    # Ajustar permisos
    echo "Ajustando permisos..."
    if [ -n "$USER" ] && getent passwd "$USER" > /dev/null 2>&1; then
        USER_ID=$(id -u "$USER" 2>/dev/null || echo "1000")
        GROUP_ID=$(id -g "$USER" 2>/dev/null || echo "1000")
        chown -R "$USER_ID:$GROUP_ID" "$USER_HOME/.config" 2>/dev/null || echo "Usuario $USER no encontrado, ajustar permisos manualmente"
        chown "$USER_ID:$GROUP_ID" "$USER_HOME/.xinitrc" 2>/dev/null || echo "Usuario $USER no encontrado, ajustar permisos manualmente"
    else
        echo "Advertencia: Usuario $USER no encontrado. Ajustar permisos manualmente después del chroot."
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
pacman -Sy archlinux-keyring --noconfirm
sleep 2
clear

# Instalación de herramientas necesarias
sleep 3
pacman -Sy reflector --noconfirm
pacman -Sy python3 --noconfirm
pacman -Sy rsync --noconfirm
clear

# Actualización de mirrorlist
echo -e "${GREEN}| Actualizando mejores listas de Mirrors |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
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
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkswap ${SELECTED_DISK}2
        mkfs.ext4 -F ${SELECTED_DISK}3
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones UEFI |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}3 /mnt
        swapon ${SELECTED_DISK}2
        mkdir -p /mnt/boot/efi
        mount ${SELECTED_DISK}1 /mnt/boot/efi

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
        mkswap ${SELECTED_DISK}1
        mkfs.ext4 -F ${SELECTED_DISK}2
        sleep 2

        # Montar particiones
        echo -e "${GREEN}| Montando particiones BIOS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        mount ${SELECTED_DISK}2 /mnt
        swapon ${SELECTED_DISK}1
        mkdir -p /mnt/boot
    fi
}

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
        mkfs.fat -F32 -v ${SELECTED_DISK}1
        mkswap ${SELECTED_DISK}2
        mkfs.btrfs -f ${SELECTED_DISK}3
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
        if mountpoint -q "${SELECTED_DISK}3" 2>/dev/null; then
            echo -e "${YELLOW}Desmontando ${SELECTED_DISK}3...${NC}"
            umount -f "${SELECTED_DISK}3" 2>/dev/null || true
            sleep 2
        fi

        echo -e "${CYAN}Montando partición BTRFS ${SELECTED_DISK}3 en /mnt...${NC}"
        mount ${SELECTED_DISK}3 /mnt || {
            echo -e "${RED}ERROR: No se pudo montar ${SELECTED_DISK}3${NC}"
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
        btrfs subvolume create /mnt/@var
        btrfs subvolume create /mnt/@tmp
        umount /mnt

        # Montar subvolúmenes
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ ${SELECTED_DISK}3 /mnt
        swapon ${SELECTED_DISK}2
        mkdir -p /mnt/{boot/efi,home,var,tmp}
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}3 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}3 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}3 /mnt/tmp
        mount ${SELECTED_DISK}1 /mnt/boot/efi

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs

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
        mkfs.ext4 -F ${SELECTED_DISK}1
        mkswap ${SELECTED_DISK}2
        mkfs.btrfs -f ${SELECTED_DISK}3
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
        if mountpoint -q "${SELECTED_DISK}3" 2>/dev/null; then
            echo -e "${YELLOW}Desmontando ${SELECTED_DISK}3...${NC}"
            umount -f "${SELECTED_DISK}3" 2>/dev/null || true
            sleep 2
        fi

        echo -e "${CYAN}Montando partición BTRFS ${SELECTED_DISK}3 en /mnt...${NC}"
        mount ${SELECTED_DISK}3 /mnt || {
            echo -e "${RED}ERROR: No se pudo montar ${SELECTED_DISK}3${NC}"
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
        btrfs subvolume create /mnt/@var
        btrfs subvolume create /mnt/@tmp
        umount /mnt

        # Montar subvolúmenes
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@ ${SELECTED_DISK}3 /mnt
        swapon ${SELECTED_DISK}2
        mkdir -p /mnt/{boot,home,var,tmp}
        mount ${SELECTED_DISK}1 /mnt/boot
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@home ${SELECTED_DISK}3 /mnt/home
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@var ${SELECTED_DISK}3 /mnt/var
        mount -o noatime,compress=zstd,space_cache=v2,subvol=@tmp ${SELECTED_DISK}3 /mnt/tmp

        # Instalar herramientas específicas para BTRFS
        pacstrap /mnt btrfs-progs
    fi
}

# Función para particionado con cifrado LUKS
partition_cifrado() {
    echo -e "${GREEN}| Particionando disco con cifrado LUKS: $SELECTED_DISK |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""
    echo -e "${CYAN}NOTA IMPORTANTE: Esta configuración implementa LUKS+LVM siguiendo mejores prácticas:${NC}"
    echo -e "${CYAN}  • Solo las particiones EFI y boot quedan sin cifrar (necesario para el bootloader)${NC}"
    echo -e "${CYAN}  • LUKS cifra toda la partición principal${NC}"
    echo -e "${CYAN}  • LVM se ejecuta sobre LUKS para flexibilidad en particiones${NC}"
    echo -e "${CYAN}  • CRITICAL: Guarda bien tu contraseña LUKS - sin ella perderás todos los datos${NC}"
    echo ""
    sleep 3

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
        mkfs.fat -F32 ${SELECTED_DISK}1
        mkfs.ext4 -F ${SELECTED_DISK}2

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
        wipefs -af ${SELECTED_DISK}3 2>/dev/null || true
        dd if=/dev/zero of=${SELECTED_DISK}3 bs=1M count=10 2>/dev/null || true

        echo -e "${CYAN}Aplicando cifrado LUKS a ${SELECTED_DISK}3...${NC}"
        echo -e "${YELLOW}IMPORTANTE: Esto puede tomar unos minutos dependiendo del tamaño del disco${NC}"
        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat --batch-mode --verify-passphrase ${SELECTED_DISK}3 -; then
            echo -e "${RED}ERROR: Falló el cifrado LUKS de la partición${NC}"
            exit 1
        fi

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open --batch-mode ${SELECTED_DISK}3 cryptlvm -; then
            echo -e "${RED}ERROR: No se pudo abrir el dispositivo cifrado${NC}"
            exit 1
        fi

        # Verificar que el dispositivo cifrado esté disponible
        if [ ! -b "/dev/mapper/cryptlvm" ]; then
            echo -e "${RED}ERROR: El dispositivo /dev/mapper/cryptlvm no está disponible${NC}"
            exit 1
        fi

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup ${SELECTED_DISK}3 --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS
        echo -e "${GREEN}| Configurando LVM sobre LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -e "${CYAN}Creando Physical Volume sobre dispositivo cifrado...${NC}"
        if ! pvcreate /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Physical Volume${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Volume Group 'vg0'...${NC}"
        if ! vgcreate vg0 /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Volume Group vg0${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'swap' de 8GB...${NC}"
        if ! lvcreate -L 8G vg0 -n swap; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume swap${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'root' con el espacio restante...${NC}"
        if ! lvcreate -l 100%FREE vg0 -n root; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume root${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Configuración LVM completada:${NC}"
        echo -e "${GREEN}  • Volume Group: vg0${NC}"
        echo -e "${GREEN}  • Swap: 8GB (/dev/vg0/swap)${NC}"
        echo -e "${GREEN}  • Root: Resto del espacio (/dev/vg0/root)${NC}"

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

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

        # Verificar que las particiones existan antes de montar
        echo -e "${CYAN}Verificando particiones antes del montaje...${NC}"
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición EFI ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición boot ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi

        # Esperar que las particiones estén completamente listas
        sleep 2
        sync

        echo -e "${CYAN}Creando directorio de montaje boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot; then
            echo -e "${RED}ERROR: Falló el montaje de la partición boot${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando directorio EFI dentro de boot...${NC}"
        mkdir -p /mnt/boot/efi

        echo -e "${CYAN}Montando partición EFI...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot/efi; then
            echo -e "${RED}ERROR: Falló el montaje de la partición EFI${NC}"
            exit 1
        fi

        # Verificar que los montajes sean exitosos (en orden correcto)
        if ! mountpoint -q /mnt/boot; then
            echo -e "${RED}ERROR: /mnt/boot no está montado correctamente${NC}"
            exit 1
        fi
        if ! mountpoint -q /mnt/boot/efi; then
            echo -e "${RED}ERROR: /mnt/boot/efi no está montado correctamente${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Todas las particiones montadas correctamente${NC}"
        echo -e "${GREEN}✓ Esquema LUKS+LVM configurado:${NC}"
        echo -e "${GREEN}  • UEFI: EFI (512MB) + boot (1GB) sin cifrar, resto cifrado${NC}"

        # Instalar herramientas específicas para cifrado
        pacstrap /mnt cryptsetup lvm2

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
        mkfs.ext4 -F ${SELECTED_DISK}1

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
        wipefs -af ${SELECTED_DISK}2 2>/dev/null || true
        dd if=/dev/zero of=${SELECTED_DISK}2 bs=1M count=10 2>/dev/null || true

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup luksFormat --batch-mode --verify-passphrase ${SELECTED_DISK}2 -; then
            echo -e "${RED}ERROR: Falló el cifrado LUKS de la partición${NC}"
            exit 1
        fi

        if ! echo -n "$ENCRYPTION_PASSWORD" | cryptsetup open --batch-mode ${SELECTED_DISK}2 cryptlvm -; then
            echo -e "${RED}ERROR: No se pudo abrir el dispositivo cifrado${NC}"
            exit 1
        fi

        # Verificar que el dispositivo cifrado esté disponible
        if [ ! -b "/dev/mapper/cryptlvm" ]; then
            echo -e "${RED}ERROR: El dispositivo /dev/mapper/cryptlvm no está disponible${NC}"
            exit 1
        fi

        # Crear backup del header LUKS (recomendación de seguridad)
        echo -e "${CYAN}Creando backup del header LUKS...${NC}"
        cryptsetup luksHeaderBackup ${SELECTED_DISK}2 --header-backup-file /tmp/luks-header-backup
        echo -e "${GREEN}✓ Backup del header LUKS guardado en /tmp/luks-header-backup${NC}"
        echo -e "${YELLOW}IMPORTANTE: Copia este archivo a un lugar seguro después de la instalación${NC}"

        # Configurar LVM sobre LUKS
        echo -e "${GREEN}| Configurando LVM sobre LUKS |${NC}"
        printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
        echo ""
        echo -e "${CYAN}Creando Physical Volume sobre dispositivo cifrado...${NC}"
        if ! pvcreate /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Physical Volume${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Volume Group 'vg0'...${NC}"
        if ! vgcreate vg0 /dev/mapper/cryptlvm; then
            echo -e "${RED}ERROR: No se pudo crear el Volume Group vg0${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'swap' de 8GB...${NC}"
        if ! lvcreate -L 8G vg0 -n swap; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume swap${NC}"
            exit 1
        fi

        echo -e "${CYAN}Creando Logical Volume 'root' con el espacio restante...${NC}"
        if ! lvcreate -l 100%FREE vg0 -n root; then
            echo -e "${RED}ERROR: No se pudo crear el Logical Volume root${NC}"
            exit 1
        fi

        echo -e "${GREEN}✓ Configuración LVM completada:${NC}"
        echo -e "${GREEN}  • Volume Group: vg0${NC}"
        echo -e "${GREEN}  • Swap: 8GB (/dev/vg0/swap)${NC}"
        echo -e "${GREEN}  • Root: Resto del espacio (/dev/vg0/root)${NC}"

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

        # Montar sistema de archivos root
        echo -e "${CYAN}Montando sistema raíz...${NC}"
        if ! mount /dev/vg0/root /mnt; then
            echo -e "${RED}ERROR: No se pudo montar /dev/vg0/root en /mnt${NC}"
            exit 1
        fi

        if ! swapon /dev/vg0/swap; then
            echo -e "${YELLOW}ADVERTENCIA: No se pudo activar el swap${NC}"
        fi

        # Verificar que la partición boot exista
        echo -e "${CYAN}Verificando partición boot antes del montaje...${NC}"
        if [ ! -b "${SELECTED_DISK}1" ]; then
            echo -e "${RED}ERROR: Partición boot ${SELECTED_DISK}1 no existe${NC}"
            exit 1
        fi

        # Esperar que la partición esté completamente lista
        sleep 2
        sync

        # Montar partición boot
        echo -e "${CYAN}Creando directorio /boot...${NC}"
        mkdir -p /mnt/boot

        echo -e "${CYAN}Montando partición boot...${NC}"
        if ! mount ${SELECTED_DISK}1 /mnt/boot; then
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
        pacstrap /mnt cryptsetup lvm2
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
                ;;
            "mkfs.f2fs")
                mkfs.f2fs -f $device
                ;;
            "mkfs.fat32")
                mkfs.fat -F32 -v $device
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
        elif [ "$mountpoint" = "/boot/EFI" ]; then
            EFI_FOUND=true
            # Verificar que la partición EFI use formato FAT
            if [ "$format" != "mkfs.fat32" ] && [ "$format" != "mkfs.fat16" ]; then
                echo -e "${YELLOW}ADVERTENCIA: Partición EFI ($device) debería usar formato FAT32 o FAT16${NC}"
                echo -e "${YELLOW}Formato actual: $format${NC}"
            fi
        elif [ "$mountpoint" = "/boot" ]; then
            BOOT_FOUND=true
        fi
    done

    # Validar configuración
    if [ "$ROOT_FOUND" = false ]; then
        echo -e "${RED}ERROR: No se encontró partición raíz (/) configurada${NC}"
        echo -e "${RED}Debe configurar al menos una partición con punto de montaje '/'${NC}"
        exit 1
    fi

    if [ "$EFI_FOUND" = true ] && [ "$BOOT_FOUND" = true ]; then
        echo -e "${GREEN}✓ Configuración detectada: /boot separado + /boot/EFI${NC}"
    elif [ "$EFI_FOUND" = true ]; then
        echo -e "${GREEN}✓ Configuración detectada: /boot/EFI (sin /boot separado)${NC}"
    fi

    echo -e "${GREEN}✓ Validaciones completadas${NC}"

    # Segunda pasada: Montaje en orden correcto
    echo -e "${CYAN}=== FASE 2: Montaje de particiones ===${NC}"

    # 1. Montar partición raíz primero
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/" ]; then
            echo -e "${GREEN}| Montando raíz: $device -> /mnt |${NC}"
            mount $device /mnt
            break
        fi
    done

    # 2. Montar /boot si existe
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/boot" ]; then
            echo -e "${GREEN}| Montando /boot: $device -> /mnt/boot |${NC}"
            mkdir -p /mnt/boot
            mount $device /mnt/boot
            break
        fi
    done

    # 3. Montar /boot/EFI (debe ir después de /boot para evitar conflictos)
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"
        if [ "$mountpoint" = "/boot/EFI" ]; then
            echo -e "${GREEN}| Montando EFI: $device -> /mnt/boot/efi |${NC}"
            mkdir -p /mnt/boot/efi
            mount $device /mnt/boot/efi
            echo -e "${CYAN}Partición EFI montada en /mnt/boot/efi${NC}"
            break
        fi
    done

    # 4. Montar todas las demás particiones
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        # Saltar las ya montadas y swap
        if [ "$mountpoint" = "/" ] || [ "$mountpoint" = "/boot" ] || [ "$mountpoint" = "/boot/EFI" ] || [ "$mountpoint" = "swap" ]; then
            continue
        fi

        echo -e "${GREEN}| Montando: $device -> /mnt$mountpoint |${NC}"
        mkdir -p /mnt$mountpoint
        mount $device /mnt$mountpoint
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

# Función para configurar montajes necesarios para chroot
setup_chroot_mounts() {
echo -e "${CYAN}Configurando montajes para chroot...${NC}"
mount --types proc /proc /mnt/proc
mount --rbind /sys /mnt/sys
mount --make-rslave /mnt/sys
mount --rbind /dev /mnt/dev
mount --make-rslave /mnt/dev
mount --bind /run /mnt/run
mount --make-slave /mnt/run
cp /etc/resolv.conf /mnt/etc/
echo -e "${GREEN}✓ Montajes para chroot configurados${NC}"
}


# Función para limpiar montajes de chroot
cleanup_chroot_mounts() {
echo -e "${CYAN}Limpiando montajes de chroot...${NC}"
umount -l /mnt/run 2>/dev/null || true
umount -l /mnt/dev 2>/dev/null || true
umount -l /mnt/sys 2>/dev/null || true
umount -l /mnt/proc 2>/dev/null || true
echo -e "${GREEN}✓ Montajes de chroot limpiados${NC}"
}


# Ejecutar limpieza de particiones
unmount_selected_disk_partitions
cleanup_chroot_mounts

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

pacstrap /mnt base
pacstrap /mnt base-devel
pacstrap /mnt lsb-release
pacstrap /mnt reflector python3 rsync
pacstrap /mnt nano
pacstrap /mnt xdg-user-dirs
pacstrap /mnt curl
pacstrap /mnt wget
pacstrap /mnt git

# Instalar herramientas específicas según el modo de particionado
if [ "$PARTITION_MODE" = "auto_btrfs" ]; then
    echo -e "${CYAN}Instalando herramientas BTRFS...${NC}"
    pacstrap /mnt btrfs-progs
elif [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${CYAN}Instalando herramientas de cifrado...${NC}"
    pacstrap /mnt cryptsetup lvm2
fi

# Configurar montajes para chroot
clear
sleep 2
setup_chroot_mounts
sleep 2
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
sleep 2
clear

# Actualización de mirrors en el sistema instalado
cp /etc/pacman.d/mirrorlist /mnt/etc/pacman.d/mirrorlist
#chroot /mnt /bin/bash -c "reflector --verbose --latest 6 --protocol https --sort rate --save /etc/pacman.d/mirrorlist"
clear
cat /mnt/etc/pacman.d/mirrorlist
sleep 3
clear

# Actualización del sistema instalado
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
cp /usr/share/arcrisgui/data/config/pacman.conf /mnt/etc/pacman.conf
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
sleep 5

# Generar fstab
if [ "$PARTITION_MODE" = "manual" ]; then
    echo -e "${GREEN}| Generando fstab para particionado manual |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    # Crear fstab base
    echo "# <file system> <mount point> <type> <options> <dump> <pass>" > /mnt/etc/fstab

    # Procesar configuraciones de particiones para fstab
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        # Omitir particiones swap (se manejan separadamente)
        if [ "$mountpoint" = "swap" ]; then
            continue
        fi

        # Para particiones no formateadas (none), detectar el sistema de archivos existente
        if [ "$format" = "none" ]; then
            DETECTED_FS=$(blkid -s TYPE -o value $device)
            if [ -z "$DETECTED_FS" ]; then
                echo -e "${YELLOW}ADVERTENCIA: No se pudo detectar sistema de archivos en $device, omitiendo del fstab${NC}"
                continue
            fi
            echo -e "${CYAN}Detectado sistema de archivos existente en $device: $DETECTED_FS${NC}"
            format_for_fstab="$DETECTED_FS"
        else
            # Para particiones formateadas, usar el formato especificado
            format_for_fstab="$format"
        fi

        # Obtener UUID de la partición
        PART_UUID=$(blkid -s UUID -o value $device)
        if [ -n "$PART_UUID" ]; then
            # Determinar el tipo de sistema de archivos
            case $format_for_fstab in
                "mkfs.fat32"|"mkfs.fat16"|"vfat")
                    FS_TYPE="vfat"
                    if [ "$mountpoint" = "/boot/EFI" ]; then
                        echo "UUID=$PART_UUID /boot/efi vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
                "mkfs.ext4"|"mkfs.ext3"|"mkfs.ext2"|"ext4"|"ext3"|"ext2")
                    if [[ "$format_for_fstab" =~ ^mkfs\. ]]; then
                        FS_TYPE="${format_for_fstab#mkfs.}"
                    else
                        FS_TYPE="$format_for_fstab"
                    fi
                    if [ "$mountpoint" = "/" ]; then
                        echo "UUID=$PART_UUID / $FS_TYPE rw,relatime 0 1" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint $FS_TYPE rw,relatime 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
                "mkfs.btrfs"|"btrfs")
                    echo "UUID=$PART_UUID $mountpoint btrfs rw,noatime,compress=zstd,space_cache=v2 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.xfs"|"xfs")
                    echo "UUID=$PART_UUID $mountpoint xfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.f2fs"|"f2fs")
                    echo "UUID=$PART_UUID $mountpoint f2fs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.ntfs"|"ntfs")
                    echo "UUID=$PART_UUID $mountpoint ntfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.reiserfs"|"reiserfs")
                    echo "UUID=$PART_UUID $mountpoint reiserfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                "mkfs.jfs"|"jfs")
                    echo "UUID=$PART_UUID $mountpoint jfs rw,relatime 0 2" >> /mnt/etc/fstab
                    ;;
                *)
                    echo -e "${YELLOW}ADVERTENCIA: Sistema de archivos no reconocido ($format_for_fstab) para $device${NC}"
                    echo -e "${YELLOW}Usando opciones genéricas en fstab${NC}"
                    if [ "$mountpoint" = "/" ]; then
                        echo "UUID=$PART_UUID / $format_for_fstab rw,relatime 0 1" >> /mnt/etc/fstab
                    else
                        echo "UUID=$PART_UUID $mountpoint $format_for_fstab rw,relatime 0 2" >> /mnt/etc/fstab
                    fi
                    ;;
            esac
        fi
    done

    # Agregar particiones swap
    for partition_config in "${PARTITIONS[@]}"; do
        IFS=' ' read -r device format mountpoint <<< "$partition_config"

        if [ "$mountpoint" = "swap" ]; then
            SWAP_UUID=$(blkid -s UUID -o value $device)
            if [ -n "$SWAP_UUID" ]; then
                echo "UUID=$SWAP_UUID none swap defaults 0 0" >> /mnt/etc/fstab
            fi
        fi
    done

    echo -e "${GREEN}✓ fstab generado para particionado manual${NC}"
else
    # Usar genfstab para modos automáticos
    genfstab -U /mnt > /mnt/etc/fstab
fi

echo ""
chroot /mnt /bin/bash -c "cat /etc/fstab"
sleep 3
clear

# Instalación del kernel seleccionado
echo -e "${GREEN}| Instalando kernel: $SELECTED_KERNEL |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$SELECTED_KERNEL" in
    "linux")
        chroot /mnt /bin/bash -c "pacman -S linux linux-firmware --noconfirm"
        ;;
    "linux-hardened")
        chroot /mnt /bin/bash -c "pacman -S linux-hardened linux-firmware --noconfirm"
        ;;
    "linux-lts")
        chroot /mnt /bin/bash -c "pacman -S linux-lts linux-firmware --noconfirm"
        ;;
    "linux-rt-lts")
        chroot /mnt /bin/bash -c "pacman -S linux-rt-lts linux-firmware --noconfirm"
        ;;
    "linux-zen")
        chroot /mnt /bin/bash -c "pacman -S linux-zen linux-firmware --noconfirm"
        ;;
    *)
        chroot /mnt /bin/bash -c "pacman -S linux linux-firmware --noconfirm"
        ;;
esac

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
chroot /mnt /bin/bash -c "pacman -S sudo --noconfirm"

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
chroot /mnt bash -c "cd /tmp && git clone https://aur.archlinux.org/yay-bin.git && cd yay-bin && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'"
sleep 2
# Instalar alsi desde AUR usando makepkg
chroot /mnt bash -c "cd /tmp && git clone https://aur.archlinux.org/alsi.git && cd alsi && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'"
sleep 2
clear


# Configuración de mkinitcpio según el modo de particionado
echo -e "${GREEN}| Configurando mkinitcpio |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

if [ "$PARTITION_MODE" = "cifrado" ]; then
    echo -e "${GREEN}Configurando mkinitcpio para cifrado LUKS+LVM...${NC}"

    # Configurar módulos específicos para LUKS+LVM (siguiendo mejores prácticas)
    echo -e "${CYAN}Configurando módulos del kernel para cifrado...${NC}"
    sed -i 's/^MODULES=.*/MODULES=(dm_mod dm_crypt dm_snapshot dm_mirror)/' /mnt/etc/mkinitcpio.conf

    # Configurar hooks para cifrado con LVM - orden crítico: encrypt antes de lvm2
    echo -e "${CYAN}Configurando hooks - ORDEN CRÍTICO: encrypt debe ir antes de lvm2${NC}"
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block encrypt lvm2 filesystems fsck)/' /mnt/etc/mkinitcpio.conf

    echo -e "${GREEN}✓ Configuración mkinitcpio actualizada para LUKS+LVM${NC}"
    echo -e "${CYAN}  • Módulos: dm_mod dm_crypt dm_snapshot dm_mirror${NC}"
    echo -e "${CYAN}  • Hooks: base udev autodetect keyboard keymap consolefont modconf block encrypt lvm2 filesystems fsck${NC}"
    echo -e "${YELLOW}  • IMPORTANTE: 'encrypt' DEBE ir antes de 'lvm2' para que funcione correctamente${NC}"
    echo -e "${YELLOW}  • keyboard y keymap son necesarios para introducir la contraseña en el boot${NC}"

elif [ "$PARTITION_MODE" = "auto_btrfs" ]; then
    echo "Configurando mkinitcpio para BTRFS..."
    # Configurar módulos específicos para BTRFS (agregando módulos de compresión adicionales)
    sed -i 's/^MODULES=.*/MODULES=(btrfs crc32c zstd lzo lz4 zlib_deflate)/' /mnt/etc/mkinitcpio.conf
    # Configurar hooks para BTRFS
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf

else
    echo "Configurando mkinitcpio para sistema estándar..."
    # Configuración estándar para ext4
    sed -i 's/^MODULES=.*/MODULES=()/' /mnt/etc/mkinitcpio.conf
    sed -i 's/^HOOKS=.*/HOOKS=(base udev autodetect keyboard keymap consolefont modconf block filesystems fsck)/' /mnt/etc/mkinitcpio.conf
fi

# Regenerar initramfs
chroot /mnt /bin/bash -c "mkinitcpio -P"
sleep 2

# Instalación de bootloader
# Instalar bootloader para todos los modos (incluyendo manual)
if true; then
    echo -e "${GREEN}| Instalando bootloader |${NC}"
    printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
    echo ""

    if [ "$FIRMWARE_TYPE" = "UEFI" ]; then
        # Verificar que la partición EFI esté montada con debug adicional
        echo -e "${CYAN}Verificando montaje de partición EFI...${NC}"
        if ! mountpoint -q /mnt/boot/efi; then
            echo -e "${RED}ERROR: Partición EFI no está montada en /mnt/boot/efi${NC}"
            echo -e "${YELLOW}Información de debug:${NC}"
            echo "- Contenido de /mnt/boot:"
            ls -la /mnt/boot/ 2>/dev/null || echo "  Directorio /mnt/boot no accesible"
            echo "- Contenido de /mnt/boot/efi:"
            ls -la /mnt/boot/efi/ 2>/dev/null || echo "  Directorio /mnt/boot/efi no accesible"
            echo "- Montajes actuales:"
            mount | grep "/mnt"
            echo "- Particiones disponibles:"
            lsblk ${SELECTED_DISK}
            exit 1
        fi
        echo -e "${GREEN}✓ Partición EFI montada correctamente en /mnt/boot/efi${NC}"

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
        # Limpiar entradas UEFI previas que puedan causar conflictos
        # echo -e "${CYAN}Limpiando entradas UEFI previas...${NC}"
        # efibootmgr | awk '/grub/i {gsub(/Boot|\*.*/, ""); system("efibootmgr -b " $1 " -B 2>/dev/null")}'
        efibootmgr | grep -i grub | cut -d'*' -f1 | sed 's/Boot//' | xargs -I {} efibootmgr -b {} -B 2>/dev/null || true
        clear
        sleep 4

        # Limpiar directorio EFI previo si existe
        if [ -d "/mnt/boot/efi/EFI/GRUB" ]; then
            rm -rf /mnt/boot/efi/EFI/GRUB
        fi

        # Crear directorio EFI si no existe
        mkdir -p /mnt/boot/efi/EFI

        echo -e "${CYAN}Instalando paquetes GRUB para UEFI...${NC}"
        chroot /mnt /bin/bash -c "pacman -S grub efibootmgr --noconfirm"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada ${SELECTED_DISK}3${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Configurar GRUB para LUKS+LVM (siguiendo mejores prácticas de la guía)
            echo -e "${CYAN}Configurando parámetros de kernel para LUKS+LVM...${NC}"
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root\"/" /mnt/etc/default/grub

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
        else
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
            echo "GRUB_PRELOAD_MODULES=\"part_gpt part_msdos\"" >> /mnt/etc/default/grub
        fi

        sleep 2
        clear

        echo -e "${CYAN}Instalando GRUB en partición EFI...${NC}"

        # Instalar GRUB en modo removible (crea /EFI/BOOT/bootx64.efi)
        echo -e "${CYAN}Instalando GRUB en modo removible...${NC}"
        chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot/efi --removable --force --recheck" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI (modo removible)${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ GRUB instalado en modo removible (/EFI/BOOT/bootx64.efi)${NC}"

        # Instalar GRUB con entrada NVRAM (crea /EFI/GRUB/grubx64.efi)
        echo -e "${CYAN}Instalando GRUB con entrada NVRAM...${NC}"
        chroot /mnt /bin/bash -c "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=GRUB --force --recheck" || {
            echo -e "${RED}ERROR: Falló la instalación de GRUB UEFI (entrada NVRAM)${NC}"
            exit 1
        }
        echo -e "${GREEN}✓ GRUB instalado con entrada NVRAM (/EFI/GRUB/grubx64.efi)${NC}"

        # Verificar que ambos bootloaders se hayan creado
        if [ ! -f "/mnt/boot/efi/EFI/BOOT/bootx64.efi" ]; then
            echo -e "${RED}ERROR: No se creó bootx64.efi${NC}"
            exit 1
        fi

        if [ ! -f "/mnt/boot/efi/EFI/GRUB/grubx64.efi" ]; then
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
        chroot /mnt /bin/bash -c "pacman -S grub --noconfirm"

        # Configuración específica según el modo de particionado ANTES de instalar
        echo -e "${CYAN}Configurando GRUB para el modo de particionado...${NC}"
        if [ "$PARTITION_MODE" = "cifrado" ]; then
            # Esperar que la partición esté lista y obtener UUID
            echo -e "${CYAN}Obteniendo UUID de la partición cifrada...${NC}"
            sleep 2
            sync
            partprobe $SELECTED_DISK 2>/dev/null || true
            sleep 1

            CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            # Reintentar si no se obtuvo UUID
            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${YELLOW}Reintentando obtener UUID...${NC}"
                sleep 2
                CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
            fi

            if [ -z "$CRYPT_UUID" ]; then
                echo -e "${RED}ERROR: No se pudo obtener UUID de la partición cifrada ${SELECTED_DISK}2${NC}"
                echo -e "${RED}Verificar que la partición esté correctamente formateada${NC}"
                exit 1
            fi
            echo -e "${GREEN}✓ UUID obtenido: ${CRYPT_UUID}${NC}"
            # Usar GRUB_CMDLINE_LINUX en lugar de GRUB_CMDLINE_LINUX_DEFAULT para mejores prácticas
            sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"cryptdevice=UUID=${CRYPT_UUID}:cryptlvm root=\/dev\/vg0\/root\"/" /mnt/etc/default/grub
            # Configurar GRUB_CMDLINE_LINUX_DEFAULT sin 'quiet' para mejor debugging en sistemas cifrados
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet"/GRUB_CMDLINE_LINUX_DEFAULT="loglevel=5"/' /mnt/etc/default/grub
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
        if [ -f "/mnt/boot/efi/EFI/GRUB/grubx64.efi" ] && [ -f "/mnt/boot/efi/EFI/BOOT/bootx64.efi" ] && [ -f "/mnt/boot/grub/grub.cfg" ]; then
            echo -e "${GREEN}✓ Bootloader UEFI verificado correctamente${NC}"
            echo -e "${GREEN}✓ Modo NVRAM: /EFI/GRUB/grubx64.efi${NC}"
            echo -e "${GREEN}✓ Modo removible: /EFI/BOOT/bootx64.efi${NC}"
        else
            echo -e "${RED}⚠ Problema con la instalación del bootloader UEFI${NC}"
            echo -e "${YELLOW}Archivos verificados:${NC}"
            echo "  - /mnt/boot/efi/EFI/GRUB/grubx64.efi: $([ -f "/mnt/boot/efi/EFI/GRUB/grubx64.efi" ] && echo "✓" || echo "✗")"
            echo "  - /mnt/boot/efi/EFI/BOOT/bootx64.efi: $([ -f "/mnt/boot/efi/EFI/BOOT/bootx64.efi" ] && echo "✓" || echo "✗")"
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
chroot /mnt /bin/bash -c "sudo -u $USER yay -S os-prober --noansweredit --noconfirm --needed"
chroot /mnt /bin/bash -c "sudo -u $USER yay -S ntfs-3g --noansweredit --noconfirm --needed"
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



sleep 3
clear



# Instalación de drivers de video
echo -e "${GREEN}| Instalando drivers de video: $DRIVER_VIDEO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_VIDEO" in
    "Open Source")
        # Detección automática de hardware de video usando VGA controller
        VGA_LINE=$(lspci | grep -i "vga compatible controller")
        echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"

        if echo "$VGA_LINE" | grep -i nvidia > /dev/null; then
            echo "Detectado hardware NVIDIA - Instalando driver open source nouveau"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-nouveau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-nouveau lib32-vulkan-nouveau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver lib32-libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"

        elif echo "$VGA_LINE" | grep -i "amd\|radeon" > /dev/null; then
            echo "Detectado hardware AMD/Radeon - Instalando driver open source amdgpu"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-amdgpu --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-ati --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-radeon lib32-vulkan-radeon --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S radeontop --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver lib32-libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"

        elif echo "$VGA_LINE" | grep -i intel > /dev/null; then
            echo "Detectado hardware Intel - Instalando driver open source intel"
            chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-intel lib32-vulkan-intel --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S intel-media-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-intel-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S intel-gpu-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
            chroot /mnt /bin/bash -c "sudo -u $USER yay -S intel-compute-runtime --noansweredit --noconfirm --needed"
            chroot /mnt /bin/bash -c "sudo -u $USER yay -S intel-hybrid-codec-driver-git --noansweredit --noconfirm --needed"
            chroot /mnt /bin/bash -c "sudo -u $USER yay -S vpl-gpu-rt --noansweredit --noconfirm --needed"

        elif echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then

            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"

            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-fbdev --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S spice-vdagent --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-qxl --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S qemu-guest-agent --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libgl --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libglvnd --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
            chroot /mnt /bin/bash -c "systemctl start qemu-guest-agent.service"



        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"

            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"

            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            chroot /mnt /bin/bash -c "pacman -S xorg-server --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xorg-xinit --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-vesa --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"
        fi
        ;;
    "nvidia")
        echo "Instalando driver NVIDIA para kernel linux"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"

        ;;
    "nvidia-lts")
        echo "Instalando driver NVIDIA para kernel LTS"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-lts --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"
        ;;
    "nvidia-dkms")
        echo "Instalando driver NVIDIA DKMS"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-dkms --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-settings --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia --noansweredit --noconfirm --needed"
        ;;
    "nvidia-470xx-dkms")
        echo "Instalando driver NVIDIA serie 470.xx con DKMS"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-dkms --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia-470xx --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-470xx-settings --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-470xx-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia-470xx --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S mhwd-nvidia-470xx --noansweredit --noconfirm --needed"
        ;;
    "nvidia-390xx-dkms")
        echo "Instalando driver NVIDIA serie 390.xx con DKMS (hardware antiguo)"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-dkms --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-nvidia-390xx --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-nvidia-390xx-utils --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-opencl-nvidia-390xx --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nvidia-390xx-settings --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S mhwd-nvidia-390xx --noansweredit --noconfirm --needed"
        ;;
    "AMD Private")
        echo "Instalando drivers privativos de AMDGPUPRO"
        chroot /mnt /bin/bash -c "pacman -S xf86-video-amdgpu mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S vulkan-radeon lib32-vulkan-radeon vulkan-tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S radeontop --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S amf-amdgpu-pro --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S amdgpu-pro-oglp --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-amdgpu-pro-oglp --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S vulkan-amdgpu-pro --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lib32-vulkan-amdgpu-pro --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S opencl-amd --noansweredit --noconfirm --needed"
        ;;
    "Intel Gen(4-9)")
        echo "Instalando drivers Modernos de Intel"
        chroot /mnt /bin/bash -c "pacman -S mesa lib32-mesa --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S xf86-video-intel --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S vulkan-intel lib32-vulkan-intel --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S intel-media-driver --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S libva-intel-driver --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S opencl-mesa opencl-rusticl-mesa --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S mesa-vdpau lib32-mesa-vdpau --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S intel-gpu-tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S vdpauinfo vainfo --noconfirm"
        ;;
    "Máquina Virtual")

    # Detección automática de hardware de video usando VGA controller
    VGA_LINE=$(lspci | grep -i "vga compatible controller")
    echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"

        if  echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then
            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"
            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-fbdev --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S spice-vdagent --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S xf86-video-qxl --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S qemu-guest-agent --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libgl --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libglvnd --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
            chroot /mnt /bin/bash -c "systemctl start qemu-guest-agent.service"


        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"
            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            chroot /mnt /bin/bash -c "pacman -S mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver  --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"

            chroot /mnt /bin/bash -c "pacman -S virtualbox-guest-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S virglrenderer --noconfirm"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            chroot /mnt /bin/bash -c "pacman -S mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-utils --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-mesa-layers --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S vulkan-tools --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S mesa-vdpau --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-libva-mesa-driver --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S lib32-mesa-vdpau --noconfirm"
        fi
        ;;
esac

clear

# Instalación de drivers de audio
echo -e "${GREEN}| Instalando drivers de audio: $DRIVER_AUDIO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_AUDIO" in
    "Alsa Audio")
        chroot /mnt /bin/bash -c "pacman -S alsa-utils alsa-plugins --noconfirm"
        ;;
    "pipewire")
        chroot /mnt /bin/bash -c "pacman -S pipewire pipewire-pulse pipewire-alsa --noconfirm"
        ;;
    "pulseaudio")
        chroot /mnt /bin/bash -c "pacman -S pulseaudio pulseaudio-alsa pavucontrol --noconfirm"
        ;;
    "Jack2")
        chroot /mnt /bin/bash -c "pacman -S jack2 --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S lib32-jack2 --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S jack2-dbus --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S carla --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S qjackctl --noconfirm"
        ;;
esac

clear

# Instalación de drivers de WiFi
echo -e "${GREEN}| Instalando drivers de WiFi: $DRIVER_WIFI |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_WIFI" in
    "Ninguno")
        echo "Sin drivers de WiFi"
        ;;
    "Open Source")
        chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        ;;
    "broadcom-wl")
        chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S broadcom-wl networkmanager --noconfirm"
        ;;
    "Realtek")
        chroot /mnt /bin/bash -c "pacman -S networkmanager --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wpa_supplicant --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S wireless_tools --noconfirm"
        chroot /mnt /bin/bash -c "pacman -S iw --noconfirm"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtl8821cu-dkms-git --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtl8821ce-dkms-git --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S rtw88-dkms-git --noansweredit --noconfirm --needed"
        ;;
esac

clear

# Instalación de drivers de Bluetooth
echo -e "${GREEN}| Instalando drivers de Bluetooth: $DRIVER_BLUETOOTH |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_BLUETOOTH" in
    "Ninguno")
        echo "Sin soporte Bluetooth"
        ;;
    "bluetoothctl (terminal)")
        chroot /mnt /bin/bash -c "pacman -S bluez bluez-utils --noconfirm"
        chroot /mnt /bin/bash -c "systemctl enable bluetooth" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
        ;;
    "blueman (Graphical)")
        chroot /mnt /bin/bash -c "pacman -S bluez bluez-utils blueman --noconfirm"
        chroot /mnt /bin/bash -c "systemctl enable bluetooth" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
        ;;
esac

sleep 2
clear


# Instalación de herramientas de red
echo -e "${GREEN}| Instalando herramientas de red |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""
chroot /mnt /bin/bash -c "pacman -S dhcp dhcpcd dhclient networkmanager wpa_supplicant --noconfirm"
# Deshabilitar dhcpcd para evitar conflictos con NetworkManager
chroot /mnt /bin/bash -c "systemctl enable dhcpcd" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
chroot /mnt /bin/bash -c "systemctl enable NetworkManager" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
clear

# Copiado de archivos de configuración
echo -e "${GREEN}| Copiando archivos de configuración |${NC}"
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
        CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}3)
    else
        CRYPT_UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2)
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
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}1) /boot/efi vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=ascii,shortname=mixed,utf8,errors=remount-ro 0 2" >> /mnt/etc/fstab
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}2) /boot ext4 rw,relatime 0 2" >> /mnt/etc/fstab
    else
        echo "UUID=$(blkid -s UUID -o value ${SELECTED_DISK}1) /boot ext4 rw,relatime 0 2" >> /mnt/etc/fstab
    fi
    echo "/dev/mapper/vg0-swap none swap defaults 0 0" >> /mnt/etc/fstab

    # Regenerar initramfs después de todas las configuraciones
    echo -e "${CYAN}Regenerando initramfs con configuración LVM...${NC}"
    chroot /mnt /bin/bash -c "mkinitcpio -P"

    # Regenerar configuración de GRUB con parámetros LVM
    echo -e "${CYAN}Regenerando configuración de GRUB...${NC}"
    chroot /mnt /bin/bash -c "grub-mkconfig -o /boot/grub/grub.cfg"

    sleep 2
fi

# Configuración adicional para BTRFS
if [ "$PARTITION_MODE" = "auto_btrfs" ]; then
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
    chroot /mnt /bin/bash -c "pacman -S --needed --noconfirm btrfs-progs-git grub-btrfs" 2>/dev/null || \
    chroot /mnt /bin/bash -c "pacman -S --needed --noconfirm grub-btrfs" 2>/dev/null || true

    # Habilitar servicios de mantenimiento BTRFS
    echo -e "${CYAN}Configurando servicios de mantenimiento BTRFS...${NC}"
    chroot /mnt /bin/bash -c "systemctl enable btrfs-scrub@-.timer" 2>/dev/null || echo -e "${YELLOW}Warning: btrfs-scrub timer no disponible${NC}"
    chroot /mnt /bin/bash -c "systemctl enable fstrim.timer" || echo -e "${RED}ERROR: Falló habilitar fstrim.timer${NC}"

    # Configurar snapshots automáticos si snapper está disponible
    if chroot /mnt /bin/bash -c "pacman -Qq snapper" 2>/dev/null; then
        echo -e "${CYAN}Configurando Snapper para snapshots automáticos...${NC}"
        chroot /mnt /bin/bash -c "snapper -c root create-config /" || echo -e "${YELLOW}Warning: No se pudo crear config de snapper${NC}"
        chroot /mnt /bin/bash -c "systemctl enable snapper-timeline.timer snapper-cleanup.timer" || echo -e "${YELLOW}Warning: Falló habilitar servicios de snapper${NC}"
    fi

    # Optimizar fstab para BTRFS
    echo -e "${CYAN}Optimizando fstab para BTRFS...${NC}"
    chroot /mnt /bin/bash -c "sed -i 's/relatime/noatime/g' /etc/fstab"

    # Agregar opciones de montaje optimizadas si no están presentes
    chroot /mnt /bin/bash -c "sed -i 's/subvol=@/subvol=@,compress=zstd:3,space_cache=v2,autodefrag/' /etc/fstab" 2>/dev/null || true

    # Verificar configuración final de fstab
    echo -e "${CYAN}Verificando configuración final de fstab...${NC}"
    if chroot /mnt /bin/bash -c "mount -a --fake" 2>/dev/null; then
        echo -e "${GREEN}✓ Configuración fstab válida${NC}"
    else
        echo -e "${YELLOW}Warning: Posibles issues en fstab, pero continuando...${NC}"
    fi

    # Crear script de mantenimiento BTRFS
    echo -e "${CYAN}Creando script de mantenimiento BTRFS...${NC}"
    cat > /mnt/usr/local/bin/btrfs-maintenance << 'EOF'
#!/bin/bash
# Script de mantenimiento BTRFS automático

echo "Iniciando mantenimiento BTRFS..."

# Balance mensual (solo si es necesario)
if [ $(date +%d) -eq 01 ]; then
    echo "Ejecutando balance BTRFS..."
    btrfs balance start -dusage=50 -musage=50 / 2>/dev/null || true
fi

# Scrub semanal
if [ $(date +%w) -eq 0 ]; then
    echo "Ejecutando scrub BTRFS..."
    btrfs scrub start / 2>/dev/null || true
fi

# Desfragmentación ligera
echo "Ejecutando desfragmentación básica..."
find /home -type f -size +100M -exec btrfs filesystem defragment {} \; 2>/dev/null || true

echo "Mantenimiento BTRFS completado."
EOF

    chmod +x /mnt/usr/local/bin/btrfs-maintenance

    # Crear servicio systemd para el mantenimiento
    cat > /mnt/etc/systemd/system/btrfs-maintenance.service << 'EOF'
[Unit]
Description=BTRFS Maintenance
After=local-fs.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/btrfs-maintenance
User=root

[Install]
WantedBy=multi-user.target
EOF

    cat > /mnt/etc/systemd/system/btrfs-maintenance.timer << 'EOF'
[Unit]
Description=Run BTRFS Maintenance Weekly
Requires=btrfs-maintenance.service

[Timer]
OnCalendar=weekly
Persistent=true

[Install]
WantedBy=timers.target
EOF

    chroot /mnt /bin/bash -c "systemctl enable btrfs-maintenance.timer" || echo -e "${YELLOW}Warning: No se pudo habilitar btrfs-maintenance.timer${NC}"

    echo -e "${GREEN}✓ Configuración BTRFS completada${NC}"
    sleep 2
fi

clear
# Actualizar base de datos de paquetes
chroot /mnt /bin/bash -c "pacman -Sy"

clear

# Instalación de entorno de escritorio/gestor de ventanas
echo -e "${GREEN}| Configurando entorno gráfico: $INSTALLATION_TYPE |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$INSTALLATION_TYPE" in
    "TERMINAL")
        echo -e "${CYAN}Instalación solo terminal - No se instalará entorno gráfico${NC}"
        ;;
    "DESKTOP")
        echo -e "${GREEN}Instalando entorno de escritorio: $DESKTOP_ENVIRONMENT${NC}"

        # Instalar X.org como base para todos los escritorios
        echo -e "${CYAN}Instalando servidor X.org...${NC}"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-server --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-server-common --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xinit --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xauth --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xsetroot --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xrandr --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-setxkbmap --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xrdb --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xwayland --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbs --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbnailer --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S freetype2 --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S poppler-glib --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S libgsf --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S libnotify --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S tumbler --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdk-pixbuf2 --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S fontconfig --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S gvfs --noansweredit --noconfirm --needed"

        case "$DESKTOP_ENVIRONMENT" in
            "GNOME")
                echo -e "${CYAN}Instalando GNOME Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-session --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-settings-daemon --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-shell --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-control-center --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S nautilus --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gvfs gvfs-goa --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-console --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-text-editor --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-calculator --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-system-monitor --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-disk-utility --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S baobab --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S dconf-editor --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-themes-extra --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-tweaks --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-backgrounds --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-keyring --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-user-docs --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-software --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-gnome --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-shell-extensions --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-browser-connector --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                echo "Installing extension-manager..."
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S extension-manager --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable gdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

                ;;
            "BUDGIE")
                echo -e "${CYAN}Instalando Budgie Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-desktop --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-extras --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-desktop-view --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S budgie-backgrounds --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S materia-gtk-theme --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papirus-icon-theme --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S nautilus --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gvfs gvfs-goa --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-console --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "CINNAMON")
                echo -e "${CYAN}Instalando Cinnamon Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S cinnamon --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S cinnamon-translations --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S engrampa --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gvfs-smb --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S bibata-cursor-theme --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hicolor-icon-theme --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mint-backgrounds --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mint-themes --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mint-x-icons --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mint-y-icons --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mintlocale --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S cinnamon-control-center --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xed --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-console --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-screenshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-keyring --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "CUTEFISH")
                echo -e "${CYAN}Instalando CUTEFISH Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S cutefish --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S polkit-kde-agent --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-console --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm-kcm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable sddm"
                ;;
            "UKUI")
                echo -e "${CYAN}Instalando UKUI Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S ukui --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-keyring --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "PANTHEON")
                echo -e "${CYAN}Instalando PANTHEON Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S pantheon --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gnome-console --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-pantheon-greeter --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                chroot /mnt /bin/bash -c "pacman -Rdd orca onboard --noconfirm"
                sed -i '$d' /mnt/etc/lightdm/Xsession
                sed -i '$a io.elementary.wingpanel &\nplank &\nexec gala' /mnt/etc/lightdm/Xsession
                ;;
            "ENLIGHTENMENT")
                echo -e "${CYAN}Instalando Enlightenment Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S enlightenment --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S terminology --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S evisum --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S econnman --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "KDE")
                echo -e "${CYAN}Instalando KDE Plasma Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-desktop --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-workspace --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-systemmonitor --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-framework5 --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kwin --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S systemsettings --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S discover --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S flatpak --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S breeze --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S polkit-kde-agent --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S powerdevil --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-pa --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-nm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S konsole --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S dolphin --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kate --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S spectacle --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S ark --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kcalc --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S gwenview --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S okular --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kdeconnect --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kde-gtk-config --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kdeplasma-addons --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kdegraphics-thumbnailers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kscreen --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S kinfocenter --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S breeze-gtk --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-kde --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbs --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-wayland-session --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S plasma-x11-session --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm-kcm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable sddm"
                ;;
            "LXDE")
                echo -e "${CYAN}Instalando LXDE Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxde --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxde-common --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxsession --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxappearance --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxappearance-obconf --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxpanel --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "LXQT")
                echo -e "${CYAN}Instalando LXQt Desktop...${NC}"
                #chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxqt --noansweredit --noconfirm --needed"
                #chroot /mnt /bin/bash -c "sudo -u $USER yay -S breeze-icons --noansweredit --noconfirm --needed"
                #chroot /mnt /bin/bash -c "sudo -u $USER yay -S nm-tray --noansweredit --noconfirm --needed"
                #chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxqt-wayland-session --noansweredit --noconfirm --needed"
                #chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noansweredit --noconfirm --needed"
                #chroot /mnt /bin/bash -c "systemctl enable sddm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                # Instalar compositor
                chroot /mnt /bin/bash -c "pacman -S labwc --noconfirm"
                # Dependencias base
                chroot /mnt /bin/bash -c "pacman -S wayland wlroots --noconfirm"
                chroot /mnt /bin/bash -c "pacman -S xdg-desktop-portal-wlr --noconfirm"
                # LXQt y componentes
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxqt --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxqt-wayland-session --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S breeze-icons --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S leafpad --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S slock --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S nm-tray --noconfirm --needed"
                # Display manager
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sddm --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable sddm"
                # Herramientas adicionales
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S qterminal wofi --noconfirm --needed"
                ;;
            "MATE")
                echo -e "${CYAN}Instalando MATE Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-extra --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-applet-dock --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-menu --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-tweak --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S brisk-menu --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-control-center --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S clapper --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-power-manager --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mate-themes --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            "XFCE4")
                echo -e "${CYAN}Instalando XFCE4 Desktop...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4 --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4-goodies --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S loupe --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S showtime --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S papers --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S pavucontrol --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S gnome-keyring --noconfirm"
                chroot /mnt /bin/bash -c "pacman -S light-locker --noconfirm"
                chroot /mnt /bin/bash -c "pacman -S xfce4-screensaver --noconfirm"
                xscreensaver
                # Instalar compositor
                chroot /mnt /bin/bash -c "pacman -S labwc --noconfirm"
                # Dependencias base
                chroot /mnt /bin/bash -c "pacman -S wayland wlroots --noconfirm"
                chroot /mnt /bin/bash -c "pacman -S xdg-desktop-portal-wlr --noconfirm"
                # lightdm
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-slick-greeter --noansweredit --noconfirm --needed"
                sed -i 's/^#greeter-session=example-gtk-gnome$/greeter-session=lightdm-slick-greeter/' /mnt/etc/lightdm/lightdm.conf
                cp /home/arcris/.config/xfce4/backgroundarch.jpg /mnt/usr/share/pixmaps/backgroundarch.jpge
                chroot /mnt /bin/bash -c "sudo -u $USER touch /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "[Greeter]" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "background=/usr/share/pixmaps/backgroundarch.jpge" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "theme-name=Adwaita-dark" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER echo "clock-format=%b %e %H:%M" >> /etc/lightdm/slick-greeter.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S accountsservice --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S mugshot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
            *)
                echo -e "${YELLOW}Entorno de escritorio no reconocido: $DESKTOP_ENVIRONMENT${NC}"
                echo -e "${CYAN}Instalando XFCE4 como alternativa...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4 --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xfce4-goodies --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lightdm-gtk-greeter --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "systemctl enable lightdm" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
                ;;
        esac
        ;;
    "WINDOW_MANAGER")
        echo -e "${GREEN}Instalando gestor de ventanas: $WINDOW_MANAGER${NC}"

        # Instalar X.org y dependencias base para gestores de ventanas
        echo -e "${CYAN}Instalando servidor X.org y dependencias base...${NC}"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-server --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xinit --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xauth --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xrandr --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xsel --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S xterm --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S dmenu --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S wofi --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nemo --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S dunst --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nano  --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S vim --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S pulseaudio  --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S pavucontrol --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S nitrogen  --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S feh --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S network-manager-applet --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S lm_sensors --noansweredit --noconfirm --needed"

        chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbs --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S ffmpegthumbnailer --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S freetype2 --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S poppler-glib --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S libgsf --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S tumbler --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S gdk-pixbuf2 --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S fontconfig --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S gvfs --noansweredit --noconfirm --needed"

        # Instalar herramientas adicionales para gestores de ventanas
        echo -e "${CYAN}Instalando Terminales...${NC}"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S alacritty --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S kitty --noansweredit --noconfirm --needed"


        # Instalar Ly display manager
        echo -e "${CYAN}Instalando Ly display manager...${NC}"
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S ly --noansweredit --noconfirm --needed"
        chroot /mnt /bin/bash -c "systemctl enable ly" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        case "$WINDOW_MANAGER" in
            "I3WM"|"I3")
                echo -e "${CYAN}Instalando i3 Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3-wm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3status --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3lock --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3blocks --noansweredit --noconfirm --needed"
                # Crear configuración básica de i3
                mkdir -p /mnt/home/$USER/.config/i3
                chroot /mnt /bin/bash -c "install -Dm644 /etc/i3/config /home/$USER/.config/i3/config"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "AWESOME")
                echo -e "${CYAN}Instalando Awesome Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S awesome --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S vicious --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S slock --noansweredit --noconfirm --needed"
                # Crear configuración básica de awesome
                mkdir -p /mnt/home/$USER/.config/awesome
                chroot /mnt /bin/bash -c "install -Dm755 /etc/xdg/awesome/rc.lua /home/$USER/.config/awesome/rc.lua"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "BSPWM")
                echo -e "${CYAN}Instalando BSPWM Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S bspwm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sxhkd --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S slock --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S polybar --noansweredit --noconfirm --needed"
                # Crear configuración básica de bspwm
                mkdir -p /mnt/home/$USER/.config/bspwm
                mkdir -p /mnt/home/$USER/.config/sxhkd
                mkdir -p /mnt/home/$USER/.config/polybar/
                chroot /mnt /bin/bash -c "install -Dm755 /usr/share/doc/bspwm/examples/bspwmrc /home/$USER/.config/bspwm/bspwmrc"
                chroot /mnt /bin/bash -c "install -Dm644 /usr/share/doc/bspwm/examples/sxhkdrc /home/$USER/.config/sxhkd/sxhkdrc"
                chroot /mnt /bin/bash -c "install -Dm644 /etc/polybar/config.ini /home/$USER/.config/polybar/config.ini"
                chroot /mnt /bin/bash -c "echo polybar >> /home/$USER/.config/bspwm/bspwmrc"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "DWM")
                echo -e "${CYAN}Instalando DWM Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S dwm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S st --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S slock --noansweredit --noconfirm --needed"
                ;;
            "DWL")
                echo -e "${CYAN}Instalando DWL Wayland Compositor...${NC}"

                # Instalar dependencias necesarias
                echo -e "${YELLOW}Instalando dependencias...${NC}"
                chroot /mnt /bin/bash -c "pacman -S base-devel --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wlroots0.18 --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S tllist --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S foot --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S mako --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wl-clipboard --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S jq --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S git --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wayland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wayland-protocols --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S pixman --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S libxkbcommon-x11 --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S libxkbcommon --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S slurp --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S grim --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wofi --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S waybar --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S libinput --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S pkg-config --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S fcft --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S pixman --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S wbg --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "pacman -S dwl --noansweredit --noconfirm --needed"

                # Instalar DWL desde AUR
                chroot /mnt /bin/bash -c "sudo -u $USER git clone https://github.com/dcalonge/dwl ; cd dwl ; sudo -u $USER make install"

                # Crear directorio de configuración
                chroot /mnt /bin/bash -c "sudo -u $USER mkdir -p /home/$USER/.config/waybar"
                chroot /mnt /bin/bash -c "sudo -u $USER mkdir -p /home/$USER/.config/foot"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"

                echo -e "${GREEN}DWL instalado correctamente!${NC}"
                ;;
            "HYPRLAND")
                echo -e "${CYAN}Instalando Hyprland Window Manager...${NC}"
                chroot /mnt /bin/bash -c "pacman -S wayland wlroots --noconfirm"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S waybar --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S wofi --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S nwg-displays --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-wlr --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-hyprland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-gtk --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprpaper --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprpicker --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hypridle  --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprcursor --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprpolkitagent --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S hyprsunset --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S grim --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S qt5-wayland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S qt6-wayland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xdg-desktop-portal-hyprland --noansweredit --noconfirm --needed"
                # Crear configuración básica de hyprland
                mkdir -p /mnt/home/$USER/.config/hypr
                chroot /mnt /bin/bash -c "install -Dm644 /usr/share/hypr/hyprland.conf /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "echo exec-once = waybar >> /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "echo exec-once = systemctl --user start hyprpolkitagent >> /home/$USER/.config/hypr/hyprland.conf"
                chroot /mnt /bin/bash -c "sudo -u $USER hyprctl keyword input:kb_layout $KEYBOARD_LAYOUT"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "OPENBOX")
                echo -e "${CYAN}Instalando Openbox Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S openbox --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxappearance-obconf --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxinput --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S lxrandr --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S archlinux-xdg-menu --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S menumaker --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S obmenu-generator --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S tint2 --noansweredit --noconfirm --needed"
                # Crear configuración básica de openbox
                mkdir -p /mnt/home/$USER/.config/openbox
                chroot /mnt /bin/bash -c "cp -a /etc/xdg/openbox /home/$USER/.config/"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "QTITLE"|"QTILE")
                echo -e "${CYAN}Instalando Qtile Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S qtile --noansweredit --noconfirm --needed"
                python-pywlroots
                # Crear configuración básica de qtile
                mkdir -p /mnt/home/$USER/.config/qtile
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "SWAY")
                echo -e "${CYAN}Instalando Sway Window Manager...${NC}"
                chroot /mnt /bin/bash -c "pacman -S wayland wlroots --noconfirm"
                chroot /mnt /bin/bash -c "pacman -S xdg-desktop-portal-wlr --noconfirm"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S sway --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xorg-xwayland --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S slurp --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S pavucontrol --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S brightnessctl --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S swaylock --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S swayidle --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S swaybg --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S wmenu --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S waybar --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S grim --noansweredit --noconfirm --needed"
                # Crear configuración básica de sway
                mkdir -p /mnt/home/$USER/.config/sway
                chroot /mnt /bin/bash -c "install -Dm644 /etc/sway/config /home/$USER/.config/sway/config"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            "XMONAD")
                echo -e "${CYAN}Instalando XMonad Window Manager...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmonad --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmonad-contrib --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S xmobar --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S ghc --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S cabal-install --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S nitrogen --noansweredit --noconfirm --needed"
                # Crear configuración básica de xmonad
                mkdir -p /mnt/home/$USER/.config/xmonad
                guardar_configuraciones_xmonad
                chroot /mnt /bin/bash -c "sudo -u $USER xmonad --recompile /home/$USER/.config/xmonad/xmonad.hs"
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
            *)
                echo -e "${YELLOW}Gestor de ventanas no reconocido: $WINDOW_MANAGER${NC}"
                echo -e "${CYAN}Instalando i3 como alternativa...${NC}"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3-wm --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3status --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3lock --noansweredit --noconfirm --needed"
                chroot /mnt /bin/bash -c "sudo -u $USER yay -S i3blocks --noansweredit --noconfirm --needed"
                mkdir -p /mnt/home/$USER/.config/i3
                echo "# i3 config file" > /mnt/home/$USER/.config/i3/config
                chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config"
                ;;
        esac



        # Configurar terminales con configuraciones básicas
        echo -e "${CYAN}Configurando terminales...${NC}"

        # Configuración básica para Kitty
        mkdir -p /mnt/home/$USER/.config/kitty
        cat > /mnt/home/$USER/.config/kitty/kitty.conf << 'EOF'
# Font settings
font_family      JetBrains Mono
bold_font        auto
italic_font      auto
bold_italic_font auto
font_size 12.0

# Cursor
cursor_shape block
cursor_beam_thickness 1.5
cursor_underline_thickness 2.0
cursor_blink_interval 0

# Scrollback
scrollback_lines 2000
scrollback_pager less --chop-long-lines --RAW-CONTROL-CHARS +INPUT_LINE_NUMBER
scrollback_pager_history_size 0
wheel_scroll_multiplier 5.0
touch_scroll_multiplier 1.0

# Mouse
mouse_hide_wait 3.0
url_color #0087bd
url_style curly
open_url_modifiers kitty_mod
open_url_with default
url_prefixes http https file ftp gemini irc gopher mailto news git
detect_urls yes

# Performance tuning
repaint_delay 10
input_delay 3
sync_to_monitor yes

# Terminal bell
enable_audio_bell no
visual_bell_duration 0.0
window_alert_on_bell yes
bell_on_tab yes
command_on_bell none

# Window layout
remember_window_size  yes
initial_window_width  640
initial_window_height 400
enabled_layouts *
window_resize_step_cells 2
window_resize_step_lines 2
window_border_width 0.5pt
draw_minimal_borders yes
window_margin_width 0
single_window_margin_width -1
window_padding_width 0
placement_strategy center
active_border_color #00ff00
inactive_border_color #cccccc
bell_border_color #ff5a00
inactive_text_alpha 1.0

# Tab bar
tab_bar_edge bottom
tab_bar_margin_width 0.0
tab_bar_style fade
tab_bar_min_tabs 2
tab_switch_strategy previous
tab_fade 0.25 0.5 0.75 1
tab_separator " ┇"
tab_title_template "{title}"
active_tab_title_template none
active_tab_foreground   #000
active_tab_background   #eee
active_tab_font_style   bold-italic
inactive_tab_foreground #444
inactive_tab_background #999
inactive_tab_font_style normal

# Color scheme (Catppuccin Mocha)
foreground              #CDD6F4
background              #1E1E2E
selection_foreground    #1E1E2E
selection_background    #F5E0DC

# Cursor colors
cursor                  #F5E0DC
cursor_text_color       #1E1E2E

# URL underline color when hovering with mouse
url_color               #F5E0DC

# Kitty window border colors
active_border_color     #B4BEFE
inactive_border_color   #6C7086
bell_border_color       #F9E2AF

# OS Window titlebar colors
wayland_titlebar_color system
macos_titlebar_color system

# Tab bar colors
active_tab_foreground   #11111B
active_tab_background   #CBA6F7
inactive_tab_foreground #CDD6F4
inactive_tab_background #181825
tab_bar_background      #11111B

# Colors for marks (marked text in the terminal)
mark1_foreground #1E1E2E
mark1_background #B4BEFE
mark2_foreground #1E1E2E
mark2_background #CBA6F7
mark3_foreground #1E1E2E
mark3_background #74C7EC

# The 16 terminal colors

# normal
color0 #45475A
color1 #F38BA8
color2 #A6E3A1
color3 #F9E2AF
color4 #89B4FA
color5 #F5C2E7
color6 #94E2D5
color7 #BAC2DE

# bright
color8  #585B70
color9  #F38BA8
color10 #A6E3A1
color11 #F9E2AF
color12 #89B4FA
color13 #F5C2E7
color14 #94E2D5
color15 #A6ADC8

# extended base16 colors
color16 #FAB387
color17 #F2CDCD

# Keyboard shortcuts
kitty_mod ctrl+shift

# Clipboard
map kitty_mod+c copy_to_clipboard
map kitty_mod+v paste_from_clipboard
map kitty_mod+s paste_from_selection
map shift+insert paste_from_selection
map kitty_mod+o pass_selection_to_program

# Scrolling
map kitty_mod+up        scroll_line_up
map kitty_mod+k         scroll_line_up
map kitty_mod+down      scroll_line_down
map kitty_mod+j         scroll_line_down
map kitty_mod+page_up   scroll_page_up
map kitty_mod+page_down scroll_page_down
map kitty_mod+home      scroll_home
map kitty_mod+end       scroll_end

# Window management
map kitty_mod+enter new_window
map kitty_mod+n new_os_window
map kitty_mod+w close_window
map kitty_mod+] next_window
map kitty_mod+[ previous_window
map kitty_mod+f move_window_forward
map kitty_mod+b move_window_backward
map kitty_mod+` move_window_to_top
map kitty_mod+r start_resizing_window
map kitty_mod+1 first_window
map kitty_mod+2 second_window
map kitty_mod+3 third_window
map kitty_mod+4 fourth_window
map kitty_mod+5 fifth_window
map kitty_mod+6 sixth_window
map kitty_mod+7 seventh_window
map kitty_mod+8 eighth_window
map kitty_mod+9 ninth_window
map kitty_mod+0 tenth_window

# Tab management
map kitty_mod+right next_tab
map kitty_mod+left  previous_tab
map kitty_mod+t     new_tab
map kitty_mod+q     close_tab
map kitty_mod+.     move_tab_forward
map kitty_mod+,     move_tab_backward
map kitty_mod+alt+t set_tab_title

# Layout management
map kitty_mod+l next_layout

# Font sizes
map kitty_mod+equal     change_font_size all +2.0
map kitty_mod+plus      change_font_size all +2.0
map kitty_mod+kp_add    change_font_size all +2.0
map kitty_mod+minus     change_font_size all -2.0
map kitty_mod+kp_subtract change_font_size all -2.0
map kitty_mod+backspace change_font_size all 0

# Select and act on visible text
map kitty_mod+e kitten hints
map kitty_mod+p>f kitten hints --type path --program -
map kitty_mod+p>shift+f kitten hints --type path
map kitty_mod+p>l kitten hints --type line --program -
map kitty_mod+p>w kitten hints --type word --program -
map kitty_mod+p>h kitten hints --type hash --program -
map kitty_mod+p>n kitten hints --type linenum

# Miscellaneous
map kitty_mod+f11    toggle_fullscreen
map kitty_mod+f10    toggle_maximized
map kitty_mod+u      kitten unicode_input
map kitty_mod+f2     edit_config_file
map kitty_mod+escape kitty_shell window

# Sending arbitrary text on key presses
map kitty_mod+alt+1 send_text all \x01
map kitty_mod+alt+2 send_text all \x02
map kitty_mod+alt+3 send_text all \x03

# You can use the special action no_op to unmap a keyboard shortcut that is
# assigned in the default configuration
map kitty_mod+space no_op

# You can combine multiple actions to be triggered by a single shortcut
map kitty_mod+e combine : clear_terminal scroll active : send_text normal,application \x0c
EOF

        # Establecer permisos correctos para las configuraciones
        chroot /mnt /bin/bash -c "chown -R $USER:$USER /home/$USER/.config/kitty"

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

# Instalación de aplicaciones adicionales basadas en configuración
echo -e "${GREEN}| Instalando aplicaciones adicionales |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

# Verificar si ESSENTIAL_APPS está habilitado
if [ "${ESSENTIAL_APPS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando shell del sistema: ${SYSTEM_SHELL:-bash}${NC}"

    case "${SYSTEM_SHELL:-bash}" in
        "bash")
            chroot /mnt /bin/bash -c "pacman -S bash bash-completion --noconfirm"
            chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
        "dash")
            chroot /mnt /bin/bash -c "pacman -S dash --noconfirm"
            chroot /mnt /bin/bash -c "chsh -s /bin/dash $USER"
            ;;
        "ksh")
            chroot /mnt /bin/bash -c "pacman -S ksh --noconfirm"
            chroot /mnt /bin/bash -c "chsh -s /usr/bin/ksh $USER"
            ;;
        "fish")
            chroot /mnt /bin/bash -c "pacman -S fish --noconfirm"
            chroot /mnt /bin/bash -c "chsh -s /usr/bin/fish $USER"
            ;;
        "zsh")
            chroot /mnt /bin/bash -c "pacman -S zsh --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S zsh-completions --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S zsh-syntax-highlighting --noconfirm"
            chroot /mnt /bin/bash -c "pacman -S zsh-autosuggestions --noconfirm"
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/home/$USER/.zshrc
            cp /usr/share/arcrisgui/data/config/zshrc /mnt/root/.zshrc
            chroot /mnt /bin/bash -c "chown $USER:$USER /home/$USER/.zshrc"
            chroot /mnt /bin/bash -c "chsh -s /bin/zsh $USER"
            ;;
        *)
            echo -e "${YELLOW}Shell no reconocida: ${SYSTEM_SHELL}, usando bash${NC}"
            chroot /mnt /bin/bash -c "pacman -S bash bash-completion --noconfirm"
            chroot /mnt /bin/bash -c "chsh -s /bin/bash $USER"
            ;;
    esac
    echo -e "${GREEN}✓ Shell del sistema configurada${NC}"
fi

# Verificar si FILESYSTEMS está habilitado
if [ "${FILESYSTEMS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de sistemas de archivos...${NC}"

    chroot /mnt /bin/bash -c "pacman -S android-file-transfer --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S android-tools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S android-udev --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S msmtp --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libmtp --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libcddb --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-afc --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-smb --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-gphoto2 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-mtp --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-goa --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-nfs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gvfs-google --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-libav --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S dosfstools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S f2fs-tools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S ntfs-3g --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S udftools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S nilfs-utils --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S polkit --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gpart --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S mtools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S cifs-utils --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S jfsutils --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S btrfs-progs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S xfsprogs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S reiserfsprogs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S e2fsprogs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S exfatprogs --noconfirm"

    echo -e "${GREEN}✓ Herramientas de sistemas de archivos instaladas${NC}"
fi

# Verificar si COMPRESSION está habilitado
if [ "${COMPRESSION_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando herramientas de compresión...${NC}"

    chroot /mnt /bin/bash -c "pacman -S xarchiver --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S unarchiver --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S binutils --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gzip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lha --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lrzip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lzip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lz4 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S p7zip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S tar --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S xz --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S bzip2 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lbzip2 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S arj --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lzop --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S cpio --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S unrar --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S unzip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S zstd --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S zip --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S unarj --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S dpkg --noconfirm"
    echo -e "${GREEN}✓ Herramientas de compresión instaladas${NC}"
fi

# Verificar si VIDEO_CODECS está habilitado
if [ "${VIDEO_CODECS_ENABLED:-false}" = "true" ]; then
    echo -e "${CYAN}Instalando codecs de video...${NC}"

    chroot /mnt /bin/bash -c "pacman -S ffmpeg --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S aom --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libde265 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S x265 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S x264 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libmpeg2 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S xvidcore --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libtheora --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libvpx --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S sdl --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gstreamer --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-plugins-bad --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-plugins-base --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-plugins-base-libs --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-plugins-good --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S gst-plugins-ugly --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S xine-lib --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libdvdcss --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libdvdread --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S dvd+rw-tools --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S lame --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S jasper --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libmng --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libraw --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libkdcraw --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S vcdimager --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S mpv --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S faac --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S faad2 --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S flac --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S opus --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libvorbis --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S wavpack --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libheif --noconfirm"
    chroot /mnt /bin/bash -c "pacman -S libavif --noconfirm"

    echo -e "${GREEN}✓ Codecs de video instalados${NC}"
fi

sleep 2
clear

echo -e "${GREEN}✓ Tipografías instaladas${NC}"
chroot /mnt pacman -S noto-fonts --noconfirm
chroot /mnt pacman -S noto-fonts-emoji --noconfirm
chroot /mnt pacman -S adobe-source-code-pro-fonts --noconfirm
chroot /mnt pacman -S ttf-cascadia-code --noconfirm
chroot /mnt pacman -S cantarell-fonts --noconfirm
chroot /mnt pacman -S ttf-roboto --noconfirm
chroot /mnt pacman -S ttf-ubuntu-font-family --noconfirm
chroot /mnt pacman -S gnu-free-fonts --noconfirm
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
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S $app --noansweredit --noconfirm --needed --overwrite '*'" || {
            echo -e "${YELLOW}⚠ No se pudo instalar $app, continuando...${NC}"
        }
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
        chroot /mnt /bin/bash -c "sudo -u $USER yay -S $program --noansweredit --noconfirm --needed --overwrite '*'" || {
            echo -e "${YELLOW}⚠ No se pudo instalar $program, continuando...${NC}"
        }
        sleep 2
    done

    echo -e "${GREEN}✓ Instalación de programas extra completada${NC}"
    echo ""
    sleep 2
fi

sleep 3
clear
cp /usr/share/arcrisgui/data/config/pacman-chroot.conf /mnt/etc/pacman.conf
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"
sleep 3
clear

echo ""
ls /mnt/home/$USER/
sleep 5
clear
# Revertir a configuración normal

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
echo -e "  Contraseña: ${GREEN}$PASSWORD_USER${NC}"
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
    echo -e "${GREEN}🔧 Comandos útiles post-instalación:${NC}"
    echo -e "${CYAN}  • Ver estado LVM: sudo vgdisplay && sudo lvdisplay${NC}"
    echo -e "${CYAN}  • Redimensionar particiones: sudo lvresize${NC}"
    echo -e "${CYAN}  • Backup adicional header: sudo cryptsetup luksHeaderBackup /dev/sdaX${NC}"
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════${NC}"
    echo ""
fi
