#!/bin/bash
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
show_progress() {
    local duration=$1
    local message=$2
    local steps=50
    local step_duration=$((duration * 1000 / steps))  # en milisegundos

    echo -e "\n${CYAN}$message${NC}"
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
        sleep $(echo "scale=3; $step_duration/1000" | bc -l) 2>/dev/null || sleep 1
    done
    echo -e "\n${GREEN}✓ Completado!${NC}\n"
}

# Función para animación de puntos
animate_dots() {
    local duration=$1
    local message=$2
    local end_time=$(($(date +%s) + duration))

    echo -e "\n${CYAN}$message${NC}"
    while [ $(date +%s) -lt $end_time ]; do
        for dots in "." ".." "..." ""; do
            printf "\r${YELLOW}Procesando$dots   ${NC}"
            sleep 0.5
        done
    done
    echo -e "\r${GREEN}✓ Procesando completado!${NC}\n"
}

# Función para contador regresivo
countdown() {
    local seconds=$1
    local message=$2

    echo -e "\n${CYAN}$message${NC}"
    for ((i=seconds; i>=1; i--)); do
        printf "\r${YELLOW}Tiempo restante: %02d segundos${NC}" "$i"
        sleep 1
    done
    echo -e "\r${GREEN}✓ Tiempo completado!        ${NC}\n"
}

# Función para spinner
spinner() {
    local duration=$1
    local message=$2
    local spin_chars="⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏"
    local end_time=$(($(date +%s) + duration))

    echo -e "\n${CYAN}$message${NC}"
    local i=0
    while [ $(date +%s) -lt $end_time ]; do
        printf "\r${YELLOW}${spin_chars:$i:1} Trabajando...${NC}"
        i=$(( (i+1) % ${#spin_chars} ))
        sleep 0.1
    done
    echo -e "\r${GREEN}✓ Trabajo completado!${NC}\n"
}

clear
echo ""
echo ""

# Texto ARCRIS
print_red "  ███████╗ ██████╗  ██████╗██████╗ ██╗███████╗"
print_red "  ██╔══██╗██╔══██╗██╔════╝██╔══██╗██║██╔════╝"
print_red "  ███████║██████╔╝██║     ██████╔╝██║███████╗"
print_red "  ██╔══██║██╔══██╗██║     ██╔══██╗██║╚════██║"
print_red "  ██║  ██║██║  ██║╚██████╗██║  ██║██║███████║"
print_red "  ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝╚══════╝"

# Animaciones de 90 segundos divididas en etapas
echo -e "\n${BOLD_RED}=== INICIANDO PROCESO DE INSTALACIÓN ===${NC}"

# Etapa 1: Barra de progreso (25 segundos)
show_progress 25 "🔧 Preparando el sistema..."

# Etapa 2: Spinner (20 segundos)
spinner 20 "📦 Descargando paquetes esenciales..."

# Etapa 3: Animación de puntos (20 segundos)
animate_dots 20 "⚙️  Configurando componentes del sistema..."

# Etapa 4: Barra de progreso (15 segundos)
show_progress 15 "🔨 Compilando módulos del kernel..."

# Etapa 5: Countdown (10 segundos)
countdown 10 "🚀 Finalizando instalación..."

# Mensaje final
echo -e "${GREEN}"
echo "  ╔════════════════════════════════════════╗"
echo "  ║                                        ║"
echo "  ║    ✓ ARCRIS LINUX INSTALADO            ║"
echo "  ║                                        ║"
echo "  ║    El sistema está listo para usar     ║"
echo "  ║                                        ║"
echo "  ╚════════════════════════════════════════╝"
echo -e "${NC}"

print_color "${CYAN}" "¡Instalación completada exitosamente! 🎉"
