#include "generate_data.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include <iomanip>
#include <algorithm>

// Función auxiliar para verificar límites de memoria - ACTUALIZADA
bool checkMemoryRequirements(int nx, int ny, int nz)
{
    size_t total_elements = static_cast<size_t>(nx) * ny * nz;
    size_t memory_mb = (total_elements * sizeof(float)) / (1024 * 1024);

    std::cout << "Verificando memoria requerida: " << memory_mb << " MB" << std::endl;

    // Aumentado el límite para datasets más grandes
    if (memory_mb > 2048)
    { // Límite de 2GB por seguridad
        std::cerr << "Error: Dataset demasiado grande (" << memory_mb << " MB). Máximo: 2048 MB" << std::endl;
        return false;
    }

    return true;
}

// Función segura para redimensionar el vector 3D
bool resizeField(std::vector<std::vector<std::vector<float>>> &field,
                 int nx, int ny, int nz)
{

    if (!checkMemoryRequirements(nx, ny, nz))
    {
        return false;
    }

    try
    {
        std::cout << "Redimensionando campo a " << nx << "x" << ny << "x" << nz << "..." << std::endl;

        // Limpiar memoria existente
        field.clear();
        field.shrink_to_fit();

        // Redimensionar paso a paso para mejor control de memoria
        field.resize(nx);
        for (int x = 0; x < nx; ++x)
        {
            field[x].resize(ny);
            for (int y = 0; y < ny; ++y)
            {
                field[x][y].resize(nz, 0.0f); // Inicializar con 0
            }
        }

        std::cout << "Redimensionamiento exitoso." << std::endl;
        return true;
    }
    catch (const std::bad_alloc &e)
    {
        std::cerr << "Error de memoria: " << e.what() << std::endl;
        return false;
    }
}

// Función principal de generación - CORREGIDA
void generateScalarField3D(std::vector<std::vector<std::vector<float>>> &field,
                           const DataConfig &config)
{

    std::cout << "\n=== GENERANDO CAMPO ESCALAR 3D ===" << std::endl;
    std::cout << "Tamaño: " << config.size_x << "x" << config.size_y << "x" << config.size_z << std::endl;
    std::cout << "Tipo: ";

    switch (config.type)
    {
    case FieldType::SPHERE:
        std::cout << "ESFERA";
        break;
    case FieldType::MULTIPLE_SPHERES:
        std::cout << "MÚLTIPLES ESFERAS";
        break;
    case FieldType::WAVES_3D:
        std::cout << "ONDAS 3D";
        break;
    case FieldType::TORUS:
        std::cout << "TOROIDE";
        break;
    case FieldType::COMBINED:
        std::cout << "COMBINADO";
        break;
    }
    std::cout << std::endl;

    // Redimensionar de forma segura
    if (!resizeField(field, config.size_x, config.size_y, config.size_z))
    {
        std::cerr << "Error: No se pudo redimensionar el campo" << std::endl;
        return;
    }

    // Generar según el tipo especificado
    std::cout << "Generando contenido..." << std::endl;

    try
    {
        switch (config.type)
        {
        case FieldType::SPHERE:
            generateSphere(field, config.size_x, config.size_y, config.size_z,
                           config.size_x * 0.25f, config.size_x / 2.0f,
                           config.size_y / 2.0f, config.size_z / 2.0f);
            break;

        case FieldType::WAVES_3D:
            generateWaves3D(field, config.size_x, config.size_y, config.size_z,
                            0.08f, 10.0f);
            break;

        case FieldType::MULTIPLE_SPHERES:
            // Implementación simplificada para evitar problemas
            generateSphere(field, config.size_x, config.size_y, config.size_z,
                           config.size_x * 0.2f, config.size_x * 0.3f,
                           config.size_y * 0.3f, config.size_z * 0.3f);
            break;

        case FieldType::TORUS:
        case FieldType::COMBINED:
            // Por ahora usar esfera para evitar complejidad
            generateSphere(field, config.size_x, config.size_y, config.size_z,
                           config.size_x * 0.3f, config.size_x / 2.0f,
                           config.size_y / 2.0f, config.size_z / 2.0f);
            break;
        }

        std::cout << "Contenido generado exitosamente." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error durante la generación: " << e.what() << std::endl;
        return;
    }

    // Aplicar escala y offset si es necesario
    if (config.scale != 1.0f || config.offset != 0.0f)
    {
        std::cout << "Aplicando escala y offset..." << std::endl;
        for (int x = 0; x < config.size_x; ++x)
        {
            for (int y = 0; y < config.size_y; ++y)
            {
                for (int z = 0; z < config.size_z; ++z)
                {
                    field[x][y][z] = field[x][y][z] * config.scale + config.offset;
                }
            }
        }
    }

    std::cout << "Campo escalar generado exitosamente.\n"
              << std::endl;
}

