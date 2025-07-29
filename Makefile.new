# Makefile para el Sistema de Optimización de Máquina de Estados

# Configuración del compilador
CXX = g++
MPICXX = mpicxx
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
MPIFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Ejecutables
TARGET = maquina_estados
TARGET_MPI = demo_analisis_con_transiciones_mpi
TARGET_CACHE = demo_analisis_con_cache_mpi

# Archivos fuente principales
MAIN_SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/escenario.cpp $(SRCDIR)/calculador_costos.cpp
MAIN_OBJECTS = $(MAIN_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Archivos para versión MPI original
MPI_SOURCES = $(SRCDIR)/demo_analisis_con_transiciones_mpi.cpp $(SRCDIR)/escenario.cpp $(SRCDIR)/calculador_costos.cpp
MPI_OBJECTS = $(MPI_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/mpi_%.o)

# Archivos para versión MPI con cache
CACHE_SOURCES = $(SRCDIR)/demo_analisis_con_cache_mpi.cpp $(SRCDIR)/escenario.cpp $(SRCDIR)/calculador_costos.cpp $(SRCDIR)/cache_prefijos_mpi.cpp
CACHE_OBJECTS = $(CACHE_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/cache_%.o)

# Regla por defecto
all: $(TARGET) $(TARGET_MPI) $(TARGET_CACHE)

# Crear el ejecutable principal
$(TARGET): $(MAIN_OBJECTS)
	$(CXX) $(MAIN_OBJECTS) -o $(TARGET)
	@echo "✓ Compilación completada: $(TARGET)"

# Crear el ejecutable MPI original
$(TARGET_MPI): $(MPI_OBJECTS)
	$(MPICXX) $(MPI_OBJECTS) -o $(TARGET_MPI)
	@echo "✓ Compilación MPI completada: $(TARGET_MPI)"

# Crear el ejecutable MPI con cache
$(TARGET_CACHE): $(CACHE_OBJECTS)
	$(MPICXX) $(CACHE_OBJECTS) -o $(TARGET_CACHE)
	@echo "✓ Compilación MPI con cache completada: $(TARGET_CACHE)"

# Compilar archivos objeto principales
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compilar archivos objeto MPI
$(OBJDIR)/mpi_%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(MPICXX) $(MPIFLAGS) -c $< -o $@

# Compilar archivos objeto MPI con cache
$(OBJDIR)/cache_%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(MPICXX) $(MPIFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -rf $(OBJDIR)/*.o $(TARGET) $(TARGET_MPI) $(TARGET_CACHE) 2>/dev/null || true
	@echo "✓ Archivos limpiados"

# Ejecutar el programa principal
run: $(TARGET)
	./$(TARGET)

# Ejecutar versión MPI original (ejemplo con 4 procesos)
run-mpi: $(TARGET_MPI)
	mpirun -np 4 ./$(TARGET_MPI)

# Ejecutar versión MPI con cache (ejemplo con 4 procesos)
run-cache: $(TARGET_CACHE)
	mpirun -np 4 ./$(TARGET_CACHE)

# Ejecutar test de rendimiento con cache
test-cache: $(TARGET_CACHE)
	@echo "Ejecutando test de rendimiento con cache (8 procesos, 10000 combinaciones)..."
	echo "10000" | mpirun -np 8 ./$(TARGET_CACHE)

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make              - Compilar todos los proyectos"
	@echo "  make run          - Compilar y ejecutar versión secuencial"
	@echo "  make run-mpi      - Compilar y ejecutar versión MPI original"
	@echo "  make run-cache    - Compilar y ejecutar versión MPI con cache"
	@echo "  make test-cache   - Test de rendimiento con cache"
	@echo "  make clean        - Limpiar archivos generados"
	@echo "  make help         - Mostrar esta ayuda"

# Reglas que no corresponden a archivos
.PHONY: all clean run run-mpi run-cache test-cache help

# Dependencias de headers
$(OBJDIR)/main.o: $(INCDIR)/escenario.hpp $(INCDIR)/calculador_costos.hpp
$(OBJDIR)/escenario.o: $(INCDIR)/escenario.hpp
$(OBJDIR)/calculador_costos.o: $(INCDIR)/calculador_costos.hpp $(INCDIR)/escenario.hpp

# Dependencias MPI
$(OBJDIR)/mpi_demo_analisis_con_transiciones_mpi.o: $(INCDIR)/escenario.hpp $(INCDIR)/calculador_costos.hpp
$(OBJDIR)/mpi_escenario.o: $(INCDIR)/escenario.hpp
$(OBJDIR)/mpi_calculador_costos.o: $(INCDIR)/calculador_costos.hpp $(INCDIR)/escenario.hpp

# Dependencias MPI con cache
$(OBJDIR)/cache_demo_analisis_con_cache_mpi.o: $(INCDIR)/escenario.hpp $(INCDIR)/calculador_costos.hpp $(INCDIR)/cache_prefijos_mpi.hpp
$(OBJDIR)/cache_escenario.o: $(INCDIR)/escenario.hpp
$(OBJDIR)/cache_calculador_costos.o: $(INCDIR)/calculador_costos.hpp $(INCDIR)/escenario.hpp
$(OBJDIR)/cache_cache_prefijos_mpi.o: $(INCDIR)/cache_prefijos_mpi.hpp $(INCDIR)/escenario.hpp $(INCDIR)/calculador_costos.hpp
