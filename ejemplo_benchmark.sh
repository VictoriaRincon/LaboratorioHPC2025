#!/bin/bash

echo "🚀 DEMOSTRACIÓN DEL SISTEMA DE BENCHMARK"
echo "======================================="
echo ""

echo "📊 Ejecutando benchmark para 4 bits (16 combinaciones)..."
echo "4" | ./optimizador_maquina --benchmark
echo ""

echo "📈 Mostrando ayuda del sistema..."
./optimizador_maquina --help
echo ""

echo "✅ Demostración completada!" 