# Guía de drivers Intel en Arch Linux

Fuentes:
[Intel graphics](https://wiki.archlinux.org/title/Intel_graphics) ·
[Hybrid graphics](https://wiki.archlinux.org/title/Hybrid_graphics) ·
[NVIDIA Optimus](https://wiki.archlinux.org/title/NVIDIA_Optimus) ·
[PRIME](https://wiki.archlinux.org/title/PRIME) ·
[Vulkan](https://wiki.archlinux.org/title/Vulkan) ·
[OpenGL](https://wiki.archlinux.org/title/OpenGL) ·
[Hardware video acceleration](https://wiki.archlinux.org/title/Hardware_video_acceleration) ·
[GPGPU / OpenCL](https://wiki.archlinux.org/title/General-purpose_computing_on_graphics_processing_units#OpenCL)

---

## Resumen rápido

| Familia GPU | Generación GPU | Driver 3D | Vulkan | VA-API | OpenCL |
|---|---|---|---|---|---|
| Intel Arc (Alchemist/Battlemage) | Xe HPG / Xe2 | `mesa` | `vulkan-intel` | `intel-media-driver` | `intel-compute-runtime` |
| Core Ultra / Raptor Lake / Alder Lake | Gen 12 | `mesa` | `vulkan-intel` | `intel-media-driver` | `intel-compute-runtime` + `vpl-gpu-rt` |
| Tiger Lake / Rocket Lake | Gen 12 | `mesa` | `vulkan-intel` | `intel-media-driver` | `intel-compute-runtime` + `vpl-gpu-rt` |
| Ice Lake | Gen 11 | `mesa` | `vulkan-intel` | `intel-media-driver` | `intel-compute-runtime-legacy` (AUR) |
| Coffee Lake / Whiskey Lake / Comet Lake | Gen 9.5 | `mesa` | `vulkan-intel` | `intel-media-driver` + `libva-intel-driver` | `intel-compute-runtime-legacy` (AUR) |
| Skylake / Kaby Lake | Gen 9 | `mesa` | `vulkan-intel` | `intel-media-driver` + `libva-intel-driver` | `intel-compute-runtime-legacy` (AUR) |
| Broadwell / Haswell / Ivy Bridge | Gen 8 / 7 | `mesa` | parcial (Gen 8) | `libva-intel-driver` | `beignet` (AUR, deprecado) |
| Sandy Bridge / Ironlake | Gen 6 / 5 | `mesa` | No | `libva-intel-driver` | No |
| GMA 4500 y anterior (pre-Gen 6) | Gen 4 y anterior | `mesa-amber` | No | No | No |

> Intel Gen N hace referencia a la generación de la GPU, **no** del procesador. Un Intel de 12ª gen de CPU puede tener GPU Gen 12.
> PowerVR (GMA 3600) no tiene soporte en drivers open source.

---

## Instalación

### 1. Driver base — aceleración 3D (OpenGL)

```bash
# Gen 3 y posterior — recomendado para la inmensa mayoría
sudo pacman -S mesa lib32-mesa mesa-utils lib32-mesa-utils

# Gen 2 a Gen 7 (Ivy Bridge) o si se prefiere driver clásico
sudo pacman -S mesa-amber lib32-mesa-amber
```

#### DDX driver para Xorg

```bash
# Opción recomendada: el driver modesetting incluido en xorg-server
# No necesita instalación adicional — funciona automáticamente.

# Alternativa legacy (Gen 2 a Gen 9) — generalmente NO recomendado
# Puede causar tearing, freezes y problemas en Gen 10+
sudo pacman -S xf86-video-intel
```

> **Nota:** La mayoría de distros (Debian, Fedora, KDE, Mozilla) desaconsejan instalar `xf86-video-intel` y recomiendan usar el `modesetting` genérico. En Gen 11 y posterior `xf86-video-intel` directamente no tiene soporte y causa problemas graves.

### 2. Firmware del kernel

```bash
sudo pacman -S linux-firmware-intel
```

### 3. Vulkan

```bash
# Broadwell (Gen 8) y posterior
sudo pacman -S vulkan-icd-loader lib32-vulkan-icd-loader
sudo pacman -S vulkan-intel lib32-vulkan-intel
```

### 4. Aceleración de vídeo por hardware (VA-API)

```bash
sudo pacman -S libva lib32-libva                 # biblioteca VA-API base
sudo pacman -S libva-utils                        # proporciona vainfo
sudo pacman -S vdpauinfo                          # verificar VDPAU
sudo pacman -S libvdpau-va-gl                     # capa de traducción VDPAU → VA-API
```

Elegir el driver VA-API según generación:

```bash
# intel-media-driver (iHD) — Broadwell (Gen 8) y posterior — RECOMENDADO para Gen 8+
sudo pacman -S intel-media-driver

# libva-intel-driver (i965) — GMA 4500 hasta Coffee Lake (Gen 4.5 a Gen 9.5)
# También sirve como fallback para modelos más viejos en sistemas Gen 8+
sudo pacman -S libva-intel-driver
```

**Soporte VA-API por generación:**

| Driver VA-API | Generación | Decodificación |
|---|---|---|
| `intel-media-driver` (iHD) | Broadwell y posterior | H.264, H.265, VP8, VP9, AV1 (Tiger Lake+) |
| `libva-intel-driver` (i965) | GMA 4500 — Coffee Lake | H.264, MPEG-2, VC-1, JPEG, VP8, VP9 (parcial) |

Variable de entorno para forzar el driver (solo si no se detecta automáticamente):

```bash
# Para intel-media-driver
export LIBVA_DRIVER_NAME=iHD

# Para libva-intel-driver (legacy)
export LIBVA_DRIVER_NAME=i965
```

### 5. OpenCL

```bash
sudo pacman -S ocl-icd      # ICD loader
sudo pacman -S clinfo        # listar dispositivos OpenCL
```

| Implementación | Paquete | Generación |
|---|---|---|
| **NEO** (open source) | `intel-compute-runtime` | Gen 12 (Rocket/Tiger Lake) y posterior + Arc |
| **NEO legacy** (open source) | `intel-compute-runtime-legacy` (AUR) | Gen 8 (Broadwell) a Gen 11 (Ice Lake) |
| **Beignet** (deprecado) | `beignet` (AUR) | Gen 7 (Ivy Bridge) y posterior |

```bash
# Gen 12 y posterior / Intel Arc — recomendado
sudo pacman -S intel-compute-runtime

# Gen 8 a Gen 11 (Broadwell, Skylake, Kaby Lake, Ice Lake)
yay -S intel-compute-runtime-legacy
```

### 6. Intel VPL (Video Processing Library)

Intel VPL reemplaza a Intel Media SDK para Tiger Lake y posterior:

```bash
sudo pacman -S vpl-gpu-rt   # Tiger Lake y posterior
```

### 7. GuC / HuC — firmware de microcontroladores (Gen 9+)

GuC y HuC ofrecen offloading de tareas multimedia y scheduling a la GPU. El driver `xe` (Gen 12 nuevo) los activa por defecto. Para el driver `i915`:

```bash
# Asegurarse de tener el firmware instalado
sudo pacman -S linux-firmware-intel

# Activar GuC + HuC en /etc/modprobe.d/i915.conf:
# enable_guc=3  →  GuC submission + HuC firmware (Gen 9.5+ recomendado)
# enable_guc=2  →  Solo HuC (Alder Lake-S Desktop por defecto)
# enable_guc=1  →  Solo GuC submission
```

```
/etc/modprobe.d/i915.conf
options i915 enable_guc=3
```

Regenerar initramfs tras el cambio:

```bash
sudo mkinitcpio -P
```

Verificar tras reiniciar:

```bash
dmesg | grep -i -e 'huc' -e 'guc'
```

> **Aviso:** Activar GuC/HuC manualmente tainta el kernel. Desactivarlo si experimentas freezes al salir de hibernación.

---

## Verificación

```bash
# Verificar driver en uso
lspci -k -d ::03xx

# Verificar VA-API
vainfo

# Verificar Vulkan
vulkaninfo | grep -i "GPU\|intel\|device" | head -10

# Verificar OpenCL
clinfo | head -30

# Verificar OpenGL
glxinfo | grep "OpenGL renderer"
```

---

## Gráficos híbridos — Intel + NVIDIA (Optimus / PRIME)

Los portátiles con gráficos híbridos tienen la iGPU Intel siempre activa y la dGPU NVIDIA que se enciende solo cuando se necesita. En Linux la solución estándar es **PRIME**.

> Todos los métodos son mutuamente excluyentes. Si pruebas uno y cambias a otro, revierte los cambios del anterior primero.

### Instalar los drivers de ambas GPUs

```bash
# iGPU Intel (ver secciones anteriores de esta guía)
sudo pacman -S mesa lib32-mesa vulkan-intel lib32-vulkan-intel intel-media-driver linux-firmware-intel

# dGPU NVIDIA — instalar el driver correspondiente a tu generación
# Ver nvidia_drivers.md para elegir el paquete correcto. Ejemplo (Turing/Ampere/Ada):
sudo pacman -S nvidia-open nvidia-utils lib32-nvidia-utils

# Paquete auxiliar para PRIME offload con driver propietario NVIDIA
sudo pacman -S nvidia-prime
```

---

### Opción A — Solo iGPU Intel (máximo ahorro de batería)

La GPU NVIDIA permanece completamente apagada. Opciones para desactivarla:

**1. BIOS/UEFI** — algunos fabricantes permiten desactivar la dGPU directamente. Es la opción más limpia.

**2. udev rules** — elimina el dispositivo NVIDIA del bus PCI al arrancar:

```
# /etc/modprobe.d/blacklist-nouveau.conf
blacklist nouveau
options nouveau modeset=0
```

```
# /etc/udev/rules.d/00-remove-nvidia.rules
ACTION=="add", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x03[0-9]*", ATTR{power/control}="auto", ATTR{remove}="1"
ACTION=="add", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x0c0330", ATTR{power/control}="auto", ATTR{remove}="1"
ACTION=="add", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x040300", ATTR{power/control}="auto", ATTR{remove}="1"
```

Reiniciar y verificar con `lspci` que la GPU NVIDIA no aparece.

> Si tras desactivarla el consumo sigue alto, comprobar que `nouveau` está cargado: `lsmod | grep nouveau`. Nouveau gestiona el power management del dispositivo aunque no renderice.

---

### Opción B — PRIME offload (recomendado: iGPU por defecto, dGPU bajo demanda)

La iGPU Intel gestiona el display y las apps normales. La dGPU NVIDIA se activa solo para apps que lo necesiten.

#### Con driver propietario NVIDIA (`nvidia-prime`)

```bash
# Ejecutar una app en la GPU NVIDIA
prime-run glxinfo | grep "OpenGL renderer"
prime-run vulkaninfo

# O manualmente con variables de entorno
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia aplicacion

# En Steam: añadir al launch options del juego
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia %command%
```

#### Con drivers open source (AMD dGPU o NVIDIA con nouveau)

```bash
# DRI_PRIME=1 usa la GPU secundaria (discreta)
DRI_PRIME=1 glxinfo | grep "OpenGL renderer"
DRI_PRIME=1 aplicacion

# Para Vulkan también instalar:
sudo pacman -S vulkan-mesa-layers lib32-vulkan-mesa-layers
DRI_PRIME=1! vulkaninfo   # ! expone solo esa GPU a la app

# Juegos Windows con DXVK (Wine/Proton)
DXVK_FILTER_DEVICE_NAME="nombre de tu GPU" aplicacion
# Obtener el nombre exacto con: vulkaninfo | grep "deviceName"
```

#### Integración con entorno de escritorio (switcheroo-control)

```bash
sudo pacman -S switcheroo-control
sudo systemctl enable --now switcheroo-control.service
```

Con el servicio activo, GNOME, KDE Plasma, Cinnamon y Budgie permiten lanzar apps en la GPU dedicada desde el menú contextual del icono (clic derecho → "Ejecutar con GPU dedicada"). También respeta la clave `PrefersNonDefaultGPU=true` en archivos `.desktop`.

#### RTD3 — apagar la dGPU NVIDIA automáticamente cuando no se usa

Para Turing (RTX 20xx) y posterior con CPU Intel Coffee Lake o superior:

```
# /etc/udev/rules.d/80-nvidia-pm.rules
ACTION=="bind", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x030000", TEST=="power/control", ATTR{power/control}="auto"
ACTION=="bind", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x030200", TEST=="power/control", ATTR{power/control}="auto"
ACTION=="unbind", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x030000", TEST=="power/control", ATTR{power/control}="on"
ACTION=="unbind", SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ATTR{class}=="0x030200", TEST=="power/control", ATTR{power/control}="on"
```

```
# /etc/modprobe.d/nvidia-pm.conf
options nvidia NVreg_DynamicPowerManagement=0x02
# Para Ampere y posterior usar: NVreg_DynamicPowerManagement=0x03
```

Verificar estado de la GPU (suspended = apagada, 0W):

```bash
cat /sys/bus/pci/devices/0000:01:00.0/power/runtime_status
```

---

### Opción C — Solo dGPU NVIDIA (máximo rendimiento, más consumo)

Usar la GPU NVIDIA para todo el renderizado, incluyendo el display:

```
# /etc/X11/xorg.conf.d/10-nvidia-drm-outputclass.conf
Section "OutputClass"
    Identifier "intel"
    MatchDriver "i915"
    Driver "modesetting"
EndSection

Section "OutputClass"
    Identifier "nvidia"
    MatchDriver "nvidia-drm"
    Driver "nvidia"
    Option "AllowEmptyInitialConfiguration"
    Option "PrimaryGPU" "yes"
    ModulePath "/usr/lib/nvidia/xorg"
    ModulePath "/usr/lib/xorg/modules"
EndSection
```

Añadir al inicio de `~/.xinitrc` (o script de display manager):

```bash
xrandr --setprovideroutputsource modesetting NVIDIA-0
xrandr --auto
```

> Requiere DRM kernel mode setting activo en NVIDIA. Ver `nvidia_drivers.md`.

---

### Herramientas de gestión alternativas (AUR)

| Herramienta | Descripción |
|---|---|
| `optimus-manager` (AUR) | Cambia entre modos con un comando (requiere re-login). Soporta AMD+NVIDIA desde v1.4 |
| `envycontrol` (AUR) | Similar a optimus-manager, sin daemon ni configuración de GDM parcheado |
| `nvidia-xrun` (AUR) | Sesión X separada en otro TTY con GPU NVIDIA |

---

### Problemas comunes

**Apps tardan 30 segundos en abrir (Vulkan con dGPU apagada)**

Cuando la GPU NVIDIA está suspendida (RTD3), Vulkan intenta inicializarla y falla con timeout. Solución para Wayland:

```
# /etc/environment
__EGL_VENDOR_LIBRARY_FILENAMES=/usr/share/glvnd/egl_vendor.d/50_mesa.json
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/intel_icd.json
__GLX_VENDOR_LIBRARY_NAME=mesa
```

O simplemente vaciar `VK_DRIVER_FILES` si la NVIDIA permanece siempre apagada:

```bash
export VK_DRIVER_FILES=
```

**Tearing en el display**

Activar DRM kernel mode setting de NVIDIA (habilita PRIME sync automáticamente):

```bash
# Añadir a parámetros del kernel:
nvidia-drm.modeset=1
```

**Verificar qué GPU usa una app**

```bash
lsof +c0 /dev/nvidia*    # procesos usando GPU NVIDIA (no la despierta)
```

---

## Herramientas de monitorización

```bash
# Monitor de actividad de GPU Intel (Video, Render, etc.)
sudo pacman -S intel-gpu-tools       # proporciona intel_gpu_top (ejecutar como root)

# Monitor interactivo general (AMD, Intel, NVIDIA)
sudo pacman -S nvtop

# Herramientas de verificación
sudo pacman -S vulkan-tools          # vulkaninfo, vkcube
sudo pacman -S mesa-utils            # glxinfo, eglinfo, glxgears
sudo pacman -S clinfo                # información detallada de OpenCL
```
