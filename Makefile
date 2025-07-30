# Makefile para el Sistema de Optimización de Máquina de Estados

# Compilador y flags
CXX = g++
MPICXX = mpicxx
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
MPI_CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DMPI_BUILD
DEBUG_FLAGS = -g -DDEBUG

# Flags OpenMP (detectar automáticamente si está disponible)
OPENMP_FLAGS = -fopenmp
MPI_OPENMP_CXXFLAGS = $(MPI_CXXFLAGS) $(OPENMP_FLAGS)

# Directorios
SRCDIR = src
INCDIR = include
OBJDIR = obj
DATADIR = data

# Archivos fuente
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Nombre del ejecutable
TARGET = maquina_estados
TARGET_MPI = analisis_exhaustivo_mpi
TARGET_BENCHMARK = benchmark_mpi

# Crear directorio obj si no existe
$(shell mkdir -p $(OBJDIR))
$(shell mkdir -p $(DATADIR))

# Regla principal
all: $(TARGET)

# Enlazado del ejecutable
$(TARGET): $(OBJECTS)
	@echo "Enlazando $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET)
	@echo "✅ Compilación exitosa!"

# Compilación de archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

# === REGLAS PARA VERSIÓN MPI ===

# Archivos específicos para MPI
MPI_SOURCES = $(SRCDIR)/calculador_costos.cpp $(SRCDIR)/escenario.cpp $(SRCDIR)/analisis_exhaustivo_mpi.cpp $(SRCDIR)/main_exhaustivo_mpi.cpp
MPI_OBJECTS = $(MPI_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%_mpi.o)

# Compilar versión MPI con OpenMP
mpi: $(TARGET_MPI)

$(TARGET_MPI): $(MPI_OBJECTS)
	@echo "Enlazando $(TARGET_MPI) con soporte MPI+OpenMP..."
	$(MPICXX) $(OPENMP_FLAGS) $(MPI_OBJECTS) -o $(TARGET_MPI)
	@echo "✅ Compilación MPI+OpenMP exitosa!"

# Compilación de archivos objeto para MPI con OpenMP
$(OBJDIR)/%_mpi.o: $(SRCDIR)/%.cpp
	@echo "Compilando $< para MPI+OpenMP..."
	$(MPICXX) $(MPI_OPENMP_CXXFLAGS) -I$(INCDIR) -c $< -o $@

# === REGLAS PARA BENCHMARK DE RENDIMIENTO ===

# Archivos específicos para Benchmark
BENCHMARK_SOURCES = $(SRCDIR)/calculador_costos.cpp $(SRCDIR)/escenario.cpp $(SRCDIR)/analisis_exhaustivo_mpi.cpp $(SRCDIR)/main_benchmark_mpi.cpp
BENCHMARK_OBJECTS = $(BENCHMARK_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%_benchmark.o)

# Compilar versión Benchmark con OpenMP
benchmark: $(TARGET_BENCHMARK)

$(TARGET_BENCHMARK): $(BENCHMARK_OBJECTS)
	@echo "Enlazando $(TARGET_BENCHMARK) con soporte MPI+OpenMP..."
	$(MPICXX) $(OPENMP_FLAGS) $(BENCHMARK_OBJECTS) -o $(TARGET_BENCHMARK)
	@echo "✅ Compilación Benchmark MPI+OpenMP exitosa!"

# Compilación de archivos objeto para Benchmark con OpenMP
$(OBJDIR)/%_benchmark.o: $(SRCDIR)/%.cpp
	@echo "Compilando $< para Benchmark MPI+OpenMP..."
	$(MPICXX) $(MPI_OPENMP_CXXFLAGS) -I$(INCDIR) -c $< -o $@

# Versión debug
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "✅ Versión debug compilada!"

# Ejecutar el programa
run: $(TARGET)
	@echo "Ejecutando $(TARGET)..."
	./$(TARGET)

# Ejecutar tests
test: $(TARGET)
	@echo "Ejecutando tests automáticos..."
	@echo "1\n4\n" | ./$(TARGET)

# Ejecutar análisis exhaustivo MPI (modo interactivo)
run-mpi: $(TARGET_MPI)
	@echo "Ejecutando análisis exhaustivo con MPI (modo interactivo)..."
	mpirun -np 4 ./$(TARGET_MPI)

