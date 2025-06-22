#!/bin/bash

echo "=== Compilador Marching Cubes Paralelo ==="
echo ""

# Verificar si CUDA está instalado
if ! command -v nvcc &> /dev/null; then
    echo "❌ ERROR: CUDA no está instalado o no está en el PATH"
    echo ""
    echo "Para instalar CUDA:"
    echo "1. Ubuntu/Debian: sudo apt install nvidia-cuda-toolkit"
    echo "2. Descargar desde: https://developer.nvidia.com/cuda-downloads"
    echo ""
    exit 1
fi

# Mostrar información de CUDA
echo "✓ CUDA encontrado:"
nvcc --version | head -n 1
echo ""

# Verificar archivos necesarios
REQUIRED_FILES=("mainParallel.cpp" "marching_cube_parallel.h" "marching_cube_parallel.cu")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "❌ ERROR: Archivo faltante: $file"
        exit 1
    fi
done

echo "✓ Todos los archivos necesarios encontrados"
echo ""

# Limpiar compilación anterior
if [ -f "marchingCubesParallel" ]; then
    echo "🧹 Limpiando compilación anterior..."
    rm -f marchingCubesParallel
fi

# Compilar
echo "🔨 Compilando Marching Cubes Paralelo..."
echo ""

NVCC_FLAGS="-std=c++14 -O3 -arch=sm_60 -lineinfo"
INCLUDES="-I/usr/local/cuda/include"
LIBS="-lcuda -lcudart"

echo "Comando: nvcc $NVCC_FLAGS $INCLUDES -o marchingCubesParallel mainParallel.cpp marching_cube_parallel.cu $LIBS"
echo ""

if nvcc $NVCC_FLAGS $INCLUDES -o marchingCubesParallel mainParallel.cpp marching_cube_parallel.cu $LIBS; then
    echo ""
    echo "✅ Compilación exitosa!"
    echo ""
    
    # Verificar que el ejecutable se creó
    if [ -f "marchingCubesParallel" ]; then
        echo "📋 Ejecutable creado: marchingCubesParallel"
        echo ""
        echo "📖 Uso:"
        echo "  ./marchingCubesParallel                    # Análisis completo con archivos de prueba"
        echo "  ./marchingCubesParallel archivo.bin [iso]  # Analizar archivo específico"
        echo ""
        echo "Ejemplos:"
        echo "  ./marchingCubesParallel test_sphere_64.bin 0.0"
        echo "  ./marchingCubesParallel test_waves_48.bin"
        echo ""
        
        # Mostrar información de GPU si está disponible
        if command -v nvidia-smi &> /dev/null; then
            echo "🖥️  GPU disponible:"
            nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits | head -n 1
            echo ""
        fi
        
        echo "¡Listo para ejecutar!"
    else
        echo "❌ Error: El ejecutable no se creó correctamente"
        exit 1
    fi
else
    echo ""
    echo "❌ Error en la compilación"
    echo ""
    echo "Posibles soluciones:"
    echo "1. Verificar que CUDA esté correctamente instalado"
    echo "2. Verificar que tu GPU sea compatible (Compute Capability >= 6.0)"
    echo "3. Intentar con un arch diferente: -arch=sm_50 o -arch=sm_75"
    echo ""
    exit 1
fi