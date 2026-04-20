# ================================================================================================
# FUNCIONES DE CONECTIVIDAD Y REINTENTOS LIMITADOS (30 INTENTOS) PARA PACMAN/YAY
# ================================================================================================
# Función para verificar conectividad a internet
check_internet() {
    # Verificación rápida y simple de conectividad
    if ping -c 1 -W 3 8.8.8.8 >/dev/null 2>&1; then
        return 0
    else
        echo -e "${RED}❌ Sin conexión a internet${NC}"
        return 1
    fi
}

# Función para esperar conexión a internet con reintentos limitados (30 intentos)
wait_for_internet() {
    local attempt=1

    while ! check_internet && [ $attempt -le 30 ]; do
        echo -e "${YELLOW}⚠️  Intento #$attempt - Sin conexión a internet${NC}"
        echo -e "${CYAN}🔄 Reintentando en 10 segundos...${NC}"
        echo ""
        echo -e "${BLUE}🔧 DIAGNÓSTICOS RECOMENDADOS:${NC}"
        echo -e "${BLUE}   1. ${YELLOW}Reiniciar Servicios:${NC}"
        echo -e "${BLUE}      • systemctl restart NetworkManager${NC}"
        echo -e "${BLUE}      • systemctl restart dhcpcd${NC}"
        echo -e "${BLUE}      • ip link set [interfaz] up${NC}"
        echo ""
        echo -e "${BLUE}   2. ${YELLOW}Router/Módem:${NC}"
        echo -e "${BLUE}      • Reiniciar router (desconectar 30 seg)${NC}"
        echo -e "${BLUE}      • Verificar que otros dispositivos tengan internet${NC}"
        echo -e "${BLUE}      • Contactar ISP si el problema persiste${NC}"
        echo ""
        echo -e "${GREEN}⏳ La instalación continuará automáticamente cuando se restablezca la conexión${NC}"
        echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

        # Mostrar comando útil para verificar conectividad manualmente
        if [ $((attempt % 3)) -eq 0 ]; then
            echo -e "${BLUE}💡 Revisa usando el comando manual: ping -c 3 www.google.com${NC}"
        fi

        sleep 10
        ((attempt++))

        # Limpiar pantalla cada 5 intentos para evitar saturación
        if (( attempt % 5 == 0 )); then
            clear
            echo -e "${YELLOW}🌐 ESPERANDO CONEXIÓN A INTERNET${NC}"
            echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
            echo -e "${YELLOW}⏱️  Intento #$attempt - Tiempo transcurrido: $((attempt * 10)) segundos${NC}"
            echo ""
        fi
    done

    # Si superó los 30 intentos sin conexión
    if [ $attempt -gt 30 ]; then
        echo -e "${RED}❌ ERROR: No se pudo establecer conexión a internet después de 30 intentos${NC}"
        echo -e "${YELLOW}⚠️  La instalación no puede continuar sin conexión a internet${NC}"
        return 1
    fi

    echo -e "${GREEN}🎉 ¡CONEXIÓN A INTERNET RESTABLECIDA!${NC}"
    echo -e "${CYAN}⏰ Continuando con la instalación...${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    sleep 3
    clear
}





# Función para actualizar sistema con pacman en chroot con bucle infinito
update_system_chroot() {
    local attempt=1

    echo -e "${GREEN}🔄 Actualizando sistema en chroot con pacman${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para actualizar sistema${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar actualización del sistema
        if chroot /mnt /bin/bash -c "pacman -Syu --noconfirm"; then
            echo -e "${GREEN}✅ Sistema actualizado correctamente${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la actualización del sistema (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: chroot /mnt /bin/bash -c \"pacman -Syu --noconfirm\"${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done
}

# Función para actualizar repositorios con pacman con bucle infinito
update_repositories() {
    local attempt=1

    echo -e "${GREEN}🔄 Actualizando repositorios con pacman${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para actualizar repositorios${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar actualización de repositorios
        if pacman -Syy; then
            echo -e "${GREEN}✅ Repositorios actualizados correctamente${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la actualización de repositorios (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: pacman -Syy${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done


}

# Función para instalar paquete con pacstrap con bucle infinito
install_pacstrap_with_retry() {
    local package="$1"
    local attempt=1

    if [[ -z "$package" ]]; then
        echo -e "${RED}❌ Error: No se especificó paquete para pacstrap${NC}"
        return 1
    fi

    echo -e "${GREEN}📦 Instalando: ${YELLOW}$package${GREEN} con pacstrap${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para instalar: $package${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar instalación con pacstrap
        if pacstrap /mnt "$package"; then
            echo -e "${GREEN}✅ $package instalado correctamente con pacstrap${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la instalación de $package (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: pacstrap /mnt \"$package\"${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done


}

# Función para instalar paquete con pacman en chroot con bucle infinito
install_pacman_chroot_with_retry() {
    local package="$1"
    local extra_args="${2:-}"
    local attempt=1

    if [[ -z "$package" ]]; then
        echo -e "${RED}❌ Error: No se especificó paquete para pacman chroot${NC}"
        return 1
    fi

    echo -e "${GREEN}📦 Instalando: ${YELLOW}$package${GREEN} con pacman en chroot${NC}"

    while [[ $attempt -le 30 ]]; do
        echo -e "${CYAN}🔄 Intento #$attempt para instalar: $package${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar instalación con pacman en chroot
        if chroot /mnt /bin/bash -c "pacman -S $package $extra_args --noconfirm"; then
            echo -e "${GREEN}✅ $package instalado correctamente con pacman en chroot${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la instalación de $package (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: chroot /mnt /bin/bash -c \"pacman -S $package $extra_args --noconfirm\"${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done

    # Si llegamos aquí, significa que se agotaron los 30 intentos
    echo -e "${RED}❌ Error: Se agotaron los 30 intentos para instalar $package con pacman en chroot${NC}"
    return 1



}

