//
//  main.cpp
//  Project5
//
//  Created by Hana Hyder on 5/23/16.
//  Copyright © 2016 UCD. All rights reserved.
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

using namespace std; 

bool dotMode = false, holemode = false, gridMode = true; 
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

enum {
  SETPTS, PLOT
}viewpoint= PLOT;


/* CALCULATIONS */


/* DISPLAY */
void luma();

void setGround(); 
void showMenu(); 

void reshape(int width1, int height1)
{
  if (height1 == 0) {
    height1 = 1;
  }
  
  glViewport(0, 0, width1, height1);
  
  glMatrixMode(GL_PROJECTION); 
  glLoadIdentity(); 
  
  gluPerspective(45, (float)width1/height1, 0.1, 100);
  
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
    glPushMatrix();
    glScaled(0.5, 0.5, 0.5);
    glTranslatef(0, -20, 0);
    luma();
    
    glPopMatrix();
  
  if(gridMode)
    setGround(); 
  
  
  glPopMatrix(); 
  glFlush(); 
  ++iterations; 
  }
}

void setGround()
{
  glBegin(GL_LINES); 
  glColor3f(1, 1, 1); 
  for(int i = -50; i < 50; i++)
  {
    glVertex3f(-60, -10, i);
    glVertex3f(60, -10, i);
    glVertex3f(i, -10, -60);
    glVertex3f(i, -10, 60);
  }
  
  
  glEnd();
  
}

void luma() {
  //glColor3f(1.0, 0.7, 0.3);
  //Parameters For glMaterialfv() function
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
}

void init() {
  // enable depth testing
  glEnable(GL_DEPTH_TEST); 
  /*GLfloat mat_diffuse[] = { 0.74, 0.36, 0.0, 1.0 }; 
  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 }; 
  GLfloat mat_shininess[] = { 100.0 }; 
  GLfloat ambientLight[] = { 1.0, 1.0, 1.0 }; 
  
  glClearColor (0.0, 0.0, 0.0, 0.0); 
  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse); 
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular); 
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess); 
  
  glEnable(GL_LIGHTING); 
  glEnable(GL_LIGHT0); 
  glEnable(GL_LIGHT1); 
  glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight); 
  //glEnable(GL_DEPTH_TEST); 
  glEnable(GL_AUTO_NORMAL); 
  glEnable(GL_NORMALIZE); 
  
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); */

}

void Exit() {

}

void keyPress(unsigned char key, int, int) {
  switch(key) {
      
      // increase the netSize
    case 'D':
    case 'd':
      dotMode = !dotMode; 
      cout << ">d:      DOT MODE: Status Changed" << endl; 
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
      viewpoint=SETPTS;
      break;
      
    case 'Z':
    case 'z':
      cout << ">z:      DRAG STATE: Rotate Model in Z" << endl; 
      moveType = 2; 
      glutPostRedisplay(); 
      break;
      
    case 'M':
    case 'm':
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
  cout << "=================== INSTRUCTIONS =================== " << endl; 
  cout << "| Follow the instructions indicated on the screen. |" << endl; 
  cout << "| INPUTS:                                          |" << endl; 
  cout << "|      Left/Right Arrow: Zoom Out/Zoom In          |" << endl; 
  cout << "|      D or d          : Enable/Disable Dot Mode   |" << endl; 
  cout << "|      H or h          : Enable/Disable Hole Mode  |" << endl; 
  cout << "|      G or g          : Enable/Disable Grid Mode  |" << endl; 
  cout << "|      J or j          : Calculate More Points     |" << endl; 
  cout << "|      F or f          : Calculate Fewer Points    |" << endl; 
  cout << "|      Drag Mouse      : Rotate Model              |" << endl; 
  cout << "|      X or x          : Rotate Model in X         |" << endl; 
  cout << "|      Y or y          : Rotate Model in Y         |" << endl; 
  cout << "|      Z or z          : Rotate Model in Z         |" << endl;
  cout << "|                                                  |" << endl;
  cout << "|      E or e          : Enter New Values          |" << endl;
  cout << "|                                                  |" << endl; 
  cout << "|      M or m          : View Menu Options         |" << endl; 
  cout << "|      Q or q          : Quit                      |" << endl; 
  cout << "|                                                  |" << endl; 
  cout << "=================== INSTRUCTIONS =================== " << endl; 
  
}


int main(int argc, char** argv)
{
  
  glutInit(&argc, argv); 
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH); 
  glutInitWindowSize(width, height); 
  glutCreateWindow("SPHERE MESH RENDER");
  load_obj("Luma.obj", vertices, faces, textCoords);
  glutDisplayFunc(display); 
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyPress);
  glutSpecialFunc(specialKeyInput); 
  init();
  glutMouseFunc(mouse); 
  glutMotionFunc(motion); 
  glutPassiveMotionFunc(passive_motion); 
  atexit(Exit);
  glutMainLoop(); 
  
  return 0; 
}
