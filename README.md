# Sistema de Optimización de Máquina de Estados

## Descripción

Este proyecto implementa un algoritmo de programación dinámica para optimizar los costos de operación de una máquina de calentamiento que puede estar en diferentes estados térmicos. El sistema modela una máquina de estados finitos con restricciones de transición y busca la secuencia de estados que minimiza el costo total mientras satisface la demanda energética.

## Problema Modelado

### Estados de la Máquina

La máquina puede estar en 6 estados diferentes:
- **ON/CALIENTE** (Costo: 5.0) - ✅ Genera energía
- **ON/TIBIO** (Costo: 2.5) - ❌ No genera energía  
- **ON/FRIO** (Costo: 1.0) - ❌ No genera energía
- **OFF/CALIENTE** (Costo: 0.0) - ❌ No genera energía
- **OFF/TIBIO** (Costo: 0.0) - ❌ No genera energía
- **OFF/FRIO** (Costo: 0.0) - ❌ No genera energía

### Reglas de Transición

Las transiciones válidas entre estados por hora son:

```
ON/CALIENTE  → OFF/CALIENTE | ON/CALIENTE
OFF/CALIENTE → ON/TIBIO     | OFF/TIBIO
ON/TIBIO     → ON/CALIENTE  | OFF/CALIENTE
OFF/TIBIO    → ON/FRIO      | OFF/FRIO
ON/FRIO      → ON/TIBIO     | OFF/TIBIO
OFF/FRIO     → ON/FRIO      | OFF/FRIO
```

### Restricciones

1. **Generación de energía**: Solo el estado `ON/CALIENTE` puede generar energía
2. **Satisfacción de demanda**: En cada hora, la demanda debe cubrirse con:
   - Energía de otras fuentes (EO), o
   - Generación propia (requiere estado `ON/CALIENTE`)
3. **Costos**: Estados `OFF` no tienen costo, estados `ON` tienen costos crecientes: `FRIO < TIBIO < CALIENTE`

## Algoritmo

### Enfoque de Programación Dinámica

El algoritmo utiliza un enfoque recursivo con memorización que:

1. **Inicia desde la hora 23** y trabaja hacia atrás hasta la hora 0
2. **Evalúa transiciones válidas** según las reglas de la máquina de estados
3. **Considera restricciones energéticas** para determinar estados válidos
4. **Minimiza costos** mientras mantiene la factibilidad
5. **Reconstruye la solución** usando backtracking

### Pseudocódigo

```
función resolver_recursivo(hora, estado_llegada):
    si hora < 0:
        retornar (costo=0, válido=true)
    
    si demanda_cubierta_con_EO(hora):
        para cada estado en estados_que_van_a(estado_llegada):
            resultado = resolver_recursivo(hora-1, estado)
            si resultado.válido:
                actualizar_mejor_solución(estado, resultado)
    sino:
        para cada estado en estados_que_van_a(estado_llegada):
            si estado.genera_energía():
                resultado = resolver_recursivo(hora-1, estado)
                si resultado.válido:
                    actualizar_mejor_solución(estado, resultado)
    
    retornar mejor_solución
```

## Estructura del Proyecto

```
LaboratorioHPC2025/
├── include/
│   ├── calculador_costos.hpp    # Lógica principal del algoritmo
│   └── escenario.hpp             # Manejo de datos de entrada
├── src/
│   ├── calculador_costos.cpp    # Implementación del algoritmo
│   ├── escenario.cpp             # Implementación del escenario
│   └── main.cpp                  # Programa principal
├── data/
│   └── parametros.in             # Datos de entrada (demanda y EO)
├── obj/                          # Archivos objeto (generados)
├── Makefile                      # Script de compilación
└── README.md                     # Esta documentación
```

## Compilación y Ejecución

### Requisitos
- Compilador C++17 compatible (g++, clang++)
- Make

### Comandos

```bash
# Compilar el proyecto
make

# Compilar y ejecutar
make run

# Limpiar archivos generados
make clean

# Ver ayuda
make help
```

### Ejecución directa
```bash
./maquina_estados
```

## Formato de Datos de Entrada

El archivo `data/parametros.in` debe contener dos líneas:

```
# Línea 1: Demanda para cada hora (0-23)
10 8 6 4 3 2 1 2 4 6 8 12 15 18 20 22 25 28 30 25 20 18 15 12

# Línea 2: Energía de otras fuentes para cada hora (0-23)  
12 10 8 6 5 4 3 4 6 8 10 14 18 20 22 24 20 18 16 14 12 20 18 15
```

## Ejemplo de Salida

```
=== SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS ===
=== ANÁLISIS DETALLADO DEL ESCENARIO ===
Horas con demanda cubierta por EO: 19/24
Horas que requieren generación: 5/24
⚖️  ESCENARIO MIXTO: Optimización necesaria

=== SOLUCIÓN ENCONTRADA ===
Costo total: 28.5

Estados por hora:
Hora    Estado          Costo   Demanda EO      Cubierta
----    ------          -----   ------- --      --------
0       OFF/CALIENTE    0       10      12      Sí
1       OFF/TIBIO       0       8       10      Sí
...
16      ON/CALIENTE     5       25      20      No
17      ON/CALIENTE     5       28      18      No
...
```

## Tipos de Escenarios

El sistema identifica automáticamente tres tipos de escenarios:

1. **Escenario Crítico** (⚠️): Todas las horas requieren generación
   - Solución: `ON/CALIENTE` durante 24 horas
   - Costo: 24 × 5.0 = 120

2. **Escenario Óptimo** (✅): Toda la demanda se cubre con EO  
   - Solución: Estados `OFF` únicamente
   - Costo: 0

3. **Escenario Mixto** (⚖️): Requiere optimización balanceada
   - Solución: Combinación óptima según transiciones válidas

## Características del Algoritmo

### Ventajas
- **Optimalidad garantizada**: Encuentra la solución de costo mínimo
- **Eficiencia**: Memoización evita recálculos O(24×6) estados
- **Flexibilidad**: Fácil modificación de costos y reglas de transición
- **Robustez**: Maneja escenarios imposibles correctamente

### Complejidad
- **Tiempo**: O(H × S²) donde H=24 horas, S=6 estados
- **Espacio**: O(H × S) para memoización + O(H) para solución

## Extensiones Posibles

1. **Múltiples máquinas**: Paralelizar múltiples unidades de generación
2. **Costos variables**: Costos que cambien según la hora del día  
3. **Restricciones adicionales**: Tiempo mínimo en estados, restricciones de arranque
4. **Optimización multi-objetivo**: Balance entre costo y emisiones
5. **Incertidumbre**: Manejo de demanda estocástica

## Autor

Implementado como parte del Laboratorio de HPC 2025 - Optimización de Sistemas Energéticos 