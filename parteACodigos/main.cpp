// main.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>

// Incluir solo el header
#include "marching_cube_serial.h"
#include <algorithm>

struct PerformanceMetrics
{
    double executionTime;
    double throughput;
    int triangleCount;
    double flops;
};

class PerformanceAnalyzer
{
private:
    std::vector<PerformanceMetrics> serialMetrics;
    std::vector<PerformanceMetrics> parallelMetrics;

public:
    // Cargar datos de volumen desde archivo binario (formato del generador)
    std::vector<float> loadVolumeData(const std::string &filename, int &gridSize)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        // Obtener tamaño del archivo
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::cout << "File size: " << fileSize << " bytes" << std::endl;

        // Formato del generador: 3 enteros (nx, ny, nz) + datos float
        int nx, ny, nz;

        // Leer dimensiones
        file.read(reinterpret_cast<char *>(&nx), sizeof(int));
        file.read(reinterpret_cast<char *>(&ny), sizeof(int));
        file.read(reinterpret_cast<char *>(&nz), sizeof(int));

        std::cout << "Dimensions from file: " << nx << "x" << ny << "x" << nz << std::endl;

        // Verificar que sea un cubo
        if (nx != ny || ny != nz)
        {
            std::cerr << "Warning: Non-cubic grid detected. Using nx=" << nx << " as grid size." << std::endl;
        }
        gridSize = nx;

        // Verificar tamaño del archivo
        size_t expectedFloats = static_cast<size_t>(nx) * ny * nz;
        size_t expectedSize = 3 * sizeof(int) + expectedFloats * sizeof(float);

        std::cout << "Expected file size: " << expectedSize << " bytes" << std::endl;
        std::cout << "Expected floats: " << expectedFloats << std::endl;

        if (fileSize != expectedSize)
        {
            std::cerr << "Warning: File size mismatch. Expected " << expectedSize
                      << " bytes, got " << fileSize << " bytes" << std::endl;
        }

        // Leer los datos
        std::vector<float> data(expectedFloats);

        std::cout << "Reading float data..." << std::endl;
        size_t elementsRead = 0;

        for (int x = 0; x < nx; x++)
        {
            for (int y = 0; y < ny; y++)
            {
                for (int z = 0; z < nz; z++)
                {
                    float value;
                    file.read(reinterpret_cast<char *>(&value), sizeof(float));

                    // Convertir de índices (x,y,z) a índice lineal (z,y,x) para Marching Cubes
                    size_t linearIndex = z * nx * ny + y * nx + x;
                    data[linearIndex] = value;
                    elementsRead++;
                }
            }

            // Mostrar progreso cada 25%
            if (x % (nx / 4) == 0)
            {
                std::cout << "Reading progress: " << (100 * x / nx) << "%" << std::endl;
            }
        }

        file.close();

        std::cout << "Successfully read " << elementsRead << " float values" << std::endl;

        // Verificar estadísticas de los datos
        auto minMax = std::minmax_element(data.begin(), data.end());
        float minVal = *minMax.first;
        float maxVal = *minMax.second;

        std::cout << "Data range: [" << minVal << ", " << maxVal << "]" << std::endl;

        // Sugerir iso-value basado en el tipo de datos
        if (filename.find("sphere") != std::string::npos)
        {
            std::cout << "Detected sphere data. Recommended iso-value: 0.0" << std::endl;
        }
        else if (filename.find("waves") != std::string::npos)
        {
            std::cout << "Detected waves data. Recommended iso-value: 5.0" << std::endl;
        }

