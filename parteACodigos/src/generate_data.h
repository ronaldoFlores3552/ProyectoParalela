#ifndef GENERATE_DATA_H
#define GENERATE_DATA_H

#include <vector>
#include <string>

enum class FieldType
{
    SPHERE,
    MULTIPLE_SPHERES,
    WAVES_3D,
    TORUS,
    COMBINED
};

struct DataConfig
{
    int size_x, size_y, size_z;
    FieldType type;
    float scale;
    float offset;
    int seed;

    DataConfig(int size = 64, FieldType field_type = FieldType::SPHERE)
        : size_x(size), size_y(size), size_z(size), type(field_type),
          scale(1.0f), offset(0.0f), seed(42) {}
};

// Funciones principales
void generateScalarField3D(std::vector<std::vector<std::vector<float>>> &field,
                           const DataConfig &config);

void generateTestDatasets();

// Funciones específicas
void generateSphere(std::vector<std::vector<std::vector<float>>> &field,
                    int nx, int ny, int nz, float radius,
                    float center_x, float center_y, float center_z);

void generateWaves3D(std::vector<std::vector<std::vector<float>>> &field,
                     int nx, int ny, int nz, float frequency, float amplitude);

// Utilidades
bool saveFieldBinary(const std::vector<std::vector<std::vector<float>>> &field,
                     const std::string &filename);

bool loadFieldBinary(std::vector<std::vector<std::vector<float>>> &field,
                     const std::string &filename, int &nx, int &ny, int &nz);

void printDatasetInfo(const std::vector<std::vector<std::vector<float>>> &field,
                      const std::string &name);

#endif
