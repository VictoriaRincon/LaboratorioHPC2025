#!/bin/bash

echo "=== ANÁLISIS MASIVO DE MÁQUINA DE ESTADOS ==="
echo "Procesamiento optimizado con análisis de transiciones"
echo ""

# Crear directorio de resultados si no existe
mkdir -p resultados

echo "Opciones de análisis:"
echo "1. Prueba rápida (1,000 combinaciones) - <1 segundo"
echo "2. Análisis pequeño (10,000 combinaciones) - <1 segundo"  
echo "3. Análisis mediano (100,000 combinaciones) - <1 segundo"
echo "4. Análisis grande (1,000,000 combinaciones) - ~4 segundos"
echo "5. Análisis por lotes personalizado"
echo "6. Análisis completo (16,777,216 combinaciones) - ~1 minuto"
echo ""

read -p "Selecciona una opción (1-6): " opcion

case $opcion in
    1)
        echo ""
        echo "🚀 INICIANDO ANÁLISIS RÁPIDO"
        echo "Combinaciones: 1,000"
        echo "Tiempo estimado: <1 segundo"
        echo ""
        tiempo_inicio=$(date +%s)
        echo "1000" | ./demo_analisis_con_transiciones
        tiempo_fin=$(date +%s)
        duracion=$((tiempo_fin - tiempo_inicio))
        echo ""
        echo "✅ Análisis completado en $duracion segundos"
        mv resultados_demo.csv resultados/analisis_1k_$(date +%Y%m%d_%H%M%S).csv
        ;;
        
    2)
        echo ""
        echo "🚀 INICIANDO ANÁLISIS PEQUEÑO"
        echo "Combinaciones: 10,000"
        echo "Tiempo estimado: <1 segundo"
        echo ""
        tiempo_inicio=$(date +%s)
        echo "10000" | ./demo_analisis_con_transiciones
        tiempo_fin=$(date +%s)
        duracion=$((tiempo_fin - tiempo_inicio))
        echo ""
        echo "✅ Análisis completado en $duracion segundos ($((duracion/60))m ${duracion%60}s)"
        mv resultados_demo.csv resultados/analisis_10k_$(date +%Y%m%d_%H%M%S).csv
        ;;
        
    3)
        echo ""
        echo "🚀 INICIANDO ANÁLISIS MEDIANO"
        echo "Combinaciones: 100,000"
        echo "Tiempo estimado: <1 segundo"
        echo ""
        tiempo_inicio=$(date +%s)
        echo "100000" | ./demo_analisis_con_transiciones
        tiempo_fin=$(date +%s)
        duracion=$((tiempo_fin - tiempo_inicio))
        echo ""
        echo "✅ Análisis completado en $duracion segundos ($((duracion/60))m ${duracion%60}s)"
        mv resultados_demo.csv resultados/analisis_100k_$(date +%Y%m%d_%H%M%S).csv
        ;;
        
    4)
        echo ""
        echo "🚀 INICIANDO ANÁLISIS GRANDE"
        echo "Combinaciones: 1,000,000"
        echo "Tiempo estimado: ~4 segundos"
        echo ""
        read -p "¿Continuar? (s/N): " confirmar
        if [ "$confirmar" = "s" ] || [ "$confirmar" = "S" ]; then
            tiempo_inicio=$(date +%s)
            echo "1000000" | ./demo_analisis_con_transiciones
            tiempo_fin=$(date +%s)
            duracion=$((tiempo_fin - tiempo_inicio))
            echo ""
            echo "✅ Análisis completado en $duracion segundos ($((duracion/3600))h $((duracion%3600/60))m)"
            mv resultados_demo.csv resultados/analisis_1M_$(date +%Y%m%d_%H%M%S).csv
        else
            echo "Análisis cancelado"
        fi
        ;;
        
    5)
        echo ""
        echo "🔧 CONFIGURACIÓN PERSONALIZADA"
        read -p "Número de combinaciones: " num_combinaciones
        read -p "Número de lotes (para dividir el trabajo): " num_lotes
        
        chunk_size=$((num_combinaciones / num_lotes))
        echo ""
        echo "Configuración:"
        echo "- Total de combinaciones: $num_combinaciones"
        echo "- Número de lotes: $num_lotes"  
        echo "- Combinaciones por lote: $chunk_size"
        echo ""
        
        tiempo_inicio_total=$(date +%s)
        
        for ((i=0; i<$num_lotes; i++)); do
            start=$((i * chunk_size))
            if [ $i -eq $((num_lotes - 1)) ]; then
                # Último lote incluye el resto
                current_chunk=$((num_combinaciones - start))
            else
                current_chunk=$chunk_size
            fi
            
            echo "📦 LOTE $((i+1))/$num_lotes - Procesando $current_chunk combinaciones"
            tiempo_lote_inicio=$(date +%s)
            
            echo "$current_chunk" | ./demo_analisis_con_transiciones
            
            tiempo_lote_fin=$(date +%s)
            duracion_lote=$((tiempo_lote_fin - tiempo_lote_inicio))
            
            mv resultados_demo.csv resultados/lote_${i}_$(date +%Y%m%d_%H%M%S).csv
            
            echo "✅ Lote $((i+1)) completado en $duracion_lote segundos"
            echo ""
        done
        
        tiempo_fin_total=$(date +%s)
        duracion_total=$((tiempo_fin_total - tiempo_inicio_total))
        echo "🎉 TODOS LOS LOTES COMPLETADOS"
        echo "Tiempo total: $duracion_total segundos ($((duracion_total/3600))h $((duracion_total%3600/60))m)"
        ;;
        
    6)
        echo ""
        echo "⚠️  ANÁLISIS COMPLETO EXTREMO"
        echo "Combinaciones: 16,777,216"
        echo "Tiempo estimado: ~1 minuto"
        echo ""
        read -p "¿Estás COMPLETAMENTE seguro? (escribir 'CONFIRMO'): " confirm
        
        if [ "$confirm" = "CONFIRMO" ]; then
            echo ""
            echo "🚀 INICIANDO ANÁLISIS COMPLETO..."
            echo "NOTA: Análisis rápido (~1 minuto), se puede interrumpir con Ctrl+C"
            echo ""
            
            # Ejecutar en background con log
            nohup bash -c '
                tiempo_inicio=$(date +%s)
                echo "16777216" | ./demo_analisis_con_transiciones
                tiempo_fin=$(date +%s)
                duracion=$((tiempo_fin - tiempo_inicio))
                echo ""
                echo "ANÁLISIS COMPLETO FINALIZADO"
                echo "Tiempo total: $duracion segundos ($((duracion/86400))d $((duracion%86400/3600))h)"
                mv resultados_demo.csv resultados/analisis_completo_$(date +%Y%m%d_%H%M%S).csv
            ' > resultados/analisis_completo.log 2>&1 &
            
            pid=$!
            echo "Proceso iniciado en background con PID: $pid"
            echo ""
            echo "Para monitorear progreso:"
            echo "  tail -f resultados/analisis_completo.log"
            echo ""
            echo "Para detener el proceso:"
            echo "  kill $pid"
        else
            echo "Análisis cancelado"
        fi
        ;;
        
    *)
        echo "Opción inválida"
        exit 1
        ;;
esac

echo ""
echo "=== INFORMACIÓN DE RESULTADOS ==="
echo "📁 Archivos guardados en: ./resultados/"
echo "📊 Formato CSV: CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,Transiciones"
echo ""
echo "📝 Nueva columna 'Transiciones':"
echo "   - Vacía: nunca se prendió la máquina"
echo "   - '0-23': prendida todo el tiempo"
echo "   - '2-5-8-15': prender h2, apagar h5, prender h8, apagar h15"
echo ""
echo "🔍 Comandos útiles para análisis:"
echo "  # Ver mejores resultados:"
echo "  sort -t, -k3 -n resultados/archivo.csv | head -10"
echo ""
echo "  # Analizar patrones de transiciones:"
echo "  ./scripts/analizar_resultados.sh resultados/archivo.csv"
