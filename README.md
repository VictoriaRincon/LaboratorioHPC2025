# Sistema de Optimización de Máquina de Estados

## 🚀 NUEVAS FUNCIONALIDADES: Comunicación Inteligente de Patrones MPI+OpenMP

### Comunicación entre Procesos para Compartir Soluciones

**Los procesos ahora pueden comunicarse entre sí para compartir soluciones** encontradas para patrones que empiezan y terminan en 0. Esta funcionalidad revoluciona el rendimiento del sistema al permitir que los procesos reutilicen automáticamente las soluciones ya calculadas por otros.

#### 🔄 Características Principales:

1. **Detección Automática de Patrones Útiles**
   - Identifica automáticamente patrones que empiezan y terminan en 0
   - Evalúa la complejidad y utilidad de cada patrón antes de compartirlo
   - Solo comparte patrones que realmente aportan valor al conjunto

2. **Comunicación Escalable y Adaptativa**
   - Frecuencia de compartición que se adapta automáticamente al tamaño del problema
   - Control inteligente para evitar saturación de la red MPI
   - Sincronización no bloqueante para prevenir deadlocks

3. **Paralelización Híbrida MPI+OpenMP**
   - **MPI**: Distribución del trabajo entre nodos/procesos
   - **OpenMP**: Paralelización dentro de cada proceso MPI (hasta 4 hilos por proceso)
   - **Comunicación inteligente**: Los procesos intercambian patrones útiles automáticamente

#### 📈 Configuración Automática por Tamaño:

- **Casos pequeños (≤8 bits)**: Compartir cada 50 soluciones, sincronizar cada 25
- **Casos medianos (9-15 bits)**: Compartir cada 100 soluciones, sincronizar cada 50  
- **Casos grandes (16-20 bits)**: Compartir cada 200 soluciones, sincronizar cada 100
- **Casos muy grandes (>20 bits)**: Compartir cada 500 soluciones, sincronizar cada 200

#### 🎯 Ejemplo de Funcionamiento:

```
Proceso 1 encuentra solución óptima para: [0, 1, 1, 0, 1, 0]
           ↓ (Comunicación automática)
Procesos 2, 3, 4 pueden reutilizar esta solución si encuentran el mismo patrón
           ↓ (Resultado)
Reducción significativa del tiempo total de cálculo
```

### Compilación con Soporte MPI+OpenMP

```bash
# Análisis exhaustivo con comunicación de patrones
make mpi

# Sistema de benchmark de rendimiento
make benchmark
```

### Ejecución

```bash
# Análisis básico con comunicación de patrones (4 procesos)
mpirun -np 4 ./analisis_exhaustivo_mpi -l 8

# Benchmark de rendimiento con paralelización híbrida
mpirun -np 4 ./benchmark_mpi -b 12 -v

# Test rápido para verificar funcionalidad
make test-mpi
```

## Descripción

Este proyecto implementa un algoritmo de programación dinámica para optimizar los costos de operación de una máquina de calentamiento que puede estar en diferentes estados térmicos. El sistema modela una máquina de estados finitos con restricciones de transición y busca la secuencia de estados que minimiza el costo total considerando la disponibilidad de energía eólica en cada período temporal.

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
2. **Satisfacción de demanda**: En cada hora se tiene información binaria:
   - **0**: Demanda NO cubierta por energía eólica → **Requiere estado `ON/CALIENTE`**
   - **1**: Demanda SÍ cubierta por energía eólica → **No requiere generación propia**
3. **Costos**: Estados `OFF` no tienen costo, estados `ON` tienen costos crecientes: `FRIO < TIBIO < CALIENTE`

## Algoritmo

### Enfoque de Programación Dinámica

El algoritmo utiliza un enfoque recursivo con memorización que:

1. **Inicia desde una hora X** y trabaja hacia atrás hasta la hora 0, permitiendo soluciones de largo variable
2. **Evalúa transiciones válidas** según las reglas de la máquina de estados
3. **Considera disponibilidad de energía eólica** (0/1) para determinar estados válidos
4. **Minimiza costos** mientras mantiene la factibilidad
5. **Reconstruye la solución** usando backtracking

