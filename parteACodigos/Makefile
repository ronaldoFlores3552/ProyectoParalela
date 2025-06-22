# Makefile para Marching Cubes Parallel

# Makefile para Marching Cubes Parallel

# Compiladores
NVCC = nvcc

# Flags de compilación
NVCCFLAGS = -std=c++14 -O3 -arch=sm_60 -lineinfo
INCLUDES = -I/usr/local/cuda/include
LIBS = -lcuda -lcudart

# Archivos fuente
SOURCES = mainParallel.cpp marching_cube_parallel.cu
TARGET = marchingCubesParallel

# Detectar si CUDA está disponible
CUDA_AVAILABLE := $(shell which nvcc 2>/dev/null)

ifdef CUDA_AVAILABLE
    # Compilación con CUDA
    all: $(TARGET)

    $(TARGET): $(SOURCES)
	@echo "Compilando con CUDA..."
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES) $(LIBS)

    # Regla para compilar con debug
    debug: NVCCFLAGS += -g -G
    debug: $(TARGET)

    # Limpiar archivos generados
    clean:
	rm -f $(TARGET) *.o

    # Ejecutar pruebas
    test: $(TARGET)
	./$(TARGET)

    # Información del sistema
    info:
	@echo "=== CUDA System Information ==="
	@nvcc --version
	@nvidia-smi

else
    # Sin CUDA disponible
    all:
	@echo "ERROR: CUDA no está disponible en el sistema"
	@echo "Por favor instala CUDA Toolkit desde:"
	@echo "https://developer.nvidia.com/cuda-downloads"
	@echo ""
	@echo "Para Ubuntu/Debian:"
	@echo "sudo apt-get install nvidia-cuda-toolkit"
	@echo ""
	@echo "Para compilar una versión serial, usa:"
	@echo "g++ -o marchingCubesSerial main.cpp marching_cube_serial.cpp"

    clean:
	@echo "Nada que limpiar (CUDA no disponible)"

    info:
	@echo "CUDA no está instalado"
	@echo "Instala CUDA Toolkit para usar la versión paralela"
endif

# Reglas adicionales
.PHONY: all clean test debug info

# Ayuda
help:
	@echo "Uso del Makefile:"
	@echo "  make all     - Compilar el programa"
	@echo "  make debug   - Compilar con información de debug"
	@echo "  make test    - Ejecutar pruebas"
	@echo "  make clean   - Limpiar archivos generados"
	@echo "  make info    - Mostrar información del sistema"
	@echo "  make help    - Mostrar esta ayuda"