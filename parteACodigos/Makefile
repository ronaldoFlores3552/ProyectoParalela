# ====================================================================
# Makefile Profesional para el Proyecto de Computación Paralela
# ====================================================================

# --- Compiladores y Herramientas ---
CXX = g++
NVCC = nvcc

# --- Flags de Compilación ---
# Flags para C++ (generador de datos)
CXXFLAGS = -std=c++14 -O3 -Wall
# Flags para CUDA. ¡IMPORTANTE: arch=sm_75 para la GPU Tesla T4 de Colab!
NVCCFLAGS = -std=c++14 -O3 -arch=sm_75 -lineinfo

# --- Archivos y Rutas ---
# Ruta al directorio padre
PARENT_DIR = ..

# Ejecutables
TARGET_PARALLEL = marchingCubesParallel
TARGET_GENERATOR = $(PARENT_DIR)/generator

# Fuentes del programa paralelo (relativas a la carpeta parteACodigos)
PARALLEL_SOURCES = mainParallel.cpp marching_cube_parallel.cu marching_cube_serial.cpp
# Fuentes del generador de datos
GENERATOR_SOURCES = src/generate_data.cpp src/test_generator.cpp

# ====================================================================
# --- Reglas de Compilación ---
# ====================================================================

# Regla por defecto: compila ambos ejecutables
all: $(TARGET_PARALLEL) $(TARGET_GENERATOR)

# Regla para compilar el programa paralelo
$(TARGET_PARALLEL): $(PARALLEL_SOURCES)
	@echo "Compilando el programa paralelo con CUDA para sm_75..."
	$(NVCC) $(NVCCFLAGS) -o $@ $(PARALLEL_SOURCES)

# Regla para compilar el generador de datos
$(TARGET_GENERATOR): $(GENERATOR_SOURCES)
	@echo "Compilando el generador de datos..."
	$(CXX) $(CXXFLAGS) -o $@ $(GENERATOR_SOURCES)

# ====================================================================
# --- Reglas de Ejecución y Utilidad ---
# ====================================================================

# Regla para generar los archivos .bin
run-generator: $(TARGET_GENERATOR)
	@echo "Ejecutando el generador de datos..."
	@# Se ejecuta desde el directorio padre para que los .bin se creen ahí
	cd $(PARENT_DIR) && ./generator

# Regla para ejecutar el análisis paralelo
run-parallel: $(TARGET_PARALLEL)
	@echo "Ejecutando el análisis paralelo..."
	@# Se ejecuta desde el directorio padre para que encuentre los .bin
	cd $(PARENT_DIR) && ./parteACodigos/$(TARGET_PARALLEL)

# Limpia todos los archivos generados
clean:
	@echo "Limpiando ejecutables y archivos objeto..."
	rm -f $(TARGET_PARALLEL) $(TARGET_GENERATOR) *.o

# Regla para ejecutar todo el flujo de trabajo
run-all:
	@# Compila todo, genera los datos y ejecuta el análisis
	$(MAKE) all
	$(MAKE) run-generator
	$(MAKE) run-parallel

.PHONY: all clean run-generator run-parallel run-all