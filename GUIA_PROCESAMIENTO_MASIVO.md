# Guía de Procesamiento Masivo - Máquina de Estados

## Sistema Optimizado para Análisis de Gran Escala

Este sistema ha sido diseñado específicamente para **procesamiento masivo** de combinaciones, enfocándose en eficiencia, seguimiento de progreso y resultados estadísticos.

## 🚀 Uso Rápido

```bash
# Ejecutar análisis masivo
./scripts/ejecutar_analisis.sh

# Analizar resultados
./scripts/analizar_resultados.sh resultados/archivo.csv
```

## 📊 Características del Sistema Simplificado

### ✅ Lo que SÍ incluye:
- **Progreso en tiempo real** con barra visual
- **Estadísticas de rendimiento** (casos/segundo, ETA)
- **Métricas de calidad** (% soluciones válidas, mejor costo)
- **Resumen final** conciso con estadísticas clave
- **Procesamiento por lotes** para análisis grandes
- **Monitoreo de tiempo** con estimaciones precisas

### ❌ Lo que NO incluye (eliminado para eficiencia):
- Salidas detalladas de cada caso individual
- Información de transiciones por caso
- Proceso de optimización paso a paso
- Headers y mensajes verbosos innecesarios

## 🎯 Opciones de Análisis

| Opción | Combinaciones | Tiempo Estimado | Uso Recomendado |
|--------|---------------|-----------------|-----------------|
| **1** | 1,000 | ~30 segundos | ✅ Prueba rápida |
| **2** | 10,000 | ~5 minutos | ✅ Exploración |
| **3** | 100,000 | ~1 hora | ✅ Análisis serio |
| **4** | 1,000,000 | ~10 horas | ⚠️ Análisis pesado |
| **5** | Personalizado | Variable | 🔧 Lotes configurables |
| **6** | 16,777,216 | Días | ⚠️ Solo extremo |

## 📈 Ejemplo de Salida Simplificada

```
🚀 INICIANDO ANÁLISIS MEDIANO
Combinaciones: 100,000
Tiempo estimado: ~1 hora

=== ANALIZADOR MASIVO MODO SILENCIOSO ===
Procesando 100000 combinaciones...
Progreso: [==========] 100.0% | Casos: 100000/100000 | Válidos: 100000 | Mejor: 83.5 | Tasa: 2847 c/s | ETA: 0m0s

=== PROCESAMIENTO COMPLETADO ===
Combinaciones procesadas: 100000
Tiempo total: 35 segundos
Tasa promedio: 2857.1 combinaciones/segundo
Soluciones válidas: 100000 (100.0%)
Mejor costo encontrado: 83.5 (combinación #1023)
Costo promedio: 109.31

✅ Análisis completado en 35 segundos (0m 35s)
```

## 📊 Análisis de Resultados

### Comando de análisis automático:
```bash
./scripts/analizar_resultados.sh resultados/archivo.csv
```

### Ejemplo de análisis estadístico:
```
📊 === ANÁLISIS DE RESULTADOS ===
🔢 ESTADÍSTICAS GENERALES:
   Total de casos: 10000
   Casos válidos: 10000 (100.0%)

🏆 TOP 5 MEJORES COSTOS:
   Costo: 83.50 | Horas críticas: 16 | Patrón: 000000000000001111111111

💰 ESTADÍSTICAS DE COSTOS:
   Costo mínimo: 83.50
   Costo máximo: 120.00
   Costo promedio: 109.32

⏰ DISTRIBUCIÓN POR HORAS CRÍTICAS:
   16 horas: 39 casos (0.3%)
   20 horas: 2731 casos (27.3%)
   24 horas: 40 casos (0.4%)
```

## 🔧 Procesamiento por Lotes

Para análisis masivos, usa la **opción 5** (personalizado):

```
Configuración:
- Total de combinaciones: 1,000,000
- Número de lotes: 10
- Combinaciones por lote: 100,000

📦 LOTE 1/10 - Procesando 100,000 combinaciones
✅ Lote 1 completado en 35 segundos

📦 LOTE 2/10 - Procesando 100,000 combinaciones
✅ Lote 2 completado en 34 segundos
...
```

## 📁 Gestión de Archivos

### Estructura de salida:
```
resultados/
├── analisis_1k_20250726_180119.csv      # Prueba rápida
├── analisis_10k_20250726_180127.csv     # Análisis pequeño
├── analisis_100k_20250726_180145.csv    # Análisis mediano
├── lote_0_20250726_180200.csv           # Lote 1 de análisis personalizado
├── lote_1_20250726_180205.csv           # Lote 2 de análisis personalizado
└── analisis_completo.log                # Log de análisis completo
```

### Comandos útiles para gestión:
```bash
# Ver archivos generados
ls -lh resultados/

# Combinar múltiples lotes
head -1 resultados/lote_0_*.csv > combined.csv
tail -n +2 resultados/lote_*.csv >> combined.csv

# Estadísticas rápidas
wc -l resultados/*.csv
```

## ⚡ Optimizaciones de Rendimiento

### El sistema incluye:
- **Supresión de stdout** durante resolución individual
- **Reportes de progreso** optimizados (cada 1% del total)
- **Escritura directa a CSV** sin buffers intermedios
- **Cálculos estadísticos** incrementales en memoria
- **Estimación de tiempo** basada en tasa real de procesamiento

### Rendimiento típico:
- **Hardware moderno**: 2,000-5,000 combinaciones/segundo
- **Análisis de 100K**: 20-50 segundos
- **Análisis de 1M**: 3-8 minutos
- **Análisis completo**: 1-3 días

## 🎯 Insights de Optimización

### Patrones encontrados en análisis masivos:

1. **Mejor costo**: 83.5 (configuraciones con 8-11 horas de eólica)
2. **Distribución óptima**: 27.3% de casos tienen 20 horas críticas
3. **Eficiencia energética**: 100% de casos tienen solución válida
4. **Patrones exitosos**: Eólica concentrada en períodos específicos

### Horas críticas más comunes:
- **16-18 horas**: Configuraciones óptimas (mejor costo)
- **19-21 horas**: Configuraciones balanceadas (mayoría)
- **22-24 horas**: Configuraciones subóptimas (peor costo)

## 🔍 Comandos de Análisis Avanzado

```bash
# Filtrar solo mejores resultados (costo < 90)
awk -F, 'NR>1 && $3<90 {print}' archivo.csv

# Contar distribución de horas críticas
awk -F, 'NR>1 {print $5}' archivo.csv | sort -n | uniq -c

# Encontrar patrones con eólica específica
grep "111111000000000000000000" archivo.csv

# Estadísticas por rango de costos
awk -F, 'NR>1 && $3>=80 && $3<=90 {count++} END {print "Costos 80-90:", count}' archivo.csv
```

Este sistema está optimizado para **análisis masivo eficiente** manteniendo **visibilidad completa** del progreso y resultados estadísticos relevantes.
