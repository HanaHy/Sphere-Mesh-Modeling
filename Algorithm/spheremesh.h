#ifndef SPHEREMESH_H
#define SPHEREMESH_H

#include <vector>
#include <string>
#include <math.h>

using namespace std;

class Vector3d {
    public: 
        float x;
        float y;
        float z;
        Vector3d(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}
        Vector3d() : x(0), y(0), z(0) {}
         static Vector3d mult(Vector3d vec1, Vector3d vec2) {
            Vector3d multed(vec1.x * vec2.x, vec1.y * vec2.y, vec1.z * vec2.z);
            return multed;
        }
        static Vector3d mult_num(Vector3d vec1, float num) {
            Vector3d multed(vec1.x * num, vec1.y * num, vec1.z * num);
            return multed;
        }
        static Vector3d dot(Vector3d vec1, Vector3d vec2) {
            float val = vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
            Vector3d dotted(val, val, val);
            return dotted;
        }
        static float dot_num(Vector3d vec1, Vector3d vec2) {
            float val = vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
            return val;
        }
        static Vector3d add(Vector3d vec1, Vector3d vec2) {
            Vector3d sum(vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z);
            return sum;
        }
        static Vector3d sub(Vector3d vec1, Vector3d vec2) {
            Vector3d diff(vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z);
            return diff;
        }
        static Vector3d cross(Vector3d vec1, Vector3d vec2) {
            Vector3d crossed(vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z, vec1.x * vec2.y - vec1.y * vec2.x);
            return crossed;
        }
        float magnitude() {
            return sqrt(x * x + y * y + z * z);
        }
        void normalize() {
            float len = sqrt(x * x + y * y + z * z);
            x = x / len;
            y = y / len;
            z = z / len;
        }
        string toString() {
            return to_string(x) + " " + to_string(y) + " " + to_string(z);
        }
};
class Sphere {
    public:
        Vector3d center;
        float radius;
        Sphere(Vector3d cen, float r) : center(cen), radius(r) {}
        Sphere() {}
};
class SphereEdge {
    public:
        int start;
        int end;
        SphereEdge(int start, int end) {}
};

//Read from file and store spheres and edges in passed arguments
void calcSphereMesh(string filename, int sphereCount, vector<Sphere> *outputSpheres, vector<SphereEdge> *outputEdges);

//Read from file and store spheres and edges in output file
void calcSphereMesh(string filename, int sphereCount, string outputFile);

#endif