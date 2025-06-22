#include "marching_cube_parallel.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

// Macro para verificar errores CUDA
#define CUDA_CHECK(call)                                                                                                  \
    do                                                                                                                    \
    {                                                                                                                     \
        cudaError_t err = call;                                                                                           \
        if (err != cudaSuccess)                                                                                           \
        {                                                                                                                 \
            std::cerr << "CUDA Error: " << cudaGetErrorString(err) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(1);                                                                                                      \
        }                                                                                                                 \
    } while (0)

// Tabla completa de aristas (256 entradas)
static const int h_edgeTable[256] = {
    0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33, 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa, 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66, 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff, 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55, 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66, 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa, 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33, 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99, 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0};

// Tabla de triangulación básica (inicializada con -1)
static int h_triTable[256][16];

static const int h_vertexOffsets[8][3] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

static const int h_edgeVertices[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

// Definición de tablas constantes en GPU
__constant__ int d_edgeTable[256];
__constant__ int d_triTable[256][16];
__constant__ int d_vertexOffsets[8][3];
__constant__ int d_edgeVertices[12][2];

// Función para inicializar la tabla de triangulación básica
void initializeTriTable()
{
    // Inicializar toda la tabla con -1
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            h_triTable[i][j] = -1;
        }
    }

    // Configuraciones básicas conocidas
    // Configuración 1: un vértice dentro
    h_triTable[1][0] = 0;
    h_triTable[1][1] = 8;
    h_triTable[1][2] = 3;

    // Configuración 2: un vértice dentro
    h_triTable[2][0] = 0;
    h_triTable[2][1] = 1;
    h_triTable[2][2] = 9;

    // Configuración 3: dos vértices adyacentes dentro
    h_triTable[3][0] = 1;
    h_triTable[3][1] = 8;
    h_triTable[3][2] = 3;
    h_triTable[3][3] = 9;
    h_triTable[3][4] = 8;
    h_triTable[3][5] = 1;

    // Más configuraciones se pueden agregar según sea necesario
    // Por simplicidad, las demás quedan inicializadas en -1
}

// Funciones device
__device__ float getScalarValueDevice(float *scalarField, int x, int y, int z, int sizeX, int sizeY, int sizeZ)
{
    if (x < 0 || x >= sizeX || y < 0 || y >= sizeY || z < 0 || z >= sizeZ)
    {
        return 0.0f;
    }
    return scalarField[z * sizeX * sizeY + y * sizeX + x];
}

__device__ Vertex interpolateVertexDevice(const Vertex &v1, float val1, const Vertex &v2, float val2, float isoValue)
{
    if (abs(isoValue - val1) < 0.00001f)
        return v1;
    if (abs(isoValue - val2) < 0.00001f)
        return v2;
    if (abs(val1 - val2) < 0.00001f)
        return v1;

    float t = (isoValue - val1) / (val2 - val1);
    return v1 + (v2 - v1) * t;
}

__device__ void processCubeDevice(int x, int y, int z, float *scalarField, int sizeX, int sizeY, int sizeZ,
                                  float isoValue, TriangleBuffer &triangleBuffer)
{
    // Obtener valores escalares en los 8 vértices del cubo
    float cubeValues[8];
    for (int i = 0; i < 8; i++)
    {
        cubeValues[i] = getScalarValueDevice(scalarField,
                                             x + d_vertexOffsets[i][0],
                                             y + d_vertexOffsets[i][1],
                                             z + d_vertexOffsets[i][2],
                                             sizeX, sizeY, sizeZ);
    }

    // Determinar índice de configuración
    int cubeIndex = 0;
    for (int i = 0; i < 8; i++)
    {
        if (cubeValues[i] < isoValue)
        {
            cubeIndex |= (1 << i);
        }
    }

    // Si no hay intersección, retornar
    if (d_edgeTable[cubeIndex] == 0)
    {
        triangleBuffer.count = 0;
        return;
    }

    // Encontrar vértices de intersección
    Vertex vertList[12];
    for (int i = 0; i < 12; i++)
    {
        if (d_edgeTable[cubeIndex] & (1 << i))
        {
            int v0 = d_edgeVertices[i][0];
            int v1 = d_edgeVertices[i][1];

            Vertex p0(x + d_vertexOffsets[v0][0], y + d_vertexOffsets[v0][1], z + d_vertexOffsets[v0][2]);
            Vertex p1(x + d_vertexOffsets[v1][0], y + d_vertexOffsets[v1][1], z + d_vertexOffsets[v1][2]);

            vertList[i] = interpolateVertexDevice(p0, cubeValues[v0], p1, cubeValues[v1], isoValue);
        }
    }

    // Crear triángulos
    triangleBuffer.count = 0;
    for (int i = 0; d_triTable[cubeIndex][i] != -1 && i < 15 && triangleBuffer.count < 5; i += 3)
    {
        if (d_triTable[cubeIndex][i] < 12 && d_triTable[cubeIndex][i + 1] < 12 && d_triTable[cubeIndex][i + 2] < 12)
        {
            Triangle tri(
                vertList[d_triTable[cubeIndex][i]],
                vertList[d_triTable[cubeIndex][i + 1]],
                vertList[d_triTable[cubeIndex][i + 2]]);
            triangleBuffer.triangles[triangleBuffer.count++] = tri;
        }
    }
}