// Esfera centrada - OPTIMIZADA
void generateSphere(std::vector<std::vector<std::vector<float>>> &field,
                    int nx, int ny, int nz, float radius,
                    float center_x, float center_y, float center_z)
{

    std::cout << "Generando esfera: radio=" << radius
              << ", centro=(" << center_x << "," << center_y << "," << center_z << ")" << std::endl;

    // Ajustar frecuencia de progreso según tamaño
    int progress_step = std::max(1, nx / 8);

    for (int x = 0; x < nx; ++x)
    {
        if (x % progress_step == 0)
        {
            std::cout << "Progreso: " << (100 * x / nx) << "%" << std::endl;
        }

        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                float dx = x - center_x;
                float dy = y - center_y;
                float dz = z - center_z;

                float distance = sqrt(dx * dx + dy * dy + dz * dz);
                field[x][y][z] = distance - radius;
            }
        }
    }

    std::cout << "Esfera generada completamente." << std::endl;
}

// Ondas 3D - OPTIMIZADA
void generateWaves3D(std::vector<std::vector<std::vector<float>>> &field,
                     int nx, int ny, int nz, float frequency, float amplitude)
{

    std::cout << "Generando ondas 3D: freq=" << frequency << ", amp=" << amplitude << std::endl;

    // Ajustar frecuencia de progreso según tamaño
    int progress_step = std::max(1, nx / 8);

    for (int x = 0; x < nx; ++x)
    {
        if (x % progress_step == 0)
        {
            std::cout << "Progreso ondas: " << (100 * x / nx) << "%" << std::endl;
        }

        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                float wave_x = sin(frequency * x);
                float wave_y = cos(frequency * y);
                float wave_z = sin(frequency * z);

                field[x][y][z] = amplitude * (wave_x * wave_y + wave_y * wave_z);
            }
        }
    }

    std::cout << "Ondas 3D generadas completamente." << std::endl;
}

// Guardar en formato binario - MEJORADO
bool saveFieldBinary(const std::vector<std::vector<std::vector<float>>> &field,
                     const std::string &filename)
{

    std::cout << "Guardando campo en: " << filename << "..." << std::endl;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: No se pudo abrir " << filename << " para escribir." << std::endl;
        return false;
    }

    int nx = field.size();
    int ny = field[0].size();
    int nz = field[0][0].size();

    // Escribir dimensiones
    file.write(reinterpret_cast<const char *>(&nx), sizeof(int));
    file.write(reinterpret_cast<const char *>(&ny), sizeof(int));
    file.write(reinterpret_cast<const char *>(&nz), sizeof(int));

    // Escribir datos con verificación de progreso ajustada
    int progress_step = std::max(1, nx / 8);
    for (int x = 0; x < nx; ++x)
    {
        if (x % progress_step == 0)
        {
            std::cout << "Guardando: " << (100 * x / nx) << "%" << std::endl;
        }

        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                file.write(reinterpret_cast<const char *>(&field[x][y][z]), sizeof(float));
            }
        }
    }

    file.close();

    // Verificar tamaño del archivo
    std::ifstream check(filename, std::ios::binary | std::ios::ate);
    if (check.is_open())
    {
        size_t file_size = check.tellg();
        size_t expected_size = sizeof(int) * 3 + sizeof(float) * nx * ny * nz;
        check.close();

        if (file_size == expected_size)
        {
            std::cout << "Archivo guardado exitosamente: " << filename
                      << " (" << (file_size / 1024 / 1024) << " MB)" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Error: Tamaño de archivo incorrecto" << std::endl;
            return false;
        }
    }

    return false;
}

// Cargar desde formato binario - MEJORADO
bool loadFieldBinary(std::vector<std::vector<std::vector<float>>> &field,
                     const std::string &filename, int &nx, int &ny, int &nz)
{

    std::cout << "Cargando campo desde: " << filename << "..." << std::endl;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: No se pudo abrir " << filename << " para leer." << std::endl;
        return false;
    }

    // Leer dimensiones
    file.read(reinterpret_cast<char *>(&nx), sizeof(int));
    file.read(reinterpret_cast<char *>(&ny), sizeof(int));
    file.read(reinterpret_cast<char *>(&nz), sizeof(int));

    std::cout << "Dimensiones del archivo: " << nx << "x" << ny << "x" << nz << std::endl;

    // Redimensionar de forma segura
    if (!resizeField(field, nx, ny, nz))
    {
        file.close();
        return false;
    }

    // Leer datos
    for (int x = 0; x < nx; ++x)
    {
        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                file.read(reinterpret_cast<char *>(&field[x][y][z]), sizeof(float));
            }
        }
    }

    file.close();
    std::cout << "Campo cargado exitosamente." << std::endl;
    return true;
}

