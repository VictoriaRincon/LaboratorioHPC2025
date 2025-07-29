# Sistema de Cache de Prefijos MPI

## 📋 Descripción General

Este sistema implementa un **cache de prefijos eficiente** usando MPI que permite:

- **Reutilización de soluciones parciales**: Evita recalcular prefijos similares entre diferentes escenarios eólicos
- **Distribución inteligente del trabajo**: Cada proceso MPI mantiene su cache local y comparte prefijos valiosos
- **Optimización automática**: Solo cachea prefijos que justifican el costo computacional
- **Intercambio colaborativo**: Los procesos comparten los mejores prefijos periódicamente

## 🏗️ Arquitectura del Sistema

### Componentes Principales

1. **`CachePrefijos`**: Clase principal que maneja el cache local de cada proceso
2. **`EntradaCache`**: Estructura que almacena información detallada de cada prefijo
3. **`PrefijoMPI`**: Estructura optimizada para comunicación entre procesos
4. **Sistema de intercambio**: Mecanismo para compartir prefijos valiosos

### Flujo de Trabajo

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Proceso 0     │    │   Proceso 1     │    │   Proceso N     │
│                 │    │                 │    │                 │
│ Cache Local     │    │ Cache Local     │    │ Cache Local     │
│ - Prefijos      │    │ - Prefijos      │    │ - Prefijos      │
│ - Estadísticas  │    │ - Estadísticas  │    │ - Estadísticas  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────────┐
                    │ Intercambio         │
                    │ Periódico de        │
                    │ Prefijos Valiosos   │
                    └─────────────────────┘
```

## 🔧 Características Técnicas

### Criterios de Cache

- **Umbral de tiempo**: Solo cachea prefijos que tomaron ≥ 10ms calcular
- **Frecuencia mínima**: Solo intercambia prefijos usados ≥ 3 veces
- **Tamaño máximo**: Límite de 10,000 entradas por proceso
- **Longitud de prefijo**: Entre 4 y 12 horas

### Estrategias de Optimización

1. **Cache por tiempo de cómputo**: Prefijos costosos tienen mayor prioridad
2. **Limpieza automática**: Elimina entradas antiguas cuando el cache se llena
3. **Intercambio inteligente**: Solo comparte prefijos "valiosos"
4. **Distribución jerárquica**: Proceso 0 filtra y redistribuye

## 📊 Métricas y Estadísticas

### Métricas Locales (por proceso)
- **Cache hits/misses**: Efectividad del cache local
- **Tasa de aciertos**: Porcentaje de éxito del cache
- **Tamaño del cache**: Número de entradas almacenadas
- **Prefijos enviados/recibidos**: Actividad de intercambio

### Métricas Globales
- **Estadísticas agregadas**: Suma de todos los procesos
- **Tasa de aciertos global**: Efectividad del sistema completo
- **Total de entradas**: Cache distribuido total

## 🚀 Uso del Sistema

### Compilación

```bash
# Compilar todas las versiones
make

# Solo la versión con cache
make demo_analisis_con_cache_mpi
```

### Ejecución

```bash
# Ejecutar con 4 procesos
make run-cache

# Ejecutar test de rendimiento
make test-cache

# Ejecución manual
mpirun -np 8 ./demo_analisis_con_cache_mpi
```

### Entrada del Programa

```
Ingrese número de combinaciones a procesar: 50000
```

## 📈 Comparación de Rendimiento

### Sin Cache (Original)
- Cada escenario se resuelve desde cero
- Tiempo de cómputo lineal con el número de combinaciones
- Sin reutilización de trabajo previo

### Con Cache de Prefijos
- Reutilización de soluciones parciales similares
- Aceleración significativa en escenarios con patrones repetitivos
- Mayor eficiencia en conjuntos de datos grandes

## ⚙️ Configuración Avanzada

### Parámetros Ajustables

```cpp
cache.configurar(
    5.0,    // umbral_tiempo_ms: Solo cachear si > 5ms
    2,      // min_frecuencia: Intercambiar si ≥ 2 usos
    5000    // max_size: Máximo 5000 entradas por proceso
);
```

### Intervalos de Intercambio

```cpp
const uint32_t INTERVALO_INTERCAMBIO = num_locales / 5;  // Cada 20% del trabajo
```

## 📋 Ejemplo de Salida

```
=== ANALIZADOR MASIVO CON CACHE DE PREFIJOS MPI ===
Procesos MPI: 4
Procesando 10000 combinaciones con 4 procesos y cache de prefijos...

Proceso 0 - Progreso: 100.0% | Casos: 2500/2500 | Tasa: 1250 c/s | Cache: 450H/1050M

Intercambio de cache: recopilados 1200 prefijos, distribuyendo 120

=== PROCESAMIENTO COMPLETADO ===
Combinaciones procesadas: 10000
Procesos MPI utilizados: 4
Tiempo total: 8 segundos
Tasa promedio: 1250.0 combinaciones/segundo
Soluciones válidas: 8500 (85.0%)
Mejor costo encontrado: 15420.50 (combinación #7854)
Costo promedio: 18750.25

=== ESTADÍSTICAS GLOBALES DE CACHE ===
Total hits (todos los procesos): 1800
Total misses (todos los procesos): 4200
Total entradas en cache: 3500
Tasa de aciertos global: 30.0%
```

## 🔍 Casos de Uso Óptimos

### Ideales para Cache
- **Escenarios con patrones**: Viento presente en horarios similares
- **Conjuntos grandes**: Miles de combinaciones
- **Demanda estable**: Patrones de demanda repetitivos

### Menos Beneficiosos
- **Escenarios únicos**: Cada combinación completamente diferente
- **Conjuntos pequeños**: Pocas combinaciones (< 1000)
- **Alta variabilidad**: Patrones de demanda muy cambiantes

## 🛠️ Desarrollo y Extensiones

### Posibles Mejoras

1. **Cache persistente**: Guardar cache en disco entre ejecuciones
2. **Algoritmos adaptativos**: Ajustar parámetros automáticamente
3. **Cache distribuido avanzado**: Intercambio más sofisticado
4. **Compresión**: Reducir memoria usando técnicas de compresión

### Monitoreo y Debug

- Estadísticas detalladas por proceso
- Análisis de efectividad del cache
- Métricas de comunicación MPI
- Perfilado de tiempo de intercambio

## 📄 Archivos del Sistema

```
include/
├── cache_prefijos_mpi.hpp      # Definiciones del sistema de cache
src/
├── cache_prefijos_mpi.cpp      # Implementación del cache
├── demo_analisis_con_cache_mpi.cpp  # Programa principal con cache
resultados/
└── resultados_demo_cache.csv   # Resultados con información del cache
```
