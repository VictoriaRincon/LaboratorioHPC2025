# Makefile para el Sistema de Optimización de Máquina de Estados

# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
DEBUG_FLAGS = -g -DDEBUG

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
	rm -rf $(OBJDIR)/*.o $(TARGET)
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
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make         - Compilar el proyecto"
	@echo "  make debug   - Compilar versión debug"
	@echo "  make run     - Compilar y ejecutar"
	@echo "  make test    - Ejecutar tests automáticos"
	@echo "  make ejemplo - Crear archivo de ejemplo"
	@echo "  make clean   - Limpiar archivos objeto y ejecutable"
	@echo "  make clean-all - Limpiar todo incluyendo datos"
	@echo "  make info    - Mostrar información del proyecto"
	@echo "  make help    - Mostrar esta ayuda"
	@echo ""
	@echo "Estructura del proyecto:"
	@echo "  $(SRCDIR)/     - Archivos fuente (.cpp)"
	@echo "  $(INCDIR)/     - Archivos header (.hpp)"
	@echo "  $(OBJDIR)/     - Archivos objeto (generados)"
	@echo "  $(DATADIR)/    - Datos de entrada"
	@echo ""
	@echo "Ejemplo de uso:"
	@echo "  make ejemplo && make run"

# Instalar dependencias (para futuro uso con MPI)
install-deps:
	@echo "Instalando dependencias..."
	@echo "⚠️  Para futuras versiones MPI se requerirá: libopenmpi-dev"

# Reglas que no corresponden a archivos
.PHONY: all debug run test clean clean-all info help ejemplo install-deps 