# Función para instalar paquete con yay en chroot con reintentos limitados (30 intentos)
install_yay_chroot_with_retry() {
    local package="$1"
    local extra_args="${2:-}"
    local user="$USER"
    local attempt=1

    if [[ -z "$package" ]]; then
        echo -e "${RED}❌ Error: No se especificó paquete para yay chroot${NC}"
        return 1
    fi

    echo -e "${GREEN}📦 Instalando: ${YELLOW}$package${GREEN} con yay en chroot${NC}"

    while [ $attempt -le 30 ]; do
        echo -e "${CYAN}🔄 Intento #$attempt para instalar: $package${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar instalación con yay en chroot
        if chroot /mnt /bin/bash -c "sudo -u $user yay -S $package $extra_args --noansweredit --noconfirm --needed"; then
            echo -e "${GREEN}✅ $package instalado correctamente con yay en chroot${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la instalación de $package (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: chroot /mnt /bin/bash -c \"sudo -u $user yay -S $package $extra_args --noansweredit --noconfirm --needed\"${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done

    # Si superó los 30 intentos
    if [ $attempt -gt 30 ]; then
        echo -e "${RED}❌ ERROR: No se pudo instalar $package con yay en chroot después de 30 intentos${NC}"
        return 1
    fi
}

# Función para instalar paquete de AUR con bucle infinito
install_aur_with_retry() {
    local package="$1"
    local attempt=1

    if [[ -z "$package" ]]; then
        echo -e "${RED}❌ Error: No se especificó paquete AUR${NC}"
        return 1
    fi

    echo -e "${GREEN}📦 Instalando paquete AUR: ${YELLOW}$package${GREEN} desde AUR${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para instalar: $package${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar instalación desde AUR
        if chroot /mnt bash -c "cd /tmp && git clone https://aur.archlinux.org/$package.git && cd $package && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'"; then
            echo -e "${GREEN}✅ $package instalado correctamente desde AUR${NC}"
            sleep 2
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la instalación de $package (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: chroot /mnt bash -c \"cd /tmp && git clone https://aur.archlinux.org/$package.git && cd $package && chown -R $USER:$USER . && su $USER -c 'makepkg -si --noconfirm'\"${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            # Limpiar directorio en caso de fallo
            chroot /mnt bash -c "rm -rf /tmp/$package" 2>/dev/null || true
            sleep 5
            ((attempt++))
        fi
    done
}

# Función para instalar paquete localmente en LiveCD con bucle infinito
install_pacman_livecd_with_retry() {
    local package="$1"
    local attempt=1

    if [[ -z "$package" ]]; then
        echo -e "${RED}❌ Error: No se especificó paquete para pacman LiveCD${NC}"
        return 1
    fi

    echo -e "${GREEN}📦 Instalando: ${YELLOW}$package${GREEN} con pacman en LiveCD${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para instalar: $package${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar instalación con pacman localmente en LiveCD
        if pacman -Syy "$package" --noconfirm --disable-download-timeout; then
            echo -e "${GREEN}✅ $package instalado correctamente en LiveCD${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la instalación de $package (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: pacman -Syy \"$package\" --noconfirm${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done


}

# Función para ejecutar un comando con reintentos y verificación de internet (bucle infinito)
run_command_with_retry() {
    local cmd="$1"
    local attempt=1

    if [[ -z "$cmd" ]]; then
        echo -e "${RED}❌ Error: No se especificó comando${NC}"
        return 1
    fi

    echo -e "${GREEN}🔄 Ejecutando: ${YELLOW}$cmd${NC}"

    while true; do
        echo -e "${CYAN}🔄 Intento #$attempt para ejecutar: $cmd${NC}"

        # Verificar conectividad antes del intento
        wait_for_internet

        # Ejecutar el comando
        if eval "$cmd"; then
            echo -e "${GREEN}✅ Comando ejecutado correctamente${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Falló la ejecución del comando (intento #$attempt)${NC}"
            echo -e "${RED}🔍 Comando ejecutado: $cmd${NC}"
            echo -e "${CYAN}🔄 Reintentando en 5 segundos...${NC}"
            sleep 5
            ((attempt++))
        fi
    done
}

# ================================================================================================


# Función para instalar un paquete con pacman -U con reintentos ante fallo de red
install_pacman_url_with_retry() {
    local url="$1"
    local pkg_name
    pkg_name=$(basename "$url")

    while true; do
        # Verificar conectividad antes del intento
        wait_for_internet
        echo -e "${CYAN}  → Instalando $pkg_name${NC}"

        if pacman -U --noconfirm "$url"; then
            echo -e "${GREEN}  ✓ $pkg_name instalado correctamente${NC}"
            return 0
        else
            echo -e "${RED}  ✗ Falló la instalación de $pkg_name, verificando red...${NC}"
            sleep 5
        fi
    done
}