        return data;
    }

    // Generar datos sintéticos (esfera)
    std::vector<float> generateSphereData(int gridSize, float radius)
    {
        std::vector<float> data(gridSize * gridSize * gridSize);
        float center = gridSize / 2.0f;

        for (int z = 0; z < gridSize; z++)
        {
            for (int y = 0; y < gridSize; y++)
            {
                for (int x = 0; x < gridSize; x++)
                {
                    float dx = x - center;
                    float dy = y - center;
                    float dz = z - center;
                    float distance = sqrt(dx * dx + dy * dy + dz * dz);
                    data[z * gridSize * gridSize + y * gridSize + x] = radius - distance;
                }
            }
        }
        return data;
    }

    // Calcular FLOPs para Marching Cubes
    double calculateFLOPs(int gridSize, int triangleCount)
    {
        int numCubes = (gridSize - 1) * (gridSize - 1) * (gridSize - 1);

        // FLOPs por cubo:
        // - 8 comparaciones para índice de configuración
        // - ~12 interpolaciones (3 muls + 3 adds por interpolación)
        // - Acceso a tabla (consideramos 0 FLOPs)
        double flopsPerCube = 8 + 12 * 6; // promedio de interpolaciones

        return numCubes * flopsPerCube;
    }

    // Ejecutar prueba serial - AHORA FUNCIONAL
    PerformanceMetrics runSerialTest(float *volumeData, int gridSize, float isoValue)
    {
        PerformanceMetrics metrics;

        if (!volumeData || gridSize <= 0)
        {
            std::cerr << "Error: Invalid input data" << std::endl;
            return metrics;
        }

        std::cout << "Running serial test with grid " << gridSize << "³, isoValue=" << isoValue << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        try
        {
            // Crear instancia de Marching Cubes y configurarla
            MarchingCubesSerial mc;
            mc.setScalarField(volumeData, gridSize, gridSize, gridSize);
            mc.setIsoValue(isoValue);

            // Ejecutar el algoritmo
            std::vector<Triangle> triangles = mc.generateIsosurface();

            auto end = std::chrono::high_resolution_clock::now();

            metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
            metrics.triangleCount = triangles.size();
            metrics.throughput = (gridSize * gridSize * gridSize) / (metrics.executionTime * 1e-3);
            metrics.flops = calculateFLOPs(gridSize, metrics.triangleCount);

            std::cout << "Serial test completed successfully" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in serial test: " << e.what() << std::endl;
            auto end = std::chrono::high_resolution_clock::now();
            metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
        }

        return metrics;
    }

    // Ejecutar prueba paralela (placeholder para futuro)
    PerformanceMetrics runParallelTest(float *volumeData, int gridSize, float isoValue, int blockSize)
    {
        PerformanceMetrics metrics;

        auto start = std::chrono::high_resolution_clock::now();

        // Por ahora, usar la versión serial
        MarchingCubesSerial mc;
        mc.setScalarField(volumeData, gridSize, gridSize, gridSize);
        mc.setIsoValue(isoValue);

        std::vector<Triangle> triangles = mc.generateIsosurface();

        auto end = std::chrono::high_resolution_clock::now();

        metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.triangleCount = triangles.size();
        metrics.throughput = (gridSize * gridSize * gridSize) / (metrics.executionTime * 1e-3);
        metrics.flops = calculateFLOPs(gridSize, metrics.triangleCount);

        return metrics;
    }

    // Test con archivos específicos
    void testWithFiles(const std::vector<std::string> &filenames, float isoValue)
    {
        std::cout << "\n=== Testing with Binary Files ===\n";

        for (const auto &filename : filenames)
        {
            try
            {
                std::cout << "\n--- Testing: " << filename << " ---\n";

                int gridSize;
                auto volumeData = loadVolumeData(filename, gridSize);

                // Ejecutar test serial
                auto metrics = runSerialTest(volumeData.data(), gridSize, isoValue);

                std::cout << "Results:\n";
                std::cout << "  Execution Time: " << std::fixed << std::setprecision(2)
                          << metrics.executionTime << " ms\n";
                std::cout << "  Triangles Generated: " << metrics.triangleCount << "\n";
                std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                          << metrics.throughput / 1e6 << " Mvoxels/s\n";
                std::cout << "  Estimated GFLOPS: " << std::fixed << std::setprecision(3)
                          << (metrics.flops / metrics.executionTime) / 1e6 << "\n";
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing " << filename << ": " << e.what() << "\n";
            }
        }
    }

    // Análisis de escalabilidad fuerte
    void strongScalingAnalysis(float *volumeData, int gridSize, float isoValue)
    {
        std::cout << "\n=== Strong Scaling Analysis ===\n";
        std::cout << "Grid Size: " << gridSize << "³\n\n";

        std::vector<int> blockSizes = {4, 8, 16, 32};
        std::vector<double> speedups;

        // Baseline serial
        auto serialMetric = runSerialTest(volumeData, gridSize, isoValue);
        std::cout << "Serial Time: " << serialMetric.executionTime << " ms\n";
        std::cout << "Serial Triangles: " << serialMetric.triangleCount << "\n\n";

        std::cout << std::setw(12) << "Block Size"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Speedup"
                  << std::setw(15) << "Efficiency\n";
        std::cout << std::string(60, '-') << "\n";

        for (int blockSize : blockSizes)
        {
            auto metric = runParallelTest(volumeData, gridSize, isoValue, blockSize);
            double speedup = serialMetric.executionTime / metric.executionTime;
            double efficiency = speedup / (blockSize * blockSize * blockSize);

            std::cout << std::setw(12) << blockSize
                      << std::setw(15) << std::fixed << std::setprecision(2)
                      << metric.executionTime
                      << std::setw(15) << speedup
                      << std::setw(15) << efficiency << "\n";

            speedups.push_back(speedup);
        }
    }

    // Análisis de escalabilidad débil
    void weakScalingAnalysis(float isoValue)
    {
        std::cout << "\n=== Weak Scaling Analysis ===\n";
        std::cout << "Work per thread: constant\n\n";

        std::vector<int> gridSizes = {64, 128, 256, 512};
        std::vector<int> blockSizes = {4, 8, 16, 32};

        std::cout << std::setw(12) << "Grid Size"
                  << std::setw(15) << "Block Size"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(20) << "Throughput (Mvox/s)\n";
        std::cout << std::string(65, '-') << "\n";

        for (int i = 0; i < gridSizes.size(); i++)
        {
            auto data = generateSphereData(gridSizes[i], gridSizes[i] * 0.4f);
            auto metric = runParallelTest(data.data(), gridSizes[i], isoValue, blockSizes[i]);

            std::cout << std::setw(12) << gridSizes[i]
                      << std::setw(15) << blockSizes[i]
                      << std::setw(15) << std::fixed << std::setprecision(2)
                      << metric.executionTime
                      << std::setw(20) << metric.throughput / 1e6 << "\n";
        }
    }

    // Análisis de rendimiento detallado
    void detailedPerformanceAnalysis(float *volumeData, int gridSize, float isoValue)
    {
        std::cout << "\n=== Detailed Performance Analysis ===\n";

        // Ejecutar múltiples iteraciones para promediar
        const int iterations = 5;
        double totalSerialTime = 0;
        double totalParallelTime = 0;
        double totalFLOPs = 0;
        int totalTriangles = 0;

        for (int i = 0; i < iterations; i++)
        {
            auto serialMetric = runSerialTest(volumeData, gridSize, isoValue);
            auto parallelMetric = runParallelTest(volumeData, gridSize, isoValue, 8);

            totalSerialTime += serialMetric.executionTime;
            totalParallelTime += parallelMetric.executionTime;
            totalFLOPs = parallelMetric.flops; // Mismo para ambos
            totalTriangles = serialMetric.triangleCount;
        }

        double avgSerialTime = totalSerialTime / iterations;
        double avgParallelTime = totalParallelTime / iterations;

        std::cout << "\nAverage Execution Times (over " << iterations << " runs):\n";
        std::cout << "  Serial:   " << avgSerialTime << " ms\n";
        std::cout << "  Parallel: " << avgParallelTime << " ms\n";
        std::cout << "  Speedup:  " << avgSerialTime / avgParallelTime << "x\n";
        std::cout << "  Triangles: " << totalTriangles << "\n";

        std::cout << "\nCompute Performance:\n";
        std::cout << "  Total FLOPs:     " << totalFLOPs << "\n";
        std::cout << "  Serial GFLOPS:   " << (totalFLOPs / avgSerialTime) / 1e6 << "\n";
        std::cout << "  Parallel GFLOPS: " << (totalFLOPs / avgParallelTime) / 1e6 << "\n";

        // Análisis de ancho de banda
        double dataSize = gridSize * gridSize * gridSize * sizeof(float);
        double bandwidth = dataSize / (avgParallelTime * 1e-3) / 1e9; // GB/s

        std::cout << "\nMemory Bandwidth:\n";
        std::cout << "  Data size:         " << dataSize / 1e9 << " GB\n";
        std::cout << "  Effective B/W:     " << bandwidth << " GB/s\n";
    }
};

