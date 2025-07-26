#!/bin/bash

echo "=== SISTEMA DE ANÁLISIS EXHAUSTIVO DE MÁQUINA DE ESTADOS ==="
echo "Este script permite ejecutar análisis completos o por lotes"
echo ""

# Crear directorio de resultados si no existe
mkdir -p resultados

echo "Opciones disponibles:"
echo "1. Análisis rápido (1,000 combinaciones)"
echo "2. Análisis mediano (100,000 combinaciones)"  
echo "3. Análisis por lotes (dividir en chunks manejables)"
echo "4. Análisis completo (16,777,216 combinaciones - ¡DÍAS de procesamiento!)"
echo ""

read -p "Selecciona una opción (1-4): " opcion

case $opcion in
    1)
        echo "Ejecutando análisis rápido..."
        echo "1000" | ./demo_analisis
        mv resultados_demo.csv resultados/analisis_rapido_$(date +%Y%m%d_%H%M%S).csv
        ;;
    2)
        echo "Ejecutando análisis mediano..."
        echo "100000" | ./demo_analisis
        mv resultados_demo.csv resultados/analisis_mediano_$(date +%Y%m%d_%H%M%S).csv
        ;;
    3)
        echo "Análisis por lotes - dividiendo el trabajo en chunks"
        read -p "¿Cuántas combinaciones por lote? (ej: 50000): " chunk_size
        read -p "¿Cuántos lotes quieres procesar? (ej: 10): " num_chunks
        
        for ((i=0; i<$num_chunks; i++)); do
            start=$((i * chunk_size))
            end=$(((i + 1) * chunk_size))
            echo "Procesando lote $((i+1))/$num_chunks: combinaciones $start-$end"
            
            echo "$chunk_size" | timeout 3600 ./demo_analisis 2>/dev/null
            if [ $? -eq 0 ]; then
                mv resultados_demo.csv resultados/lote_${i}_$(date +%Y%m%d_%H%M%S).csv
                echo "Lote $((i+1)) completado"
            else
                echo "Lote $((i+1)) interrumpido o con error"
            fi
        done
        ;;
    4)
        echo "⚠️  ADVERTENCIA: El análisis completo tomará DÍAS o SEMANAS"
        echo "Se procesarán 16,777,216 combinaciones"
        read -p "¿Estás absolutamente seguro? (escribir 'SI' para confirmar): " confirm
        
        if [ "$confirm" = "SI" ]; then
            echo "Iniciando análisis completo..."
            echo "Este proceso se puede interrumpir con Ctrl+C"
            nohup bash -c 'echo "16777216" | ./demo_analisis' > resultados/analisis_completo.log 2>&1 &
            pid=$!
            echo "Proceso iniciado con PID: $pid"
            echo "Para monitorear: tail -f resultados/analisis_completo.log"
            echo "Para detener: kill $pid"
        else
            echo "Análisis cancelado"
        fi
        ;;
    *)
        echo "Opción inválida"
        ;;
esac

echo ""
echo "=== INFORMACIÓN DE RESULTADOS ==="
echo "Los archivos CSV contienen:"
echo "- CombinacionID: Número único (0 a 16777215)"
echo "- PatronEolica: Patrón binario (0=sin eólica, 1=con eólica 500MW)"
echo "- CostoTotal: Costo óptimo encontrado"
echo "- SolucionValida: SI/NO"
echo "- HorasCriticas: Horas que requieren generación propia"
echo ""
echo "Para analizar resultados:"
echo "- Mejores costos: sort -t, -k3 -n archivo.csv | head"
echo "- Por horas críticas: sort -t, -k5 -n archivo.csv"
echo "- Combinaciones con menos costo: awk -F, '\$3<100 {print}' archivo.csv"
