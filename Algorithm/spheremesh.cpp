#include "spheremesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "Eigen/Dense"

#define epsilon 0.0001

using Eigen::Matrix4f;
using Eigen::Vector4f;
using Eigen::Matrix3f;
using Eigen::Vector3f;
using Eigen::Matrix2f;
using Eigen::Vector2f;

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
        float area;
        Face(Vector3d norm, Vertex* ap, Vertex* bp, Vertex *cp) : normal(norm), a(ap), b(bp), c(cp) {
            area = normal.magnitude() * 0.5;
            normal.normalize();
        }
        Face() {}
};
class SQEM {
    public:
        Matrix4f A;
        Vector4f b;
        float c;
        SQEM() {
            A = Matrix4f::Zero();
            b = Vector4f::Zero();
            c = 0;
        }
        SQEM(Vertex* vertex) {
            A = Matrix4f::Zero();
            b = Vector4f::Zero();
            c = 0;
            vector<Face*> faces = vertex->faces;
            for (int i = 0; i < faces.size(); i++) {
                Face* face = faces[i];
                float coeff = face->area * 1/3;
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
            return left - middle + c;
        }
        float computeError2d(float lambda, float radius, Matrix2f ahat, Vector2f bhat, float chat) {
            Vector2f s;
            s << lambda, radius;
            Vector2f lr = ahat * s;
            float left = 0.5 * s.dot(lr);
            float middle = bhat.dot(s);

            return left - middle + chat;
        }
        //For start state, when we don't have u and v (getting min sphere without edge contraction)
        float getMinSphere(Vector3d point, float maxRadius, Sphere* sphere) {
            return getMinSphere(point, point, maxRadius, sphere);
        }
        //Get min sphere for edge contraction
        float getMinSphere(Vector3d u, Vector3d v, float maxRadius, Sphere* sphere) {
            if (A.determinant() > epsilon) {
                //Need to optimize inverses!!!
                Matrix4f sqemInverse = A.inverse();
                Vector4f minimum = sqemInverse * b;
                float radius = minimum(3);
                if (radius >= 0 && radius <= maxRadius) {
                    Vector3d center(minimum(0), minimum(1), minimum(2));
                    sphere->center = center;
                    sphere->radius = radius;
                    return computeError(*sphere);
                } else {
                    //Check this math please - solving for fixed radii
                    Matrix3f qem;
                    qem << A(0, 0), A(0, 1), A(0, 2),
                           A(1, 0), A(1, 1), A(1, 2),
                           A(2, 0), A(2, 1), A(2, 2);
                    if (qem.determinant() > epsilon) {
                        Matrix3f qemInverse = qem.inverse();
                        //radius = 0
                        Vector3f qemB;
                        qemB << b(0), b(1), b(2);
                        Vector3f qemNullCenter = qemInverse * qemB;
                        Sphere nullSphere(Vector3d(qemNullCenter(0), qemNullCenter(1), qemNullCenter(2)), 0);

                        //radius = maxRadius
                        Vector3f bias;
                        bias << A(0, 3), A(1, 3), A(2, 3);
                        Vector3f qemMaxCenter = qemInverse * (qemB - 2 * maxRadius * bias);
                        Sphere maxSphere(Vector3d(qemMaxCenter(0), qemMaxCenter(1), qemMaxCenter(2)), maxRadius);

                        float errorNull = computeError(nullSphere);
                        float errorMax = computeError(maxSphere);
                        if (errorNull <= errorMax) {
                            sphere->center = nullSphere.center;
                            sphere->radius = nullSphere.radius;
                            return errorNull;
                        } else {
                            sphere->center = maxSphere.center;
                            sphere->radius = maxSphere.radius;
                            return errorMax;
                        }
                    }
                }
            }
            //A not invertible
            //Look for point on [uv]
            Vector3d uv = Vector3d::sub(v, u);
            float atl = (A(0, 0) * uv.x + A(0, 1) * uv.y + A(0, 2) * uv.z) * uv.x + 
                        (A(1, 0) * uv.x + A(1, 1) * uv.y + A(1, 2) * uv.z) * uv.y + 
                        (A(2, 0) * uv.x + A(2, 1) * uv.y + A(2, 2) * uv.z) * uv.z;
            float atr = A(3, 0) * uv.x + A(3, 1) * uv.y + A(3, 2) * uv.z;
            float abl = atr;
            float abr = A(3, 3);
            float bt = (b(0) * uv.x + b(1) * uv.y + b(2) * uv.z) - (
                        (A(0, 0) * uv.x + A(0, 1) * uv.y + A(0, 2) * uv.z) * u.x + 
                        (A(1, 0) * uv.x + A(1, 1) * uv.y + A(1, 2) * uv.z) * u.y + 
                        (A(2, 0) * uv.x + A(2, 1) * uv.y + A(2, 2) * uv.z) * u.z);
            float bb = b(3) - (A(3, 0) * u.x + A(3, 1) * u.y + A(3, 2) * u.z);
            float chat = c - (b(0) * u.x + b(1) * u.y + b(2) * u.z) + 0.5 * (
                    (A(0, 0) * u.x + A(0, 1) * u.y + A(0, 2) * u.z) * u.x + 
                    (A(1, 0) * u.x + A(1, 1) * u.y + A(1, 2) * u.z) * u.y + 
                    (A(2, 0) * u.x + A(2, 1) * u.y + A(2, 2) * u.z) * u.z);
            Matrix2f ahat;
            ahat << atl, atr,
                    abl, abr;
            Vector2f bhat;
            bhat << bt, bb;
            float ahatDeterminant = atl * abr - atr * abl;
            if (ahatDeterminant > epsilon) {
                Matrix2f ahatinv = ahat.inverse();
                Vector2f result = ahatinv * bhat;
                //If minimizer is outside of allowed range, test boundaries.
                if (result(0) < 0 || result(0) > 1 || result(1) < 0 || result(1) > maxRadius) {
                    //lambda = 0
                    float lambda = 0;
                    float radius = (bb - lambda * abl)/abr;
                    if (radius < 0) {
                        radius = 0;
                    }
                    if (radius > maxRadius) {
                        radius = maxRadius;
                    }
                    Vector3d lambdauv = Vector3d::mult_num(uv, lambda);
                    Vector3d ncenter = Vector3d::add(u, lambdauv);
                    Sphere nullLambdaSphere(ncenter, radius);
                    float nullLambdaCost = computeError2d(lambda, radius, ahat, bhat, chat);

                    //lambda = 1;
                    lambda = 1;
                    radius = (bb - lambda * abl)/abr;
                    if (radius < 0) {
                        radius = 0;
                    }
                    if (radius > maxRadius) {
                        radius = maxRadius;
                    }
                    lambdauv = Vector3d::mult_num(uv, lambda);
                    ncenter = Vector3d::add(u, lambdauv);
                    Sphere oneLambdaSphere(ncenter, radius);
                    float oneLambdaCost = computeError2d(lambda, radius, ahat, bhat, chat);

                    //radius = 0;
                    radius = 0;
                    lambda = (bt - radius * abl)/atl;
                    if (lambda < 0) {
                        lambda = 0;
                    }
                    if (lambda > 1) {
                        lambda = 1;
                    }
                    lambdauv = Vector3d::mult_num(uv, lambda);
                    ncenter = Vector3d::add(u, lambdauv);
                    Sphere nullRadiusSphere(ncenter, radius);
                    float nullRadiusCost = computeError2d(lambda, radius, ahat, bhat, chat);
                    
                    //radius = maxRadius;
                    radius = maxRadius;
                    lambda = (bt - radius * abl)/atl;
                    if (lambda < 0) {
                        lambda = 0;
                    }
                    if (lambda > 1) {
                        lambda = 1;
                    }
                    lambdauv = Vector3d::mult_num(uv, lambda);
                    ncenter = Vector3d::add(u, lambdauv);
                    Sphere maxRadiusSphere(ncenter, radius);
                    float maxRadiusCost = computeError2d(lambda, radius, ahat, bhat, chat);
                    //hopefully I didn't type these wrong...
                    if (maxRadiusCost <= nullRadiusCost && maxRadiusCost <= oneLambdaCost && maxRadiusCost <= nullLambdaCost) {
                        sphere->center = maxRadiusSphere.center;
                        sphere->radius = maxRadiusSphere.radius;
                        return maxRadiusCost;
                    }
                    if (nullRadiusCost <= maxRadiusCost && nullRadiusCost <= oneLambdaCost && nullRadiusCost <= nullLambdaCost) {
                        sphere->center = nullRadiusSphere.center;
                        sphere->radius = nullRadiusSphere.radius;
                        return nullRadiusCost;
                    }
                    if (oneLambdaCost <= nullRadiusCost && oneLambdaCost <= maxRadiusCost && oneLambdaCost <= nullLambdaCost) {
                        sphere->center = oneLambdaSphere.center;
                        sphere->radius = oneLambdaSphere.radius;
                        return oneLambdaCost;
                    }
                    if (nullLambdaCost <= nullRadiusCost && nullLambdaCost <= maxRadiusCost && nullLambdaCost <= oneLambdaCost) {
                        sphere->center = nullLambdaSphere.center;
                        sphere->radius = nullLambdaSphere.radius;
                        return nullLambdaCost;
                    }
                } else {
                    float lambda = result(0);
                    float radius = result(1);
                    Vector3d lambdauv = Vector3d::mult_num(uv, lambda);
                    Vector3d ncenter = Vector3d::add(u, lambdauv);
                    sphere->center = ncenter;
                    sphere->radius = radius;
                    return computeError2d(lambda, radius, ahat, bhat, chat);
                }
            }
            //If above not invertible, solve for fixed point on midpoint, variant radius
            float lambda = 0.5;
            float radius = (bb - lambda * abl)/abr;
            if (radius < 0) {
                radius = 0;
            }
            if (radius > maxRadius) {
                radius = maxRadius;
            }
            Vector3d lambdauv = Vector3d::mult_num(uv, lambda);
            Vector3d ncenter = Vector3d::add(u, lambdauv);
            float midpointSphereCost = computeError2d(lambda, radius, ahat, bhat, chat);
            sphere->center = ncenter;
            sphere->radius = radius;
            return midpointSphereCost;
        }
};
class SphereVertex {
    public:
        SQEM sqem;
        Vector3d u;
        Vector3d v;
        Sphere sphere;
        vector<SphereVertex*> neighbors;
        SphereVertex(Vertex* vertex) {
            sqem = SQEM(vertex);
            sqem.getMinSphere(vertex->point, 1000, &sphere);
        }
        SphereVertex(SQEM sq, Sphere sph) : sqem(sq), sphere(sph) {
        }
        void addNeighbor(SphereVertex* neighbor) {
            if (find(neighbors.begin(), neighbors.end(), neighbor) == neighbors.end()) {
                neighbors.push_back(neighbor);
            }
        }
        void removeNeighbor(SphereVertex* neighbor) {
            neighbors.erase(remove(neighbors.begin(), neighbors.end(), neighbor), neighbors.end());
        }
};
class SVEdge {
    public:
        SphereVertex* one;
        SphereVertex* two;
        float cost;
        SQEM sqem;
        Sphere minSphere;
        SVEdge(SphereVertex* o, SphereVertex* t) : one(o), two(t) {
            sqem = SQEM::addSQEMs(o->sqem, t->sqem);
            cost = sqem.getMinSphere(o->sphere.center, t->sphere.center, 1000, &minSphere);
        }
};

void split(string s, char delim, vector<string> *elems) {
    stringstream stream;
    stream.str(s);
    string str;
    while (getline(stream, str, delim)) {
        if (!str.empty()) {
            elems->push_back(str);
        }
    }
}

void readObjFile(string filename, vector<Vertex*> *vertices, vector<Face*> *faces) {
    ifstream file(filename);
    string str;
    while (getline(file, str)) {
        vector<string> elems;
        split(str, ' ', &elems);
        if (elems.size() == 4) {
            string command = elems[0];
            if (command == "v") {
                Vector3d point(stof(elems[1]), stof(elems[2]), stof(elems[3]));
                Vertex* vertex = new Vertex(point);
                vertices->push_back(vertex);
            } else if (command == "f") {
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
                face->area = normal.magnitude() * 0.5;
                face->normal = normal;
                face->normal.normalize();
                faces->push_back(face);
            }
        }
    }
}
bool equalsEdge(SVEdge* one, SVEdge* two) {
    if (one->one == two->one && one->two == two->two) {
        return true;
    }
    if (one->one == two->two && one->two == two->one) {
        return true;
    }
    return false;
}
bool equalsEdge(SVEdge* one, SphereVertex* a, SphereVertex* b) {
    if (one->one == a && one->two == b) {
        return true;
    }
    if (one->one == b && one->two == a) {
        return true;
    }
    return false;
}
bool contains(SVEdge* find, vector<SVEdge*> *edges) {
    for (int i = 0; i < edges->size(); i++) {
        if (equalsEdge(find, edges->at(i))) {
            return true;
        }
    }
    return false;
}
class EdgeComparator {
    public:
        bool operator() (SVEdge* one, SVEdge* two) {
            return one->cost>two->cost;
        }
};

class EdgePriorityQueue : public priority_queue<SVEdge*, vector<SVEdge*>, EdgeComparator> {
    public: 
        bool remove(SVEdge* edgeR) {
            vector<SVEdge*> copyC(this->c);
            for (int i = 0; i < copyC.size(); i++) {
                SVEdge* edge = copyC[i];
                if (equalsEdge(edge, edgeR)) {
                    this->c.erase(this->c.begin() + i);
                    return true;
                }
            }
            return false;
        }
        bool remove(SphereVertex* one, SphereVertex* two) {
            vector<SVEdge*> copyC(this->c);
            for (int i = 0; i < copyC.size(); i++) {
                SVEdge* edge = copyC[i];
                if (equalsEdge(edge, one, two)) {
                    this->c.erase(this->c.begin() + i);
                    return true;
                }
            }
            return false;
        }
        bool contains(SVEdge* edgeR) {
            vector<SVEdge*> copyC(this->c);
            for (int i = 0; i < copyC.size(); i++) {
                SVEdge* edge = copyC[i];
                if (equalsEdge(edge, edgeR)) {
                    return true;
                }
            }
            return false;
        }
        void refresh() {
            make_heap(this->c.begin(), this->c.end(), this->comp);
        }
        void print() {
            for (int i = 0; i < this->c.size(); i++) {
                SVEdge* edge = this->c[i];
                cout << edge->one->sphere.center.toString() << " to " << edge->two->sphere.center.toString() << " " <<edge->cost<< "\n";
            }
        }
};
void calcSqem(int sphereCount, vector<Vertex*> vertices, vector<Face*> faces, vector<SphereVertex*> *outputSpheres, vector<SVEdge*> *outputEdges) {
    unordered_map<Vertex*, SphereVertex*> vertexSphereMap;
    vector<SphereVertex*> sphereVertices;
    //Map with vertex pointer as key for building edges from vertices
    for (int i = 0; i < vertices.size(); i++) {
        Vertex* vertex = vertices[i];
        SphereVertex* sv = new SphereVertex(vertex);
        vertexSphereMap[vertex] = sv;
        sphereVertices.push_back(sv);
    }
    printf("Done loading into spherevertices!\n");
    //use set with custom hash function for fast checking of duplicate edges
    vector<SVEdge*> sphereEdges;
    for (int i = 0; i < faces.size(); i++) {
        Face* face = faces[i];
        //Go through face, add edges to set
        SphereVertex* a = vertexSphereMap[face->a];
        SphereVertex* b = vertexSphereMap[face->b];
        SphereVertex* c = vertexSphereMap[face->c];

        a->addNeighbor(b);
        a->addNeighbor(c);
        b->addNeighbor(a);
        b->addNeighbor(c);
        c->addNeighbor(a);
        c->addNeighbor(b);
        SVEdge* ab = new SVEdge(a, b);
        SVEdge* ac = new SVEdge(a, c);
        SVEdge* bc = new SVEdge(b, c);
        if (!contains(ab, &sphereEdges)) {
            sphereEdges.push_back(ab);
        } else {
            delete ab;
        }
        if (!contains(ac, &sphereEdges)) {
            sphereEdges.push_back(ac);
        } else {
            delete ac;
        }
        if (!contains(bc, &sphereEdges)) {
            sphereEdges.push_back(bc);
        } else {
            delete bc;
        }
    }
    printf("Done loading edges!\n");
    //begin partitioning
    EdgePriorityQueue edges;
    for (SVEdge* edge: sphereEdges) {
        edges.push(edge);
    }
    while (!edges.empty() && sphereVertices.size() > sphereCount) {
        //printf("Start size: %d\n", edges.size());
        SVEdge* edge = edges.top();
        edges.pop();
        //cout << edge->one->sphere.center.toString() << " to " << edge->two->sphere.center.toString() << "\n";
        //cout << "Sphere: " << edge->minSphere.center.toString() << " " << edge->minSphere.radius << "\n";
        //cout << "Cost: " << edge->cost << "\n";
        SphereVertex* newVertex = new SphereVertex(edge->sqem, edge->minSphere);
        newVertex->u = edge->one->sphere.center;
        newVertex->v = edge->two->sphere.center;
        //remove old edges
        sphereVertices.erase(std::remove(sphereVertices.begin(), sphereVertices.end(), edge->one), sphereVertices.end());
        sphereVertices.erase(std::remove(sphereVertices.begin(), sphereVertices.end(), edge->two), sphereVertices.end());
        int countRemoves = 0;
        for (SphereVertex* neighbor: edge->one->neighbors) {
            if (neighbor != edge->two) {
                //cout << "Neighbor: " << neighbor->sphere.center.toString() << "\n";
                bool b = edges.remove(edge->one, neighbor);
                if (b) {
                    countRemoves++;
                } else {
                    //printf("Didn't remove:\n");
                    //cout << edge->one->sphere.center.toString() << " to " << neighbor->sphere.center.toString() << "\n";
                }
                neighbor->removeNeighbor(edge->one);
            }
        }
        for (SphereVertex* neighbor: edge->two->neighbors) {
            if (neighbor != edge->one) {
                //cout << "Neighbor: " << neighbor->sphere.center.toString() << "\n";
                bool b = edges.remove(edge->two, neighbor);
                if (b) {
                    countRemoves++;
                } else {
                    //printf("Didn't remove:\n");
                    //cout << edge->two->sphere.center.toString() << " to " << neighbor->sphere.center.toString() << "\n";
                }
                neighbor->removeNeighbor(edge->two);
            }
        }
        //printf("%d out of %d\n", countRemoves, edge->one->neighbors.size() + edge->two->neighbors.size());
        edges.refresh();
        //printf("After remove: %d\n", edges.size());
        //add new edges
        for (SphereVertex* neighbor: edge->one->neighbors) {
            if (neighbor != edge->two) {
                SVEdge* nedge = new SVEdge(newVertex, neighbor);
                if (!edges.contains(nedge)) {   
                    edges.push(nedge);
                }
                neighbor->addNeighbor(newVertex);
                newVertex->addNeighbor(neighbor);
            }
        }
        for (SphereVertex* neighbor: edge->two->neighbors) {
            if (neighbor != edge->one) {
                SVEdge* nedge = new SVEdge(newVertex, neighbor);
                if (!edges.contains(nedge)) {   
                    edges.push(nedge);
                }
                neighbor->addNeighbor(newVertex);
                newVertex->addNeighbor(neighbor);
            }
        }
        //printf("After add: %d\n", edges.size());
        //printf("Current edges:\n");
        //edges.print();
        sphereVertices.push_back(newVertex);
        //printf("Spheres remaining: %d\n", sphereVertices.size());
        //cleanup
        delete edge->one;
        delete edge->two;
        delete edge;
    }
    for (SphereVertex* vertex: sphereVertices) {
        outputSpheres->push_back(vertex);
    }
    while(!edges.empty()) {
        outputEdges->push_back(edges.top());
        edges.pop();
    }
}
//Read from file and store spheres and edges in passed arguments
void calcSphereMesh(string filename, int sphereCount, vector<Sphere> *outputSpheres, vector<SphereEdge> *outputEdges) {
    printf("outputing to vectors!\n");
    vector<Vertex*> vertices;
    vector<Face*> faces;
    readObjFile(filename, &vertices, &faces);
    vector<SphereVertex*> spheres;
    vector<SVEdge*> edges;
    calcSqem(sphereCount, vertices, faces, &spheres, &edges);

    for (SphereVertex* vertex: spheres) {
        outputSpheres->push_back(vertex->sphere);
    }
    for (SVEdge* edge: edges) {
        int start = distance(spheres.begin(), find(spheres.begin(), spheres.end(), edge->one));
        int end = distance(spheres.begin(), find(spheres.begin(), spheres.end(), edge->two));
        SphereEdge outputEdge(start, end);
        outputEdges->push_back(outputEdge);
    }
}

//Read from file and store spheres and edges in output file
void calcSphereMesh(string filename, int sphereCount, string outputFile) {
    vector<Sphere> spheres;
    vector<SphereEdge> edges;
    calcSphereMesh(filename, sphereCount, &spheres, &edges);
    ofstream output;
    output.open(outputFile);
    for (Sphere sphere: spheres) {
        output << "sphere " << sphere.center.toString() << " " << sphere.radius << "\n";
    }
    for (SphereEdge edge: edges) {
        output << "edge " << edge.start << " " << edge.end << "\n";
    }
    output.close();
}


float calcTotalCost(vector<Sphere*> *spheres, vector<SphereVertex*> *sphereVertices) {
    float totalCost = 0;
    for (int i = 0; i < sphereVertices->size(); i++) {
        float minCost = sphereVertices->at(i)->sqem.computeError(*(spheres->at(0)));
        for (int j = 1; j < spheres->size(); j++) {
            float cost = sphereVertices->at(i)->sqem.computeError(*(spheres->at(j)));
            if (cost < minCost) {
                minCost = cost;
            }
        }
        totalCost += minCost;
        printf("%f\n", minCost);
    }
    return totalCost;
}
//Read from file and store spheres and edges in output file
void calcSphereMeshGradient(string filename, int sphereCount, string outputFile, float threshold) {
    vector<Vertex*> vertices;
    vector<Face*> faces;
    readObjFile(filename, &vertices, &faces);
    vector<Sphere*> spheres;
    vector<SphereVertex*> sphereVertices;
    //Map with vertex pointer as key for building edges from vertices
    float minX = 10000;
    float maxX = -10000;
    float minY = 10000;
    float maxY = -10000;
    float minZ = 10000;
    float maxZ = -10000;
    for (int i = 0; i < vertices.size(); i++) {
        Vertex* vertex = vertices[i];
        SphereVertex* sv = new SphereVertex(vertex);
        sphereVertices.push_back(sv);
        if (vertex->point.x < minX) {
            minX = vertex->point.x;
        }
        if (vertex->point.x > maxX) {
            maxX = vertex->point.x;
        }
        if (vertex->point.y < minY) {
            minY = vertex->point.y;
        }
        if (vertex->point.y > maxY) {
            maxY = vertex->point.y;
        }
        if (vertex->point.z < minZ) {
            minZ = vertex->point.z;
        }
        if (vertex->point.z > maxZ) {
            maxZ = vertex->point.z;
        }
    }
    srand((unsigned)time(NULL));
    for (int i = 0; i < sphereCount; i++) {
        float x = ((double) rand() / (RAND_MAX));
        float y = ((double) rand() / (RAND_MAX));
        float z = ((double) rand() / (RAND_MAX));
        float radius = 5;
        Sphere* sphere = new Sphere(Vector3d(x, y, z), radius);
        spheres.push_back(sphere);
    }
    float totalCost = calcTotalCost(&spheres, &sphereVertices);
    printf("%f\n", totalCost);
    while (totalCost > threshold) {
        unordered_map<Sphere*, SQEM> sphereSQMap;
        for (SphereVertex* vertex: sphereVertices) {
            float minCost = vertex->sqem.computeError(*(spheres.at(0)));
            Sphere* minSphere = spheres.at(0);
            for (int j = 1; j < spheres.size(); j++) {
                float cost = vertex->sqem.computeError(*(spheres.at(j)));
                if (cost < minCost) {
                    minCost = cost;
                    minSphere = spheres.at(j);
                }
            }
            if (!sphereSQMap.count(minSphere)) {
                sphereSQMap[minSphere] = vertex->sqem;
            } else {
                sphereSQMap[minSphere] = SQEM::addSQEMs(vertex->sqem, sphereSQMap[minSphere]);
            }
        }
    }
}