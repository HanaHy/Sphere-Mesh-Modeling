//
//  main.cpp
//  Project 1 - SQEM
//
//  Created by Hana Hyder, Dorian Chan on 5/23/16.
//  Copyright © 2016 UCB. All rights reserved.
//

#define PI 3.14159265

#include <stdlib.h>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <iostream>
#include <math.h>
#include <time.h>
#include <vector>
#include <sstream>
#include <fstream>
#include "spheremesh.h"
#include "Eigen/Dense"

using namespace std;
string inputMesh, outputMesh;
int sphereCount;

bool sphereMode = false, interpMode = false, meshMode = true, holemode = false, gridMode = true;
int once = 0, stptOnce = 0; 
int button, state, dragX, dragY, moveType = 0; 
double zoomX = 0.2;
int width= 700, height= 500, isSpinning, tX = 0, tY = 0, tZ = 0, ttX = 0, ttY = 0, ttZ = 0, just_up = 0, 
tW = 0, ttW = 0; 
double winRadius= 660, transfac = .003, dX, dY, tt; 
int iterations = 0, delay = 4000; 
clock_t ticks1, ticks2, ticks3, ticks4; 


struct Point {
  float x; 
  float y; 
  float z; 
  
}; // Make struct instead for point manip

struct Face
{
  GLushort Pta;
  GLushort Ptb;
  GLushort Ptc;
  
  GLushort Na;
  GLushort Nb;
  GLushort Nc;
  
  GLushort Txa;
  GLushort Txb;
  GLushort Txc;
};

struct SphereDisp
{
  float x;
  float y;
  float z;
  float radius;
  
};

struct Edge
{
  int a;
  int b;
};

enum {
  SETPTS, PLOT
}viewpoint= PLOT;


/* CALCULATIONS */


/* DISPLAY */
void luma();
void sphereMesh();
void setGround(); 
void showMenu();
void loader();

void reshape(int width1, int height1)
{
  if (height1 == 0) {
    height1 = 1;
  }
  
  glViewport(0, 0, width1, height1);
  
  glMatrixMode(GL_PROJECTION); 
  glLoadIdentity(); 
  
  gluPerspective(45, (float)width1/height1, 0.1, 300);
  
  glMatrixMode(GL_MODELVIEW); 
  glLoadIdentity();
  
}

void doTranslation(int tX, int tY, int tZ) {
  float m[16]; 
  glMatrixMode(GL_MODELVIEW); 
  glGetFloatv(GL_MODELVIEW_MATRIX, m); 
  glLoadIdentity(); 
  glTranslatef(transfac*tX, transfac*tY, transfac*tZ); 
  glMultMatrixf(m); 
}


void myRotated(double a, double X, double Y, double Z) {

  float m[16];
  glMatrixMode(GL_MODELVIEW); 
  glGetFloatv(GL_MODELVIEW_MATRIX, m); 
  glLoadIdentity(); 
  
  glTranslatef(transfac*ttX, transfac*ttY, transfac*ttZ); 
  glRotated(a, X, Y, Z); 
  glTranslatef(-transfac*ttX, -transfac*ttY, -transfac*ttZ); 
  glMultMatrixf(m); 
}


void doRotation(double dX, double dY) {
  double offX = 2.0*(double)dragX/(double)width-1.0; 
  double offY = 1.0-2.0*(double)dragY/(double)height; 
  double offZ = 0.25; 
  double X, Y, Z, a;
  X = -offZ*dY; 
  Y = offZ*dX; 
  Z = offX*dY - offY*dX; 
  a = 180.*1.5*sqrt(dX*dX + dY*dY)/(PI*winRadius); 
  myRotated(a, X, Y, Z); 
  return; 
}

void load_obj(const char* filename, vector<SphereDisp> &spheres, vector<Edge> &edges)
{
  ifstream in(filename, ios::in);
  if(!in)
  {
    cerr << "Cannot open " << filename << endl; exit(1);
  }
  
  string line;
  while(getline(in, line)) {
    if(line.substr(0,7) == "sphere ")
    {
      istringstream s(line.substr(7));
      SphereDisp v; s >> v.x; s >> v.y; s >> v.z; s >> v.radius;
      spheres.push_back(v);
    } // handles spheres
    else if(line.substr(0, 5) == "edge ")
    {
      istringstream s(line.substr(5));
      Edge e; s >> e.a; s >> e.b;
      edges.push_back(e);
    } // handle edges
    else
    {
      // This is garbage.
    }
  }
}

