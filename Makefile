# Makefile para Sistema de Optimización de Máquina de Estados
# Versión: Secuencial con preparación para MPI

# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -Iinclude

# Directorios
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

# Archivos fuente
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Ejecutable
TARGET = optimizador_maquina

# Regla por defecto
all: $(TARGET)

# Crear el ejecutable
$(TARGET): $(OBJECTS) | $(OBJ_DIR)
	$(CXX) $(OBJECTS) -o $@
	@echo "✅ Compilación exitosa: $(TARGET)"

# Compilar archivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Crear directorio de objetos
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Ejecutar el programa
run: $(TARGET)
	@echo "🚀 Ejecutando optimizador..."
	./$(TARGET)

# Prueba rápida
test-quick: $(TARGET)
	@echo "⚡ Ejecutando prueba rápida..."
	./$(TARGET) --quick

# Pruebas de validación
test: $(TARGET)
	@echo "🧪 Ejecutando pruebas de validación..."
	./$(TARGET) --test

# Ejecutar con archivo de parámetros
run-file: $(TARGET)
	@echo "📁 Ejecutando con archivo de parámetros..."
	./$(TARGET) --file

# Mostrar ayuda
help-program: $(TARGET)
	./$(TARGET) --help

# Ejecutar benchmark de rendimiento
benchmark: $(TARGET)
	@echo "📊 Ejecutando benchmark de rendimiento..."
	./$(TARGET) --benchmark

# Limpiar archivos generados
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "🧹 Limpieza completada"

# Recompilar todo
rebuild: clean all

# Preparación para MPI (para futuro desarrollo)
mpi-prepare:
	@echo "🔧 Preparando estructura para MPI..."
	@echo "  • Verificando compilador MPI..."
	@which mpicxx > /dev/null 2>&1 && echo "  ✅ mpicxx encontrado" || echo "  ❌ mpicxx no encontrado - instalar con: sudo apt-get install libopenmpi-dev"
	@echo "  • Headers actuales listos para extensión MPI"
	@echo "  • Estructura de clases preparada para paralelización"

# Información del proyecto
info:
	@echo "================================================================"
	@echo "SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS"
	@echo "================================================================"
	@echo "Algoritmo: Backtracking con memoización"
	@echo "Versión: Secuencial (preparada para MPI)"
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "----------------------------------------------------------------"
	@echo "Comandos disponibles:"
	@echo "  make          - Compilar el proyecto"
	@echo "  make run      - Ejecutar interfaz interactiva"
	@echo "  make test     - Ejecutar pruebas de validación"
	@echo "  make test-quick - Ejecutar prueba rápida"
	@echo "  make run-file - Ejecutar con archivo de parámetros"
	@echo "  make benchmark - Ejecutar benchmark de rendimiento"
	@echo "  make clean    - Limpiar archivos generados"
	@echo "  make rebuild  - Recompilar completamente"
	@echo "  make info     - Mostrar esta información"
	@echo "  make help     - Mostrar ayuda del programa"
	@echo "================================================================"

# Crear estructura de directorios si no existe
setup:
	@echo "📁 Configurando estructura del proyecto..."
	mkdir -p $(SRC_DIR) $(INCLUDE_DIR) $(OBJ_DIR) data resultados
	@echo "✅ Estructura creada"

# Crear directorio de resultados automáticamente
$(TARGET): | resultados

resultados:
	mkdir -p resultados

# Verificar dependencias
check-deps:
	@echo "🔍 Verificando dependencias..."
	@$(CXX) --version > /dev/null 2>&1 && echo "  ✅ Compilador C++ disponible" || echo "  ❌ Compilador C++ no encontrado"
	@echo "  📋 Headers requeridos: <vector>, <string>, <unordered_map>, <iostream>, etc."
	@echo "  ✅ Todas las dependencias están incluidas en C++17 estándar"

# Ayuda del Makefile
help:
	@echo "================================================================"
	@echo "AYUDA - MAKEFILE DEL OPTIMIZADOR"
	@echo "================================================================"
	@echo "Targets principales:"
	@echo "  all           - Compilar el proyecto (default)"
	@echo "  run           - Compilar y ejecutar interfaz interactiva"
	@echo "  test          - Ejecutar suite de pruebas"
	@echo "  test-quick    - Ejecutar prueba rápida"
	@echo "  clean         - Eliminar archivos generados"
	@echo ""
	@echo "Targets de utilidad:"
	@echo "  info          - Información del proyecto"
	@echo "  setup         - Crear estructura de directorios"
	@echo "  check-deps    - Verificar dependencias"
	@echo "  mpi-prepare   - Preparar para desarrollo MPI"
	@echo "  help          - Mostrar esta ayuda"
	@echo "================================================================"

# Archivos que no son targets
.PHONY: all run test test-quick clean rebuild info setup check-deps help mpi-prepare help-program run-file

# Dependencias de headers
$(OBJ_DIR)/estados_maquina.o: $(INCLUDE_DIR)/estados_maquina.hpp
$(OBJ_DIR)/optimizador.o: $(INCLUDE_DIR)/optimizador.hpp $(INCLUDE_DIR)/estados_maquina.hpp
$(OBJ_DIR)/benchmark_sistema.o: $(INCLUDE_DIR)/benchmark_sistema.hpp $(INCLUDE_DIR)/optimizador.hpp $(INCLUDE_DIR)/estados_maquina.hpp
$(OBJ_DIR)/interfaz_terminal.o: $(INCLUDE_DIR)/interfaz_terminal.hpp $(INCLUDE_DIR)/optimizador.hpp $(INCLUDE_DIR)/estados_maquina.hpp $(INCLUDE_DIR)/benchmark_sistema.hpp
$(OBJ_DIR)/main.o: $(INCLUDE_DIR)/interfaz_terminal.hpp 