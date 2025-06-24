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

    // Test con archivos específicos (removido - lógica movida al main)

    // Análisis de escalabilidad fuerte
    void strongScalingAnalysis(float *volumeData, int gridSize, float isoValue)
    {
        std::cout << "\n=== Strong Scaling Analysis ===\n";
        std::cout << "Grid Size: " << gridSize << "³\n\n";

        // NOTA: Actualmente solo tenemos implementación serial
        std::cout << "NOTE: Parallel implementation not yet available.\n";
        std::cout << "Running multiple serial tests to show baseline performance.\n\n";

        std::vector<int> threadCounts = {1, 2, 4, 8};
        std::vector<double> times;

        // Baseline serial
        auto serialMetric = runSerialTest(volumeData, gridSize, isoValue);
        std::cout << "Baseline Serial Time: " << serialMetric.executionTime << " ms\n";
        std::cout << "Baseline Triangles: " << serialMetric.triangleCount << "\n\n";

        std::cout << std::setw(12) << "Threads"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Speedup"
                  << std::setw(15) << "Efficiency\n";
        std::cout << std::string(60, '-') << "\n";

        for (int threads : threadCounts)
        {
            // Por ahora, simular tiempos para mostrar el formato esperado
            // En una implementación real, aquí iría la versión paralela
            double time = serialMetric.executionTime; // Mismo tiempo (serial)
            double speedup = 1.0;                     // Sin speedup real
            double efficiency = speedup / threads;

            std::cout << std::setw(12) << threads
                      << std::setw(15) << std::fixed << std::setprecision(2)
                      << time
                      << std::setw(15) << speedup
                      << std::setw(15) << efficiency << "\n";

            times.push_back(time);
        }

        std::cout << "\n💡 TODO: Implement CUDA/OpenMP parallel version for real scaling analysis\n";
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
        double totalFLOPs = 0;
        int totalTriangles = 0;

        std::cout << "Running " << iterations << " iterations for statistical accuracy...\n";

        for (int i = 0; i < iterations; i++)
        {
            auto serialMetric = runSerialTest(volumeData, gridSize, isoValue);
            totalSerialTime += serialMetric.executionTime;
            totalFLOPs = serialMetric.flops; // Mismo para todas las iteraciones
            totalTriangles = serialMetric.triangleCount;
        }

        double avgSerialTime = totalSerialTime / iterations;

        std::cout << "\n📊 Performance Metrics (average over " << iterations << " runs):\n";
        std::cout << "  Average Execution Time: " << std::fixed << std::setprecision(2)
                  << avgSerialTime << " ms\n";
        std::cout << "  Triangles Generated: " << totalTriangles << "\n";
        std::cout << "  Voxels Processed: " << (gridSize * gridSize * gridSize) << "\n";

        std::cout << "\n⚡ Compute Performance:\n";
        std::cout << "  Estimated FLOPs: " << std::fixed << std::setprecision(0) << totalFLOPs << "\n";
        std::cout << "  Serial GFLOPS: " << std::fixed << std::setprecision(3)
                  << (totalFLOPs / avgSerialTime) / 1e6 << "\n";

        // Análisis de throughput
        double voxelsPerSecond = (gridSize * gridSize * gridSize) / (avgSerialTime * 1e-3);
        std::cout << "\n📈 Throughput:\n";
        std::cout << "  Voxels/second: " << std::fixed << std::setprecision(0) << voxelsPerSecond << "\n";
        std::cout << "  Mvoxels/second: " << std::fixed << std::setprecision(2) << voxelsPerSecond / 1e6 << "\n";

        // Análisis de memoria
        double dataSize = gridSize * gridSize * gridSize * sizeof(float);
        double bandwidth = dataSize / (avgSerialTime * 1e-3) / 1e9; // GB/s

        std::cout << "\n💾 Memory Analysis:\n";
        std::cout << "  Input data size: " << std::fixed << std::setprecision(3) << dataSize / 1e6 << " MB\n";
        std::cout << "  Effective bandwidth: " << std::fixed << std::setprecision(3) << bandwidth << " GB/s\n";

        // Análisis de escalabilidad teórica
        std::cout << "\n🎯 Scalability Potential:\n";
        std::cout << "  Cubes processed: " << ((gridSize - 1) * (gridSize - 1) * (gridSize - 1)) << "\n";
        std::cout << "  Avg triangles/cube: " << std::fixed << std::setprecision(3)
                  << (double)totalTriangles / ((gridSize - 1) * (gridSize - 1) * (gridSize - 1)) << "\n";
        std::cout << "  Time per triangle: " << std::fixed << std::setprecision(3)
                  << avgSerialTime / totalTriangles << " ms\n";

        std::cout << "\n💡 Next steps: Implement CUDA/OpenMP version for real parallel analysis\n";
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

        if (argc > 1)
        {
            // Probar archivo específico
            std::string filename = argv[1];
            std::cout << "Testing single file: " << filename << std::endl;

            int gridSize;
            auto volumeData = analyzer.loadVolumeData(filename, gridSize);

            // Determinar iso-value apropiado basado en el nombre del archivo
            float isoValue = 0.0f; // Default
            if (filename.find("sphere") != std::string::npos)
            {
                isoValue = 0.0f;
                std::cout << "Using sphere iso-value: " << isoValue << std::endl;
            }
            else if (filename.find("waves") != std::string::npos)
            {
                isoValue = 5.0f;
                std::cout << "Using waves iso-value: " << isoValue << std::endl;
            }

            std::cout << "Grid size: " << gridSize << "³" << std::endl;
            std::cout << "Iso-value: " << isoValue << std::endl;

            // Ejecutar análisis completo
            analyzer.strongScalingAnalysis(volumeData.data(), gridSize, isoValue);
            analyzer.detailedPerformanceAnalysis(volumeData.data(), gridSize, isoValue);
        }
        else
        {
            // Probar todos los archivos
            std::cout << "Testing all binary files..." << std::endl;

            for (const auto &filename : testFiles)
            {
                try
                {
                    std::cout << "\n"
                              << std::string(60, '=') << std::endl;
                    std::cout << "Testing: " << filename << std::endl;

                    int gridSize;
                    auto volumeData = analyzer.loadVolumeData(filename, gridSize);

                    // Determinar iso-value apropiado
                    float isoValue = 0.0f;
                    if (filename.find("sphere") != std::string::npos)
                    {
                        isoValue = 0.0f;
                    }
                    else if (filename.find("waves") != std::string::npos)
                    {
                        isoValue = 5.0f;
                    }

                    // Ejecutar test básico
                    auto metrics = analyzer.runSerialTest(volumeData.data(), gridSize, isoValue);

                    std::cout << "\nResults for " << filename << ":" << std::endl;
                    std::cout << "  Grid Size: " << gridSize << "³" << std::endl;
                    std::cout << "  Iso-value: " << isoValue << std::endl;
                    std::cout << "  Execution Time: " << std::fixed << std::setprecision(2)
                              << metrics.executionTime << " ms" << std::endl;
                    std::cout << "  Triangles Generated: " << metrics.triangleCount << std::endl;
                    std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                              << metrics.throughput / 1e6 << " Mvoxels/s" << std::endl;
                    std::cout << "  Estimated GFLOPS: " << std::fixed << std::setprecision(3)
                              << (metrics.flops / metrics.executionTime) / 1e6 << std::endl;
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error processing " << filename << ": " << e.what() << std::endl;
                }
            }

            // Análisis adicional con datos sintéticos
            std::cout << "\n"
                      << std::string(60, '=') << std::endl;
            std::cout << "=== Additional Analysis with Synthetic Data ===" << std::endl;
            analyzer.weakScalingAnalysis(0.0f);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}