void load_obj(const char* filename, vector<Point> &vertices, vector<Face> &faces, vector<Point> &textCoords)
{
  ifstream in(filename, ios::in);
  if (!in)
  {
    cerr << "Cannot open " << filename << endl; exit(1);
  }
  
  string line;
  while (getline(in, line))
  {
    if (line.substr(0, 2) == "v ")
    {
      istringstream s(line.substr(2));
      Point v; s >> v.x; s >> v.y; s >> v.z;
      vertices.push_back(v);
    }
    else if(line.substr(0, 2) == "vt")
    {
      istringstream s(line.substr(2));
      Point v; s >> v.x; s >> v.y; s >> v.z;
      textCoords.push_back(v);
    }
    else if (line.substr(0, 2) == "f ")
    {
      istringstream s(line.substr(2));
      Face temp;
      char dummy;
      s >> temp.Pta >> dummy >> temp.Txa; temp.Na = 1;// >> dummy >> temp.Na;
      temp.Pta--; temp.Txa--; temp.Na--;
      s >> temp.Ptb >> dummy >> temp.Txb; temp.Nb = 1;// >> dummy >> temp.Nb;
      temp.Ptb--; temp.Txb--; temp.Nb--;
      s >> temp.Ptc >> dummy >> temp.Txc; temp.Nc = 1;// >> dummy >> temp.Nc;
      temp.Ptc--; temp.Txc--; temp.Nc--;
      faces.push_back(temp);
    }
    else if (line[0] == '#')
    {
      /* ignoring this line */
    }
    else
    {
      /* ignoring this line */
    }
  }
}

vector<Point> vertices;
vector<Point> textCoords;
vector<Face> faces;

vector<SphereDisp> spheres;
vector<Edge> edges;

void loader()
{
  vertices.clear();
  textCoords.clear();
  faces.clear();
  
  spheres.clear();
  edges.clear();
  
  load_obj(inputMesh.c_str(), vertices, faces, textCoords);
  load_obj(outputMesh.c_str(), spheres, edges);
  
}

void display (void)
{

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  if(viewpoint == PLOT)
  {

    if(once == 0)
    {
      showMenu(); 
    }
  if(once != 2)
  {
    glScalef(0.5, 0.5, 0.5);
    once++; 
  }
  
    glPushMatrix(); 
    glScalef(zoomX, zoomX, zoomX);
  
  int count = 0; 
  int i, j; 
  
  glColor3f(1, 0, 1); 
  
  ticks4 = clock(); 
  tt = (double)(ticks4 - ticks3)/(double)CLOCKS_PER_SEC; 
  if(state == GLUT_DOWN && tt > .05) isSpinning = 0; 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
  j = iterations - delay; 
  glColor4f(1., .2, 0.2, .7); 
  if(isSpinning) {
    glMatrixMode(GL_MODELVIEW); 
    doRotation(dX, dY); 
  }

/* PLOT OBJECT */
    //glutSolidSphere(50, 10, 10);
    glPushMatrix();
    glScaled(0.5, 0.5, 0.5);
    glTranslatef(0, -20, 0);
    if(meshMode)
      luma();
    if(gridMode)
      setGround();
    if(sphereMode || interpMode)
      sphereMesh();
    
    glPopMatrix();

  
    //glScalef(0.5, 0.5, 0.5);
  glPopMatrix(); 
  glFlush(); 
  ++iterations; 
  }
}

void setGround()
{
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glBegin(GL_LINES); 
  glColor3f(1, 1, 1); 
  for(int i = -50; i < 50; i++)
  {
    glVertex3f(-100, -10, 5*i);
    glVertex3f(100, -10, 5*i);
    glVertex3f(5*i, -10, -100);
    glVertex3f(5*i, -10, 100);
  }
  
  
  glEnd();
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  
}