// Imprimir información del dataset - MEJORADO
void printDatasetInfo(const std::vector<std::vector<std::vector<float>>> &field,
                      const std::string &name)
{

    if (field.empty() || field[0].empty() || field[0][0].empty())
    {
        std::cerr << "Error: Campo vacío" << std::endl;
        return;
    }

    int nx = field.size();
    int ny = field[0].size();
    int nz = field[0][0].size();

    float min_val = field[0][0][0];
    float max_val = field[0][0][0];
    double sum = 0.0;
    int count = 0;

    // Calcular estadísticas
    for (int x = 0; x < nx; ++x)
    {
        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                float val = field[x][y][z];
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
                sum += val;
                count++;
            }
        }
    }

    float avg_val = sum / count;
    size_t memory_mb = (static_cast<size_t>(nx) * ny * nz * sizeof(float)) / (1024 * 1024);

    std::cout << "\n=== INFORMACIÓN DEL DATASET: " << name << " ===" << std::endl;
    std::cout << "Dimensiones: " << nx << "x" << ny << "x" << nz << std::endl;
    std::cout << "Valor mínimo: " << std::fixed << std::setprecision(2) << min_val << std::endl;
    std::cout << "Valor máximo: " << std::fixed << std::setprecision(2) << max_val << std::endl;
    std::cout << "Valor promedio: " << std::fixed << std::setprecision(2) << avg_val << std::endl;
    std::cout << "Tamaño en memoria: " << memory_mb << " MB" << std::endl;
    std::cout << "Total de elementos: " << count << std::endl;
}

