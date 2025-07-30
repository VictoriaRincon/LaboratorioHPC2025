#!/bin/bash

echo "🚀 DEMOSTRACIÓN DE PARALELIZACIÓN OPENMP"
echo "========================================"
echo ""

echo "📊 Información del sistema:"
echo "Núcleos de CPU disponibles:"
sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo "No disponible"
echo ""

echo "📈 Probando benchmark con diferentes configuraciones de hilos..."
echo ""

echo "🔧 PRUEBA 1: Modo secuencial (1 hilo)"
echo "5
n
n
1" | ./optimizador_maquina --benchmark

echo ""
echo "🔧 PRUEBA 2: Paralelización moderada (2 hilos)"
echo "5
n
n
2" | ./optimizador_maquina --benchmark

echo ""
echo "🔧 PRUEBA 3: Paralelización máxima (todos los hilos)"
echo "5
n
n
0" | ./optimizador_maquina --benchmark

echo ""
echo "📊 COMPARACIÓN DE RENDIMIENTO:"
echo "La diferencia en velocidad debería ser notable con más hilos"
echo "Métricas clave a observar:"
echo "• Hilos utilizados"
echo "• Speedup obtenido"
echo "• Eficiencia paralela"
echo "• Combinaciones por segundo"
echo ""

echo "✅ Pruebas de OpenMP completadas!"
echo ""
echo "💡 Para casos más grandes (>10 bits), la diferencia será más significativa"
echo "🔄 La paralelización es especialmente útil para largos de 15+ bits" 