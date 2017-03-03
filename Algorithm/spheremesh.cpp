#include "spheremesh.h"
#include <fstream>
#include <sstream>
#include "Eigen/Dense"

using Eigen::Matrix4f;
using Eigen::Vector4f;

class Face;
class Vertex {
    public:
        Vector3d point;
        vector<Face*> faces;
        Vertex(Vector3d p, vector<Face*> faceList) : point(p), faces(faceList) {}
        Vertex(Vector3d p) : point(p) {}
};
class Face {
    public:
        Vector3d normal;
        Vertex* a;
        Vertex* b;
        Vertex* c;
        Face(Vector3d norm, Vertex* ap, Vertex* bp, Vertex *cp) : normal(norm), a(ap), b(bp), c(cp) {}
        Face() {}
        float getArea() {
            Vector3d one = Vector3d::sub(b->point, a->point);
            Vector3d two = Vector3d::sub(c->point, a->point);
            Vector3d cross = Vector3d::cross(one, two);
            return cross.magnitude() * 0.5;
        }
};
class SQEM {
    public:
        Matrix4f A;
        Vector4f b;
        float c;
        SQEM() {
            A = Matrix4f::Zero();
            b = Vector4f::Zero();
        }
        SQEM(Vertex* vertex) {
            A = Matrix4f::Zero();
            b = Vector4f::Zero();
            vector<Face*> faces = vertex->faces;
            for (int i = 0; i < faces.size(); i++) {
                Face* face = faces[i];
                float coeff = face->getArea() * 1/3;
                SQEM faceSqem(face);
                A += coeff * faceSqem.A;
                b += coeff * faceSqem.b;
                c += coeff * faceSqem.c;
            }
        }
        SQEM(Face* face) {
            Vector3d normal = face->normal;
            A << 2 * normal.x * normal.x, 2 * normal.x * normal.y, 2 * normal.x * normal.z, 2 * normal.x,
                 2 * normal.y * normal.x, 2 * normal.y * normal.y, 2 * normal.y * normal.z, 2 * normal.y,
                 2 * normal.z * normal.x, 2 * normal.z * normal.y, 2 * normal.z * normal.z, 2 * normal.z,
                 2 * normal.x,            2 * normal.y,            2 * normal.z,            2 * 1;
            float coeff = Vector3d::dot_num(normal, face->a->point);
            b << 2 * coeff * normal.x, 2 * coeff * normal.y, 2 * coeff * normal.z, 2 * coeff * 1;
            c = coeff * coeff;
        }
        static SQEM addSQEMs(SQEM one, SQEM two) {
            SQEM n;
            n.A = one.A + two.A;
            n.b = one.b + two.b;
            n.c = one.c + two.c;
            return n;
        }
        float computeError(Sphere sphere) {
            Vector4f s;
            s << sphere.center.x, sphere.center.y, sphere.center.z, sphere.radius;
            Vector4f lr = A * s;
            float left = 0.5 * s.dot(lr);
            float middle = b.dot(s);

            return left + middle + c;
        }
        Sphere getMinSphere() {
            Sphere sphere;
            return sphere;
        }
};
class SphereVertex {
    public:
        SQEM sqem;
        vector<SphereVertex*> neighbors;
        SphereVertex() {}
};

void split(string s, char delim, vector<string> *elems) {
    stringstream stream;
    stream.str(s);
    string str;
    while (getline(stream, str, delim)) {
        elems->push_back(str);
    }
}

void readObjFile(string filename, vector<Vertex*> *vertices, vector<Face*> *faces) {
    ifstream file(filename);
    string str;
    while (getline(file, str)) {
        vector<string> elems;
        split(str, ' ', &elems);
        if (elems.size() >= 4) {
            string command = elems[0];
            if (command == "v") {
                Vector3d point(stof(elems[1]), stof(elems[2]), stof(elems[3]));
                Vertex* vertex = new Vertex(point);
                vertices->push_back(vertex);
            } else if (command == "f") {
                if (elems.size() - 1 == 3) {
                    vector<Vertex*> faceVertices;
                    Face* face = new Face();
                    for (int i = 1; i <= 3; i++) {
                        vector<string> params;
                        split(elems[i], '/', &params);
                        int index = stoi(params[0]) - 1;
                        faceVertices.push_back(vertices->at(index));
                        vertices->at(index)->faces.push_back(face);
                    }
                    Vector3d vec1 = Vector3d::sub(faceVertices[1]->point, faceVertices[0]->point);
                    Vector3d vec2 = Vector3d::sub(faceVertices[2]->point, faceVertices[0]->point);
                    Vector3d normal = Vector3d::cross(vec1, vec2);
                    face->a = faceVertices[0];
                    face->b = faceVertices[1];
                    face->c = faceVertices[2];
                    face->normal = normal;
                    faces->push_back(face);
                } else {
                    printf("This sphere mesh implementation only supports triangular faces!");
                }
            }
        }
    }
}
void calcSqem(vector<Vertex*> *vertices, vector<Face*> *faces, vector<SphereVertex*> *spheres) {

}
//Read from file and store spheres and edges in passed arguments
void calcSphereMesh(string filename, int sphereCount, vector<Sphere> *outputSpheres, vector<SphereEdge> *outputEdges) {
    printf("outputing to vectors!\n");
    vector<Vertex*> vertices;
    vector<Face*> faces;
    readObjFile(filename, &vertices, &faces);
    vector<SphereVertex*> spheres;
    calcSqem(&vertices, &faces, &spheres);
}

//Read from file and store spheres and edges in output file
void calcSphereMesh(string filename, int sphereCount, string outputFile) {
    printf("writing to file!\n");
}