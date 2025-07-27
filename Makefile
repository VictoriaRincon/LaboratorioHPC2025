# Makefile para el Sistema de Optimización de Máquina de Estados

# Configuración del compilador
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
TARGET = maquina_estados
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Archivos fuente y objeto
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Regla por defecto
all: $(TARGET)

# Crear el ejecutable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)
	@echo "✓ Compilación completada: $(TARGET)"

# Compilar archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -rf $(OBJDIR)/*.o $(TARGET)
	@echo "✓ Archivos limpiados"

# Ejecutar el programa
run: $(TARGET)
	./$(TARGET)

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make        - Compilar el proyecto"
	@echo "  make run    - Compilar y ejecutar"
	@echo "  make clean  - Limpiar archivos generados"
	@echo "  make help   - Mostrar esta ayuda"

# Reglas que no corresponden a archivos
.PHONY: all clean run help

# Dependencias de headers
$(OBJDIR)/main.o: $(INCDIR)/escenario.hpp $(INCDIR)/calculador_costos.hpp
$(OBJDIR)/escenario.o: $(INCDIR)/escenario.hpp
$(OBJDIR)/calculador_costos.o: $(INCDIR)/calculador_costos.hpp $(INCDIR)/escenario.hpp 
# Nuevo ejecutable para análisis exhaustivo
ANALISIS_TARGET = analisis_exhaustivo
ANALISIS_SOURCES = src/analizador_exhaustivo.cpp src/escenario.cpp src/calculador_costos.cpp src/main_analisis.cpp
ANALISIS_OBJECTS = $(ANALISIS_SOURCES:src/%.cpp=$(OBJDIR)/%.o)

# Compilar el analizador exhaustivo
$(ANALISIS_TARGET): $(ANALISIS_OBJECTS)
	$(CXX) $(ANALISIS_OBJECTS) -o $(ANALISIS_TARGET)
	@echo "✓ Compilación del analizador exhaustivo completada: $(ANALISIS_TARGET)"

# Regla para compilar todos los proyectos
all-projects: $(TARGET) $(ANALISIS_TARGET)

# Ejecutar análisis exhaustivo
run-analisis: $(ANALISIS_TARGET)
./$(ANALISIS_TARGET)

# Limpiar todos los ejecutables
clean-all:
rm -rf $(OBJDIR)/*.o $(TARGET) $(ANALISIS_TARGET)
@echo "✓ Todos los archivos limpiados"

# Actualizar ayuda
help-extended:
@echo "Comandos disponibles:"
@echo "  make                - Compilar proyecto principal"
@echo "  make run            - Compilar y ejecutar proyecto principal"
@echo "  make analisis_exhaustivo - Compilar analizador exhaustivo"
@echo "  make run-analisis   - Compilar y ejecutar analizador exhaustivo"
@echo "  make all-projects   - Compilar ambos proyectos"
@echo "  make clean          - Limpiar archivos del proyecto principal"
@echo "  make clean-all      - Limpiar todos los archivos generados"
@echo "  make help           - Mostrar ayuda básica"
@echo "  make help-extended  - Mostrar ayuda extendida"

# Dependencias adicionales
$(OBJDIR)/analizador_exhaustivo.o: $(INCDIR)/analizador_exhaustivo.hpp $(INCDIR)/calculador_costos.hpp $(INCDIR)/escenario.hpp
$(OBJDIR)/main_analisis.o: $(INCDIR)/analizador_exhaustivo.hpp

.PHONY: all-projects run-analisis clean-all help-extended
