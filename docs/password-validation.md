# Sistema de Validación de Contraseñas - Arcris2

## Descripción General

El sistema de validación de contraseñas en Arcris2 proporciona retroalimentación visual en tiempo real para ayudar a los usuarios a crear contraseñas seguras. El sistema valida automáticamente las contraseñas mientras el usuario escribe y proporciona indicadores visuales claros sobre la fortaleza y validez de la contraseña.

## Características Principales

### 1. Indicadores de Fortaleza de Contraseña

La fortaleza de la contraseña se muestra mediante:
- **Etiqueta de texto**: Muestra "Muy débil", "Media", o "Fuerte"
- **Barra de progreso**: Indica visualmente el nivel de fortaleza
- **Colores**: 
  - 🔴 **Rojo**: Contraseña muy débil
  - 🟠 **Naranja**: Contraseña media
  - 🟢 **Verde**: Contraseña fuerte

### 2. Validación de Requisitos

El sistema verifica los siguientes requisitos:
- ✅ **Longitud mínima**: Al menos 8 caracteres
- ✅ **Mayúsculas**: Al menos una letra mayúscula
- ✅ **Números**: Al menos un dígito

### 3. Verificación de Coincidencia

- Compara la contraseña original con la confirmación
- Muestra un mensaje de error si no coinciden
- Iconos visuales para indicar el estado de coincidencia

## Algoritmo de Fortaleza

### Criterios de Evaluación

La fortaleza se calcula basándose en los siguientes puntos:

1. **Longitud**:
   - +1 punto por tener al menos 8 caracteres
   - +1 punto adicional por tener al menos 12 caracteres

2. **Complejidad**:
   - +1 punto por contener al menos una mayúscula
   - +1 punto por contener al menos un número
   - +1 punto por contener caracteres especiales

### Niveles de Fortaleza

- **Muy débil (0-1 puntos)**: 🔴 Rojo
- **Media (2-3 puntos)**: 🟠 Naranja  
- **Fuerte (4+ puntos)**: 🟢 Verde

## Implementación Técnica

### Estructura de Datos

```c
typedef struct _Page4Data {
    // Widgets de validación
    GtkLabel *password_strength_label;
    GtkProgressBar *password_strength_bar;
    GtkLabel *password_match_label;
    GtkImage *password_match_icon;
    
    // Widgets de requisitos
    GtkImage *length_check_icon;
    GtkLabel *length_check_label;
    GtkImage *uppercase_check_icon;
    GtkLabel *uppercase_check_label;
    GtkImage *number_check_icon;
    GtkLabel *number_check_label;
    
    // Estado de validación
    gboolean password_has_length;
    gboolean password_has_uppercase;
    gboolean password_has_number;
    gboolean passwords_match;
    gint password_strength; // 0=débil, 1=medio, 2=fuerte
} Page4Data;
```

### Funciones Principales

1. **`page4_validate_password()`**: Función principal de validación
2. **`page4_check_password_match()`**: Verifica coincidencia de contraseñas
3. **`page4_update_password_strength()`**: Actualiza indicadores visuales
4. **`page4_calculate_password_strength()`**: Calcula la fortaleza numérica

### Callbacks de Eventos

- **`on_page4_password_changed()`**: Se ejecuta cuando cambia la contraseña
- **`on_page4_password_confirm_changed()`**: Se ejecuta cuando cambia la confirmación

## Estilos CSS

### Clases de Fortaleza

```css
.strength-weak {
    color: #e74c3c;
    font-weight: bold;
}

.strength-medium {
    color: #f39c12;
    font-weight: bold;
}

.strength-strong {
    color: #27ae60;
    font-weight: bold;
}
```

### Clases de Validación

```css
.requirement-met {
    color: #27ae60;
}

.requirement-not-met {
    color: #e74c3c;
}

.password-match-success {
    color: #27ae60;
}

.password-match-error {
    color: #e74c3c;
}
```

## Interfaz de Usuario

### Widgets Principales

1. **Campo de contraseña** (`AdwPasswordEntryRow`)
2. **Campo de confirmación** (`AdwPasswordEntryRow`)
3. **Etiqueta de fortaleza** (`GtkLabel`)
4. **Barra de progreso** (`GtkProgressBar`)
5. **Lista de requisitos** con iconos y etiquetas

### Comportamiento Visual

- **Actualización en tiempo real**: La validación ocurre mientras el usuario escribe
- **Transiciones suaves**: Los cambios de color y estado son animados
- **Iconos dinámicos**: Los iconos cambian de ❌ a ✅ según el cumplimiento
- **Mensajes claros**: Texto descriptivo para cada estado

## Flujo de Validación

1. **Usuario escribe contraseña** → Trigger `on_page4_password_changed()`
2. **Validar requisitos** → `page4_validate_password()`
3. **Calcular fortaleza** → `page4_calculate_password_strength()`
4. **Actualizar interfaz** → `page4_update_password_strength()`
5. **Verificar coincidencia** → `page4_check_password_match()`
6. **Validar formulario** → `page4_is_form_valid()`

## Beneficios del Sistema

### Para el Usuario
- **Retroalimentación inmediata**: Saben instantáneamente si su contraseña es segura
- **Guía clara**: Los requisitos están claramente mostrados
- **Experiencia visual**: Colores y iconos facilitan la comprensión

### Para el Sistema
- **Seguridad mejorada**: Asegura contraseñas más fuertes
- **Prevención de errores**: Evita contraseñas que no coinciden
- **Experiencia de usuario**: Interfaz intuitiva y amigable

## Configuración y Personalización

### Modificar Requisitos

Para cambiar los requisitos de contraseña, edita las funciones:
- `page4_check_password_length()` para longitud mínima
- `page4_check_password_uppercase()` para mayúsculas
- `page4_check_password_number()` para números

### Personalizar Colores

Los colores se pueden modificar en el archivo CSS:
- `#e74c3c` para rojo (débil)
- `#f39c12` para naranja (medio)
- `#27ae60` para verde (fuerte)

### Agregar Nuevos Requisitos

1. Agregar campo booleano en `Page4Data`
2. Crear función de validación
3. Actualizar `page4_calculate_password_strength()`
4. Agregar widgets UI en `page4.ui`
5. Implementar actualización visual

## Archivos Relacionados

- **`src/page4.c`**: Implementación principal
- **`src/page4.h`**: Definiciones y prototipos
- **`data/page4.ui`**: Interfaz de usuario
- **`data/styles/password-validation.css`**: Estilos CSS
- **`data/gresource.xml`**: Recursos de la aplicación

## Ejemplo de Uso

```c
// Inicializar la página
page4_init(builder, carousel, revealer);

// El sistema se encarga automáticamente de:
// - Validar contraseñas en tiempo real
// - Mostrar indicadores visuales
// - Verificar coincidencia de contraseñas
// - Actualizar estado del formulario
```

---

*Documentación actualizada para Arcris2 - Sistema de Validación de Contraseñas*