#!/bin/bash

# Script de ejecución para Análisis Exhaustivo MPI
# Automatiza la ejecución con diferentes números de procesos

echo "=== ANALIZADOR EXHAUSTIVO MPI - SCRIPT DE EJECUCIÓN ==="
echo "Este script facilitará la ejecución del análisis MPI con múltiples procesos"
echo ""

# Verificar que MPI esté instalado
if ! command -v mpirun &> /dev/null; then
    echo "❌ Error: MPI no está instalado o no está en el PATH"
    echo "Instala MPI primero (ej: sudo apt install mpich openmpi-bin)"
    exit 1
fi

# Verificar que el ejecutable exista
if [ ! -f "./analisis_exhaustivo_mpi" ]; then
    echo "❌ Error: No se encuentra el ejecutable 'analisis_exhaustivo_mpi'"
    echo "Compila primero con: make analisis_exhaustivo_mpi"
    exit 1
fi

# Detectar número de núcleos disponibles
NUM_CORES=$(nproc)
echo "🖥️  Núcleos de CPU detectados: $NUM_CORES"
echo ""

# Mostrar menú de opciones
echo "Selecciona el modo de ejecución:"
echo "1. Prueba rápida MPI (1,000 combinaciones)"
echo "2. Prueba mediana MPI (100,000 combinaciones)"  
echo "3. Prueba grande MPI (1,000,000 combinaciones)"
echo "4. Análisis personalizado (especificar parámetros)"
echo "5. Comparación de rendimiento (1 vs 2 vs 4 vs 8 procesos)"
echo "6. Máximo rendimiento (usar todos los núcleos: $NUM_CORES procesos)"
echo "0. Salir"
echo ""
read -p "Opción: " opcion

case $opcion in
    1)
        echo ""
        echo "🚀 EJECUTANDO PRUEBA RÁPIDA MPI"
        read -p "¿Cuántos procesos usar? (recomendado: 2-4): " num_proc
        echo "Ejecutando con $num_proc procesos..."
        echo "Comando: mpirun -np $num_proc ./analisis_exhaustivo_mpi"
        echo ""
        
        # Usar input automático para seleccionar opción 3 (prueba pequeña)
        echo "3" | mpirun -np $num_proc ./analisis_exhaustivo_mpi
        ;;
        
    2)
        echo ""
        echo "🚀 EJECUTANDO PRUEBA MEDIANA MPI"
        read -p "¿Cuántos procesos usar? (recomendado: 4-8): " num_proc
        echo "Ejecutando con $num_proc procesos..."
        echo "Esto tomará varios minutos con paralelización..."
        echo ""
        
        # Usar input automático para seleccionar opción 4 (prueba mediana)
        echo "4" | mpirun -np $num_proc ./analisis_exhaustivo_mpi
        ;;
        
    3)
        echo ""
        echo "🚀 EJECUTANDO PRUEBA GRANDE MPI"
        read -p "¿Cuántos procesos usar? (recomendado: 8-16): " num_proc
        echo "Ejecutando con $num_proc procesos..."
        echo "Esto tomará aproximadamente 30-60 minutos con paralelización..."
        echo ""
        
        # Usar input automático para seleccionar opción 5 (prueba grande)
        echo "5" | mpirun -np $num_proc ./analisis_exhaustivo_mpi
        ;;
        
    4)
        echo ""
        echo "🔧 ANÁLISIS PERSONALIZADO"
        read -p "¿Cuántos procesos usar?: " num_proc
        echo "Ejecutando con $num_proc procesos..."
        echo "Usa el menú interactivo para configurar parámetros..."
        echo ""
        
        mpirun -np $num_proc ./analisis_exhaustivo_mpi
        ;;
        
    5)
        echo ""
        echo "📊 COMPARACIÓN DE RENDIMIENTO"
        echo "Ejecutando la misma prueba con diferentes números de procesos..."
        echo "Esto puede tomar varios minutos..."
        echo ""
        
        PRUEBA_SIZE=10000
        echo "Usando $PRUEBA_SIZE combinaciones para la comparación"
        echo ""
        
        # Crear input file temporal para automatizar
        cat > temp_input.txt << EOF
2
0
$PRUEBA_SIZE
0
EOF
        
        for procs in 1 2 4 8; do
            if [ $procs -le $NUM_CORES ]; then
                echo "--- Ejecutando con $procs proceso(s) ---"
                start_time=$(date +%s)
                mpirun -np $procs ./analisis_exhaustivo_mpi < temp_input.txt > /dev/null 2>&1
                end_time=$(date +%s)
                duration=$((end_time - start_time))
                echo "✅ $procs proceso(s): $duration segundos"
                echo ""
            fi
        done
        
        rm -f temp_input.txt
        echo "📈 Comparación completada. Revisa los archivos de resultados generados."
        ;;
        
    6)
        echo ""
        echo "⚡ MÁXIMO RENDIMIENTO"
        echo "Usando todos los núcleos disponibles ($NUM_CORES procesos)"
        read -p "¿Continuar? (s/N): " confirmar
        
        if [[ $confirmar == [sS]* ]]; then
            echo "Ejecutando con $NUM_CORES procesos..."
            echo "Usa el menú interactivo para seleccionar el análisis deseado..."
            echo ""
            
            mpirun -np $NUM_CORES ./analisis_exhaustivo_mpi
        else
            echo "Operación cancelada."
        fi
        ;;
        
    0)
        echo "Saliendo..."
        exit 0
        ;;
        
    *)
        echo "❌ Opción inválida"
        exit 1
        ;;
esac

echo ""
echo "=== ANÁLISIS COMPLETADO ==="
echo ""
echo "📁 Archivos generados en el directorio actual:"
ls -la *_rank*.csv *_rank*.txt 2>/dev/null | head -10

echo ""
echo "💡 SUGERENCIAS POST-ANÁLISIS:"
echo "1. Fusionar resultados: ./scripts/fusionar_resultados_mpi.sh"
echo "2. Analizar rendimiento: grep 'segundos' *_rank*.txt"
echo "3. Ver mejores resultados: sort -t',' -k3,3n *_rank*.csv | head -10"
echo ""
echo "🎉 ¡Análisis MPI completado exitosamente!" 