#ifndef MARCHING_CUBES_PARALLEL_H
#define MARCHING_CUBES_PARALLEL_H

#include <vector>

// Verificar si CUDA está disponible
#ifdef __CUDACC__
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#define CUDA_AVAILABLE 1
#else
#define CUDA_AVAILABLE 0
// Definiciones para cuando CUDA no está disponible
#define __host__
#define __device__
#define __global__

// Estructura dim3 simulada
struct dim3
{
    unsigned int x, y, z;
    dim3(unsigned int x_ = 1, unsigned int y_ = 1, unsigned int z_ = 1) : x(x_), y(y_), z(z_) {}
};

// Definiciones de error de CUDA simuladas
typedef int cudaError_t;
#define cudaSuccess 0
#endif

// Estructura para representar un vértice 3D (compatible con CUDA)
struct Vertex
{
    float x, y, z;

    __host__ __device__ Vertex() : x(0), y(0), z(0) {}
    __host__ __device__ Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    __host__ __device__ Vertex operator-(const Vertex &other) const
    {
        return Vertex(x - other.x, y - other.y, z - other.z);
    }

    __host__ __device__ Vertex operator+(const Vertex &other) const
    {
        return Vertex(x + other.x, y + other.y, z + other.z);
    }

    __host__ __device__ Vertex operator*(float scalar) const
    {
        return Vertex(x * scalar, y * scalar, z * scalar);
    }
};

// Estructura para representar un triángulo
struct Triangle
{
    Vertex v0, v1, v2;

    __host__ __device__ Triangle() {}
    __host__ __device__ Triangle(const Vertex &a, const Vertex &b, const Vertex &c)
        : v0(a), v1(b), v2(c) {}
};

// Estructura para almacenar múltiples triángulos por hilo
struct TriangleBuffer
{
    Triangle triangles[15]; // Máximo triángulos por cubo
    int count;

    __host__ __device__ TriangleBuffer() : count(0) {}
};

// Clase principal para el algoritmo Marching Cubes paralelo
class MarchingCubesParallel
{
private:
    // Datos del volumen en GPU
    float *d_scalarField;
    TriangleBuffer *d_triangleBuffers;
    Triangle *d_outputTriangles;
    int *d_triangleCounts;

    // Datos del volumen en CPU
    float *h_scalarField;
    int sizeX, sizeY, sizeZ;
    float isoValue;

    // Configuración de ejecución CUDA
    dim3 blockSize;
    dim3 gridSize;

    // Métodos privados
    void allocateGPUMemory();
    void freeGPUMemory();
    void copyDataToGPU();
    void copyResultsFromGPU(std::vector<Triangle> &triangles);

public:
    // Constructor
    MarchingCubesParallel();

    // Destructor
    ~MarchingCubesParallel();

    // Configura los datos del volumen
    void setScalarField(float *data, int sx, int sy, int sz);

    // Establece el isovalor
    void setIsoValue(float value) { isoValue = value; }

    // Configura los parámetros de ejecución CUDA
    void setExecutionConfig(dim3 blockSize, dim3 gridSize);

    // Ejecuta el algoritmo y devuelve los triángulos generados
    std::vector<Triangle> generateIsosurface();

    // Versión que devuelve el número de triángulos generados
    int generateIsosurface(std::vector<Triangle> &triangles);

    // Obtener información de la GPU
    static void printGPUInfo();
};

#if CUDA_AVAILABLE
// Declaraciones de kernels CUDA
__global__ void marchingCubesKernel(
    float *scalarField,
    int sizeX, int sizeY, int sizeZ,
    float isoValue,
    TriangleBuffer *triangleBuffers,
    int *triangleCounts);

__global__ void compactTrianglesKernel(
    TriangleBuffer *triangleBuffers,
    int *triangleCounts,
    Triangle *outputTriangles,
    int *prefixSum,
    int numCubes);

// Funciones device auxiliares
__device__ float getScalarValueDevice(float *scalarField, int x, int y, int z, int sizeX, int sizeY, int sizeZ);
__device__ Vertex interpolateVertexDevice(const Vertex &v1, float val1, const Vertex &v2, float val2, float isoValue);
__device__ void processCubeDevice(int x, int y, int z, float *scalarField, int sizeX, int sizeY, int sizeZ,
                                  float isoValue, TriangleBuffer &triangleBuffer);
#endif

#endif // MARCHING_CUBES_PARALLEL_H