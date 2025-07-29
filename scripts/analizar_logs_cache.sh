#!/bin/bash

# Script para analizar los logs de cache de prefijos

echo "========================================"
echo "ANÁLISIS DE LOGS DEL CACHE DE PREFIJOS"
echo "========================================"

# Verificar que existan archivos de log
if [ ! -f cache_log_proceso_0.txt ]; then
    echo "❌ No se encontraron archivos de log. Ejecute primero el programa con cache."
    exit 1
fi

echo "📊 Archivos de log encontrados:"
ls -la cache_log_proceso_*.txt
echo ""

# Función para analizar un archivo de log
analizar_log() {
    local archivo=$1
    local proceso=$(echo $archivo | grep -o '[0-9]\+')
    
    echo "=== ANÁLISIS DEL PROCESO $proceso ==="
    
    # Contar tipos de eventos
    local cache_hits=$(grep -c "CACHE_HIT" $archivo)
    local cache_misses=$(grep -c "CACHE_MISS" $archivo)
    local cache_adds=$(grep -c "CACHE_ADD" $archivo)
    local cache_skips=$(grep -c "CACHE_SKIP" $archivo)
    local enviados=$(grep -c "ENVIADO" $archivo)
    local recibidos=$(grep -c "RECIBIDO" $archivo)
    local limpiezas=$(grep -c "LIMPIEZA" $archivo)
    
    echo "Eventos registrados:"
    echo "  Cache Hits: $cache_hits"
    echo "  Cache Misses: $cache_misses"
    echo "  Prefijos Agregados: $cache_adds"
    echo "  Prefijos Descartados: $cache_skips"
    echo "  Prefijos Enviados: $enviados"
    echo "  Prefijos Recibidos: $recibidos"
    echo "  Limpiezas: $limpiezas"
    
    # Calcular tasa de aciertos si hay hits y misses
    if [ $((cache_hits + cache_misses)) -gt 0 ]; then
        local tasa=$(echo "scale=2; $cache_hits * 100 / ($cache_hits + $cache_misses)" | bc -l)
        echo "  Tasa de aciertos: ${tasa}%"
    fi
    
    echo ""
    
    # Mostrar algunos ejemplos de prefijos cacheados
    if [ $cache_adds -gt 0 ]; then
        echo "📋 Ejemplos de prefijos agregados al cache:"
        grep "CACHE_ADD" $archivo | head -5 | while read linea; do
            patron=$(echo $linea | cut -d',' -f3)
            longitud=$(echo $linea | cut -d',' -f4)
            costo=$(echo $linea | cut -d',' -f5)
            tiempo=$(echo $linea | cut -d',' -f6)
            echo "  Patrón: $patron (longitud: $longitud, costo: $costo, tiempo: ${tiempo}ms)"
        done
        echo ""
    fi
    
    # Mostrar prefijos más utilizados
    if [ $cache_hits -gt 0 ]; then
        echo "🔥 Prefijos más reutilizados:"
        grep "CACHE_HIT" $archivo | cut -d',' -f3,7 | sort | uniq -c | sort -nr | head -5 | while read count patron_freq; do
            patron=$(echo $patron_freq | cut -d',' -f1)
            freq=$(echo $patron_freq | cut -d',' -f2)
            echo "  $patron (usado $count veces, frecuencia actual: $freq)"
        done
        echo ""
    fi
}

# Analizar cada archivo de log
for archivo in cache_log_proceso_*.txt; do
    if [ -f "$archivo" ]; then
        analizar_log "$archivo"
    fi
done

# Estadísticas globales
echo "========================================"
echo "ESTADÍSTICAS GLOBALES"
echo "========================================"

total_hits=0
total_misses=0
total_adds=0
total_skips=0

for archivo in cache_log_proceso_*.txt; do
    if [ -f "$archivo" ]; then
        hits=$(grep -c "CACHE_HIT" $archivo)
        misses=$(grep -c "CACHE_MISS" $archivo)
        adds=$(grep -c "CACHE_ADD" $archivo)
        skips=$(grep -c "CACHE_SKIP" $archivo)
        
        total_hits=$((total_hits + hits))
        total_misses=$((total_misses + misses))
        total_adds=$((total_adds + adds))
        total_skips=$((total_skips + skips))
    fi
done

echo "Total de eventos en todos los procesos:"
echo "  Cache Hits: $total_hits"
echo "  Cache Misses: $total_misses"
echo "  Prefijos Agregados: $total_adds"
echo "  Prefijos Descartados: $total_skips"

if [ $((total_hits + total_misses)) -gt 0 ]; then
    tasa_global=$(echo "scale=2; $total_hits * 100 / ($total_hits + $total_misses)" | bc -l)
    echo "  Tasa de aciertos global: ${tasa_global}%"
fi

# Generar resumen CSV
echo ""
echo "📁 Generando resumen en CSV..."
echo "Proceso,CacheHits,CacheMisses,PrefijosAgregados,PrefijosDescartados,TasaAciertos" > resumen_cache_logs.csv

for archivo in cache_log_proceso_*.txt; do
    if [ -f "$archivo" ]; then
        proceso=$(echo $archivo | grep -o '[0-9]\+')
        hits=$(grep -c "CACHE_HIT" $archivo)
        misses=$(grep -c "CACHE_MISS" $archivo)
        adds=$(grep -c "CACHE_ADD" $archivo)
        skips=$(grep -c "CACHE_SKIP" $archivo)
        
        if [ $((hits + misses)) -gt 0 ]; then
            tasa=$(echo "scale=2; $hits * 100 / ($hits + $misses)" | bc -l)
        else
            tasa="0.00"
        fi
        
        echo "$proceso,$hits,$misses,$adds,$skips,$tasa" >> resumen_cache_logs.csv
    fi
done

echo "✅ Resumen guardado en: resumen_cache_logs.csv"
echo ""
echo "========================================"
echo "Para ver los logs completos:"
echo "  head -50 cache_log_proceso_0.txt"
echo "  grep 'CACHE_HIT' cache_log_proceso_*.txt"
echo "  grep 'CACHE_ADD' cache_log_proceso_*.txt"
echo "========================================"
