#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Uso: $0 <archivo_resultados.csv>"
    echo ""
    echo "Archivos disponibles:"
    ls -1 resultados/*.csv 2>/dev/null || echo "No hay archivos de resultados"
    exit 1
fi

archivo=$1

if [ ! -f "$archivo" ]; then
    echo "Error: El archivo $archivo no existe"
    exit 1
fi

echo "📊 === ANÁLISIS DE RESULTADOS CON TRANSICIONES ==="
echo "Archivo: $(basename $archivo)"
echo ""

# Verificar si el archivo tiene la nueva columna de transiciones
header=$(head -1 "$archivo")
if [[ $header == *"Transiciones"* ]]; then
    TIENE_TRANSICIONES=true
    echo "✅ Archivo con análisis de transiciones detectado"
else
    TIENE_TRANSICIONES=false
    echo "ℹ️  Archivo sin análisis de transiciones (formato anterior)"
fi
echo ""

# Estadísticas básicas
total_casos=$(tail -n +2 "$archivo" | wc -l)
casos_validos=$(awk -F, 'NR>1 && $4=="SI" {count++} END {print count+0}' "$archivo")
porcentaje_exito=$(echo "scale=1; $casos_validos * 100 / $total_casos" | bc)

echo "🔢 ESTADÍSTICAS GENERALES:"
echo "   Total de casos: $total_casos"
echo "   Casos válidos: $casos_validos ($porcentaje_exito%)"
echo ""

# Mejores resultados
echo "🏆 TOP 5 MEJORES COSTOS:"
if [ "$TIENE_TRANSICIONES" = true ]; then
    awk -F, 'NR>1 {print $0}' "$archivo" | sort -t, -k3 -n | head -5 | while IFS=, read id patron costo valido horas transiciones; do
        echo "   Costo: $costo | Horas críticas: $horas | Transiciones: [$transiciones] | Patrón: $patron"
    done
else
    awk -F, 'NR>1 {print $0}' "$archivo" | sort -t, -k3 -n | head -5 | while IFS=, read id patron costo valido horas; do
        echo "   Costo: $costo | Horas críticas: $horas | Patrón: $patron"
    done
fi
echo ""

# Estadísticas de costos
costo_min=$(awk -F, 'NR>1 && $4=="SI" {print $3}' "$archivo" | sort -n | head -1)
costo_max=$(awk -F, 'NR>1 && $4=="SI" {print $3}' "$archivo" | sort -n | tail -1)
costo_promedio=$(awk -F, 'NR>1 && $4=="SI" {sum+=$3; count++} END {printf "%.2f", sum/count}' "$archivo")

echo "💰 ESTADÍSTICAS DE COSTOS:"
echo "   Costo mínimo: $costo_min"
echo "   Costo máximo: $costo_max"
echo "   Costo promedio: $costo_promedio"
echo ""

# Distribución por horas críticas
echo "⏰ DISTRIBUCIÓN POR HORAS CRÍTICAS:"
awk -F, 'NR>1 {print $5}' "$archivo" | sort -n | uniq -c | while read count horas; do
    porcentaje=$(echo "scale=1; $count * 100 / $total_casos" | bc)
    echo "   $horas horas: $count casos ($porcentaje%)"
done
echo ""

# Análisis de transiciones (solo si está disponible)
if [ "$TIENE_TRANSICIONES" = true ]; then
    echo "🔄 ANÁLISIS DE TRANSICIONES:"
    
    # Casos que nunca se prenden
    nunca_prendido=$(awk -F, 'NR>1 && $6=="" {count++} END {print count+0}' "$archivo")
    porcentaje_nunca=$(echo "scale=1; $nunca_prendido * 100 / $total_casos" | bc)
    echo "   Nunca prendido: $nunca_prendido casos ($porcentaje_nunca%)"
    
    # Casos siempre prendidos (0-23)
    siempre_prendido=$(awk -F, 'NR>1 && $6=="0-23" {count++} END {print count+0}' "$archivo")
    porcentaje_siempre=$(echo "scale=1; $siempre_prendido * 100 / $total_casos" | bc)
    echo "   Siempre prendido (0-23): $siempre_prendido casos ($porcentaje_siempre%)"
    
    # Casos con transiciones intermedias
    con_transiciones=$(awk -F, 'NR>1 && $6!="" && $6!="0-23" {count++} END {print count+0}' "$archivo")
    porcentaje_trans=$(echo "scale=1; $con_transiciones * 100 / $total_casos" | bc)
    echo "   Con transiciones: $con_transiciones casos ($porcentaje_trans%)"
    
    echo ""
    echo "🎯 PATRONES DE TRANSICIÓN MÁS COMUNES:"
    awk -F, 'NR>1 && $6!="" {print $6}' "$archivo" | sort | uniq -c | sort -nr | head -10 | while read count pattern; do
        porcentaje=$(echo "scale=1; $count * 100 / $total_casos" | bc)
        echo "   [$pattern]: $count casos ($porcentaje%)"
    done
    echo ""
    
    echo "🏅 MEJORES TRANSICIONES (Top 5 por costo):"
    awk -F, 'NR>1 {print $0}' "$archivo" | sort -t, -k3 -n | head -5 | while IFS=, read id patron costo valido horas transiciones; do
        if [ -z "$transiciones" ]; then
            transiciones="nunca prendido"
        fi
        echo "   Costo: $costo | Transiciones: [$transiciones]"
    done
    echo ""
fi

# Patrones más frecuentes de las mejores soluciones
echo "🎯 PATRONES EN LAS 10 MEJORES SOLUCIONES:"
if [ "$TIENE_TRANSICIONES" = true ]; then
    awk -F, 'NR>1 {print $0}' "$archivo" | sort -t, -k3 -n | head -10 | awk -F, '{
        pattern = $2
        ones = gsub(/1/, "1", pattern)
        transiciones = ($6 == "") ? "nunca" : $6
        print "   Eólica en " ones " horas | Costo: " $3 " | Críticas: " $5 " | Trans: [" transiciones "]"
    }'
else
    awk -F, 'NR>1 {print $0}' "$archivo" | sort -t, -k3 -n | head -10 | awk -F, '{
        pattern = $2
        ones = gsub(/1/, "1", pattern)
        print "   Eólica en " ones " horas | Costo: " $3 " | Críticas: " $5
    }'
fi

echo ""
echo "📋 COMANDOS ÚTILES ADICIONALES:"
if [ "$TIENE_TRANSICIONES" = true ]; then
    echo "   # Filtrar casos que nunca se prenden:"
    echo "   awk -F, 'NR>1 && \$6==\"\" {print}' $archivo"
    echo ""
    echo "   # Filtrar casos siempre prendidos:"
    echo "   awk -F, 'NR>1 && \$6==\"0-23\" {print}' $archivo"
    echo ""
    echo "   # Buscar patrón específico de transiciones:"
    echo "   grep '2-5-8-15' $archivo"
fi