# Test MPI con ejemplo pequeño
test-mpi: $(TARGET_MPI)
	@echo "Ejecutando test MPI con longitud 5..."
	mpirun -np 2 ./$(TARGET_MPI) -l 5

# Ejecutar análisis rápido (para pruebas)
quick-mpi: $(TARGET_MPI)
	@echo "Ejecutando análisis rápido (4 bits, 2 procesos)..."
	mpirun -np 2 ./$(TARGET_MPI) -l 4

# === REGLAS PARA BENCHMARK DE RENDIMIENTO ===

# Ejecutar benchmark interactivo
run-benchmark: $(TARGET_BENCHMARK)
	@echo "Ejecutando benchmark de rendimiento (modo interactivo)..."
	mpirun -np 4 ./$(TARGET_BENCHMARK)

# Benchmark rápido para pruebas
test-benchmark: $(TARGET_BENCHMARK)
	@echo "Ejecutando benchmark de prueba (12 bits, 2 procesos)..."
	mpirun -np 2 ./$(TARGET_BENCHMARK) -b 12 -v

# Benchmark intensivo configurable
# Variables por defecto (pueden ser sobrescritas)
INTENSIVE_BITS ?= 20
INTENSIVE_PROCS ?= 8
INTENSIVE_CORES ?= 8
INTENSIVE_OPTS ?= -v

intensive-benchmark: $(TARGET_BENCHMARK)
	@echo "Ejecutando benchmark intensivo configurable..."
	@echo "  Bits: $(INTENSIVE_BITS)"
	@echo "  Procesos MPI: $(INTENSIVE_PROCS)"
	@echo "  Núcleos CPU: $(INTENSIVE_CORES)"
	@echo "  Opciones: $(INTENSIVE_OPTS)"
	@echo "===========================================" 
	mpirun -np $(INTENSIVE_PROCS) ./$(TARGET_BENCHMARK) -b $(INTENSIVE_BITS) -c $(INTENSIVE_CORES) $(INTENSIVE_OPTS)

# Super benchmark para sistemas de alto rendimiento
super-intensive-benchmark: $(TARGET_BENCHMARK)
	@echo "Ejecutando SUPER benchmark intensivo (25 bits, 16 procesos)..."
	mpirun -np 16 ./$(TARGET_BENCHMARK) -b 25 -c 16 -v

# Benchmark extremo para clusters
extreme-benchmark: $(TARGET_BENCHMARK)
	@echo "Ejecutando benchmark EXTREMO (28 bits, 32 procesos)..."
	mpirun -np 32 ./$(TARGET_BENCHMARK) -b 28 -c 32 -v

# Crear archivo de ejemplo
ejemplo: $(DATADIR)/parametros.in

$(DATADIR)/parametros.in:
	@echo "Creando archivo de ejemplo..."
	@echo "# Valores binarios para cada hora (0 hasta hora_final)" > $(DATADIR)/parametros.in
	@echo "# 0 = Energía eólica insuficiente (requiere generación)" >> $(DATADIR)/parametros.in
	@echo "# 1 = Energía eólica suficiente (no requiere generación)" >> $(DATADIR)/parametros.in
	@echo "1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 1 1 1 1 1" >> $(DATADIR)/parametros.in
	@echo "✅ Archivo de ejemplo creado en $(DATADIR)/parametros.in"

