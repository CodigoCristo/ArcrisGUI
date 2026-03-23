# Guía de drivers AMD en Arch Linux

Fuentes:
[AMDGPU](https://wiki.archlinux.org/title/AMDGPU) ·
[ATI](https://wiki.archlinux.org/title/ATI) ·
[AMDGPU PRO](https://wiki.archlinux.org/title/AMDGPU_PRO) ·
[Vulkan](https://wiki.archlinux.org/title/Vulkan) ·
[OpenGL](https://wiki.archlinux.org/title/OpenGL) ·
[Hardware video acceleration](https://wiki.archlinux.org/title/Hardware_video_acceleration) ·
[GPGPU / OpenCL / ROCm](https://wiki.archlinux.org/title/General-purpose_computing_on_graphics_processing_units) ·
[DaVinci Resolve](https://wiki.archlinux.org/title/DaVinci_Resolve)

---

## Resumen rápido

| Familia GPU | Arquitectura | Kernel driver | Estado |
|---|---|---|---|
| RX 7000 / RX 9000 | RDNA 3 / RDNA 4 | `amdgpu` (automático) | Totalmente soportado |
| RX 6000 | RDNA 2 | `amdgpu` (automático) | Totalmente soportado |
| RX 5000 | RDNA 1 | `amdgpu` (automático) | Totalmente soportado |
| Vega / RX 500 / RX 400 | GCN 5 / GCN 4 / GCN 3 | `amdgpu` (automático) | Totalmente soportado |
| R9 200-300 / HD 7000 | GCN 2 (Sea Islands) | `amdgpu` con param. kernel | Ver nota SI/CIK |
| HD 7000 (GCN 1) | GCN 1 (Southern Islands) | `amdgpu` con param. kernel | Ver nota SI/CIK |
| HD 6000 / HD 5000 / HD 4000… | pre-GCN | `radeon` (ATI) | Ver sección ATI |

> **Nota:** En la inmensa mayoría de casos el driver `amdgpu` carga automáticamente. No hace falta configuración adicional salvo en tarjetas GCN 1/2 muy antiguas.

---

## Instalación (tarjetas GCN 3+ — lo habitual)

### 1. Driver base — aceleración 3D (OpenGL)

```bash
# Mesa — driver OpenGL y base de todo lo demás
sudo pacman -S mesa lib32-mesa mesa-utils lib32-mesa-utils

# Firmware del kernel para amdgpu (necesario para cargar correctamente)
sudo pacman -S linux-firmware-amdgpu

# DDX driver para Xorg (opcional — el modesetting genérico funciona en la mayoría de casos)
# Puede ser necesario para TearFree o en hardware antiguo GCN 1/2
sudo pacman -S xf86-video-amdgpu
```

### 2. Vulkan

```bash
sudo pacman -S vulkan-icd-loader lib32-vulkan-icd-loader
sudo pacman -S vulkan-radeon lib32-vulkan-radeon
```

### 3. OpenCL / ROCm

Primero instalar el ICD loader (necesario para que coexistan múltiples implementaciones):

```bash
sudo pacman -S ocl-icd          # ICD loader recomendado, más actualizado
sudo pacman -S clinfo           # listar plataformas y dispositivos OpenCL
```

Elegir **una** de las siguientes implementaciones:

| Implementación | Paquete | GPUs compatibles | Notas |
|---|---|---|---|
| **Rusticl** (Mesa) | `opencl-mesa` | Cualquier GPU con Mesa | Más amplio, activa con `RUSTICL_ENABLE=radeonsi` |
| **ROCm** (open source) | `rocm-opencl-runtime` | Polaris (RX 400/500) y posterior | Pre-Vega necesita `ROC_ENABLE_PRE_VEGA=1` |
| **ROCm + ORCA legacy** | `opencl-amd` (AUR) | GCN 1+ con soporte legacy | Equivale a `opencl=rocr,legacy` de AMD Ubuntu |
| **ORCA legacy** (solo pre-Vega) | `opencl-legacy-amdgpu-pro` (AUR) | RX 580 y similares | Solo para GPUs antiguas sin ROCm |

```bash
# Opción recomendada — Rusticl vía Mesa (cualquier GPU AMD con amdgpu)
sudo pacman -S opencl-mesa
# Activar (añadir a ~/.bashrc / ~/.zshrc):
export RUSTICL_ENABLE=radeonsi

# Opción ROCm — Polaris (RX 400/500) y posterior
sudo pacman -S rocm-opencl-runtime
# Pre-Vega (RX 400/500 series), añadir a ~/.bashrc / ~/.zshrc:
export ROC_ENABLE_PRE_VEGA=1

# Opción ROCm + legacy (DaVinci Resolve con RX 580, etc.)
yay -S opencl-amd
```

**Verificar soporte de imágenes OpenCL** (necesario para Darktable, etc.):

```bash
/opt/rocm/bin/clinfo | grep -i "image support"
```

#### ROCm y HIP

ROCm incluye HIP (Heterogeneous Interface for Portability), el equivalente AMD a CUDA:

```bash
sudo pacman -S rocm-hip-runtime rocm-hip-sdk
# Variables de entorno se configuran automáticamente vía /etc/profile.d/rocm.sh
```

> Verificar que la GPU aparece en ROCm: `/opt/rocm/bin/rocminfo`

> PyTorch con ROCm: `sudo pacman -S python-pytorch-rocm`

### 4. Aceleración de vídeo por hardware (VA-API)

```bash
sudo pacman -S libva lib32-libva                        # biblioteca VA-API base
sudo pacman -S libva-mesa-driver lib32-libva-mesa-driver # driver VA-API para amdgpu/radeonsi
sudo pacman -S libva-utils                              # proporciona vainfo
sudo pacman -S vdpauinfo                                # verificar VDPAU
sudo pacman -S libvdpau-va-gl                           # capa de traducción VDPAU → VA-API (OpenGL)
```

Variable mínima necesaria (añadir a `~/.bashrc` / `~/.zshrc`):

```bash
export LIBVA_DRIVER_NAME=radeonsi
```

**Soporte de decodificación VA-API por generación:**

| Codec | Desde |
|---|---|
| H.264 | Radeon HD 2000 y posterior |
| H.265 / HEVC 8bit | Radeon R9 Fury (Fiji) y posterior |
| H.265 / HEVC 10bit | Radeon RX 400 y posterior |
| VP9 8bit | Raven Ridge + RX 5000 y posterior |
| AV1 8bit/10bit | RX 6600 y posterior |

**Soporte de codificación VA-API por generación:**

| Codec | Desde |
|---|---|
| H.264 | Radeon HD 7000 y posterior |
| H.265 / HEVC 8bit | Radeon RX 400 y posterior |
| AV1 | RX 7900 y posterior |

---

## Nota para tarjetas GCN 1 (Southern Islands) y GCN 2 (Sea Islands)

Estas tarjetas no cargan `amdgpu` automáticamente. Añadir a los parámetros del kernel (en `/etc/default/grub` o equivalente):

```
# Southern Islands (GCN 1 — HD 7000 series)
radeon.si_support=0 amdgpu.si_support=1

# Sea Islands (GCN 2 — R9 200/300 series)
radeon.cik_support=0 amdgpu.cik_support=1
```

Añadir también `amdgpu` antes de `radeon` en `MODULES=()` de `/etc/mkinitcpio.conf` y regenerar el initramfs:

```bash
sudo mkinitcpio -P
```

---

## Tarjetas pre-GCN — driver ATI (`radeon`)

Para GPUs anteriores a GCN (HD 6000 series y más antiguas: HD 5000, HD 4000, R200, R100…) se usa el driver open source `radeon`, no `amdgpu`.

```bash
# Instalación base
sudo pacman -S mesa lib32-mesa mesa-utils lib32-mesa-utils

# GPUs R200 y anteriores (muy antiguas)
sudo pacman -S mesa-amber lib32-mesa-amber

# Firmware del kernel para el driver radeon
sudo pacman -S linux-firmware-radeon

# DDX driver Xorg para el driver radeon
sudo pacman -S xf86-video-ati
```

El driver `radeon` carga automáticamente. No necesita configuración adicional.

> No hay soporte de Vulkan ni ROCm en tarjetas pre-GCN.

---

## AMDGPU PRO — nota histórica

> **Estado: discontinuado.** AMD dejó de distribuir los componentes propietarios de AMDGPU PRO. El paquete AUR `amdgpu-pro-installer` está en estado final y sin mantenimiento activo.

AMDGPU PRO era un stack propietario que añadía sobre el driver open source `amdgpu`:

- **OpenGL PRO** (`amdgpu-pro-oglp`) — en benchmarks, `radeonsi` (Mesa) supera al driver propietario en prácticamente todos los juegos. No recomendado para uso general.
- **Vulkan PRO** (`vulkan-amdgpu-pro`) — requerido únicamente como dependencia de AMF.
- **AMF** (`amf-amdgpu-pro`) — framework de codificación/decodificación de AMD. Ahora disponible como paquete independiente en AUR: `amf-amdgpu-pro`.
- **OpenCL ORCA** (solo para Polaris/GCN 4) — reemplazado por ROCm en GPUs más modernas.

Para la gran mayoría de usuarios, el stack open source (`mesa` + `vulkan-radeon` + `rocm-opencl-runtime`) es más que suficiente y mejor mantenido.

---

## Verificación

```bash
# Verificar driver en uso
lspci -k -d ::03xx

# Verificar VA-API
vainfo

# Verificar Vulkan
vulkaninfo | grep -i "GPU\|radeon\|amd" | head -10

# Verificar OpenCL
clinfo | head -30

# Verificar OpenGL
glxinfo | grep "OpenGL renderer"
```

---

## Herramientas de monitorización

```bash
# Monitor interactivo en terminal (AMD, Intel, NVIDIA)
sudo pacman -S nvtop

# Monitor detallado específico para AMD (RDNA/GCN moderno)
sudo pacman -S amdgpu_top

# Monitor clásico para tarjetas radeon/amdgpu (más ligero)
sudo pacman -S radeontop

# Herramientas de verificación adicionales
sudo pacman -S vulkan-tools          # vulkaninfo, vkcube
sudo pacman -S mesa-utils            # glxinfo, eglinfo, glxgears
sudo pacman -S clinfo                # información detallada de OpenCL
```

Herramientas GUI opcionales (AUR):

```bash
yay -S lact        # GTK — monitorización y control de GPU AMD (recomendado)
yay -S corectrl    # Qt — perfiles de rendimiento por aplicación
```