int main(int argc, char *argv[])
{
    try
    {
        PerformanceAnalyzer analyzer;

        // Archivos de test específicos
        std::vector<std::string> testFiles = {
            "test_sphere_32.bin",
            "test_sphere_48.bin",
            "test_sphere_64.bin",
            "test_waves_48.bin"};

        float isoValue = 0.0f;

        if (argc > 1)
        {
            // Probar archivo específico
            std::string filename = argv[1];
            std::cout << "Testing single file: " << filename << std::endl;

            int gridSize;
            auto volumeData = analyzer.loadVolumeData(filename, gridSize);

            std::cout << "Grid size: " << gridSize << "³\n";
            std::cout << "Iso-value: " << isoValue << "\n";

            // Ejecutar análisis completo
            analyzer.strongScalingAnalysis(volumeData.data(), gridSize, isoValue);
            analyzer.detailedPerformanceAnalysis(volumeData.data(), gridSize, isoValue);
        }
        else
        {
            // Probar todos los archivos
            std::cout << "Testing all binary files...\n";
            analyzer.testWithFiles(testFiles, isoValue);

            // Análisis adicional con datos sintéticos
            std::cout << "\n=== Additional Analysis with Synthetic Data ===\n";
            analyzer.weakScalingAnalysis(isoValue);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}