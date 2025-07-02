#!/bin/bash

# Script de desarrollo para Arcris 2.0
# Facilita la compilación, testing y desarrollo del proyecto

set -e  # Salir si hay errores

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Variables del proyecto
PROJECT_NAME="Arcris"
BUILD_DIR="builddir"
EXECUTABLE="src/arcris"

# Función para mostrar mensajes con colores
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Función para mostrar ayuda
show_help() {
    echo -e "${BLUE}Script de Desarrollo - Arcris 2.0${NC}"
    echo ""
    echo "Uso: $0 [COMANDO]"
    echo ""
    echo "Comandos disponibles:"
    echo "  build, b       - Compilar el proyecto"
    echo "  clean, c       - Limpiar archivos de compilación"
    echo "  run, r         - Ejecutar la aplicación"
    echo "  debug, d       - Ejecutar con información de debug"
    echo "  setup, s       - Configurar el entorno de compilación"
    echo "  install, i     - Instalar la aplicación"
    echo "  reconfigure    - Reconfigurar el sistema de build"
    echo "  test, t        - Ejecutar tests básicos"
    echo "  check          - Verificar dependencias"
    echo "  watch, w       - Compilar y ejecutar automáticamente al cambiar archivos"
    echo "  help, h        - Mostrar esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  $0 build       # Compilar el proyecto"
    echo "  $0 run         # Ejecutar la aplicación"
    echo "  $0 debug       # Ejecutar con logs detallados"
    echo "  $0 clean build # Limpiar y compilar"
}

