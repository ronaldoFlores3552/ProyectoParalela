#ifndef MARCHING_CUBES_SERIAL_H
#define MARCHING_CUBES_SERIAL_H

#include <vector>
#include <array>

// Estructura para representar un vértice 3D
struct Vertex
{
    float x, y, z;

    Vertex() : x(0), y(0), z(0) {}
    Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vertex operator-(const Vertex &other) const
    {
        return Vertex(x - other.x, y - other.y, z - other.z);
    }
    // Operador suma para interpolar vértices
    Vertex operator+(const Vertex &other) const
    {
        return Vertex(x + other.x, y + other.y, z + other.z);
    }

    // Operador multiplicación escalar
    Vertex operator*(float scalar) const
    {
        return Vertex(x * scalar, y * scalar, z * scalar);
    }
};

// Estructura para representar un triángulo
struct Triangle
{
    Vertex v0, v1, v2;

    Triangle() {}
    Triangle(const Vertex &a, const Vertex &b, const Vertex &c)
        : v0(a), v1(b), v2(c) {}
};

// Clase principal para el algoritmo Marching Cubes
class MarchingCubesSerial
{
private:
    // Tablas de lookup para el algoritmo
    static const int edgeTable[256];
    static const int triTable[256][16];

    // Datos del volumen
    float *scalarField;
    int sizeX, sizeY, sizeZ;
    float isoValue;

    // Vértices de un cubo
    static const int vertexOffsets[8][3];

    // Aristas del cubo
    static const int edgeVertices[12][2];

    // Interpola entre dos vértices basándose en el isovalor
    Vertex interpolateVertex(const Vertex &v1, float val1,
                             const Vertex &v2, float val2) const;

    // Obtiene el valor escalar en una posición del grid
    float getScalarValue(int x, int y, int z) const;

    // Procesa un cubo individual
    void processCube(int x, int y, int z, std::vector<Triangle> &triangles);

public:
    // Constructor
    MarchingCubesSerial();

    // Destructor
    ~MarchingCubesSerial();

    // Configura los datos del volumen
    void setScalarField(float *data, int sx, int sy, int sz);

    // Establece el isovalor
    void setIsoValue(float value) { isoValue = value; }

    // Ejecuta el algoritmo y devuelve los triángulos generados
    std::vector<Triangle> generateIsosurface();

    // Versión que devuelve el número de triángulos generados
    int generateIsosurface(std::vector<Triangle> &triangles);
};

#endif // MARCHING_CUBES_SERIAL_H