void DrawCircle(float cx, float cy, float r, int num_segments)
{
  float theta = 2 * 3.1415926 / float(num_segments);
  float tangetial_factor = tanf(theta);//calculate the tangential factor
  
  float radial_factor = cosf(theta);//calculate the radial factor
  
  float x = r;//we start at angle = 0
  
  float y = 0;
  
  glBegin(GL_LINE_LOOP);
  for(int ii = 0; ii < num_segments; ii++)
  {
    glVertex2f(x + cx, y + cy);//output vertex
    
    //calculate the tangential vector
    //remember, the radial vector is (x, y)
    //to get the tangential vector we flip those coordinates and negate one of them
    
    float tx = -y;
    float ty = x;
    
    //add the tangential vector
    
    x += tx * tangetial_factor;
    y += ty * tangetial_factor;
    
    //correct using the radial factor
    
    x *= radial_factor;
    y *= radial_factor; 
  } 
  glEnd(); 
}

void sphereMesh() {
  
  glColor3f(0.5, 0, 0);
  for(int i = 0; i < spheres.size(); i++)
  {
    //cout << "Drawing sphere " << i << " with info (" << spheres[i].x << ", " << spheres[i].y << ", " << spheres[i].z << ") " << endl;
    glPushMatrix();
    glTranslatef(spheres[i].x, spheres[i].y, spheres[i].z);
    glutSolidSphere(spheres[i].radius, 100, 100);
    //gluSphere(quad, spheres[i].radius, 100, 100);
    glPopMatrix();
  }
  
  
  /*GLUquadric *quad;
  quad = gluNewQuadric();
  gluQuadricOrientation(quad, GLU_INSIDE);
  gluSphere(quad, 50, 500, 500);
  */

    glColor3f(0.6, 0.6, 1.0);
  
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  
  glBegin(GL_LINES);
  glEnable (GL_LINE_SMOOTH);
  glLineWidth(3);
  SphereDisp a;
  SphereDisp b;
  for(int i = 0; i < edges.size(); i++)
  {
    a = spheres.at(edges[i].a);
    b = spheres.at(edges[i].b);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
  }
  glLineWidth(1);
  glDisable(GL_LINE_SMOOTH);
  glEnd();
//  glEnable(GL_LIGHTING);
//  glEnable(GL_LIGHT0);
 // glutSolidSphere(50, 50, 50);
  a = spheres.at(edges[0].a);
  b = spheres.at(edges[0].b); // Make b the bigger sphere ALWAYS
  
  glShadeModel(GL_SMOOTH);
  //DrawCircle(a.x, a.y, a.radius+10, 500);
  if(interpMode)
  {
    //Get vector direction of edge
    Point vDir;
    vDir.x = a.x - b.x;
    vDir.y = a.y - b.y;
    vDir.z = a.z - b.z;
    
    //tan Vect is (1, 1, z)
    float tanDir = (-vDir.x - vDir.y)/(vDir.z);
    
    float unitLength = sqrt(1 + 1 + tanDir*tanDir);
    
    Point s;
    s.x = a.x + (1.0/unitLength)*(a.radius);
    s.y = a.y + (1.0/unitLength)*(a.radius);
    s.z = a.z + (tanDir/unitLength)*(a.radius);
    
    Point sB;
    sB.x = b.x + (1.0/unitLength)*(b.radius);
    sB.y = b.y + (1.0/unitLength)*(b.radius);
    sB.z = b.z + (tanDir/unitLength)*(b.radius);
    
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    
    //glVertex3f(a.x, a.y, a.z);
    glVertex3f(s.x, s.y, s.z);
    
    //glVertex3f(b.x, b.y, b.z);
    glColor3f(0, 1, 0);
    glVertex3f(sB.x, sB.y, sB.z);
    
   // glVertex3f(s.x, s.y, s.z);
    
    //glVertex3f(b.x, b.y, b.z);
    //glVertex3f(sB.x, sB.y, sB.z);
    
    
    
    glEnd();
    
  }
  glDisable(GL_SMOOTH);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}

