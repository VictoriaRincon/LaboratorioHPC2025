#!/bin/bash

# Script para comparar rendimiento entre versiones MPI con y sin cache

echo "========================================================"
echo "PRUEBA DE RENDIMIENTO: MPI CON VS SIN CACHE DE PREFIJOS"
echo "========================================================"

# Configuración de la prueba
NUM_PROCESOS=4
NUM_COMBINACIONES=20000

echo "Configuración de la prueba:"
echo "- Procesos MPI: $NUM_PROCESOS"
echo "- Combinaciones: $NUM_COMBINACIONES"
echo ""

# Compilar ambas versiones
echo "Compilando versiones..."
make demo_analisis_con_transiciones_mpi demo_analisis_con_cache_mpi > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Error en la compilación"
    exit 1
fi

echo "✅ Compilación exitosa"
echo ""

# Función para ejecutar y medir tiempo
medir_tiempo() {
    local ejecutable=$1
    local nombre=$2
    local archivo_resultado=$3
    
    echo "🔄 Ejecutando: $nombre..."
    start_time=$(date +%s.%N)
    echo "$NUM_COMBINACIONES" | mpirun -np $NUM_PROCESOS ./$ejecutable > temp_output.txt 2>&1
    end_time=$(date +%s.%N)
    
    # Calcular tiempo transcurrido
    tiempo_total=$(echo "$end_time - $start_time" | bc -l)
    
    # Extraer estadísticas del output
    combinaciones_procesadas=$(grep "Combinaciones procesadas:" temp_output.txt | awk '{print $3}')
    soluciones_validas=$(grep "Soluciones válidas:" temp_output.txt | awk '{print $3}')
    mejor_costo=$(grep "Mejor costo encontrado:" temp_output.txt | awk '{print $4}')
    
    # Para la versión con cache, extraer estadísticas del cache
    if [[ "$nombre" == *"Cache"* ]]; then
        cache_hits=$(grep "Total hits" temp_output.txt | awk '{print $6}')
        cache_misses=$(grep "Total misses" temp_output.txt | awk '{print $6}')
        tasa_aciertos=$(grep "Tasa de aciertos global:" temp_output.txt | awk '{print $5}' | sed 's/%//')
        entradas_cache=$(grep "Total entradas en cache:" temp_output.txt | awk '{print $5}')
    fi
    
    echo "✅ $nombre completado en ${tiempo_total}s"
    
    # Guardar resultados
    echo "$nombre,$tiempo_total,$combinaciones_procesadas,$soluciones_validas,$mejor_costo" >> $archivo_resultado
    
    if [[ "$nombre" == *"Cache"* ]]; then
        echo "   Cache hits: $cache_hits, misses: $cache_misses"
        echo "   Tasa de aciertos: $tasa_aciertos%"
        echo "   Entradas en cache: $entradas_cache"
        echo "$nombre,$cache_hits,$cache_misses,$tasa_aciertos,$entradas_cache" >> estadisticas_cache.csv
    fi
    
    # Renombrar archivo de resultados si existe
    if [ -f "resultados_demo.csv" ]; then
        mv resultados_demo.csv "resultados_${nombre,,}.csv"
    elif [ -f "resultados_demo_cache.csv" ]; then
        mv resultados_demo_cache.csv "resultados_${nombre,,}.csv"
    fi
    
    echo ""
}

# Crear archivos de resultados
echo "Version,TiempoTotal,Combinaciones,SolucionesValidas,MejorCosto" > comparacion_rendimiento.csv
echo "Version,CacheHits,CacheMisses,TasaAciertos,EntradasCache" > estadisticas_cache.csv

# Ejecutar pruebas
echo "Iniciando pruebas de rendimiento..."
echo ""

# Versión sin cache
medir_tiempo "demo_analisis_con_transiciones_mpi" "MPI_Sin_Cache" "comparacion_rendimiento.csv"

# Versión con cache
medir_tiempo "demo_analisis_con_cache_mpi" "MPI_Con_Cache" "comparacion_rendimiento.csv"

# Analizar resultados
echo "========================================================"
echo "ANÁLISIS DE RESULTADOS"
echo "========================================================"

if [ -f "comparacion_rendimiento.csv" ]; then
    echo "Comparación de rendimiento:"
    cat comparacion_rendimiento.csv | column -t -s ','
    echo ""
    
    # Calcular mejora en rendimiento
    tiempo_sin_cache=$(tail -n 2 comparacion_rendimiento.csv | head -n 1 | cut -d',' -f2)
    tiempo_con_cache=$(tail -n 1 comparacion_rendimiento.csv | cut -d',' -f2)
    
    if [ $(echo "$tiempo_sin_cache > 0" | bc -l) -eq 1 ] && [ $(echo "$tiempo_con_cache > 0" | bc -l) -eq 1 ]; then
        mejora=$(echo "scale=2; ($tiempo_sin_cache - $tiempo_con_cache) / $tiempo_sin_cache * 100" | bc -l)
        speedup=$(echo "scale=2; $tiempo_sin_cache / $tiempo_con_cache" | bc -l)
        
        echo "📊 MÉTRICAS DE RENDIMIENTO:"
        echo "   Tiempo sin cache: ${tiempo_sin_cache}s"
        echo "   Tiempo con cache: ${tiempo_con_cache}s"
        
        if [ $(echo "$mejora > 0" | bc -l) -eq 1 ]; then
            echo "   ✅ Mejora: ${mejora}% más rápido"
            echo "   ⚡ Speedup: ${speedup}x"
        else
            echo "   ⚠️  El cache no mostró mejora significativa"
            echo "      (Normal con patrones muy diversos)"
        fi
    fi
fi

echo ""
if [ -f "estadisticas_cache.csv" ]; then
    echo "Estadísticas del cache:"
    cat estadisticas_cache.csv | column -t -s ','
fi

# Limpiar archivos temporales
rm -f temp_output.txt

echo ""
echo "========================================================"
echo "Archivos generados:"
echo "- comparacion_rendimiento.csv"
echo "- estadisticas_cache.csv"
echo "- resultados_mpi_sin_cache.csv"
echo "- resultados_mpi_con_cache.csv"
echo "========================================================"