// Kernel principal de Marching Cubes
__global__ void marchingCubesKernel(float *scalarField, int sizeX, int sizeY, int sizeZ, float isoValue,
                                    TriangleBuffer *triangleBuffers, int *triangleCounts)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    int idz = blockIdx.z * blockDim.z + threadIdx.z;

    if (idx >= sizeX - 1 || idy >= sizeY - 1 || idz >= sizeZ - 1)
        return;

    int cubeId = idz * (sizeX - 1) * (sizeY - 1) + idy * (sizeX - 1) + idx;

    TriangleBuffer buffer;
    processCubeDevice(idx, idy, idz, scalarField, sizeX, sizeY, sizeZ, isoValue, buffer);

    triangleBuffers[cubeId] = buffer;
    triangleCounts[cubeId] = buffer.count;
}

// Kernel para compactar triángulos
__global__ void compactTrianglesKernel(TriangleBuffer *triangleBuffers, int *triangleCounts,
                                       Triangle *outputTriangles, int *prefixSum, int numCubes)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numCubes)
        return;

    if (triangleCounts[idx] > 0)
    {
        int outputIndex = prefixSum[idx];
        for (int i = 0; i < triangleCounts[idx]; i++)
        {
            outputTriangles[outputIndex + i] = triangleBuffers[idx].triangles[i];
        }
    }
}

// Constructor
MarchingCubesParallel::MarchingCubesParallel()
    : d_scalarField(nullptr), d_triangleBuffers(nullptr), d_outputTriangles(nullptr), d_triangleCounts(nullptr),
      h_scalarField(nullptr), sizeX(0), sizeY(0), sizeZ(0), isoValue(0.0f),
      blockSize(dim3(8, 8, 8)), gridSize(dim3(1, 1, 1))
{
    // Inicializar tabla de triangulación
    initializeTriTable();

    // Copiar tablas a memoria constante
    CUDA_CHECK(cudaMemcpyToSymbol(d_edgeTable, h_edgeTable, sizeof(h_edgeTable)));
    CUDA_CHECK(cudaMemcpyToSymbol(d_triTable, h_triTable, sizeof(h_triTable)));
    CUDA_CHECK(cudaMemcpyToSymbol(d_vertexOffsets, h_vertexOffsets, sizeof(h_vertexOffsets)));
    CUDA_CHECK(cudaMemcpyToSymbol(d_edgeVertices, h_edgeVertices, sizeof(h_edgeVertices)));
}

// Destructor
MarchingCubesParallel::~MarchingCubesParallel()
{
    freeGPUMemory();
}

void MarchingCubesParallel::setScalarField(float *data, int sx, int sy, int sz)
{
    h_scalarField = data;
    sizeX = sx;
    sizeY = sy;
    sizeZ = sz;

    // Calcular configuración de grid
    gridSize = dim3(
        (sizeX - 1 + blockSize.x - 1) / blockSize.x,
        (sizeY - 1 + blockSize.y - 1) / blockSize.y,
        (sizeZ - 1 + blockSize.z - 1) / blockSize.z);
}

void MarchingCubesParallel::setExecutionConfig(dim3 bs, dim3 gs)
{
    blockSize = bs;
    gridSize = gs;
}

void MarchingCubesParallel::allocateGPUMemory()
{
    size_t volumeSize = sizeX * sizeY * sizeZ * sizeof(float);
    int numCubes = (sizeX - 1) * (sizeY - 1) * (sizeZ - 1);

    CUDA_CHECK(cudaMalloc(&d_scalarField, volumeSize));
    CUDA_CHECK(cudaMalloc(&d_triangleBuffers, numCubes * sizeof(TriangleBuffer)));
    CUDA_CHECK(cudaMalloc(&d_triangleCounts, numCubes * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_outputTriangles, numCubes * 15 * sizeof(Triangle))); // Max triángulos
}