### Pseudocódigo

```
función resolver_recursivo(hora, estado_llegada):
    si hora < 0:
        retornar (costo=0, válido=true)
    
    si energia_eolica_suficiente[hora] == 1:
        // No se requiere generación propia
        para cada estado en estados_que_van_a(estado_llegada):
            resultado = resolver_recursivo(hora-1, estado)
            si resultado.válido:
                actualizar_mejor_solución(estado, resultado)
    sino:  // energia_eolica_suficiente[hora] == 0
        // Se requiere generación propia (ON/CALIENTE)
        para cada estado en estados_que_van_a(estado_llegada):
            si estado.genera_energía():  // Solo ON/CALIENTE
                resultado = resolver_recursivo(hora-1, estado)
                si resultado.válido:
                    actualizar_mejor_solución(estado, resultado)
    
    retornar mejor_solución

función resolver_problema(hora_inicial, hora_final=0):
    retornar resolver_recursivo(hora_inicial, todos_los_estados_finales)
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
│   └── parametros.in             # Datos de entrada (disponibilidad energía eólica 0/1)
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

El archivo `data/parametros.in` debe contener una línea:

```
# Valores binarios para cada hora (0 hasta hora_final)
# 0 = Energía eólica insuficiente (requiere generación)
# 1 = Energía eólica suficiente (no requiere generación)
1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 1 1 1 1 1
```

**Nota**: El número de valores determina el horizonte temporal de la optimización. Se puede trabajar con cualquier cantidad de horas.

## Ejemplo de Salida

```
=== SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS ===
=== ANÁLISIS DETALLADO DEL ESCENARIO ===
Horas con energía eólica suficiente: 19/24
Horas que requieren generación: 5/24
⚖️  ESCENARIO MIXTO: Optimización necesaria

=== SOLUCIÓN ENCONTRADA ===
Costo total: 28.5
Horizonte temporal: 24 horas (0-23)

Estados por hora:
Hora    Estado          Costo   Eólica  Requiere_Gen
----    ------          -----   ------  ------------
0       OFF/CALIENTE    0       1       No
1       OFF/TIBIO       0       1       No
...
12      ON/CALIENTE     5       0       Sí
13      ON/CALIENTE     5       0       Sí
...
```

## Tipos de Escenarios

El sistema identifica automáticamente tres tipos de escenarios:

1. **Escenario Crítico** (⚠️): Todas las horas tienen valor 0 (energía eólica insuficiente)
   - Solución: `ON/CALIENTE` durante todo el horizonte temporal
   - Costo: N × 5.0 (donde N = número de horas)

2. **Escenario Óptimo** (✅): Todas las horas tienen valor 1 (energía eólica suficiente)
   - Solución: Estados `OFF` únicamente
   - Costo: 0

3. **Escenario Mixto** (⚖️): Combinación de horas con valores 0 y 1
   - Solución: Combinación óptima según transiciones válidas

## Características del Algoritmo

### Ventajas
- **Optimalidad garantizada**: Encuentra la solución de costo mínimo
- **Eficiencia**: Memoización evita recálculos O(H×S) estados
- **Flexibilidad**: Fácil modificación de costos, reglas de transición y horizonte temporal
- **Escalabilidad**: Funciona con horizontes temporales variables
- **Robustez**: Maneja escenarios imposibles correctamente

### Complejidad
- **Tiempo**: O(H × S²) donde H=número de horas, S=6 estados
- **Espacio**: O(H × S) para memoización + O(H) para solución

## Extensiones Posibles

1. **Múltiples máquinas**: Paralelizar múltiples unidades de generación
2. **Costos variables**: Costos que cambien según la hora del día  
3. **Restricciones adicionales**: Tiempo mínimo en estados, restricciones de arranque
4. **Optimización multi-objetivo**: Balance entre costo y emisiones
5. **Probabilidades**: Manejo de disponibilidad eólica probabilística en lugar de binaria
6. **Valores continuos**: Extensión a porcentajes de cobertura eólica (0.0-1.0) en lugar de binario

## Autor

Implementado como parte del Laboratorio de HPC 2025 - Optimización de Sistemas Energéticos 