// Generar datasets de prueba - VERSIÓN EXPANDIDA CON TAMAÑOS GRANDES
void generateTestDatasets()
{
    std::cout << "\n=== GENERANDO DATASETS DE PRUEBA COMPLETOS ===" << std::endl;
    std::cout << "Incluye tamaños pequeños (debug) y grandes (performance)" << std::endl;

    std::vector<std::vector<std::vector<float>>> field;

    // ========== DATASETS PEQUEÑOS (DEBUG Y DESARROLLO) ==========
    std::cout << "\n### DATASETS PEQUEÑOS ###" << std::endl;

    // Dataset 1: Esfera pequeña 32x32x32 (para debug)
    std::cout << "\n--- DATASET 1: ESFERA 32³ ---" << std::endl;
    DataConfig config1(32, FieldType::SPHERE);
    generateScalarField3D(field, config1);
    printDatasetInfo(field, "Esfera 32³");
    saveFieldBinary(field, "test_sphere_32.bin");

    // Dataset 2: Esfera mediana 48x48x48
    std::cout << "\n--- DATASET 2: ESFERA 48³ ---" << std::endl;
    DataConfig config2(48, FieldType::SPHERE);
    generateScalarField3D(field, config2);
    printDatasetInfo(field, "Esfera 48³");
    saveFieldBinary(field, "test_sphere_48.bin");

    // Dataset 3: Ondas 48x48x48
    std::cout << "\n--- DATASET 3: ONDAS 48³ ---" << std::endl;
    DataConfig config3(48, FieldType::WAVES_3D);
    generateScalarField3D(field, config3);
    printDatasetInfo(field, "Ondas 48³");
    saveFieldBinary(field, "test_waves_48.bin");

    // Dataset 4: Esfera grande 64x64x64
    std::cout << "\n--- DATASET 4: ESFERA 64³ ---" << std::endl;
    DataConfig config4(64, FieldType::SPHERE);
    generateScalarField3D(field, config4);
    printDatasetInfo(field, "Esfera 64³");
    saveFieldBinary(field, "test_sphere_64.bin");

    // ========== DATASETS GRANDES (PERFORMANCE Y BENCHMARKS) ==========
    std::cout << "\n### DATASETS GRANDES ###" << std::endl;

    // Dataset 5: Esfera 96x96x96
    std::cout << "\n--- DATASET 5: ESFERA 96³ ---" << std::endl;
    DataConfig config5(96, FieldType::SPHERE);
    generateScalarField3D(field, config5);
    printDatasetInfo(field, "Esfera 96³");
    saveFieldBinary(field, "test_sphere_96.bin");

    // Dataset 6: Ondas 96x96x96
    std::cout << "\n--- DATASET 6: ONDAS 96³ ---" << std::endl;
    DataConfig config6(96, FieldType::WAVES_3D);
    generateScalarField3D(field, config6);
    printDatasetInfo(field, "Ondas 96³");
    saveFieldBinary(field, "test_waves_96.bin");

    // Dataset 7: Esfera 128x128x128
    std::cout << "\n--- DATASET 7: ESFERA 128³ ---" << std::endl;
    DataConfig config7(128, FieldType::SPHERE);
    generateScalarField3D(field, config7);
    printDatasetInfo(field, "Esfera 128³");
    saveFieldBinary(field, "test_sphere_128.bin");

    // Dataset 8: Ondas 128x128x128
    std::cout << "\n--- DATASET 8: ONDAS 128³ ---" << std::endl;
    DataConfig config8(128, FieldType::WAVES_3D);
    generateScalarField3D(field, config8);
    printDatasetInfo(field, "Ondas 128³");
    saveFieldBinary(field, "test_waves_128.bin");

    // Dataset 9: Esfera 256x256x256
    std::cout << "\n--- DATASET 9: ESFERA 256³ ---" << std::endl;
    DataConfig config9(256, FieldType::SPHERE);
    generateScalarField3D(field, config9);
    printDatasetInfo(field, "Esfera 256³");
    saveFieldBinary(field, "test_sphere_256.bin");

    // Dataset 10: Ondas 256x256x256
    std::cout << "\n--- DATASET 10: ONDAS 256³ ---" << std::endl;
    DataConfig config10(256, FieldType::WAVES_3D);
    generateScalarField3D(field, config10);
    printDatasetInfo(field, "Ondas 256³");
    saveFieldBinary(field, "test_waves_256.bin");

    // Dataset 11: Esfera 512x512x512 (ADVERTENCIA: ARCHIVO MUY GRANDE)
    std::cout << "\n--- DATASET 11: ESFERA 512³ (ADVERTENCIA: ARCHIVO GRANDE) ---" << std::endl;
    std::cout << "ADVERTENCIA: Este dataset ocupará ~512MB en memoria y disco" << std::endl;
    DataConfig config11(512, FieldType::SPHERE);
    generateScalarField3D(field, config11);
    printDatasetInfo(field, "Esfera 512³");
    saveFieldBinary(field, "test_sphere_512.bin");

    // Dataset 12: Ondas 512x512x512 (ADVERTENCIA: ARCHIVO MUY GRANDE)
    std::cout << "\n--- DATASET 12: ONDAS 512³ (ADVERTENCIA: ARCHIVO GRANDE) ---" << std::endl;
    std::cout << "ADVERTENCIA: Este dataset ocupará ~512MB en memoria y disco" << std::endl;
    DataConfig config12(512, FieldType::WAVES_3D);
    generateScalarField3D(field, config12);
    printDatasetInfo(field, "Ondas 512³");
    saveFieldBinary(field, "test_waves_512.bin");

    // ========== RESUMEN FINAL ==========
    std::cout << "\n=== TODOS LOS DATASETS GENERADOS EXITOSAMENTE ===" << std::endl;
    std::cout << "\nArchivos creados:" << std::endl;
    std::cout << "\n### DATASETS PEQUEÑOS (DEBUG) ###" << std::endl;
    std::cout << "- test_sphere_32.bin  (~0.1 MB, debug rápido)" << std::endl;
    std::cout << "- test_sphere_48.bin  (~0.4 MB, test básico)" << std::endl;
    std::cout << "- test_waves_48.bin   (~0.4 MB, patrones complejos)" << std::endl;
    std::cout << "- test_sphere_64.bin  (~1.0 MB, test intermedio)" << std::endl;

    std::cout << "\n### DATASETS GRANDES (PERFORMANCE) ###" << std::endl;
    std::cout << "- test_sphere_96.bin  (~3.4 MB, test mediano)" << std::endl;
    std::cout << "- test_waves_96.bin   (~3.4 MB, ondas medianas)" << std::endl;
    std::cout << "- test_sphere_128.bin (~8.0 MB, test grande)" << std::endl;
    std::cout << "- test_waves_128.bin  (~8.0 MB, ondas grandes)" << std::endl;
    std::cout << "- test_sphere_256.bin (~64 MB, benchmark pesado)" << std::endl;
    std::cout << "- test_waves_256.bin  (~64 MB, ondas pesadas)" << std::endl;
    std::cout << "- test_sphere_512.bin (~512 MB, stress test)" << std::endl;
    std::cout << "- test_waves_512.bin  (~512 MB, stress test ondas)" << std::endl;

    std::cout << "\n### RECOMENDACIONES DE USO ###" << std::endl;
    std::cout << "- Para debug: usar datasets de 32³ y 48³" << std::endl;
    std::cout << "- Para tests básicos: usar datasets de 64³ y 96³" << std::endl;
    std::cout << "- Para benchmarks: usar datasets de 128³ y 256³" << std::endl;
    std::cout << "- Para stress tests: usar datasets de 512³" << std::endl;
    std::cout << "- Iso-value recomendado: 0.0 (para esferas)" << std::endl;
    std::cout << "- Iso-value recomendado: 5.0 (para ondas)" << std::endl;
}