# Análisis Exhaustivo de Escenarios con MPI

## Descripción

Este sistema implementa un análisis exhaustivo de todas las combinaciones posibles de escenarios de cubrimiento de demanda para un largo determinado, utilizando MPI para distribuir el trabajo entre múltiples procesos.

## Funcionalidad

El sistema genera todas las combinaciones binarias posibles de un largo específico y para cada una:
1. Ejecuta el algoritmo de optimización de costos
2. Identifica las posiciones donde se debe encender la máquina
3. Determina el tipo de encendido (Frío, Tibio, Caliente)
4. Calcula el costo total
5. Genera un archivo CSV con los resultados

## Compilación y Ejecución

### Compilar
```bash
make mpi
```

### Ejecutar
```bash
# Ejemplo básico (4 combinaciones, 2 procesos)
mpirun -np 2 ./analisis_exhaustivo_mpi -l 4 -o resultados.csv

# Ejemplo con más procesos
mpirun -np 4 ./analisis_exhaustivo_mpi -l 5 -o resultados_5bits.csv

# Ver ayuda
./analisis_exhaustivo_mpi -h
```

## Formato de Salida CSV

El archivo CSV generado tiene las siguientes columnas:

- **Combinacion**: La secuencia binaria analizada (ej: "0 1 0 1")
- **Encendidos**: Secuencia completa de acciones necesarias (ej: "0C - 1C - 2T")
- **Costo_Total**: Costo total de la solución
- **Valida**: Si la solución es válida

### Códigos de Acciones

- **F**: Encendido en Frío / Apagado manteniendo frío (costo 1.0)
- **T**: Encendido en Tibio / Apagado manteniendo tibio (costo 2.5)
- **C**: Encendido en Caliente / Apagado manteniendo caliente (costo 5.0)

**Nota**: El contexto determina si es encendido o apagado según la transición detectada.

## Interpretación de Resultados

### Ejemplo 1: Combinación "0 1 0 1"
- Posición 0: Valor 0 → Requiere generación → Máquina ON/CALIENTE
- Posición 1: Valor 1 → No requiere generación → Puede estar OFF
- Posición 2: Valor 0 → Requiere generación → Máquina ON/CALIENTE  
- Posición 3: Valor 1 → No requiere generación → Puede estar OFF

**Resultado**: "0C - 3C" indica encendido caliente en posición 0 y acción en posición 3.

### Ejemplo 2: Combinación "0 1 1 0"
- Posición 0: Valor 0 → Requiere generación → Máquina ON/CALIENTE
- Posición 1: Valor 1 → No requiere generación → Apagar manteniendo calor
- Posición 2: Valor 1 → No requiere generación → Encender tibio para preparar  
- Posición 3: Valor 0 → Requiere generación → Máquina ON/CALIENTE

**Resultado**: "0C - 1C - 2T" indica la secuencia completa de acciones necesarias.

### Ejemplo 3: Combinación "1 1 0 0"
- Posición 0: Valor 1 → No requiere generación → Puede estar OFF
- Posición 1: Valor 1 → No requiere generación → Encender en Tibio para preparación
- Posición 2: Valor 0 → Requiere generación → Máquina ON/CALIENTE
- Posición 3: Valor 0 → Requiere generación → Máquina ON/CALIENTE

**Resultado**: "1T" indica que se enciende en posición 1 en estado Tibio.

## Reglas del Sistema

### Estados de la Máquina
- **ON/CALIENTE**: Genera energía (costo 5.0)
- **ON/TIBIO**: No genera energía (costo 2.5)
- **ON/FRIO**: No genera energía (costo 1.0)
- **OFF/CALIENTE**: No genera energía (costo 0.0)
- **OFF/TIBIO**: No genera energía (costo 0.0)
- **OFF/FRIO**: No genera energía (costo 0.0)

### Transiciones Válidas
- ON/CALIENTE → OFF/CALIENTE | ON/CALIENTE
- OFF/CALIENTE → ON/TIBIO | OFF/TIBIO
- ON/TIBIO → ON/CALIENTE | OFF/CALIENTE
- OFF/TIBIO → ON/FRIO | OFF/FRIO
- ON/FRIO → ON/TIBIO | OFF/TIBIO
- OFF/FRIO → ON/FRIO | OFF/FRIO

### Restricciones
1. Solo el estado ON/CALIENTE puede generar energía
2. Cuando el valor es 0, se DEBE generar energía (estado ON/CALIENTE)
3. Cuando el valor es 1, no se requiere generación propia

## Optimización y MPI

- El sistema distribuye automáticamente las combinaciones entre todos los procesos MPI disponibles
- Cada proceso ejecuta su porción del análisis de forma independiente
- El proceso 0 recopila todos los resultados y genera el archivo CSV final
- Se utiliza la caché del algoritmo original para optimizar el rendimiento

## Limitaciones

- Longitudes máximas recomendadas: hasta 15 bits (32,768 combinaciones)
- Para longitudes > 15, el tiempo de ejecución puede ser considerable
- El sistema está optimizado para ejecutarse en clusters con MPI instalado

## Archivos Relacionados

- `include/analisis_exhaustivo_mpi.hpp`: Definiciones de la clase principal
- `src/analisis_exhaustivo_mpi.cpp`: Implementación del análisis exhaustivo
- `src/main_exhaustivo_mpi.cpp`: Programa principal
- `Makefile`: Reglas de compilación (usar `make mpi`) 