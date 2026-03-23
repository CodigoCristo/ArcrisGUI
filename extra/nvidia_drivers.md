# Guía de drivers Nvidia en Arch Linux

Fuente: [Arch Wiki — NVIDIA](https://wiki.archlinux.org/title/NVIDIA)

---

## Resumen rápido

| Familia GPU | Arquitectura | Driver principal | Alternativa | Estado |
|---|---|---|---|---|
| RTX 50xx / PRO 5000 | Blackwell (GB) | `nvidia-open` | — | Recomendado por upstream |
| RTX 40xx / RTX 30xx | Ada / Ampere | `nvidia-580xx-dkms` (AUR) | `nvidia-open` | Soportado |
| RTX 20xx / GTX 16xx | Turing | `nvidia-580xx-dkms` (AUR) | x (sin RTD3) | Soportado |
| GTX 10xx / GTX 900 / GTX 750 / Titan V | Pascal / Maxwell / Volta | `nvidia-580xx-dkms` (AUR) | — | Legacy, soportado |
| GTX 700 / GTX 600 | Kepler (GK) | `nvidia-470xx-dkms` (AUR) | — | Legacy, sin soporte |
| GTX 500 / GTX 400 | Fermi (GF) | `nvidia-390xx-dkms` (AUR) | — | Legacy |
| 300/200/100/9xx/8xx | Tesla (G80-GT200) | `nvidia-340xx-dkms` (AUR) | `nouveau` | Legacy |
| Curie (G70) y anterior | — | sin driver propietario | `nouveau` | Sin soporte |

> Turing (RTX 20xx / GTX 16xx): `nvidia-open` no tiene RTD3 → sin alternativa, usar solo `nvidia-580xx-dkms`.

---

## Instalación por driver

```bash
# Blackwell — Linux estándar - necesita linux-headers
sudo pacman -S nvidia-open nvidia-utils lib32-nvidia-utils opencl-nvidia lib32-opencl-nvidia nvidia-settings

# Blackwell — Linux-lts  - necesita linux-headers
sudo pacman -S nvidia-open-lts nvidia-utils lib32-nvidia-utils opencl-nvidia lib32-opencl-nvidia nvidia-settings

# Blackwell — Cualquier kernel - necesita linux-headers
sudo pacman -S nvidia-open-dkms nvidia-utils lib32-nvidia-utils opencl-nvidia lib32-opencl-nvidia nvidia-settings
```

```bash
# Ada / Ampere / Turing — driver principal
# Maxwell / Pascal / Volta — legacy
# 
# Cualquier kernel - necesita linux-headers
yay -S nvidia-580xx-dkms nvidia-580xx-utils lib32-nvidia-580xx-utils opencl-nvidia-580xx lib32-opencl-nvidia-580xx nvidia-580xx-settings
```

```bash
# Kepler — legacy sin soporte
# Cualquier kernel - necesita linux-headers
yay -S nvidia-470xx-dkms nvidia-470xx-utils lib32-nvidia-470xx-utils opencl-nvidia-470xx lib32-opencl-nvidia-470xx nvidia-470xx-settings mhwd-nvidia-470xx
```

```bash
# Fermi — legacy
yay -S nvidia-390xx-dkms nvidia-390xx-utils lib32-nvidia-390xx-utils opencl-nvidia-390xx lib32-opencl-nvidia-390xx nvidia-390xx-settings mhwd-nvidia-390xx
```

```bash
# Tesla — legacy
yay -S nvidia-340xx-dkms nvidia-340xx-utils lib32-nvidia-340xx-utils opencl-nvidia-340xx lib32-opencl-nvidia-340xx nvidia-340xx-settings
```

```bash
# Curie y anterior — open source
sudo pacman -S xf86-video-nouveau vulkan-nouveau lib32-vulkan-nouveau
```

---

### Aceleración de vídeo por hardware (VA-API / VDPAU / NVDEC)
```bash
# libva-nvidia-driver — VA-API vía CUDA/NVDEC (Fermi y posterior)
# Necesario para decodificación HW en mpv, Firefox, Chromium
yay -S libva-nvidia-driver

# Herramientas de verificación
sudo pacman -S libva-utils   # vainfo — verificar VA-API
sudo pacman -S vdpauinfo      # verificar VDPAU
```

> Con driver >= 580.105.08, añadir a `~/.bashrc` / `~/.zshrc`:
> ```bash
> export CUDA_DISABLE_PERF_BOOST=1   # evita mayor consumo que CPU con libva-nvidia
> export LIBVA_DRIVER_NAME=nvidia    # fuerza VA-API a usar el driver Nvidia
> export VDPAU_DRIVER=nvidia         # fuerza VDPAU a usar el driver Nvidia
> ```

**Soporte por API según generación:**

| API | Desde |
|---|---|
| VDPAU | GeForce 8 y posterior |
| NVDEC (decodificación HW) | Fermi y posterior |
| NVENC (codificación HW) | Kepler y posterior |
| Vulkan Video | Pascal y posterior |
| AV1 NVDEC | RTX 30xx (Ampere) y posterior |
| AV1 NVENC | RTX 40xx (Ada Lovelace) y posterior |

### Herramientas de monitorización y desarrollo
```bash
# nvtop — monitor interactivo GPU en terminal
sudo pacman -S nvtop

# Vulkan
sudo pacman -S mesa-utils vulkan-tools vulkan-icd-loader lib32-vulkan-icd-loader
```

### Verificar instalación completa
```bash
nvidia-smi                          # driver y GPU
vainfo                              # VA-API (necesita libva-nvidia-driver)
vdpauinfo                           # VDPAU
vulkaninfo --summary                # Vulkan
clinfo | grep "Device Name"         # OpenCL
```

---

## Blackwell (GB) — `nvidia-open`

> RTX 50xx Series · 2025 · Recomendado por upstream

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 5090 | GB202 | 2025 | `nvidia-open` | x |
| GeForce RTX 5090 D | GB202 | 2025 | `nvidia-open` | x |
| GeForce RTX 5080 | GB203 | 2025 | `nvidia-open` | x |
| GeForce RTX 5070 Ti | GB203 | 2025 | `nvidia-open` | x |
| GeForce RTX 5070 | GB205 | 2025 | `nvidia-open` | x |
| GeForce RTX 5060 Ti | GB206 | 2025 | `nvidia-open` | x |
| GeForce RTX 5060 | GB206 | 2025 | `nvidia-open` | x |
| GeForce RTX 5050 | GB207 | 2025 | `nvidia-open` | x |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 5090 Laptop GPU | GB203 | 2025 | `nvidia-open` | x |
| GeForce RTX 5080 Laptop GPU | GB203 | 2025 | `nvidia-open` | x |
| GeForce RTX 5070 Ti Laptop GPU | GB205 | 2025 | `nvidia-open` | x |
| GeForce RTX 5070 Laptop GPU | GB205 | 2025 | `nvidia-open` | x |
| GeForce RTX 5060 Laptop GPU | GB206 | 2025 | `nvidia-open` | x |
| GeForce RTX 5050 Laptop GPU | GB207 | 2025 | `nvidia-open` | x |

### Workstation Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| RTX PRO 6000 Blackwell | GB202 | 2025 | `nvidia-open` | x |
| RTX PRO 5000 Blackwell | GB203 | 2025 | `nvidia-open` | x |
| RTX PRO 4500 Blackwell | GB205 | 2025 | `nvidia-open` | x |
| RTX PRO 4000 Blackwell | GB206 | 2025 | `nvidia-open` | x |

### Workstation Mobile
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| RTX PRO 3000 Mobile | GB206 | 2025 | `nvidia-open` | x |
| RTX PRO 2000 Mobile | GB206 | 2025 | `nvidia-open` | x |
| RTX PRO 1000 Mobile | GB207 | 2025 | `nvidia-open` | x |
| RTX PRO 500 Mobile | GB207 | 2025 | `nvidia-open` | x |

---

## Ada Lovelace (AD) — `nvidia-580xx-dkms` (AUR)

> RTX 40xx Series · 2022–2024
> Alternativa: `nvidia-open` (sin RTD3 Power Management)

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 4090 | AD102 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4080 Super | AD103 | 2024 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4080 | AD103 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 Ti Super | AD103 | 2024 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 Ti | AD104 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 Super | AD104 | 2024 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 | AD104 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4060 Ti | AD106 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4060 | AD107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 4090 Laptop GPU | AD102 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4080 Laptop GPU | AD103 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 Ti Laptop GPU | AD104 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4070 Laptop GPU | AD106 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4060 Laptop GPU | AD106 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 4050 Laptop GPU | AD107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |

### Workstation
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| RTX 6000 Ada Generation | AD102 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 5000 Ada Generation | AD103 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 4500 Ada Generation | AD104 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 4000 Ada Generation | AD106 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 4000 SFF Ada Generation | AD106 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 3500 Ada Generation | AD106 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 3000 Ada Generation | AD107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX 2000 Ada Generation | AD107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |

---

## Ampere (GA) — `nvidia-580xx-dkms` (AUR)

> RTX 30xx Series · 2020–2022
> Alternativa: `nvidia-open` (posibles cuelgues en laptops)

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 3090 Ti | GA102 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3090 | GA102 | 2020 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3080 Ti | GA102 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3080 12GB | GA102 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3080 | GA102 | 2020 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3070 Ti | GA104 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3070 | GA104 | 2020 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3060 Ti | GA104 | 2020 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3060 | GA106 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3050 8GB | GA106 | 2022 | `nvidia-580xx-dkms` | `nvidia-open` |
| GeForce RTX 3050 6GB | GA107 | 2024 | `nvidia-580xx-dkms` | `nvidia-open` |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 3080 Ti Laptop GPU | GA103 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3080 Laptop GPU | GA104 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3070 Ti Laptop GPU | GA104 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3070 Laptop GPU | GA104 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3060 Laptop GPU | GA106 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3050 Ti Laptop GPU | GA107 | 2021 | `nvidia-580xx-dkms` | x |
| GeForce RTX 3050 Laptop GPU | GA107 | 2021 | `nvidia-580xx-dkms` | x |

### Workstation
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| RTX A6000 | GA102 | 2020 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A5000 | GA102 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A4000 | GA104 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A3000 | GA104 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A2000 | GA106 | 2021 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A1000 | GA107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |
| RTX A500 | GA107 | 2023 | `nvidia-580xx-dkms` | `nvidia-open` |

---

## Turing (TU) — `nvidia-580xx-dkms` (AUR)

> RTX 20xx / GTX 16xx Series · 2018–2020
> Alternativa: `nvidia-open`* (sin RTD3 Power Management)

### Desktop RTX
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 2080 Ti | TU102 | 2018 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2080 Super | TU104 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2080 | TU104 | 2018 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2070 Super | TU104 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2070 | TU106 | 2018 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2060 Super | TU106 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2060 | TU106 | 2019 | `nvidia-580xx-dkms` | x |

### Desktop GTX
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 1660 Ti | TU116 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1660 Super | TU116 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1660 | TU116 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1650 Super | TU116 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1650 | TU117 | 2019 | `nvidia-580xx-dkms` | x |

### Laptop RTX
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce RTX 2080 Super Laptop GPU | TU104 | 2020 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2080 Laptop GPU | TU104 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2070 Super Laptop GPU | TU104 | 2020 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2070 Laptop GPU | TU106 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce RTX 2060 Laptop GPU | TU106 | 2019 | `nvidia-580xx-dkms` | x |

### Laptop GTX
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 1660 Ti Laptop GPU | TU116 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1650 Ti Laptop GPU | TU117 | 2020 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1650 Laptop GPU | TU117 | 2019 | `nvidia-580xx-dkms` | x |

### MX Series (Turing)
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce MX550 | TU117 | 2022 | `nvidia-580xx-dkms` | x |
| GeForce MX450 | TU117 | 2020 | `nvidia-580xx-dkms` | x |

---

## Volta (GV) — `nvidia-580xx-dkms` (AUR · Legacy)

> 2017–2018

| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| Titan V | GV100 | 2017 | `nvidia-580xx-dkms` | x |
| Titan V CEO Edition | GV100 | 2018 | `nvidia-580xx-dkms` | x |
| Quadro GV100 | GV100 | 2018 | `nvidia-580xx-dkms` | x |

---

## Pascal (GP) — `nvidia-580xx-dkms` (AUR · Legacy)

> GTX 10xx Series · 2016–2018

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 1080 Ti | GP102 | 2017 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1080 | GP104 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1070 Ti | GP104 | 2017 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1070 | GP104 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1060 6GB | GP106 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1060 3GB | GP106 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1050 Ti | GP107 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1050 | GP107 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GT 1030 | GP108 | 2017 | `nvidia-580xx-dkms` | x |
| Titan Xp | GP102 | 2017 | `nvidia-580xx-dkms` | x |
| Titan X (Pascal) | GP102 | 2016 | `nvidia-580xx-dkms` | x |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 1080 Laptop GPU | GP104 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1070 Laptop GPU | GP104 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1060 Laptop GPU | GP106 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1050 Ti Laptop GPU | GP107 | 2016 | `nvidia-580xx-dkms` | x |
| GeForce GTX 1050 Laptop GPU | GP107 | 2016 | `nvidia-580xx-dkms` | x |

### MX Series (Pascal)
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce MX350 | GP107 | 2020 | `nvidia-580xx-dkms` | x |
| GeForce MX330 | GP108 | 2020 | `nvidia-580xx-dkms` | x |
| GeForce MX250 | GP108 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce MX230 | GP108 | 2019 | `nvidia-580xx-dkms` | x |
| GeForce MX150 | GP108 | 2017 | `nvidia-580xx-dkms` | x |
| GeForce MX130 | GP108 | 2017 | `nvidia-580xx-dkms` | x |

---

## Maxwell 2 (GM2xx) — `nvidia-580xx-dkms` (AUR · Legacy)

> GTX 900 Series · 2014–2016

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 980 Ti | GM200 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 980 | GM204 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 970 | GM204 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 960 | GM206 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 950 | GM206 | 2015 | `nvidia-580xx-dkms` | x |
| Titan X (Maxwell) | GM200 | 2015 | `nvidia-580xx-dkms` | x |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 980M | GM204 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 970M | GM204 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 965M | GM206 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 960M | GM206 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 950M | GM206 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 940M | GM108 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 930M | GM108 | 2015 | `nvidia-580xx-dkms` | x |
| GeForce GTX 920M | GM108 | 2015 | `nvidia-580xx-dkms` | x |

---

## Maxwell 1 (GM1xx) — `nvidia-580xx-dkms` (AUR · Legacy)

> GTX 750 Series · 2014

### Desktop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 750 Ti | GM107 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 750 | GM107 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 745 | GM107 | 2014 | `nvidia-580xx-dkms` | x |

### Laptop
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 860M | GM107 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 850M | GM107 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 840M | GM108 | 2014 | `nvidia-580xx-dkms` | x |
| GeForce GTX 830M | GM108 | 2014 | `nvidia-580xx-dkms` | x |

---

## Kepler (GK) — `nvidia-470xx-dkms` (AUR · Legacy · Sin soporte oficial)

> GTX 700 / GTX 600 Series · 2012–2014

### Desktop GTX 700 (Kepler)
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| Titan Z | GK110 | 2014 | `nvidia-470xx-dkms` | x |
| Titan Black | GK110 | 2014 | `nvidia-470xx-dkms` | x |
| Titan (Kepler) | GK110 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 780 Ti | GK110 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 780 | GK110 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 770 | GK104 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 760 Ti | GK104 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 760 | GK104 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 740 | GK107 | 2014 | `nvidia-470xx-dkms` | x |
| GeForce GTX 730 (GDDR5) | GK208 | 2014 | `nvidia-470xx-dkms` | x |
| GeForce GTX 720 | GK208 | 2014 | `nvidia-470xx-dkms` | x |
| GeForce GT 730 | GK208 | 2014 | `nvidia-470xx-dkms` | x |
| GeForce GT 720 | GK208 | 2014 | `nvidia-470xx-dkms` | x |
| GeForce GT 710 | GK208 | 2014 | `nvidia-470xx-dkms` | x |

### Desktop GTX 600
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 690 | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 680 | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 670 | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 660 Ti | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 660 | GK106 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 650 Ti Boost | GK106 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 650 Ti | GK106 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 650 | GK107 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 645 | GK107 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GT 640 (GDDR5) | GK107 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GT 630 (GK208) | GK208 | 2014 | `nvidia-470xx-dkms` | x |

### Laptop Kepler
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 780M | GK104 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 770M | GK106 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 765M | GK106 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 760M | GK106 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 680MX | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 675MX | GK104 | 2013 | `nvidia-470xx-dkms` | x |
| GeForce GTX 670MX | GK104 | 2012 | `nvidia-470xx-dkms` | x |
| GeForce GTX 660M | GK107 | 2012 | `nvidia-470xx-dkms` | x |

---

## Fermi (GF) — `nvidia-390xx-dkms` (AUR · Legacy)

> GTX 500 / GTX 400 Series · 2010–2012

### Desktop GTX 500
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 590 | GF110 | 2011 | `nvidia-390xx-dkms` | x |
| GeForce GTX 580 | GF110 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTX 570 | GF110 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTX 560 Ti | GF114 | 2011 | `nvidia-390xx-dkms` | x |
| GeForce GTX 560 | GF116 | 2011 | `nvidia-390xx-dkms` | x |
| GeForce GTX 550 Ti | GF116 | 2011 | `nvidia-390xx-dkms` | x |
| GeForce GTX 530 | GF108 | 2011 | `nvidia-390xx-dkms` | x |
| GeForce GT 520 | GF119 | 2011 | `nvidia-390xx-dkms` | x |

### Desktop GTX 400
| Modelo | Chip | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce GTX 480 | GF100 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTX 470 | GF100 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTX 465 | GF100 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTX 460 | GF104 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GTS 450 | GF106 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GT 440 | GF108 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GT 430 | GF108 | 2010 | `nvidia-390xx-dkms` | x |
| GeForce GT 420 | GF108 | 2010 | `nvidia-390xx-dkms` | x |

---

## Tesla (G80/GT200) — `nvidia-340xx-dkms` (AUR · Legacy)

> Serie 300 / 200 / 100 / 9xxx / 8xxx · 2006–2010

| Serie | Ejemplos | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce 300 | GT 340, GT 330, GT 320, 315, 310 | 2009–2010 | `nvidia-340xx-dkms` | `nouveau` |
| GeForce 200 | GTX 295, GTX 285, GTX 280, GTX 275, GTX 260, GTS 250, GT 240, GT 220 | 2008–2009 | `nvidia-340xx-dkms` | `nouveau` |
| GeForce 100 | GT 130, GT 120, G100 | 2009 | `nvidia-340xx-dkms` | `nouveau` |
| GeForce 9xxx | 9800 GTX+, 9800 GT, 9600 GT, 9500 GT, 9400 GT | 2008 | `nvidia-340xx-dkms` | `nouveau` |
| GeForce 8xxx | 8800 Ultra, 8800 GTX, 8800 GTS, 8800 GT, 8600 GT, 8500 GT, 8400 GS | 2006–2007 | `nvidia-340xx-dkms` | `nouveau` |

---

## Curie (NV40/G70) y anterior — Sin driver propietario

> Usar `nouveau` (open-source, módulo del kernel Linux)

```bash
sudo pacman -S xf86-video-nouveau
```

| Serie | Ejemplos | Año | Driver | Alternativa |
|---|---|---|---|---|
| GeForce 7xxx | 7900 GTX, 7800 GTX, 7600 GT, 7300 GT | 2005–2006 | x | `nouveau` |
| GeForce 6xxx | 6800 Ultra, 6800 GT, 6600 GT | 2004–2005 | x | `nouveau` |
| GeForce FX (5xxx) | FX 5950 Ultra, FX 5900, FX 5800 | 2003–2004 | x | `nouveau` |

---

## Notas importantes

### El paquete `nvidia` ya no existe en Arch Linux
El paquete `nvidia` de los repos oficiales fue reemplazado. La distribución actual es:
- **Blackwell** → `nvidia-open` (repos oficiales)
- **Turing → Ada Lovelace** → `nvidia-580xx-dkms` (AUR) como principal
- **Maxwell → Volta** → `nvidia-580xx-dkms` (AUR) como legacy

### nvidia-open como alternativa en Turing/Ampere
- En **Turing**: sin RTD3 Power Management (gestión de energía limitada)
- En **laptops Ampere**: posibles cuelgues, preferir `nvidia-580xx-dkms`
- En **Ada Lovelace**: funciona bien como alternativa

### GPUs Turing sin RTD3 Power Management (si usas nvidia-open)

> RTD3 (Runtime D3) permite que la GPU entre en estado de bajo consumo cuando no se usa.
> En todas las GPUs Turing, `nvidia-open` **no implementa RTD3** — la GPU no apaga/duerme entre usos.
> Impacto real: mayor consumo en reposo, especialmente en laptops.
> Solución: usar `nvidia-580xx-dkms` en su lugar.

**Desktop afectado:**

| Modelo | Chip | Año |
|---|---|---|
| GeForce RTX 2080 Ti | TU102 | 2018 |
| GeForce RTX 2080 Super | TU104 | 2019 |
| GeForce RTX 2080 | TU104 | 2018 |
| GeForce RTX 2070 Super | TU104 | 2019 |
| GeForce RTX 2070 | TU106 | 2018 |
| GeForce RTX 2060 Super | TU106 | 2019 |
| GeForce RTX 2060 | TU106 | 2019 |
| GeForce GTX 1660 Ti | TU116 | 2019 |
| GeForce GTX 1660 Super | TU116 | 2019 |
| GeForce GTX 1660 | TU116 | 2019 |
| GeForce GTX 1650 Super | TU116 | 2019 |
| GeForce GTX 1650 | TU117 | 2019 |

**Laptop afectado (mayor impacto en batería):**

| Modelo | Chip | Año |
|---|---|---|
| GeForce RTX 2080 Super Laptop GPU | TU104 | 2020 |
| GeForce RTX 2080 Laptop GPU | TU104 | 2019 |
| GeForce RTX 2070 Super Laptop GPU | TU104 | 2020 |
| GeForce RTX 2070 Laptop GPU | TU106 | 2019 |
| GeForce RTX 2060 Laptop GPU | TU106 | 2019 |
| GeForce GTX 1660 Ti Laptop GPU | TU116 | 2019 |
| GeForce GTX 1650 Ti Laptop GPU | TU117 | 2020 |
| GeForce GTX 1650 Laptop GPU | TU117 | 2019 |
| GeForce MX550 | TU117 | 2022 |
| GeForce MX450 | TU117 | 2020 |

### Conflicto de módulos — blacklist nouveau
Obligatorio cuando se usa cualquier driver propietario:

```bash
# Crear /etc/modprobe.d/blacklist-nouveau.conf
echo "blacklist nouveau" | sudo tee /etc/modprobe.d/blacklist-nouveau.conf
echo "options nouveau modeset=0" | sudo tee -a /etc/modprobe.d/blacklist-nouveau.conf

# Regenerar initramfs
sudo mkinitcpio -P
```

### Kernel headers — requeridos para todos los paquetes -dkms
```bash
# linux estándar
sudo pacman -S linux-headers

# linux-lts
sudo pacman -S linux-lts-headers

# linux-zen
sudo pacman -S linux-zen-headers

# linux-hardened
sudo pacman -S linux-hardened-headers
```

### DKMS — verificar compilación del módulo
```bash
# Ver estado de todos los módulos dkms
dkms status

# Recompilar manualmente si falla
sudo dkms autoinstall
```


### Verificar instalación completa
```bash
# Ver GPU y driver cargado
nvidia-smi

# Ver dispositivos OpenCL
clinfo | grep "Device Name"

# Ver info Vulkan
vulkaninfo --summary
```