void MarchingCubesParallel::freeGPUMemory()
{
    if (d_scalarField)
    {
        cudaFree(d_scalarField);
        d_scalarField = nullptr;
    }
    if (d_triangleBuffers)
    {
        cudaFree(d_triangleBuffers);
        d_triangleBuffers = nullptr;
    }
    if (d_triangleCounts)
    {
        cudaFree(d_triangleCounts);
        d_triangleCounts = nullptr;
    }
    if (d_outputTriangles)
    {
        cudaFree(d_outputTriangles);
        d_outputTriangles = nullptr;
    }
}

void MarchingCubesParallel::copyDataToGPU()
{
    size_t volumeSize = sizeX * sizeY * sizeZ * sizeof(float);
    CUDA_CHECK(cudaMemcpy(d_scalarField, h_scalarField, volumeSize, cudaMemcpyHostToDevice));
}

void MarchingCubesParallel::copyResultsFromGPU(std::vector<Triangle> &triangles)
{
    int numCubes = (sizeX - 1) * (sizeY - 1) * (sizeZ - 1);

    // Copiar conteos de triángulos
    std::vector<int> h_triangleCounts(numCubes);
    CUDA_CHECK(cudaMemcpy(h_triangleCounts.data(), d_triangleCounts, numCubes * sizeof(int), cudaMemcpyDeviceToHost));

    // Calcular prefix sum en CPU para simplificar
    std::vector<int> prefixSum(numCubes + 1, 0);
    for (int i = 0; i < numCubes; i++)
    {
        prefixSum[i + 1] = prefixSum[i] + h_triangleCounts[i];
    }
    int totalTriangles = prefixSum[numCubes];

    if (totalTriangles > 0)
    {
        // Copiar prefix sum a GPU
        int *d_prefixSum;
        CUDA_CHECK(cudaMalloc(&d_prefixSum, (numCubes + 1) * sizeof(int)));
        CUDA_CHECK(cudaMemcpy(d_prefixSum, prefixSum.data(), (numCubes + 1) * sizeof(int), cudaMemcpyHostToDevice));

        // Compactar triángulos
        dim3 compactBlockSize(256);
        dim3 compactGridSize((numCubes + compactBlockSize.x - 1) / compactBlockSize.x);
        compactTrianglesKernel<<<compactGridSize, compactBlockSize>>>(
            d_triangleBuffers, d_triangleCounts, d_outputTriangles, d_prefixSum, numCubes);

        CUDA_CHECK(cudaDeviceSynchronize());

        // Copiar resultados finales
        triangles.resize(totalTriangles);
        CUDA_CHECK(cudaMemcpy(triangles.data(), d_outputTriangles, totalTriangles * sizeof(Triangle), cudaMemcpyDeviceToHost));

        cudaFree(d_prefixSum);
    }
}

std::vector<Triangle> MarchingCubesParallel::generateIsosurface()
{
    std::vector<Triangle> triangles;
    generateIsosurface(triangles);
    return triangles;
}

int MarchingCubesParallel::generateIsosurface(std::vector<Triangle> &triangles)
{
    triangles.clear();

    if (!h_scalarField || sizeX <= 0 || sizeY <= 0 || sizeZ <= 0)
    {
        std::cerr << "Error: Campo escalar no configurado correctamente." << std::endl;
        return 0;
    }

    // Alocar memoria GPU
    allocateGPUMemory();

    // Copiar datos a GPU
    copyDataToGPU();

    // Ejecutar kernel principal
    marchingCubesKernel<<<gridSize, blockSize>>>(
        d_scalarField, sizeX, sizeY, sizeZ, isoValue, d_triangleBuffers, d_triangleCounts);

    CUDA_CHECK(cudaDeviceSynchronize());

    // Copiar resultados de vuelta a CPU
    copyResultsFromGPU(triangles);

    // Liberar memoria
    freeGPUMemory();

    return triangles.size();
}

void MarchingCubesParallel::printGPUInfo()
{
    int deviceCount;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));

    std::cout << "=== GPU Information ===" << std::endl;
    std::cout << "Number of CUDA devices: " << deviceCount << std::endl;

    for (int i = 0; i < deviceCount; i++)
    {
        cudaDeviceProp prop;
        CUDA_CHECK(cudaGetDeviceProperties(&prop, i));

        std::cout << "\nDevice " << i << ": " << prop.name << std::endl;
        std::cout << "  Compute capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "  Global memory: " << prop.totalGlobalMem / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Shared memory per block: " << prop.sharedMemPerBlock / 1024 << " KB" << std::endl;
        std::cout << "  Max threads per block: " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "  Max grid size: " << prop.maxGridSize[0] << " x " << prop.maxGridSize[1] << " x " << prop.maxGridSize[2] << std::endl;
    }
}