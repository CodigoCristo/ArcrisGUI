{-
=============================================================================
               CONFIGURACIÓN XMONAD PARA ARCH LINUX
=============================================================================

🚀 ATAJOS DE TECLADO:

📱 APLICACIONES BÁSICAS:
  Mod + Enter        → Abrir terminal (alacritty)
  Mod + D            → Dmenu (lanzador básico)
  Mod + Shift + D    → Rofi (lanzador avanzado)
  Mod + W            → Firefox
  Mod + E            → Thunar (explorador de archivos)
  Mod + X            → XKill (matar ventana con cursor)

🖼️ CONTROL DE VENTANAS:
  Mod + Q            → Cerrar ventana actual
  Mod + R            → Reiniciar XMonad
  Mod + Ctrl + Q     → Salir completamente de XMonad

🧭 NAVEGACIÓN:
  Mod + J            → Foco ventana abajo
  Mod + K            → Foco ventana arriba
  Mod + M            → Foco ventana master
  Mod + Tab          → Siguiente ventana

🔄 MOVIMIENTO DE VENTANAS:
  Mod + Shift + J    → Mover ventana abajo
  Mod + Shift + K    → Mover ventana arriba
  Mod + Shift + Enter → Intercambiar con ventana master

📏 REDIMENSIONAR:
  Mod + H            → Encoger ventana master
  Mod + L            → Expandir ventana master
  Mod + ,            → Más ventanas en área master
  Mod + .            → Menos ventanas en área master

🎨 LAYOUTS:
  Mod + Space        → Cambiar layout
  Mod + Shift + Space → Reset layout
  Mod + T            → Quitar flotante (volver a tiling)
  Mod + B            → Ocultar/mostrar barra de estado

🏢 WORKSPACES:
  Mod + 1-9          → Cambiar a workspace 1-9
  Mod + Shift + 1-9  → Mover ventana a workspace 1-9

📷 CAPTURAS:
  Mod + Shift + S    → Captura con selección
  Print              → Captura de pantalla completa

🔊 MULTIMEDIA (Teclas especiales):
  Vol+/Vol-/Mute     → Control de volumen
  Brillo+/-          → Control de brillo (laptops)

🖱️ MOUSE:
  Mod + Clic Izq     → Elevar ventana
  Mod + Clic Medio   → Mover ventana flotante
  Mod + Clic Der     → Redimensionar ventana flotante

=============================================================================
-}

-- Configuración XMonad ultra simple para Arch Linux
-- Solo funciones básicas de xmonad, sin extensiones complicadas

import XMonad
import XMonad.Hooks.DynamicLog
import XMonad.Hooks.ManageDocks
import XMonad.Layout.Spacing
import XMonad.Util.Run
import Graphics.X11.ExtraTypes.XF86
import qualified XMonad.StackSet as W
import qualified Data.Map as M
import System.Exit

-- Configuración básica
myTerminal      = "alacritty"
myModMask       = mod4Mask  -- Tecla Windows
myBorderWidth   = 2
myWorkspaces    = ["1", "2", "3", "4", "5", "6", "7", "8", "9"]

-- Colores
myNormalBorderColor  = "#3c3836"
myFocusedBorderColor = "#fb4934"

-- Layouts básicos
myLayout = avoidStruts $ spacingWithEdge 3 $
    Tall 1 (3/100) (1/2) |||
    Mirror (Tall 1 (3/100) (1/2)) |||
    Full

