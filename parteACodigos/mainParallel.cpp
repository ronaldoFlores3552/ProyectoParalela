#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <string>

// Incluir las implementaciones
#include "marching_cube_parallel.h"

struct PerformanceMetrics
{
    double executionTime;
    double throughput;
    int triangleCount;
    double flops;
    double memoryBandwidth;
};

class ParallelPerformanceAnalyzer
{
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

        std::cout << "Loaded " << filename << " with grid size: " << gridSize << "³" << std::endl;
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

    // Ejecutar prueba paralela
    PerformanceMetrics runParallelTest(float *volumeData, int gridSize, float isoValue,
                                       dim3 blockSize = dim3(8, 8, 8))
    {
        PerformanceMetrics metrics;

        MarchingCubesParallel mc;
        mc.setScalarField(volumeData, gridSize, gridSize, gridSize);
        mc.setIsoValue(isoValue);
        mc.setExecutionConfig(blockSize, dim3((gridSize - 1 + blockSize.x - 1) / blockSize.x,
                                              (gridSize - 1 + blockSize.y - 1) / blockSize.y,
                                              (gridSize - 1 + blockSize.z - 1) / blockSize.z));

        // Incluir tiempo de transferencia de datos
        auto start = std::chrono::high_resolution_clock::now();

        auto triangles = mc.generateIsosurface();

        auto end = std::chrono::high_resolution_clock::now();

        metrics.executionTime = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.triangleCount = triangles.size();
        metrics.throughput = (gridSize * gridSize * gridSize) / (metrics.executionTime * 1e-3);
        metrics.flops = calculateFLOPs(gridSize, metrics.triangleCount);

        // Calcular ancho de banda efectivo
        double dataSize = gridSize * gridSize * gridSize * sizeof(float);
        metrics.memoryBandwidth = dataSize / (metrics.executionTime * 1e-3) / 1e9; // GB/s

        return metrics;
    }

    // Análisis de escalabilidad con diferentes tamaños de bloque
    void blockSizeAnalysis(float *volumeData, int gridSize, float isoValue)
    {
        std::cout << "\n=== Block Size Analysis ===\n";
        std::cout << "Grid Size: " << gridSize << "³\n\n";

        std::vector<dim3> blockSizes = {
            dim3(4, 4, 1), dim3(8, 8, 1), dim3(16, 16, 1),
            dim3(32, 32, 1), dim3(64, 16, 1), dim3(128, 8, 1)};

        std::cout << std::setw(15) << "Block Size"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Triangles"
                  << std::setw(20) << "Throughput (Mvox/s)"
                  << std::setw(15) << "GFLOPS\n";
        std::cout << std::string(80, '-') << "\n";

        for (const auto &blockSize : blockSizes)
        {
            try
            {
                auto metric = runParallelTest(volumeData, gridSize, isoValue, blockSize);
                double gflops = (metric.flops / metric.executionTime) / 1e6;

                std::cout << std::setw(15) << "(" << blockSize.x << "," << blockSize.y << "," << blockSize.z << ")"
                          << std::setw(15) << std::fixed << std::setprecision(2)
                          << metric.executionTime
                          << std::setw(15) << metric.triangleCount
                          << std::setw(20) << metric.throughput / 1e6
                          << std::setw(15) << gflops << "\n";
            }
            catch (const std::exception &e)
            {
                std::cout << std::setw(15) << "(" << blockSize.x << "," << blockSize.y << "," << blockSize.z << ")"
                          << std::setw(15) << "ERROR"
                          << std::setw(15) << "-"
                          << std::setw(20) << "-"
                          << std::setw(15) << "-" << "\n";
            }
        }
    }

    // Análisis con múltiples datasets
    void multiDatasetAnalysis(const std::vector<std::string> &filenames, float isoValue)
    {
        std::cout << "\n=== Multi-Dataset Analysis ===\n\n";

        std::cout << std::setw(20) << "Dataset"
                  << std::setw(12) << "Grid Size"
                  << std::setw(15) << "Time (ms)"
                  << std::setw(15) << "Triangles"
                  << std::setw(20) << "Throughput (Mvox/s)"
                  << std::setw(15) << "B/W (GB/s)\n";
        std::cout << std::string(100, '-') << "\n";

        for (const auto &filename : filenames)
        {
            try
            {
                int gridSize;
                auto data = loadVolumeData(filename, gridSize);
                auto metric = runParallelTest(data.data(), gridSize, isoValue);

                std::cout << std::setw(20) << filename
                          << std::setw(12) << gridSize
                          << std::setw(15) << std::fixed << std::setprecision(2)
                          << metric.executionTime
                          << std::setw(15) << metric.triangleCount
                          << std::setw(20) << metric.throughput / 1e6
                          << std::setw(15) << metric.memoryBandwidth << "\n";
            }
            catch (const std::exception &e)
            {
                std::cout << std::setw(20) << filename
                          << " - ERROR: " << e.what() << "\n";
            }
        }
    }