void luma() {
  //glColor3f(1.0, 0.7, 0.3);
  //Parameters For glMaterialfv() function
  
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  
  GLfloat specular[] = { 0.7, 0.7, 0.7, 1.0 };
  GLfloat ambient[]={1,1,1,1},diffuse[]={0.7,0.7,0.7,1};
  GLfloat full_shininess[]={100.0};
  
  //Material Properties
  glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
  glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
  glMaterialfv(GL_FRONT,GL_SHININESS, full_shininess);
  //glColor3f(0.54,0.75,0.0);
  glBegin(GL_LINES);
  for (int i = 0; i < faces.size(); i++)
  {
    glColor3f(1, 0.7, 0.3);
    //glColor3f(rand()/RAND_MAX, 1, 0);
    //glColor3f(1, 0.7 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.16))), 0.3);
    //glColor3f(1, (rand()) / static_cast <float> (RAND_MAX), 0, 1);
    glVertex3f(vertices[faces[i].Pta].x, vertices[faces[i].Pta].y, vertices[faces[i].Pta].z);
    glVertex3f(vertices[faces[i].Ptb].x, vertices[faces[i].Ptb].y, vertices[faces[i].Ptb].z);
    
    glVertex3f(vertices[faces[i].Pta].x, vertices[faces[i].Pta].y, vertices[faces[i].Pta].z);
    glVertex3f(vertices[faces[i].Ptc].x, vertices[faces[i].Ptc].y, vertices[faces[i].Ptc].z);
    
    glVertex3f(vertices[faces[i].Ptb].x, vertices[faces[i].Ptb].y, vertices[faces[i].Ptb].z);
    glVertex3f(vertices[faces[i].Ptc].x, vertices[faces[i].Ptc].y, vertices[faces[i].Ptc].z);
  }
  glEnd();
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}

void init() {
  // enable depth testing
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 50.0 };
  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
  glClearColor (0.0, 0.0, 0.0, 0.0);
  glShadeModel (GL_SMOOTH);
  
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  
  glEnable(GL_DEPTH_TEST);
  
  glEnable(GL_NORMALIZE);


}

void Exit() {

}

void keyPress(unsigned char key, int, int) {

  switch(key) {
      
      // increase the netSize
    case 'S':
    case 's':
      sphereMode = !sphereMode;
      cout << ">s:    SPHERE MODE: Status Changed" << endl;
      break;
    case 'M':
    case 'm':
      meshMode = !meshMode;
      cout << ">m:      MESH MODE: Status Changed" << endl;
      break;
    case 'I':
    case 'i':
      interpMode = !interpMode;
      cout << ">m:    INTERP MODE: Status Changed" << endl;
      break;
    case 'G':
    case 'g':
      gridMode = !gridMode; 
      cout << ">g:      GRID MODE: Status Changed" << endl; 
      break; 
    case 'H':
    case 'h':
      holemode = !holemode; 
      cout << ">h:      HOLE MODE: Status Changed" << endl; 
      break; 
    case 'X':
    case 'x':
      cout << ">x:      DRAG STATE: Rotate Model in X" << endl; 
      moveType = 0; 
      glutPostRedisplay(); 
      break;
      
    case 'Y':
    case 'y':
      cout << ">y:      DRAG STATE: Rotate Model in Y" << endl; 
      moveType = 1; 
      glutPostRedisplay(); 
      break;
    case 'E':
      case 'e':
      cout << ">e:      LOAD STATE: Enter new obj:   ";
      cin >> inputMesh;
      cout << inputMesh << " is being loaded..." << endl;
      cout << ">e:      LOAD STATE: Enter # spheres: ";
      cin >> sphereCount;
      calcSphereMesh(inputMesh, sphereCount, "output.sm");
      loader();
      //viewpoint=SETPTS;
      break;
    case 'Z':
    case 'z':
      cout << ">z:      DRAG STATE: Rotate Model in Z" << endl; 
      moveType = 2; 
      glutPostRedisplay(); 
      break;
      
    case 'D':
    case 'd':
      cout << ">m:      MENU STATE: Display Menu" << endl; 
      showMenu(); 
      break;
      
    case 27:
    case 'Q':
    case 'q':
      exit(0);
      
    default:
      break; 
  }
  
  glutPostRedisplay(); 
}