-- Atajos de teclado modernos
myKeys conf@(XConfig {XMonad.modMask = modm}) = M.fromList $
    -- Aplicaciones básicas
    [ ((modm, xK_Return), spawn myTerminal)                    -- Mod + Enter: Terminal
    , ((modm, xK_d), spawn "dmenu_run")                        -- Mod + D: Dmenu
    , ((modm .|. shiftMask, xK_d), spawn "rofi -show drun")   -- Mod + Shift + D: Rofi

    -- Control de ventanas
    , ((modm, xK_q), kill)                                    -- Mod + Q: Cerrar ventana
    , ((modm, xK_r), spawn "xmonad --recompile; xmonad --restart") -- Mod + R: Reiniciar
    , ((modm .|. controlMask, xK_q), io exitSuccess)          -- Mod + Ctrl + Q: Salir

    -- Navegación
    , ((modm, xK_j), windows W.focusDown)                     -- Mod + J: Foco abajo
    , ((modm, xK_k), windows W.focusUp)                       -- Mod + K: Foco arriba
    , ((modm, xK_m), windows W.focusMaster)                   -- Mod + M: Foco master
    , ((modm, xK_Tab), windows W.focusDown)                   -- Mod + Tab: Siguiente ventana

    -- Movimiento de ventanas
    , ((modm .|. shiftMask, xK_j), windows W.swapDown)        -- Mod + Shift + J
    , ((modm .|. shiftMask, xK_k), windows W.swapUp)          -- Mod + Shift + K
    , ((modm .|. shiftMask, xK_Return), windows W.swapMaster) -- Mod + Shift + Enter

    -- Redimensionamiento
    , ((modm, xK_h), sendMessage Shrink)                      -- Mod + H: Encoger
    , ((modm, xK_l), sendMessage Expand)                      -- Mod + L: Expandir

    -- Layouts
    , ((modm, xK_space), sendMessage NextLayout)              -- Mod + Space: Siguiente layout
    , ((modm .|. shiftMask, xK_space), setLayout $ XMonad.layoutHook conf) -- Reset layout

    -- Floating
    , ((modm, xK_t), withFocused $ windows . W.sink)          -- Mod + T: Quitar float

    -- Master area
    , ((modm, xK_comma), sendMessage (IncMasterN 1))          -- Mod + ,: Más ventanas master
    , ((modm, xK_period), sendMessage (IncMasterN (-1)))      -- Mod + .: Menos ventanas master

    -- Aplicaciones útiles
    , ((modm, xK_w), spawn "firefox")                         -- Mod + W: Firefox
    , ((modm, xK_e), spawn "thunar")                          -- Mod + E: Explorador
    , ((modm, xK_x), spawn "xkill")                           -- Mod + X: Matar ventana

    -- Control de volumen
    , ((0, xF86XK_AudioRaiseVolume), spawn "pactl set-sink-volume @DEFAULT_SINK@ +5%")
    , ((0, xF86XK_AudioLowerVolume), spawn "pactl set-sink-volume @DEFAULT_SINK@ -5%")
    , ((0, xF86XK_AudioMute), spawn "pactl set-sink-mute @DEFAULT_SINK@ toggle")

    -- Brillo (para laptops)
    , ((0, xF86XK_MonBrightnessUp), spawn "brightnessctl set +10%")
    , ((0, xF86XK_MonBrightnessDown), spawn "brightnessctl set 10%-")

    -- Capturas de pantalla
    , ((modm .|. shiftMask, xK_s), spawn "scrot -s")          -- Mod + Shift + S: Captura selección
    , ((0, xK_Print), spawn "scrot")                          -- Print: Captura completa

    -- Toggle status bar
    , ((modm, xK_b), sendMessage ToggleStruts)                -- Mod + B: Toggle barra
    ]
    ++
    -- Workspaces 1-9
    [((m .|. modm, k), windows $ f i)
        | (i, k) <- zip (XMonad.workspaces conf) [xK_1 .. xK_9]
        , (f, m) <- [(W.greedyView, 0), (W.shift, shiftMask)]]

-- Mouse bindings modernos
myMouseBindings (XConfig {XMonad.modMask = modm}) = M.fromList $
    -- Mod + clic izquierdo: elevar ventana
    [ ((modm, button1), (\w -> focus w >> windows W.shiftMaster))

    -- Mod + clic medio: mover ventana flotante
    , ((modm, button2), (\w -> focus w >> mouseMoveWindow w >> windows W.shiftMaster))

    -- Mod + clic derecho: redimensionar ventana flotante
    , ((modm, button3), (\w -> focus w >> mouseResizeWindow w >> windows W.shiftMaster))
    ]

-- Reglas de ventanas
myManageHook = composeAll
    [ className =? "MPlayer"        --> doFloat
    , className =? "Gimp"           --> doFloat
    , className =? "Steam"          --> doFloat
    , className =? "Pavucontrol"    --> doFloat
    , className =? "Arandr"         --> doFloat
    , resource  =? "desktop_window" --> doIgnore
    , resource  =? "kdesktop"       --> doIgnore
    ]

-- Hook de inicio
myStartupHook = do
    spawn "feh --bg-scale /usr/share/pixmaps/backgroundarch.jpg &"
    spawn "picom &"
    spawn "nm-applet &"
    spawn "volumeicon &"

-- Configuración principal
main = do
    xmproc <- spawnPipe "xmobar"
    xmonad $ docks $ def
        { modMask            = myModMask
        , terminal           = myTerminal
        , borderWidth        = myBorderWidth
        , workspaces         = myWorkspaces
        , normalBorderColor  = myNormalBorderColor
        , focusedBorderColor = myFocusedBorderColor
        , keys               = myKeys
        , mouseBindings      = myMouseBindings
        , layoutHook         = myLayout
        , manageHook         = manageDocks <+> myManageHook
        , logHook            = dynamicLogWithPP xmobarPP
            { ppOutput = hPutStrLn xmproc
            , ppTitle = xmobarColor "green" "" . shorten 50
            , ppCurrent = xmobarColor "yellow" "" . wrap "[" "]"
            }
        , startupHook        = myStartupHook
        , focusFollowsMouse  = True
        , clickJustFocuses   = False
        }
