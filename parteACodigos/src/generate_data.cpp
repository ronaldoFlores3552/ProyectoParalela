#include "generate_data.h"
#include <iostream>
#include <chrono>

int main()
{
    std::cout << "=== GENERADOR DE DATOS DE PRUEBA PARA MARCHING CUBES ===" << std::endl;
    std::cout << "Versión expandida con datasets grandes (96³, 128³, 256³, 512³)" << std::endl;
    std::cout << "Tiempo estimado: 2-5 minutos (depende del hardware)" << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    try
    {
        // Generar todos los datasets de prueba
        generateTestDatasets();

        std::cout << "\n=== PRUEBA DE CARGA Y VERIFICACIÓN ===" << std::endl;

        // Probar cargar el dataset más pequeño para verificar
        std::vector<std::vector<std::vector<float>>> loaded_field;
        int nx, ny, nz;

        std::cout << "Probando carga del dataset más pequeño..." << std::endl;
        if (loadFieldBinary(loaded_field, "test_sphere_32.bin", nx, ny, nz))
        {
            printDatasetInfo(loaded_field, "Esfera Cargada (Verificación)");
            std::cout << "✓ Prueba de carga exitosa" << std::endl;
        }
        else
        {
            std::cout << "✗ Error en prueba de carga" << std::endl;
        }

        // Probar cargar un dataset mediano
        std::cout << "\nProbando carga del dataset mediano..." << std::endl;
        if (loadFieldBinary(loaded_field, "test_sphere_96.bin", nx, ny, nz))
        {
            printDatasetInfo(loaded_field, "Esfera 96³ (Verificación)");
            std::cout << "✓ Prueba de carga dataset mediano exitosa" << std::endl;
        }
        else
        {
            std::cout << "✗ Error en prueba de carga dataset mediano" << std::endl;
        }

    }
    catch (const std::exception &e)
    {
        std::cerr << "Error durante la ejecución: " << e.what() << std::endl;
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    std::cout << "\n=== GENERACIÓN COMPLETADA EXITOSAMENTE ===" << std::endl;
    std::cout << "Tiempo total de generación: " << duration.count() << " segundos" << std::endl;
    std::cout << "Los archivos están listos para usar en tus pruebas paralelas." << std::endl;

    std::cout << "\n=== GUÍA DE USO RÁPIDO ===" << std::endl;
    std::cout << "\nPara cargar en tu código:" << std::endl;
    std::cout << "```cpp" << std::endl;
    std::cout << "std::vector<std::vector<std::vector<float>>> field;" << std::endl;
    std::cout << "int nx, ny, nz;" << std::endl;
    std::cout << "loadFieldBinary(field, \"test_sphere_128.bin\", nx, ny, nz);" << std::endl;
    std::cout << "```" << std::endl;

    std::cout << "\nPara diferentes tipos de pruebas:" << std::endl;
    std::cout << "- Debug rápido: test_sphere_32.bin" << std::endl;
    std::cout << "- Tests básicos: test_sphere_64.bin, test_waves_64.bin" << std::endl;
    std::cout << "- Benchmarks medianos: test_sphere_128.bin, test_waves_128.bin" << std::endl;
    std::cout << "- Benchmarks pesados: test_sphere_256.bin, test_waves_256.bin" << std::endl;
    std::cout << "- Stress tests: test_sphere_512.bin, test_waves_512.bin" << std::endl;

    std::cout << "\nIso-values recomendados:" << std::endl;
    std::cout << "- Esferas: 0.0 (superficie de la esfera)" << std::endl;
    std::cout << "- Ondas: 5.0 (nivel intermedio de las ondas)" << std::endl;

    std::cout << "\nTamaños de archivos aproximados:" << std::endl;
    std::cout << "- 32³: ~0.1 MB" << std::endl;
    std::cout << "- 48³: ~0.4 MB" << std::endl;
    std::cout << "- 64³: ~1.0 MB" << std::endl;
    std::cout << "- 96³: ~3.4 MB" << std::endl;
    std::cout << "- 128³: ~8.0 MB" << std::endl;
    std::cout << "- 256³: ~64 MB" << std::endl;
    std::cout << "- 512³: ~512 MB" << std::endl;

    return 0;
}
