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
echo "7. Análisis completo MPI (16,777,216 combinaciones) - ~15-30 segundos"
echo ""

read -p "Selecciona una opción (1-7): " opcion

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
        
    7)
        echo ""
        echo "⚡ ANÁLISIS COMPLETO MPI EXTREMO"
        echo "Combinaciones: 16,777,216"
        echo "Versión paralelizada con Message Passing Interface"
        echo "Tiempo estimado: ~15-30 segundos (dependiendo del número de procesos)"
        echo ""
        
        # Verificar si MPI está disponible
        if ! command -v mpirun &> /dev/null; then
            echo "❌ ERROR: MPI no está instalado"
            echo "Para instalar en macOS: brew install mpich (o open-mpi)"
            echo "Para instalar en Ubuntu: sudo apt-get install libmpich-dev (o openmpi-bin openmpi-dev)"
            exit 1
        fi
        
        if ! command -v mpicxx &> /dev/null; then
            echo "❌ ERROR: Compilador MPI (mpicxx) no está disponible"
            exit 1
        fi
        
        # Detectar número de CPUs disponibles
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS
            num_cpus=$(sysctl -n hw.ncpu)
        else
            # Linux
            num_cpus=$(nproc)
        fi
        
        echo "✅ MPI detectado correctamente"
        echo "CPUs detectadas: $num_cpus"
        # Usar máximo 4 procesos para evitar problemas de memoria
        procesos_recomendados=$((num_cpus > 4 ? 4 : num_cpus))
        echo "Procesos recomendados: $procesos_recomendados (máximo 4 para evitar problemas de memoria)"
        echo ""
        
        read -p "Número de procesos MPI a usar [$procesos_recomendados]: " num_procesos
        num_procesos=${num_procesos:-$procesos_recomendados}
        
        # Verificar si vale la pena usar MPI
        if [ $num_procesos -le 1 ]; then
            echo ""
            echo "⚠️  ADVERTENCIA: Con 1 proceso, MPI será más lento que la versión secuencial"
            echo "Se recomienda usar al menos 2 procesos para obtener beneficios de MPI"
            echo ""
            read -p "¿Deseas usar la versión secuencial (opción 6) en su lugar? (S/n): " usar_secuencial
            if [ "$usar_secuencial" != "n" ] && [ "$usar_secuencial" != "N" ]; then
                echo ""
                echo "🚀 EJECUTANDO VERSIÓN SECUENCIAL..."
                tiempo_inicio=$(date +%s)
                echo "16777216" | ./demo_analisis_con_transiciones
                tiempo_fin=$(date +%s)
                duracion=$((tiempo_fin - tiempo_inicio))
                echo ""
                echo "✅ Análisis secuencial completado en $duracion segundos"
                mv resultados_demo.csv resultados/analisis_secuencial_$(date +%Y%m%d_%H%M%S).csv
                exit 0
            fi
        fi
        
        echo ""
        read -p "¿Estás COMPLETAMENTE seguro? (escribir 'CONFIRMO'): " confirm
        
        if [ "$confirm" = "CONFIRMO" ]; then
            # Compilar versión MPI si no existe o está desactualizada
            if [ ! -f demo_analisis_con_transiciones_mpi ] || [ src/demo_analisis_con_transiciones_mpi.cpp -nt demo_analisis_con_transiciones_mpi ]; then
                echo ""
                echo "🔨 Compilando versión MPI optimizada..."
                mpicxx -std=c++17 -Wall -Wextra -Iinclude -O3 -march=native \
                       -DNDEBUG -ffast-math \
                       src/demo_analisis_con_transiciones_mpi.cpp \
                       src/escenario.cpp \
                       src/calculador_costos.cpp \
                       -o demo_analisis_con_transiciones_mpi
                
                if [ $? -ne 0 ]; then
                    echo "❌ Error en la compilación MPI"
                    exit 1
                fi
                echo "✅ Compilación MPI optimizada exitosa"
            fi
            
            echo ""
            echo "🚀 INICIANDO ANÁLISIS COMPLETO MPI..."
            echo "Procesos MPI: $num_procesos"
            
            # Calcular distribución óptima de trabajo
            combinaciones_totales=16777216
            combinaciones_por_proceso=$((combinaciones_totales / num_procesos))
            resto=$((combinaciones_totales % num_procesos))
            
            echo "Combinaciones por proceso: $combinaciones_por_proceso"
            if [ $resto -ne 0 ]; then
                echo "Combinaciones extras en último proceso: $resto"
            fi
            
            # Configurar variables de entorno para optimizar MPI
            export OMP_NUM_THREADS=1  # Evitar conflictos con OpenMP
            export MPI_BUFFER_SIZE=1024  # Buffer pequeño para reducir latencia
            
            echo ""
            tiempo_inicio=$(date +%s)
            
            # Usar parámetros optimizados para MPI
            if [[ "$OSTYPE" == "darwin"* ]]; then
                # macOS - usar configuración simple para mejor compatibilidad
                echo "16777216" | mpirun -np $num_procesos ./demo_analisis_con_transiciones_mpi 2>/dev/null || \
                echo "16777216" | mpirun -np $num_procesos ./demo_analisis_con_transiciones_mpi
            else
                # Linux - usar configuración optimizada
                echo "16777216" | mpirun -np $num_procesos --bind-to core --map-by core ./demo_analisis_con_transiciones_mpi
            fi
            
            tiempo_fin=$(date +%s)
            duracion=$((tiempo_fin - tiempo_inicio))
            
            echo ""
            echo "🎉 ANÁLISIS COMPLETO MPI FINALIZADO"
            echo "Tiempo total: $duracion segundos ($((duracion/60))m ${duracion%60}s)"
            
            # Calcular y mostrar speedup real
            if [ -f "tiempo_secuencial.txt" ]; then
                tiempo_secuencial=$(cat tiempo_secuencial.txt)
                speedup_real=$(echo "scale=2; $tiempo_secuencial / $duracion" | bc -l 2>/dev/null || echo "N/A")
                echo "Speedup real vs versión secuencial: ${speedup_real}x"
            else
                echo "Speedup teórico estimado: ~${num_procesos}x"
                echo "NOTA: Ejecuta primero la opción 6 para obtener speedup real"
            fi
            
            mv resultados_demo.csv resultados/analisis_mpi_completo_$(date +%Y%m%d_%H%M%S).csv
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
echo ""