# Limpiar archivos generados
clean:
	@echo "Limpiando archivos generados..."
	rm -rf $(OBJDIR)/*.o $(TARGET) $(TARGET_MPI) $(TARGET_BENCHMARK) *.csv
	@echo "✅ Limpieza completada!"

# Limpiar todo incluyendo datos
clean-all: clean
	rm -rf $(DATADIR)/parametros.in
	@echo "✅ Limpieza completa!"

# Mostrar información del proyecto
info:
	@echo "=== INFORMACIÓN DEL PROYECTO ==="
	@echo "Compilador: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Archivos fuente: $(SOURCES)"
	@echo "Ejecutable: $(TARGET)"
	@echo "Directorios: $(SRCDIR), $(INCDIR), $(OBJDIR), $(DATADIR)"

# Mostrar ayuda
help:
	@echo "=== SISTEMA DE OPTIMIZACIÓN DE MÁQUINA DE ESTADOS ==="
	@echo "🔄 Ahora con soporte MPI+OpenMP para máximo rendimiento!"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make         - Compilar el proyecto original"
	@echo "  make debug   - Compilar versión debug"
	@echo "  make run     - Compilar y ejecutar versión original"
	@echo "  make test    - Ejecutar tests automáticos"
	@echo "  make mpi     - Compilar análisis exhaustivo con MPI+OpenMP 🚀"
	@echo "  make run-mpi - Ejecutar análisis exhaustivo MPI (modo interactivo)"
	@echo "  make test-mpi- Test MPI con longitud 5 (2 procesos)"
	@echo "  make quick-mpi- Test rápido con longitud 4 (2 procesos)"
	@echo "  make benchmark - Compilar sistema de benchmark con MPI+OpenMP 🚀"
	@echo "  make run-benchmark - Ejecutar benchmark interactivo"
	@echo "  make test-benchmark - Benchmark de prueba (12 bits, 2 procesos)"
	@echo "  make intensive-benchmark - Benchmark intensivo CONFIGURABLE"
	@echo "  make super-intensive-benchmark - Super benchmark (25 bits, 16 procesos)"
	@echo "  make extreme-benchmark - Benchmark extremo (28 bits, 32 procesos)"
	@echo "  make ejemplo - Crear archivo de ejemplo"
	@echo "  make clean   - Limpiar archivos generados"
	@echo "  make clean-all - Limpiar todo incluyendo datos"
	@echo "  make info    - Mostrar información del proyecto"
	@echo "  make help    - Mostrar esta ayuda"
	@echo ""
	@echo "🧵 PARALELIZACIÓN HÍBRIDA MPI+OpenMP:"
	@echo "  • MPI: Distribución entre nodos/cores"
	@echo "  • OpenMP: Paralelización dentro de cada proceso MPI"
	@echo "  • Comunicación inteligente de patrones entre procesos"
	@echo "  • Sincronización optimizada para evitar deadlocks"
	@echo ""
	@echo "Estructura del proyecto:"
	@echo "  $(SRCDIR)/     - Archivos fuente (.cpp)"
	@echo "  $(INCDIR)/     - Archivos header (.hpp)"
	@echo "  $(OBJDIR)/     - Archivos objeto (generados)"
	@echo "  $(DATADIR)/    - Datos de entrada"
	@echo ""
	@echo "Ejemplos de uso:"
	@echo "  make ejemplo && make run"
	@echo "  make mpi && make test-mpi"
	@echo "  make mpi && mpirun -np 2 ./$(TARGET_MPI) -l 5 -o resultados.csv"
	@echo ""
	@echo "Configuración de benchmark intensivo:"
	@echo "  make intensive-benchmark                              # Usar valores por defecto"
	@echo "  make intensive-benchmark INTENSIVE_BITS=22           # Personalizar solo bits"
	@echo "  make intensive-benchmark INTENSIVE_PROCS=16          # Personalizar solo procesos"
	@echo "  make intensive-benchmark INTENSIVE_BITS=24 INTENSIVE_PROCS=32 INTENSIVE_CORES=32"
	@echo "  make intensive-benchmark INTENSIVE_OPTS='-v -s'      # Modo verbose + guardar resultados"
	@echo ""
	@echo "Variables configurables:"
	@echo "  INTENSIVE_BITS  = Número de bits (por defecto: 20)"
	@echo "  INTENSIVE_PROCS = Procesos MPI (por defecto: 8)"  
	@echo "  INTENSIVE_CORES = Núcleos CPU (por defecto: 8)"
	@echo "  INTENSIVE_OPTS  = Opciones adicionales (por defecto: -v)"

# Instalar dependencias (para futuro uso con MPI)
install-deps:
	@echo "Instalando dependencias..."
	@echo "⚠️  Para futuras versiones MPI se requerirá: libopenmpi-dev"

# Reglas que no corresponden a archivos
.PHONY: all debug run test mpi run-mpi test-mpi quick-mpi benchmark run-benchmark test-benchmark intensive-benchmark super-intensive-benchmark extreme-benchmark clean clean-all info help ejemplo install-deps 