void mouse(int but, int sta, int x, int y) {
  button = but; 
  state = sta; 
  if(state == GLUT_DOWN) {
    dragX = x; 
    dragY = y; 
    dX = 0; 
    dY = 0; 
  }
  if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
    /*
     dX = x - dragX; 
     dY = dragY - y; 
     if(dX!= 0||dY!= 0)
     isSpinning = 1; 
     else
     isSpinning = 0; 
     */
    ticks1 = clock(); 
    isSpinning = 0; 
    just_up = 1; 
  }
}
void motion(int x, int y) {
  //if (state == GLUT_DOWN && (button == GLUT_LEFT_BUTTON || moveType == 0)) {
  if (state == GLUT_DOWN && (moveType == 0)) {
    dX = x - dragX; 
    dY = dragY - y; 
    glMatrixMode(GL_MODELVIEW); 
    doRotation(dX, dY);
    dragX = x; 
    dragY = y; 
  }
  //if (state == GLUT_DOWN && (button == GLUT_MIDDLE_BUTTON || moveType == 1)) {
  if (state == GLUT_DOWN && (moveType == 1)) {
    tX = x - dragX; 
    tY = dragY - y; 
    ttX += tX; 
    ttY += tY; 
    dragX = x; 
    dragY = y;
    doTranslation(tX, tY, 0);
  }
  //if (state == GLUT_DOWN && (button == GLUT_RIGHT_BUTTON || moveType == 2)) {
  if (state == GLUT_DOWN && (moveType == 2)) {
    tZ = x - dragX; 
    ttZ += tZ; 
    tW = y - dragY; 
    ttW += tW; 
    dragX = x; 
    dragY = y; 
    doTranslation(0, 0, tZ);
  }
  ticks3 = clock(); 
  tt = (double)(ticks3 - ticks4)/(double)CLOCKS_PER_SEC; 
  display(); 
}
void passive_motion(int x, int y) {
  double t; 
  if(just_up) {
    ticks2 = clock(); 
    t = (double)(ticks2 - ticks1)/(double)CLOCKS_PER_SEC; 
    just_up = 0; 
    if (t < .01) {
      dX = .2*(x - dragX); 
      dY = .2*(dragY - y); 
      isSpinning = 1; 
    }
  }
}


void specialKeyInput(int key, int x, int y)
{
  if(key == GLUT_KEY_LEFT)
  {
    zoomX *= 0.8; 
    cout << ">^L:     ZOOM STATE: Zoom Out" << endl; 
  }
  if(key == GLUT_KEY_RIGHT)
  {
    zoomX *= 1.25; 
    cout << ">^R:     ZOOM STATE: Zoom In" << endl; 
  }
  
  glutPostRedisplay(); 
  
}

void showMenu()
{
  cout << "===================== INSTRUCTIONS ==================== " << endl;
  cout << "| Follow the instructions indicated on the screen.    |" << endl;
  cout << "| INPUTS:                                             |" << endl;
  cout << "|      Left/Right Arrow: Zoom Out/Zoom In             |" << endl;
  cout << "|      S or s          : Enable/Disable Sphere Mesh   |" << endl;
  cout << "|      I or i          : Enable/Disable Interpolation |" << endl;
  cout << "|      M or m          : Enable/Disable Mesh Mode     |" << endl;
  cout << "|      G or g          : Enable/Disable Grid Mode     |" << endl;
  cout << "|                                                     |" << endl;
  cout << "|      J or j          : Calculate New Spheres Mesh   |" << endl;
  cout << "|                                                     |" << endl;
  cout << "|      Drag Mouse      : Rotate Model                 |" << endl;
  cout << "|      X or x          : Rotate Model in X            |" << endl;
  cout << "|      Y or y          : Rotate Model in Y            |" << endl;
  cout << "|      Z or z          : Rotate Model in Z            |" << endl;
  cout << "|                                                     |" << endl;
  cout << "|      E or e          : Enter New Files              |" << endl;
  cout << "|                                                     |" << endl;
  cout << "|      D or d          : View Menu Options            |" << endl;
  cout << "|      Q or q          : Quit                         |" << endl;
  cout << "|                                                     |" << endl;
  cout << "===================== INSTRUCTIONS =================== " << endl;
  
}


int main(int argc, char** argv)
{
  inputMesh = "Luma.obj";
  outputMesh = "output.sm";
  sphereCount = 10;
  
  glutInit(&argc, argv); 
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH); 
  glutInitWindowSize(width, height); 
  glutCreateWindow("SPHERE MESH RENDER");
  calcSphereMesh("Luma.obj", 10, "output.sm");
  load_obj(inputMesh.c_str(), vertices, faces, textCoords);
  load_obj(outputMesh.c_str(), spheres, edges);
    init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyPress);
  glutSpecialFunc(specialKeyInput);
  glutMouseFunc(mouse); 
  glutMotionFunc(motion); 
  glutPassiveMotionFunc(passive_motion); 
  atexit(Exit);
  glutMainLoop(); 
  
  return 0; 
}