# Función para verificar dependencias
check_dependencies() {
    log_info "Verificando dependencias..."

    local missing_deps=()

    # Verificar herramientas de build
    if ! command -v meson &> /dev/null; then
        missing_deps+=("meson")
    fi

    if ! command -v ninja &> /dev/null; then
        missing_deps+=("ninja")
    fi

    # Verificar dependencias de desarrollo
    if ! pkg-config --exists gtk4; then
        missing_deps+=("gtk4-devel")
    fi

    if ! pkg-config --exists libadwaita-1; then
        missing_deps+=("libadwaita-devel")
    fi

    if [ ${#missing_deps[@]} -eq 0 ]; then
        log_success "Todas las dependencias están disponibles"
        return 0
    else
        log_error "Dependencias faltantes: ${missing_deps[*]}"
        log_info "Instala las dependencias faltantes antes de continuar"
        return 1
    fi
}

# Función para configurar el entorno
setup_build() {
    log_info "Configurando entorno de compilación..."

    if [ -d "$BUILD_DIR" ]; then
        log_warning "Directorio de build existe, reconfigurando..."
        meson setup $BUILD_DIR --reconfigure
    else
        meson setup $BUILD_DIR
    fi

    log_success "Entorno configurado correctamente"
}

# Función para compilar
build_project() {
    log_info "Compilando $PROJECT_NAME..."

    if [ ! -d "$BUILD_DIR" ]; then
        log_warning "Directorio de build no existe, configurando..."
        setup_build
    fi

    ninja -C $BUILD_DIR

    if [ $? -eq 0 ]; then
        log_success "Compilación exitosa"
    else
        log_error "Error en la compilación"
        exit 1
    fi
}

# Función para limpiar
clean_build() {
    log_info "Limpiando archivos de compilación..."

    if [ -d "$BUILD_DIR" ]; then
        rm -rf $BUILD_DIR
        log_success "Archivos de compilación eliminados"
    else
        log_warning "No hay archivos de compilación para limpiar"
    fi
}

# Función para ejecutar
run_application() {
    log_info "Ejecutando $PROJECT_NAME..."

    if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
        log_warning "Ejecutable no encontrado, compilando primero..."
        build_project
    fi

    ./$BUILD_DIR/$EXECUTABLE
}

# Función para ejecutar con debug
debug_application() {
    log_info "Ejecutando $PROJECT_NAME con información de debug..."

    if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
        log_warning "Ejecutable no encontrado, compilando primero..."
        build_project
    fi

    # Configurar variables de entorno para debug
    export G_MESSAGES_DEBUG=all
    export GTK_DEBUG=interactive

    ./$BUILD_DIR/$EXECUTABLE
}

# Función para reconfigurar
reconfigure_build() {
    log_info "Reconfigurando sistema de build..."
    meson setup $BUILD_DIR --reconfigure
    log_success "Reconfiguración completada"
}

# Función para instalar
install_application() {
    log_info "Instalando $PROJECT_NAME..."

    if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
        log_warning "Ejecutable no encontrado, compilando primero..."
        build_project
    fi

    sudo ninja -C $BUILD_DIR install
    log_success "Instalación completada"
}

# Función para tests básicos
test_application() {
    log_info "Ejecutando tests básicos..."

    # Test 1: Verificar que el ejecutable existe
    if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
        log_error "Test fallido: Ejecutable no encontrado"
        return 1
    fi
    log_success "✓ Ejecutable encontrado"

    # Test 2: Verificar que el ejecutable tiene permisos de ejecución
    if [ ! -x "$BUILD_DIR/$EXECUTABLE" ]; then
        log_error "Test fallido: Ejecutable sin permisos de ejecución"
        return 1
    fi
    log_success "✓ Permisos de ejecución correctos"

    # Test 3: Verificar que los recursos UI existen
    if [ ! -f "data/window.ui" ]; then
        log_error "Test fallido: Archivo UI principal no encontrado"
        return 1
    fi
    log_success "✓ Archivos UI encontrados"

    # Test 4: Verificar compilación reciente
    local exec_time=$(stat -c %Y "$BUILD_DIR/$EXECUTABLE" 2>/dev/null || echo 0)
    local source_time=$(find src -name "*.c" -exec stat -c %Y {} \; | sort -n | tail -1)

    if [ "$exec_time" -lt "$source_time" ]; then
        log_warning "⚠ El ejecutable parece desactualizado respecto al código fuente"
    else
        log_success "✓ Ejecutable actualizado"
    fi

    log_success "Todos los tests básicos pasaron"
}

# Función para watch (compilar automáticamente)
watch_and_run() {
    log_info "Iniciando modo watch - compilación automática..."
    log_info "Presiona Ctrl+C para salir"

    # Verificar si inotify-tools está disponible
    if ! command -v inotifywait &> /dev/null; then
        log_error "inotify-tools no está instalado. Usando fallback con sleep."

        while true; do
            build_project
            run_application &
            APP_PID=$!
            sleep 5
            kill $APP_PID 2>/dev/null || true
        done
    else
        while true; do
            build_project
            run_application &
            APP_PID=$!

            # Esperar cambios en archivos fuente
            inotifywait -q -r -e modify,create,delete src/ data/

            # Matar la aplicación anterior
            kill $APP_PID 2>/dev/null || true
            sleep 1
        done
    fi
}

# Función principal
main() {
    case "${1:-help}" in
        build|b)
            build_project
            ;;
        clean|c)
            clean_build
            ;;
        run|r)
            run_application
            ;;
        debug|d)
            debug_application
            ;;
        setup|s)
            check_dependencies && setup_build
            ;;
        install|i)
            install_application
            ;;
        reconfigure)
            reconfigure_build
            ;;
        test|t)
            test_application
            ;;
        check)
            check_dependencies
            ;;
        watch|w)
            watch_and_run
            ;;
        help|h|--help|-h)
            show_help
            ;;
        *)
            # Permitir múltiples comandos
            for cmd in "$@"; do
                case "$cmd" in
                    build|b) build_project ;;
                    clean|c) clean_build ;;
                    run|r) run_application ;;
                    debug|d) debug_application ;;
                    setup|s) check_dependencies && setup_build ;;
                    install|i) install_application ;;
                    reconfigure) reconfigure_build ;;
                    test|t) test_application ;;
                    check) check_dependencies ;;
                    *)
                        log_error "Comando desconocido: $cmd"
                        show_help
                        exit 1
                        ;;
                esac
            done
            ;;
    esac
}

# Verificar que estamos en el directorio correcto
if [ ! -f "meson.build" ]; then
    log_error "Este script debe ejecutarse desde el directorio raíz del proyecto"
    exit 1
fi

# Ejecutar función principal con todos los argumentos
main "$@"
