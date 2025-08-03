#!/bin/bash

# Script de instalación simplificado para testing rápido
# Este script simula el proceso de instalación pero termina en ~10 segundos

# Colores
RED='\033[0;31m'
BOLD_RED='\033[1;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Función para imprimir en color
print_color() {
    echo -e "$1$2${NC}"
}

# Función para simular progreso
simulate_progress() {
    local duration=$1
    local message=$2
    echo -e "${CYAN}$message${NC}"
    for i in $(seq 1 $duration); do
        echo -n "."
        sleep 0.5
    done
    echo -e " ${GREEN}✓${NC}"
}

# Mensaje de inicio
echo
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    ARCRIS LINUX INSTALLER                       ${NC}"
echo -e "${BLUE}                      (Modo de Testing)                         ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo

print_color "${YELLOW}" "🚀 Iniciando instalación de Arcris Linux..."
sleep 1

# Etapa 1: Verificación del sistema
simulate_progress 2 "🔍 Verificando requisitos del sistema"

# Etapa 2: Preparación de particiones
simulate_progress 2 "💾 Preparando particiones"

# Etapa 3: Instalación de paquetes base
simulate_progress 3 "📦 Instalando paquetes base"

# Etapa 4: Configuración del sistema
simulate_progress 2 "⚙️ Configurando sistema"

# Etapa 5: Instalación del gestor de arranque
simulate_progress 1 "🔧 Instalando gestor de arranque"

echo
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}                                                                ${NC}"
echo -e "${GREEN}    ✓ ARCRIS LINUX INSTALADO CORRECTAMENTE                      ${NC}"
echo -e "${GREEN}                                                                ${NC}"
echo -e "${GREEN}    El sistema está listo para usar                            ${NC}"
echo -e "${GREEN}                                                                ${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════════${NC}"
echo

print_color "${CYAN}" "¡Instalación completada exitosamente! 🎉"
print_color "${YELLOW}" "El sistema se reiniciará automáticamente o puede salir manualmente."

echo
echo -e "${GREEN}[TEST MODE] Script terminado - debería navegar a page9 en 1 segundo${NC}"

# Salir con código 0 para indicar éxito
exit 0
