#include "generate_data.h"
#include <iostream>

int main()
{
    std::cout << "=== GENERADOR DE DATOS DE PRUEBA PARA MARCHING CUBES ===" << std::endl;
    std::cout << "Versión segura y optimizada" << std::endl;

    try
    {
        // Generar todos los datasets de prueba
        generateTestDatasets();

        std::cout << "\n=== PRUEBA DE CARGA ===" << std::endl;

        // Probar cargar el dataset más pequeño
        std::vector<std::vector<std::vector<float>>> loaded_field;
        int nx, ny, nz;

        if (loadFieldBinary(loaded_field, "test_sphere_32.bin", nx, ny, nz))
        {
            printDatasetInfo(loaded_field, "Esfera Cargada (Verificación)");
            std::cout << "✓ Prueba de carga exitosa" << std::endl;
        }
        else
        {
            std::cout << "✗ Error en prueba de carga" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error durante la ejecución: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== GENERACIÓN COMPLETADA EXITOSAMENTE ===" << std::endl;
    std::cout << "Los archivos están listos para usar en tus pruebas paralelas." << std::endl;
    std::cout << "\nPara usar en tu código:" << std::endl;
    std::cout << "- Iso-value recomendado: 0.0 (para esferas)" << std::endl;
    std::cout << "- Iso-value recomendado: 5.0 (para ondas)" << std::endl;

    return 0;
}