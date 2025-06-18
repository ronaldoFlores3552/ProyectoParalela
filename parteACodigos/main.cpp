// main.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>

// Incluir las implementaciones
#include "marching_cube_serial.cpp"

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
    // Cargar datos de volumen desde archivo binario
    std::vector<float> loadVolumeData(const std::string &filename, int &gridSize)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        // Leer tamaño (asumiendo que está al inicio del archivo)
        file.read(reinterpret_cast<char *>(&gridSize), sizeof(int));

        // Leer datos
        int totalSize = gridSize * gridSize * gridSize;
        std::vector<float> data(totalSize);
        file.read(reinterpret_cast<char *>(data.data()), totalSize * sizeof(float));

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

    // Ejecutar prueba serial
    PerformanceMetrics runSerialTest(float *volumeData, int gridSize, float isoValue)
    {
        PerformanceMetrics metrics;

        auto start = std::chrono::high_resolution_clock::now();

        // MarchingCubesSerial mc(volumeData, gridSize, isoValue);
        // auto triangles = mc.extractIsosurface();

        auto end = std::chrono::high_resolution_clock::now();

        metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.triangleCount = 0; // triangles.size();
        metrics.throughput = (gridSize * gridSize * gridSize) / (metrics.executionTime * 1e3);
        metrics.flops = calculateFLOPs(gridSize, metrics.triangleCount);

        return metrics;
    }

    // Ejecutar prueba paralela
    PerformanceMetrics runParallelTest(float *volumeData, int gridSize, float isoValue, int blockSize)
    {
        PerformanceMetrics metrics;

        // Incluir tiempo de transferencia de datos
        auto start = std::chrono::high_resolution_clock::now();

        // MarchingCubesCUDAOptimized mc(volumeData, gridSize, isoValue);
        // auto triangles = mc.extractIsosurface();

        auto end = std::chrono::high_resolution_clock::now();

        metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.triangleCount = 0; // triangles.size();
        metrics.throughput = (gridSize * gridSize * gridSize) / (metrics.executionTime * 1e3);
        metrics.flops = calculateFLOPs(gridSize, metrics.triangleCount);

        return metrics;
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
        std::cout << "Serial Time: " << serialMetric.executionTime << " ms\n\n";

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
        const int iterations = 10;
        double totalSerialTime = 0;
        double totalParallelTime = 0;
        double totalFLOPs = 0;

        for (int i = 0; i < iterations; i++)
        {
            auto serialMetric = runSerialTest(volumeData, gridSize, isoValue);
            auto parallelMetric = runParallelTest(volumeData, gridSize, isoValue, 8);

            totalSerialTime += serialMetric.executionTime;
            totalParallelTime += parallelMetric.executionTime;
            totalFLOPs = parallelMetric.flops; // Mismo para ambos
        }

        double avgSerialTime = totalSerialTime / iterations;
        double avgParallelTime = totalParallelTime / iterations;

        std::cout << "\nAverage Execution Times (over " << iterations << " runs):\n";
        std::cout << "  Serial:   " << avgSerialTime << " ms\n";
        std::cout << "  Parallel: " << avgParallelTime << " ms\n";
        std::cout << "  Speedup:  " << avgSerialTime / avgParallelTime << "x\n";

        std::cout << "\nCompute Performance:\n";
        std::cout << "  Total FLOPs:     " << totalFLOPs << "\n";
        std::cout << "  Serial GFLOPS:   " << (totalFLOPs / avgSerialTime) / 1e6 << "\n";
        std::cout << "  Parallel GFLOPS: " << (totalFLOPs / avgParallelTime) / 1e6 << "\n";

        // Análisis de ancho de banda
        double dataSize = gridSize * gridSize * gridSize * sizeof(float);
        double bandwidth = dataSize / (avgParallelTime * 1e6); // GB/s

        std::cout << "\nMemory Bandwidth:\n";
        std::cout << "  Data size:         " << dataSize / 1e9 << " GB\n";
        std::cout << "  Effective B/W:     " << bandwidth << " GB/s\n";
    }

    // Generar gráficas (datos para gnuplot)
    void generatePlotData()
    {
        std::ofstream speedupFile("speedup_data.txt");
        std::ofstream flopsFile("flops_data.txt");

        // Datos de speedup vs número de threads
        speedupFile << "# Threads Speedup Efficiency\n";
        for (int p = 1; p <= 32; p *= 2)
        {
            double speedup = p * 0.85; // Modelo simplificado
            double efficiency = speedup / p;
            speedupFile << p << " " << speedup << " " << efficiency << "\n";
        }

        // Datos de FLOPS vs tamaño del problema
        flopsFile << "# GridSize GFLOPS_Serial GFLOPS_Parallel\n";
        std::vector<int> sizes = {32, 64, 128, 256, 512};
        for (int size : sizes)
        {
            double flops = calculateFLOPs(size, size * size * 0.1);
            double serialGFLOPS = flops / (size * size * 0.001) / 1e9;
            double parallelGFLOPS = serialGFLOPS * 15; // Factor de speedup estimado
            flopsFile << size << " " << serialGFLOPS << " " << parallelGFLOPS << "\n";
        }

        speedupFile.close();
        flopsFile.close();

        std::cout << "\nPlot data generated: speedup_data.txt, flops_data.txt\n";
    }
};

int main(int argc, char *argv[])
{
    try
    {
        PerformanceAnalyzer analyzer;

        // Parámetros
        int gridSize = 256;
        float isoValue = 0.0f;

        // Generar o cargar datos
        std::vector<float> volumeData;

        if (argc > 1)
        {
            // Cargar desde archivo
            volumeData = analyzer.loadVolumeData(argv[1], gridSize);
            std::cout << "Loaded volume data from " << argv[1] << "\n";
        }
        else
        {
            // Generar esfera sintética
            volumeData = analyzer.generateSphereData(gridSize, gridSize * 0.4f);
            std::cout << "Generated synthetic sphere data\n";
        }

        std::cout << "Grid size: " << gridSize << "³\n";
        std::cout << "Iso-value: " << isoValue << "\n";

        // Ejecutar análisis
        analyzer.strongScalingAnalysis(volumeData.data(), gridSize, isoValue);
        analyzer.weakScalingAnalysis(isoValue);
        analyzer.detailedPerformanceAnalysis(volumeData.data(), gridSize, isoValue);
        analyzer.generatePlotData();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}