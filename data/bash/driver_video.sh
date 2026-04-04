# Instalación de drivers de video
echo -e "${GREEN}| Instalando drivers de video: $DRIVER_VIDEO |${NC}"
printf '%*s\n' "${COLUMNS:-$(tput cols)}" '' | tr ' ' _
echo ""

case "$DRIVER_VIDEO" in
    "Open Source")
        # Detección automática de hardware de video usando VGA controller
        VGA_LINE=$(lspci | grep -i "vga compatible controller")
        echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"

        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        # Detectar si hay múltiples GPUs para casos híbridos
        ALL_GPUS=$(lspci | grep -i -E "(vga|display)")
        HAS_NVIDIA=$(echo "$ALL_GPUS" | grep -i nvidia > /dev/null && echo "yes" || echo "no")
        HAS_AMD=$(echo "$ALL_GPUS" | grep -i -E "amd|radeon" > /dev/null && echo "yes" || echo "no")
        HAS_INTEL=$(echo "$ALL_GPUS" | grep -i intel > /dev/null && echo "yes" || echo "no")

        # Configuración para GPUs híbridas Intel + NVIDIA
        if [[ "$HAS_INTEL" == "yes" && "$HAS_NVIDIA" == "yes" ]]; then
            echo -e "${YELLOW}Detectada configuración híbrida Intel + NVIDIA - Instalando drivers para ambas${NC}"
            # Drivers Intel
            install_pacman_chroot_with_retry "vulkan-intel"
            install_pacman_chroot_with_retry "lib32-vulkan-intel"
            install_pacman_chroot_with_retry "vulkan-icd-loader"
            install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
            install_pacman_chroot_with_retry "linux-firmware-intel"
            install_pacman_chroot_with_retry "intel-media-driver"  # Gen 8+ (VA-API moderna)
            install_pacman_chroot_with_retry "intel-compute-runtime"  # OpenCL
            install_pacman_chroot_with_retry "intel-gpu-tools" # Monitoreo de GPU Intel
            install_pacman_chroot_with_retry "vpl-gpu-rt"
            install_pacman_chroot_with_retry "libva"
            install_pacman_chroot_with_retry "lib32-libva"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "libvdpau-va-gl"
            # Drivers NVIDIA open source
            install_pacman_chroot_with_retry "xf86-video-nouveau"
            install_pacman_chroot_with_retry "vulkan-nouveau"
            install_pacman_chroot_with_retry "lib32-vulkan-nouveau"


        # Configuración para GPUs híbridas Intel + AMD
        elif [[ "$HAS_INTEL" == "yes" && "$HAS_AMD" == "yes" ]]; then
            echo -e "${YELLOW}Detectada configuración híbrida Intel + AMD - Instalando drivers para ambas${NC}"
            # Drivers Intel
            install_pacman_chroot_with_retry "vulkan-intel"
            install_pacman_chroot_with_retry "lib32-vulkan-intel"
            install_pacman_chroot_with_retry "vulkan-icd-loader"
            install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
            install_pacman_chroot_with_retry "linux-firmware-intel"
            install_pacman_chroot_with_retry "intel-media-driver"  # Gen 8+ (VA-API moderna)
            install_pacman_chroot_with_retry "intel-compute-runtime"  # OpenCL
            install_pacman_chroot_with_retry "intel-gpu-tools" # Monitoreo de GPU Intel
            install_pacman_chroot_with_retry "vpl-gpu-rt"
            install_pacman_chroot_with_retry "libva"
            install_pacman_chroot_with_retry "lib32-libva"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "libvdpau-va-gl"
            # Drivers AMD
            install_pacman_chroot_with_retry "linux-firmware-radeon"
            install_pacman_chroot_with_retry "linux-firmware-amdgpu"
            install_pacman_chroot_with_retry "xf86-video-amdgpu"
            install_pacman_chroot_with_retry "xf86-video-ati"
            install_pacman_chroot_with_retry "vulkan-icd-loader"
            install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
            install_pacman_chroot_with_retry "opencl-mesa" # Borrar opencl-mesa para instalar opencl-amd de AUR
            install_pacman_chroot_with_retry "libva"
            install_pacman_chroot_with_retry "lib32-libva"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "libvdpau-va-gl"
            install_pacman_chroot_with_retry "vulkan-radeon"
            install_pacman_chroot_with_retry "lib32-vulkan-radeon"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "radeontop"

            chroot /mnt /bin/bash -c "usermod -aG render,video $USER"


        elif echo "$VGA_LINE" | grep -i nvidia > /dev/null; then
            echo "Detectado hardware NVIDIA - Instalando driver open source nouveau"
            install_pacman_chroot_with_retry "xf86-video-nouveau"
            install_pacman_chroot_with_retry "vulkan-nouveau"
            install_pacman_chroot_with_retry "lib32-vulkan-nouveau"


        elif echo "$VGA_LINE" | grep -i "amd\|radeon" > /dev/null; then
            echo "Detectado hardware AMD/Radeon - Instalando driver open source amdgpu"
            install_pacman_chroot_with_retry "linux-firmware-radeon"
            install_pacman_chroot_with_retry "linux-firmware-amdgpu"
            install_pacman_chroot_with_retry "xf86-video-amdgpu"
            install_pacman_chroot_with_retry "xf86-video-ati"
            install_pacman_chroot_with_retry "vulkan-icd-loader"
            install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
            install_pacman_chroot_with_retry "opencl-mesa" # Borrar opencl-mesa para instalar opencl-amd de AUR
            install_pacman_chroot_with_retry "libva"
            install_pacman_chroot_with_retry "lib32-libva"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "libvdpau-va-gl"
            install_pacman_chroot_with_retry "vulkan-radeon"
            install_pacman_chroot_with_retry "lib32-vulkan-radeon"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "radeontop"
            chroot /mnt /bin/bash -c "usermod -aG render,video $USER"

        elif echo "$VGA_LINE" | grep -i intel > /dev/null; then
            echo "Detectado hardware Intel - Instalando driver open source intel"
            # Drivers Intel
            install_pacman_chroot_with_retry "vulkan-intel"
            install_pacman_chroot_with_retry "lib32-vulkan-intel"
            install_pacman_chroot_with_retry "vulkan-icd-loader"
            install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
            install_pacman_chroot_with_retry "linux-firmware-intel"
            install_pacman_chroot_with_retry "intel-media-driver"  # Gen 8+ (VA-API moderna)
            install_pacman_chroot_with_retry "intel-compute-runtime"  # OpenCL
            install_pacman_chroot_with_retry "intel-gpu-tools" # Monitoreo de GPU Intel
            install_pacman_chroot_with_retry "vpl-gpu-rt"
            install_pacman_chroot_with_retry "libva"
            install_pacman_chroot_with_retry "lib32-libva"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "libvdpau-va-gl"
            chroot /mnt /bin/bash -c "usermod -aG render,video $USER"


        elif echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then

            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"
            install_pacman_chroot_with_retry "spice-vdagent"
            install_pacman_chroot_with_retry "qemu-guest-agent"
            install_pacman_chroot_with_retry "virglrenderer"
            install_pacman_chroot_with_retry "libgl"
            install_pacman_chroot_with_retry "libglvnd"
            chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
            chroot /mnt /bin/bash -c "systemctl start qemu-guest-agent.service"



        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"
            install_pacman_chroot_with_retry "xf86-video-fbdev"
            install_pacman_chroot_with_retry "virtualbox-guest-utils"
            install_pacman_chroot_with_retry "virglrenderer"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            install_pacman_chroot_with_retry "xf86-video-fbdev"
            install_pacman_chroot_with_retry "virtualbox-guest-utils"
            install_pacman_chroot_with_retry "virglrenderer"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            install_pacman_chroot_with_retry "xorg-server"
            install_pacman_chroot_with_retry "xorg-xinit"
            install_pacman_chroot_with_retry "xf86-video-vesa"
        fi
        ;;
    "nvidia-open")
        echo "Instalando driver NVIDIA open (kernel linux)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"

        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_pacman_chroot_with_retry "nvidia-open"
        install_pacman_chroot_with_retry "nvidia-utils"
        install_pacman_chroot_with_retry "lib32-nvidia-utils"
        install_pacman_chroot_with_retry "opencl-nvidia"
        install_pacman_chroot_with_retry "lib32-opencl-nvidia"
        install_pacman_chroot_with_retry "nvidia-settings"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        ;;
    "nvidia-open-lts")
        echo "Instalando driver NVIDIA open (kernel linux-lts)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"

        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_pacman_chroot_with_retry "nvidia-open-lts"
        install_pacman_chroot_with_retry "nvidia-utils"
        install_pacman_chroot_with_retry "lib32-nvidia-utils"
        install_pacman_chroot_with_retry "opencl-nvidia"
        install_pacman_chroot_with_retry "lib32-opencl-nvidia"
        install_pacman_chroot_with_retry "nvidia-settings"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        ;;
    "nvidia-open-dkms")
        echo "Instalando driver NVIDIA open DKMS"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"

        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_pacman_chroot_with_retry "nvidia-open-dkms"
        install_pacman_chroot_with_retry "nvidia-utils"
        install_pacman_chroot_with_retry "lib32-nvidia-utils"
        install_pacman_chroot_with_retry "opencl-nvidia"
        install_pacman_chroot_with_retry "lib32-opencl-nvidia"
        install_pacman_chroot_with_retry "nvidia-settings"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        ;;
    "nvidia-580xx-dkms")
        echo "Instalando driver NVIDIA serie 580.xx con DKMS (AUR)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_yay_chroot_with_retry "nvidia-580xx-dkms"
        install_yay_chroot_with_retry "nvidia-580xx-utils"
        install_yay_chroot_with_retry "lib32-nvidia-580xx-utils"
        install_yay_chroot_with_retry "opencl-nvidia-580xx"
        install_yay_chroot_with_retry "lib32-opencl-nvidia-580xx"
        install_yay_chroot_with_retry "nvidia-580xx-settings"
        ;;
    "nvidia-470xx-dkms")
        echo "Instalando driver NVIDIA serie 470.xx con DKMS (AUR)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_yay_chroot_with_retry "nvidia-470xx-dkms"
        install_yay_chroot_with_retry "nvidia-470xx-utils"
        install_yay_chroot_with_retry "lib32-nvidia-470xx-utils"
        install_yay_chroot_with_retry "opencl-nvidia-470xx"
        install_yay_chroot_with_retry "lib32-opencl-nvidia-470xx"
        install_yay_chroot_with_retry "nvidia-470xx-settings"
        install_yay_chroot_with_retry "mhwd-nvidia-470xx"
        ;;
    "nvidia-390xx-dkms")
        echo "Instalando driver NVIDIA serie 390.xx con DKMS (AUR, hardware antiguo)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_yay_chroot_with_retry "nvidia-390xx-dkms"
        install_yay_chroot_with_retry "nvidia-390xx-utils"
        install_yay_chroot_with_retry "lib32-nvidia-390xx-utils"
        install_yay_chroot_with_retry "opencl-nvidia-390xx"
        install_yay_chroot_with_retry "lib32-opencl-nvidia-390xx"
        install_yay_chroot_with_retry "nvidia-390xx-settings"
        install_yay_chroot_with_retry "mhwd-nvidia-390xx"
        ;;
    "nvidia-340xx-dkms")
        echo "Instalando driver NVIDIA serie 340.xx con DKMS (AUR, hardware muy antiguo)"
        install_pacman_chroot_with_retry "mesa"
        install_pacman_chroot_with_retry "lib32-mesa"
        install_pacman_chroot_with_retry "mesa-utils"
        install_pacman_chroot_with_retry "lib32-mesa-utils"
        install_pacman_chroot_with_retry "libva-nvidia-driver"
        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "libva-utils"
        install_pacman_chroot_with_retry "vulkan-tools"
        install_pacman_chroot_with_retry "vulkan-icd-loader"
        install_pacman_chroot_with_retry "lib32-vulkan-icd-loader"
        install_pacman_chroot_with_retry "clinfo"
        install_pacman_chroot_with_retry "ocl-icd"

        install_yay_chroot_with_retry "nvidia-340xx-dkms"
        install_yay_chroot_with_retry "nvidia-340xx-utils"
        install_yay_chroot_with_retry "lib32-nvidia-340xx-utils"
        install_yay_chroot_with_retry "opencl-nvidia-340xx"
        install_yay_chroot_with_retry "lib32-opencl-nvidia-340xx"
        install_yay_chroot_with_retry "nvidia-340xx-settings"
        ;;
    "mesa-amber")
        echo "Instalando drivers Modernos de Intel"
        # DRI/3D (obligatorio)
        install_pacman_chroot_with_retry "mesa-amber"
        install_pacman_chroot_with_retry "lib32-mesa-amber"
        install_pacman_chroot_with_retry "libva-intel-driver"  # Fallback para modelos más viejos
        install_pacman_chroot_with_retry "vdpauinfo"
        install_pacman_chroot_with_retry "xf86-video-intel"
        install_pacman_chroot_with_retry "xf86-video-nouveau"
        install_pacman_chroot_with_retry "xf86-video-ati"
        ;;

    "Máquina Virtual")
    # Detección automática de hardware de video usando VGA controller
    VGA_LINE=$(lspci | grep -i "vga compatible controller")
    echo -e "${CYAN}Tarjeta de video detectada: $VGA_LINE${NC}"

        if  echo "$VGA_LINE" | grep -i "virtio\|qemu\|red hat.*virtio" > /dev/null; then
            echo "Detectado hardware virtual (QEMU/KVM/Virtio) - Instalando driver genérico"
            install_pacman_chroot_with_retry "mesa"
            install_pacman_chroot_with_retry "lib32-mesa"
            install_pacman_chroot_with_retry "mesa-utils"
            install_pacman_chroot_with_retry "lib32-mesa-utils"
            install_pacman_chroot_with_retry "opencl-mesa"
            install_pacman_chroot_with_retry "vdpauinfo"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "vulkan-mesa-layers"

            install_pacman_chroot_with_retry "spice-vdagent"
            install_pacman_chroot_with_retry "xf86-video-qxl"
            install_pacman_chroot_with_retry "qemu-guest-agent"
            install_pacman_chroot_with_retry "virglrenderer"
            install_pacman_chroot_with_retry "libgl"
            install_pacman_chroot_with_retry "libglvnd"
            chroot /mnt /bin/bash -c "systemctl enable qemu-guest-agent.service" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"
            chroot /mnt /bin/bash -c "systemctl start qemu-guest-agent.service"


        elif echo "$VGA_LINE" | grep -i virtualbox > /dev/null; then
            echo "Detectado VirtualBox - Instalando guest utils y driver vmware"
            install_pacman_chroot_with_retry "mesa"
            install_pacman_chroot_with_retry "lib32-mesa"
            install_pacman_chroot_with_retry "mesa-utils"
            install_pacman_chroot_with_retry "lib32-mesa-utils"
            install_pacman_chroot_with_retry "opencl-mesa"
            install_pacman_chroot_with_retry "vdpauinfo"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "vulkan-mesa-layers"

            install_pacman_chroot_with_retry "virtualbox-guest-utils"
            install_pacman_chroot_with_retry "virglrenderer"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        elif echo "$VGA_LINE" | grep -i vmware > /dev/null; then
            echo "Detectado VMware - Instalando driver vmware"
            install_pacman_chroot_with_retry "mesa"
            install_pacman_chroot_with_retry "lib32-mesa"
            install_pacman_chroot_with_retry "mesa-utils"
            install_pacman_chroot_with_retry "lib32-mesa-utils"
            install_pacman_chroot_with_retry "opencl-mesa"
            install_pacman_chroot_with_retry "vdpauinfo"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "vulkan-mesa-layers"

            install_pacman_chroot_with_retry "virtualbox-guest-utils"
            install_pacman_chroot_with_retry "virglrenderer"
            chroot /mnt /bin/bash -c "systemctl enable vboxservice" || echo -e "${RED}ERROR: Falló systemctl enable${NC}"

        else
            echo "Hardware no detectado - Instalando driver genérico vesa"
            install_pacman_chroot_with_retry "mesa"
            install_pacman_chroot_with_retry "lib32-mesa"
            install_pacman_chroot_with_retry "mesa-utils"
            install_pacman_chroot_with_retry "lib32-mesa-utils"
            install_pacman_chroot_with_retry "opencl-mesa"
            install_pacman_chroot_with_retry "vdpauinfo"
            install_pacman_chroot_with_retry "libva-utils"
            install_pacman_chroot_with_retry "vulkan-tools"
            install_pacman_chroot_with_retry "vulkan-mesa-layers"
        fi
        ;;
esac