    // Análisis detallado de rendimiento
    void detailedPerformanceAnalysis(float *volumeData, int gridSize, float isoValue)
    {
        std::cout << "\n=== Detailed Performance Analysis ===\n";

        const int iterations = 5;
        double totalTime = 0;
        int totalTriangles = 0;
        double totalFLOPs = 0;
        double totalBandwidth = 0;

        std::cout << "Running " << iterations << " iterations...\n\n";

        for (int i = 0; i < iterations; i++)
        {
            auto metric = runParallelTest(volumeData, gridSize, isoValue);
            totalTime += metric.executionTime;
            totalTriangles = metric.triangleCount; // Debería ser el mismo
            totalFLOPs = metric.flops;             // Mismo para todas las iteraciones
            totalBandwidth += metric.memoryBandwidth;

            std::cout << "Iteration " << (i + 1) << ": " << metric.executionTime << " ms, "
                      << metric.triangleCount << " triangles\n";
        }

        double avgTime = totalTime / iterations;
        double avgBandwidth = totalBandwidth / iterations;

        std::cout << "\nPerformance Summary:\n";
        std::cout << "  Average Time:     " << avgTime << " ms\n";
        std::cout << "  Triangle Count:   " << totalTriangles << "\n";
        std::cout << "  Avg Throughput:   " << (gridSize * gridSize * gridSize) / (avgTime * 1e-3) / 1e6 << " Mvox/s\n";
        std::cout << "  Avg GFLOPS:       " << (totalFLOPs / avgTime) / 1e6 << "\n";
        std::cout << "  Avg Bandwidth:    " << avgBandwidth << " GB/s\n";

        // Información del volumen
        double volumeSize = gridSize * gridSize * gridSize * sizeof(float) / 1e9;
        int numCubes = (gridSize - 1) * (gridSize - 1) * (gridSize - 1);

        std::cout << "\nVolume Information:\n";
        std::cout << "  Grid Size:        " << gridSize << "³\n";
        std::cout << "  Volume Size:      " << volumeSize << " GB\n";
        std::cout << "  Number of Cubes:  " << numCubes << "\n";
        std::cout << "  Triangles/Cube:   " << (double)totalTriangles / numCubes << "\n";
    }

    // Generar datos para gráficas
    void generatePerformanceReport(const std::string &outputFile)
    {
        std::ofstream report(outputFile);
        if (!report.is_open())
        {
            std::cerr << "Cannot create report file: " << outputFile << std::endl;
            return;
        }

        report << "Marching Cubes Parallel Performance Report\n";
        report << "==========================================\n\n";

        // Información de GPU
        report << "GPU Configuration:\n";
        report << "-----------------\n";
        // Aquí se podría agregar información específica de la GPU

        report << "\nTest Results:\n";
        report << "-------------\n";
        report << "Timestamp: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "\n";

        report.close();
        std::cout << "Performance report generated: " << outputFile << std::endl;
    }
};

int main(int argc, char *argv[])
{
    try
    {
        // Mostrar información de GPU
        MarchingCubesParallel::printGPUInfo();

        ParallelPerformanceAnalyzer analyzer;

        // Parámetros por defecto
        float isoValue = 0.0f;

        // Lista de archivos de prueba
        std::vector<std::string> testFiles = {
            "test_sphere_32.bin",
            "test_sphere_48.bin",
            "test_sphere_64.bin",
            "test_waves_48.bin"};

        if (argc > 1)
        {
            // Analizar archivo específico
            std::string filename = argv[1];
            if (argc > 2)
            {
                isoValue = std::stof(argv[2]);
            }

            try
            {
                int gridSize;
                auto volumeData = analyzer.loadVolumeData(filename, gridSize);

                std::cout << "Analyzing: " << filename << std::endl;
                std::cout << "Grid size: " << gridSize << "³" << std::endl;
                std::cout << "Iso-value: " << isoValue << std::endl;

                analyzer.blockSizeAnalysis(volumeData.data(), gridSize, isoValue);
                analyzer.detailedPerformanceAnalysis(volumeData.data(), gridSize, isoValue);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing " << filename << ": " << e.what() << std::endl;
            }
        }
        else
        {
            // Ejecutar análisis completo con todos los archivos de prueba
            std::cout << "Running comprehensive analysis with test files...\n";
            std::cout << "Iso-value: " << isoValue << std::endl;

            // Analizar múltiples datasets
            analyzer.multiDatasetAnalysis(testFiles, isoValue);

            // Si existe al menos un archivo, hacer análisis detallado
            for (const auto &filename : testFiles)
            {
                std::ifstream test(filename);
                if (test.good())
                {
                    test.close();
                    try
                    {
                        int gridSize;
                        auto volumeData = analyzer.loadVolumeData(filename, gridSize);

                        std::cout << "\n"
                                  << std::string(50, '=') << std::endl;
                        std::cout << "Detailed analysis for: " << filename << std::endl;

                        analyzer.blockSizeAnalysis(volumeData.data(), gridSize, isoValue);
                        analyzer.detailedPerformanceAnalysis(volumeData.data(), gridSize, isoValue);
                        break; // Solo hacer análisis detallado del primer archivo encontrado
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error with " << filename << ": " << e.what() << std::endl;
                    }
                }
            }

            // Generar reporte
            analyzer.generatePerformanceReport("parallel_performance_report.txt");
        }

        std::cout << "\nAnalysis completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}