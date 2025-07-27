# Análisis Exhaustivo de Máquina de Estados

## Descripción

Este sistema realiza un análisis exhaustivo de **todas las combinaciones posibles** de energía eólica para encontrar los patrones óptimos de operación de la máquina de estados.

## Problema Analizado

- **Demanda fija**: 300 200 100 100 100 200 300 500 800 1000 1000 1000 1000 900 800 800 800 1000 1000 1000 600 600 400 300
- **Energía eólica variable**: Cada hora puede tener 0 MW o 500 MW
- **Total de combinaciones**: 2^24 = 16,777,216 posibilidades
- **Objetivo**: Encontrar qué combinaciones de energía eólica minimizan el costo de operación

## Uso del Sistema

### Opción 1: Usar el programa demo directamente

```bash
./demo_analisis
# Ingresa el número de combinaciones a analizar (ej: 1000)
```

### Opción 2: Usar el script automatizado

```bash
./scripts/ejecutar_analisis.sh
```

El script ofrece opciones:
1. **Análisis rápido** (1,000 combinaciones) - ~1 minuto
2. **Análisis mediano** (100,000 combinaciones) - ~1-2 horas  
3. **Análisis por lotes** (configurable) - múltiples sesiones
4. **Análisis completo** (16,777,216 combinaciones) - días/semanas

## Interpretación de Resultados

### Formato del archivo CSV

```
CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas
63,000000000000000000111111,93.50,SI,18
```

- **CombinacionID**: Número único (0 a 16,777,215)
- **PatronEolica**: Patrón binario de 24 bits (posición = hora, 0=sin eólica, 1=con eólica)
- **CostoTotal**: Costo óptimo de operación encontrado
- **SolucionValida**: SI si se encontró solución válida
- **HorasCriticas**: Número de horas que requieren generación propia

### Análisis de Patrones

**Ejemplo de patrón óptimo encontrado:**
- Combinación 63: `000000000000000000111111` 
- Esto significa: energía eólica en horas 0,1,2,3,4,5
- Costo resultante: 93.50
- Horas críticas: 18

### Comandos útiles para analizar resultados

```bash
# Ver las 10 mejores combinaciones
sort -t, -k3 -n resultados.csv | head -10

# Ver combinaciones por número de horas críticas
sort -t, -k5 -n resultados.csv | head -10

# Filtrar solo combinaciones con costo menor a 100
awk -F, '$3<100 {print}' resultados.csv

# Contar soluciones válidas
grep -c ",SI," resultados.csv

# Estadísticas de costos
awk -F, 'NR>1 && $4=="SI" {sum+=$3; count++} END {print "Promedio:", sum/count}' resultados.csv
```

## Escalabilidad y Rendimiento

### Estimaciones de tiempo

| Combinaciones | Tiempo estimado | Recomendación |
|--------------|----------------|---------------|
| 1,000 | ~30 segundos | ✅ Prueba rápida |
| 10,000 | ~5 minutos | ✅ Exploración inicial |
| 100,000 | ~1-2 horas | ✅ Análisis serio |
| 1,000,000 | ~10-20 horas | ⚠️ Ejecución nocturna |
| 16,777,216 | Días/semanas | ⚠️ Solo si es crítico |

### Estrategias para análisis completo

1. **Análisis por lotes**: Dividir en chunks de 100,000
2. **Análisis distribuido**: Usar múltiples máquinas
3. **Análisis dirigido**: Enfocar en rangos prometedores
4. **Análisis muestreado**: Analizar subconjuntos representativos

## Insights del Análisis

### Patrones Observados (muestra de 100 combinaciones)

- **Mejor costo encontrado**: 93.50 (Combinación 63)
- **Patrón óptimo**: Energía eólica en las primeras 6 horas
- **Tasa de éxito**: 100% (todas las combinaciones tienen solución válida)
- **Rango de costos**: 93.50 - 120.00

### Hipótesis para optimización

1. **Horas tempranas críticas**: Tener eólica en horas 0-5 reduce significativamente costos
2. **Demanda pico**: Las horas de mayor demanda (9-19) son más costosas sin eólica
3. **Transiciones de estado**: El algoritmo optimiza las transiciones para minimizar costos intermedios

## Uso Avanzado

### Modificar parámetros

Para cambiar la demanda o costos de mantenimiento, edita:
- **Demanda**: Línea `demanda_fija` en `src/demo_analisis.cpp`
- **Costos**: Línea `configurarCostos(1.0, 2.5, 5.0)` 

### Análisis personalizado

```cpp
// Ejemplo: analizar solo combinaciones con eólica en horas específicas
for (uint32_t combinacion = 0; combinacion < num_combinaciones; combinacion++) {
    std::bitset<24> patron(combinacion);
    
    // Filtro: solo analizar si tiene eólica en hora 0
    if (!patron[0]) continue;
    
    // ... resto del análisis
}
```

## Limitaciones

1. **Memoria**: ~1 GB para análisis completo
2. **Disco**: ~2 GB para almacenar todos los resultados  
3. **CPU**: Intensivo en cómputo, beneficia de múltiples cores
4. **Tiempo**: El análisis completo es prohibitivamente largo para uso interactivo

## Conclusiones

Este sistema permite:
- ✅ Encontrar patrones óptimos de energía eólica
- ✅ Cuantificar el impacto de diferentes configuraciones
- ✅ Identificar horas críticas donde la eólica es más valiosa
- ✅ Proporcionar datos para optimización posterior

**Recomendación**: Comienza con análisis de 10,000-100,000 combinaciones para identificar patrones prometedores, luego enfócate en rangos específicos para análisis más detallados.
