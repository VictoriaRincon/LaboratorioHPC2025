#!/bin/bash

# Script para fusionar resultados de múltiples procesos MPI
# Combina todos los archivos CSV generados por los diferentes ranks

echo "=== FUSIONADOR DE RESULTADOS MPI ==="
echo "Este script combina los resultados de múltiples procesos MPI en un archivo único"
echo ""

# Función para mostrar ayuda
mostrar_ayuda() {
    echo "Uso: $0 [patrón_archivo] [archivo_salida]"
    echo ""
    echo "Ejemplos:"
    echo "  $0                                    # Fusionar todos los archivos *_rank*.csv"
    echo "  $0 resultados_prueba_100k_mpi        # Fusionar archivos que coincidan con este patrón"
    echo "  $0 resultados_custom resultado_final # Especificar patrón y archivo de salida"
    echo ""
    exit 1
}

# Procesar argumentos
PATRON=""
ARCHIVO_SALIDA=""

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    mostrar_ayuda
fi

if [ $# -eq 0 ]; then
    # Modo automático: buscar todos los archivos MPI
    PATRON="*_rank*.csv"
    ARCHIVO_SALIDA="resultados_mpi_fusionados.csv"
elif [ $# -eq 1 ]; then
    # Solo patrón especificado
    PATRON="${1}*_rank*.csv"
    ARCHIVO_SALIDA="${1}_fusionado.csv"
elif [ $# -eq 2 ]; then
    # Patrón y archivo de salida especificados
    PATRON="${1}*_rank*.csv"
    ARCHIVO_SALIDA="${2}.csv"
else
    echo "❌ Error: Demasiados argumentos"
    mostrar_ayuda
fi

echo "🔍 Buscando archivos con patrón: $PATRON"

# Encontrar archivos que coincidan
archivos=($(ls $PATRON 2>/dev/null))

if [ ${#archivos[@]} -eq 0 ]; then
    echo "❌ Error: No se encontraron archivos CSV con el patrón '$PATRON'"
    echo ""
    echo "Archivos disponibles:"
    ls -la *_rank*.csv 2>/dev/null || echo "  (ningún archivo encontrado)"
    exit 1
fi

echo "📁 Archivos encontrados: ${#archivos[@]}"
for archivo in "${archivos[@]}"; do
    size=$(wc -l < "$archivo" 2>/dev/null || echo "0")
    echo "  - $archivo ($((size-1)) resultados)"
done

echo ""
echo "🔄 Fusionando resultados en: $ARCHIVO_SALIDA"

# Crear archivo temporal para ordenar
TEMP_FILE=$(mktemp)

# Escribir header una sola vez
echo "CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,ProcesoMPI" > "$TEMP_FILE"

# Procesar cada archivo
total_resultados=0
for i in "${!archivos[@]}"; do
    archivo="${archivos[$i]}"
    
    # Extraer número de rank del nombre del archivo
    rank=$(echo "$archivo" | sed -n 's/.*_rank\([0-9]\+\)\.csv/\1/p')
    
    # Agregar columna de proceso MPI y skip header
    if [ -f "$archivo" ]; then
        tail -n +2 "$archivo" | while IFS= read -r line; do
            echo "$line,$rank"
        done >> "$TEMP_FILE"
        
        lines=$(tail -n +2 "$archivo" | wc -l)
        total_resultados=$((total_resultados + lines))
        echo "  ✅ Procesado $archivo (rank $rank): $lines resultados"
    fi
done

# Ordenar por CombinacionID para mantener orden lógico
echo ""
echo "📊 Ordenando resultados por ID de combinación..."
(head -n 1 "$TEMP_FILE"; tail -n +2 "$TEMP_FILE" | sort -t',' -k1,1n) > "$ARCHIVO_SALIDA"

# Limpiar archivo temporal
rm -f "$TEMP_FILE"

# Calcular estadísticas
echo ""
echo "=== ESTADÍSTICAS DE FUSIÓN ==="
total_lines=$(wc -l < "$ARCHIVO_SALIDA")
total_results=$((total_lines - 1))
echo "📈 Total de resultados fusionados: $total_results"

# Analizar soluciones válidas
soluciones_validas=$(tail -n +2 "$ARCHIVO_SALIDA" | grep -c ",SI,")
echo "✅ Soluciones válidas: $soluciones_validas"

if [ $soluciones_validas -gt 0 ]; then
    # Encontrar mejor y peor costo
    mejor_costo=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",SI," | cut -d',' -f3 | sort -n | head -1)
    peor_costo=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",SI," | cut -d',' -f3 | sort -n | tail -1)
    
    echo "💰 Mejor costo encontrado: $mejor_costo"
    echo "💸 Peor costo encontrado: $peor_costo"
    
    # Mostrar las mejores soluciones
    echo ""
    echo "🏆 TOP 5 MEJORES SOLUCIONES:"
    echo "Rank | CombID | Patrón | Costo | Horas Críticas | Proceso"
    echo "-----|--------|--------|-------|----------------|--------"
    tail -n +2 "$ARCHIVO_SALIDA" | grep ",SI," | sort -t',' -k3,3n | head -5 | \
    awk -F',' '{printf "%4d | %6s | %s | %5.1f | %13s | %7s\n", NR, $1, substr($2,1,8)"...", $3, $5, $6}'
fi

# Analizar distribución por proceso
echo ""
echo "📊 DISTRIBUCIÓN POR PROCESO MPI:"
echo "Proceso | Resultados | Soluciones Válidas"
echo "--------|------------|------------------"
for archivo in "${archivos[@]}"; do
    rank=$(echo "$archivo" | sed -n 's/.*_rank\([0-9]\+\)\.csv/\1/p')
    total_proc=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",$rank$" | wc -l)
    validas_proc=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",$rank$" | grep ",SI," | wc -l)
    printf "%7s | %10s | %17s\n" "$rank" "$total_proc" "$validas_proc"
done

echo ""
echo "=== FUSIÓN COMPLETADA ==="
echo "📄 Archivo generado: $ARCHIVO_SALIDA"
echo "📊 Total de líneas: $total_lines (incluyendo header)"
echo ""

# Generar estadísticas adicionales
STATS_FILE="${ARCHIVO_SALIDA%.csv}_estadisticas.txt"
echo "📈 Generando estadísticas detalladas en: $STATS_FILE"

cat > "$STATS_FILE" << EOF
=== ESTADÍSTICAS DETALLADAS MPI ===
Generado: $(date)
Archivos fusionados: ${#archivos[@]}
Patrón usado: $PATRON

RESUMEN GENERAL:
- Total de combinaciones analizadas: $total_results
- Soluciones válidas encontradas: $soluciones_validas
- Porcentaje de éxito: $(echo "scale=2; $soluciones_validas * 100 / $total_results" | bc 2>/dev/null || echo "N/A")%

ANÁLISIS DE COSTOS:
$(if [ $soluciones_validas -gt 0 ]; then
    echo "- Costo mínimo: $mejor_costo"
    echo "- Costo máximo: $peor_costo"
    promedio=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",SI," | cut -d',' -f3 | awk '{sum+=$1; count++} END {if(count>0) print sum/count; else print "N/A"}')
    echo "- Costo promedio: $promedio"
else
    echo "- No hay soluciones válidas para analizar"
fi)

DISTRIBUCIÓN POR PROCESO:
$(for archivo in "${archivos[@]}"; do
    rank=$(echo "$archivo" | sed -n 's/.*_rank\([0-9]\+\)\.csv/\1/p')
    total_proc=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",$rank$" | wc -l)
    validas_proc=$(tail -n +2 "$ARCHIVO_SALIDA" | grep ",$rank$" | grep ",SI," | wc -l)
    echo "- Proceso $rank: $total_proc resultados ($validas_proc válidos)"
done)

ARCHIVOS ORIGINALES:
$(for archivo in "${archivos[@]}"; do
    echo "- $archivo"
done)
EOF

echo "✅ Estadísticas guardadas en: $STATS_FILE"
echo ""
echo "💡 COMANDOS ÚTILES PARA ANÁLISIS:"
echo "1. Ver mejores resultados:"
echo "   head -20 $ARCHIVO_SALIDA"
echo ""
echo "2. Buscar patrones específicos:"
echo "   grep '111111' $ARCHIVO_SALIDA | head -10"
echo ""
echo "3. Analizar por rango de costos:"
echo "   awk -F',' '\$3 < 100 && \$4 == \"SI\"' $ARCHIVO_SALIDA"
echo ""
echo "🎉 ¡Fusión de resultados MPI completada exitosamente!" 