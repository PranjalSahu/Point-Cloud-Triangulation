#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <queue>
#include <list>
#include <vector>
//1363680307
//1363682010
//1363682772
//1363682932
//1363683031
#define EPS .00001

int maxdepth = -1;

float xcoor[200000];
float ycoor[200000];
float zcoor[200000];

float minx  =  100000;
float maxx  = -100000;
float miny  =  100000;
float maxy  = -100000;
float minz  =  100000;
float maxz  = -100000;
	
	
// for showing the points from the file
FILE *fp;
int n  = 100000;
float zoom = 2.5;
char etext[1000];

typedef struct mypoint{
	float x, y, z;
}mypoint;

#define MAXEDGECOUNT 20000
char edgematrix[40000][40000] = {0};
int *xcolist;
int *ycolist;
int tcount=0;


char s[100];

int theta = 45, phi = 45;
int gluLookAt_On = 1;
int width, height;
int mycount = 0;

const double radianFactor = 2 * 3.1415926535 / 360; 

class Point2d{
	public:
		int x, y;						// indices of the vertex
		Point2d();
		Point2d(int xa, int ya);
};

Point2d::Point2d(){
	x = 0;
	y = 0;
}

Point2d::Point2d(int xa, int ya){
	x = xa;
	y = ya;
}

std::list<int> myoriglist;
std::list<int> mydestlist;
std::list<int> myoriglistc;
std::list<int> mydestlistc;

int myoriglistint[50000];
int mydestlistint[50000];
int edgecount = 0;
//#define MAXEDGECOUNT 20000

class triangle;

class Edge {
	public:
		Edge *nextedge;			// next edge on the same face in ccw
		Edge *dualedge;			// dual edge of this edge
		int   origin;
		int   dest;				// origin and destination points on the edge
		int num;
		triangle *leftface;		// pointer to the left face
		triangle *rightface;	// pointer to the right face
		Edge();
};

class triangle {
	public:
		triangle *child[3];
		Edge *startingEdge;
		triangle *Locate(int);
		int a, b, c;
		triangle(int a, int b, int c);
		triangle();
		void InsertSite(int p);
};

triangle *start;
Edge *globalstart;			// the latest edge which has been added

// triplet of three edges which are going to be inserted in a new site
class triplet{
	public:
		Edge *e[3];
};

inline Edge::Edge(){
	num 	  = 0;
	origin 	  = 0;
	dest   	  = 0;
	nextedge  = NULL;
	dualedge  = NULL;
	leftface  = NULL;
	rightface = NULL;
}

inline float TriArea(int a, int b, int c){
	return (xcoor[b] - xcoor[a])*(ycoor[c] - ycoor[a]) - (ycoor[b] - ycoor[a])*(xcoor[c] - xcoor[a]);	
}

// Returns TRUE if the points a, b, c are in a counterclockwise order
int ccw(int a, int b, int c){
	return (TriArea(a, b, c) > 0);
}

int InCircle(triangle *t, int d){
	int a = t->a;	int b = t->b;	int c = t->c;
	return (xcoor[a]*xcoor[a] + ycoor[a]*ycoor[a]) * TriArea(b, c, d) -
			(xcoor[b]*xcoor[b] + ycoor[b]*ycoor[b]) * TriArea(a, c, d) +
			(xcoor[c]*xcoor[c] + ycoor[c]*ycoor[c]) * TriArea(a, b, d) -
			(xcoor[d]*xcoor[d] + ycoor[d]*ycoor[d]) * TriArea(a, b, c) > 0;	
}

int InTriangle(int p, triangle *node){
	if(ccw(node->a, node->b, p) && ccw(node->b, node->c, p) && ccw(node->c, node->a, p))
		return 1;
	else
		return 0;
}

//check if point x is rightt of edge e
int RightOf(int x, Edge* e){
	return ccw(x, e->dest, e->origin);
}

//check if point x is left of edge e
int LeftOf(int x, Edge* e){
	return ccw(x, e->origin, e->dest);
}

triangle *mylocate(triangle *start, int p){
	if(start->child[0] == NULL && start->child[1] == NULL && start->child[2] == NULL){
		return start;
	}

	triangle *e;
	triangle *current;
	current = start;

	for(int i=0;i<3;++i){
		if(current->child[i] != NULL){
			if(InTriangle(p, current->child[i])){
				e = mylocate(current->child[i], p);
				return e;
			}
		}
	}
	return e;
}

triangle* triangle::Locate(int x){
	return mylocate(start, x);
}

triangle::triangle(){
	a = 0;	b = 0; c = 0;
	startingEdge = NULL;
	for(int i=0;i<3;++i)
			child[i] = NULL;
}

triangle::triangle(int a1, int b1, int c1){
	if(!ccw(a1, b1, c1)){	// check if the three points are in ccw sense
		int temp;
		temp = b1;
		b1   = c1;
		c1   = temp;		
	}

	a = a1;
	b = b1;
	c = c1;

	Edge *e1, *e2, *e3;
	e1 = new Edge();
	e2 = new Edge();
	e3 = new Edge();

	e1->nextedge = e2;		e2->nextedge = e3;		e3->nextedge = e1;
	e1->dualedge = NULL;	e2->dualedge = NULL;	e3->dualedge = NULL;
	e1->leftface = this;	e2->leftface = this;	e3->leftface = this;	
	e1->rightface= NULL;	e2->rightface= NULL;	e3->rightface= NULL;
	
	e1->origin = a1; e1->dest = b1;
	e2->origin = b1; e2->dest = c1;
	e3->origin = c1; e3->dest = a1;

	startingEdge = e1;

	for(int i=0;i<3;++i)
		child[i] = NULL;
}

void correctdual(triplet *edges){
	for(int i=0;i<3;++i){
		edges->e[i]->dualedge->origin   = edges->e[i]->dest;
		edges->e[i]->dualedge->dest     = edges->e[i]->origin;
		edges->e[i]->dualedge->nextedge = edges->e[(edges->e[i]->num+2)%3];
	}
}

int legalizeedge(int pr, Edge *pipj, triangle* t){
	if(pipj->rightface == NULL){
		return 1;						// no problem
	}	
	
	Edge *temp = pipj->nextedge;
	
	int v1 = pipj->dest;
	int v2 = pipj->nextedge->dest;
	int v3 = pipj->origin;
	int v4 = pipj->dualedge->nextedge->dest;		
	
	if(InCircle(pipj->leftface, v4)){
		edgematrix[pipj->origin][pipj->dest] = '0';
		edgematrix[pipj->dest][pipj->origin] = '0';
		
		Edge *e1 = pipj->nextedge;
		Edge *e2 = pipj->nextedge->nextedge;
		Edge *e3 = pipj->dualedge->nextedge;
		Edge *e4 = pipj->dualedge->nextedge->nextedge;
		
		Edge *e 	= new Edge();
		Edge *edual = new Edge();
		
		e->dualedge 	= edual;
		edual->dualedge = e;
		
		e->origin     = pr;
		e->dest       = v4;
		edual->origin = e->dest;
		edual->dest   = e->origin;
		
		edgematrix[e->origin][e->dest] = '1';
		edgematrix[e->dest][e->origin] = '1';
		
		// make two new faces and set the pointers
		triangle *f1 = new triangle();
		triangle *f2 = new triangle();		 	
		
		t->child[0]  			  = f1;
		t->child[1]  			  = f2;	
		pipj->rightface->child[0] = f1;
		pipj->rightface->child[1] = f2;
		
		// set the points of the faces
		f2->a = pr;		f2->b = v3;		f2->c = v4;
		f1->a = pr;		f1->b = v4;		f1->c = v1;				
		
		f1->startingEdge = e;
		f2->startingEdge = edual;
		
		// set the faces
		e->leftface      = f1;
		e->rightface	 = f2;
		edual->leftface  = f2;
		edual->rightface = f1;
		
		e1->leftface    = f1;	if(e1->dualedge != NULL)	e1->dualedge->rightface = f1;
		e2->leftface	= f2;	if(e2->dualedge != NULL)	e2->dualedge->rightface = f2;
		e3->leftface	= f2;	if(e3->dualedge != NULL)	e3->dualedge->rightface = f2;
		e4->leftface	= f1;	if(e4->dualedge != NULL) 	e4->dualedge->rightface = f1;		
		
		// set the nextedges
		e->nextedge 	= e4;
		e4->nextedge    = e1;
		e1->nextedge    = e;
		edual->nextedge = e2;
		e2->nextedge    = e3;
		e3->nextedge    = edual;				
		
		legalizeedge(pr, e3, f2);
		legalizeedge(pr, e4, f1);
		return 0;
	}
	else{		
		return 1;
	}
}

void triangle::InsertSite(int x){
	triangle* t = Locate(x);
		
	triplet *edges 	   =  new triplet();
	triplet *dualedges =  new triplet();

	for(int i=0;i<3;++i){
		edges->e[i]     		  = new Edge();
		dualedges->e[i] 		  = new Edge();
		edges->e[i]->dualedge     = dualedges->e[i];
		dualedges->e[i]->dualedge = edges->e[i];
		edges->e[i]->origin   	  = x;
		edges->e[i]->num      	  = i;
	}

	Edge *se[3];
	se[0] = t->startingEdge;
	se[1] = se[0]->nextedge;
	se[2] = se[1]->nextedge;
	
	// setting the destination of new edges
	for(int i=0;i<3;++i){
		edges->e[i]->dest 	  = se[i]->origin;	
		edges->e[i]->nextedge = se[i];
		if(edgematrix[edges->e[i]->dest][edges->e[i]->origin] != '1'){
			edgematrix[edges->e[i]->dest][edges->e[i]->origin]  = '1';
			edgematrix[edges->e[i]->origin][edges->e[i]->dest]	 = '1';	
		}
	}	

	// correct the face cycles
	se[0]->nextedge = edges->e[1]->dualedge;
	se[1]->nextedge = edges->e[2]->dualedge;
	se[2]->nextedge = edges->e[0]->dualedge;

	// correct the properties of the dual edges
	correctdual(edges);
	correctdual(edges);
	correctdual(edges);

	triangle *face[3];
	for(int i=0;i<3;++i){
		face[i] = new triangle();
		face[i]->startingEdge = edges->e[i];
		face[i]->a = x;
		face[i]->b = edges->e[i]->dest;
		face[i]->c = edges->e[i]->nextedge->dest;
		t->child[i] = face[i];
		edges->e[i]->leftface = face[i];
		se[i]->leftface       = face[i];
		if(se[i]->dualedge !=  NULL)
			se[i]->dualedge->rightface  = face[i];
	}
	
	// correcting the right faces
	for(int i=0;i<3;++i){
		edges->e[i]->rightface     = face[(i+2)%3];
		dualedges->e[i]->leftface  = edges->e[i]->rightface;
		dualedges->e[i]->rightface = edges->e[i]->leftface;
	}

	globalstart = edges->e[2];	
	
	edgecount = edgecount+3;
	for(int i=0;i<3;++i)
		legalizeedge(x, se[i], face[i]);	
}


// making a list of edges
void addnodes(triangle * start, int depth){
	printf("in addnodes depth = %d\n", depth);
	if(maxdepth<depth)
		maxdepth = depth;
	if(start->child[0] == NULL){
		Edge *et1 = start->startingEdge;				
		
		if(edgematrix[et1->origin][et1->dest] != '1'){
			edgematrix[et1->origin][et1->dest] 				= '1';
			edgematrix[et1->dest][et1->origin] 				= '1';
			++edgecount;
		}
		if(edgematrix[et1->origin][et1->dest] != '1'){
			edgematrix[et1->dest][et1->nextedge->dest] 		= '1';
			edgematrix[et1->nextedge->dest][et1->dest] 		= '1';
			++edgecount;
		}
		if(edgematrix[et1->origin][et1->dest] != '1'){
			edgematrix[et1->nextedge->dest][et1->origin] 	= '1';
			edgematrix[et1->origin][et1->nextedge->dest] 	= '1';
			++edgecount;
		}		
		return;
	}
	
	for(int i=0;i<3;++i){
		if(start->child[i] != NULL){
			addnodes(start->child[i], depth+1);
		}
	}
	return;
}

void *font = GLUT_BITMAP_TIMES_ROMAN_24;

void outputCharacter(float x, float y, float z, char *string) {
  int len, i;
  glRasterPos3f(x, y, z);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

void changeSize(int w, int h){
    width = w;
    height = h;
    
    if(h == 0) h = 1;
    float ratio = 1.0 * w / h;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();    
    
    glViewport(0, 0, w, h);
	
	gluPerspective(45, ratio, 1, 1000);	
	
	float r = 5.0f;
	float eyeX = r * sin(theta * radianFactor) * cos(phi * radianFactor);
	float eyeY = r * sin(theta * radianFactor) * sin(phi * radianFactor);
	float eyeZ = r * cos(radianFactor * theta);

	float centerX = 0, centerY = 0, centerZ = 0;
	float upX = 0, upY = 1.0f, upZ = 0;
	
	if(gluLookAt_On) {
		gluLookAt(eyeX, eyeY, eyeZ, 
				  centerX, centerY, centerZ,
				  upX, upY, upZ); 
	}

	glScalef(zoom, zoom, zoom);
	glMatrixMode(GL_MODELVIEW);		
}

void renderScene(void){
	int temporig, tempdest;
	
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity(); 	
	
	/*glColor3f(1.0,1.0,1.0);  
    glBegin(GL_LINES);
        glVertex3f(-0.8f, 0.0f,0.0f);
        glVertex3f(0.8f, 0.0f,0.0f);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(0.0f, -0.8f,0.0f);
        glVertex3f(0.0f, 0.8f,0.0f);
    glEnd();
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, -0.8f);
        glVertex3f(0.0f, 0.0f, 0.8f);
    glEnd();*/
        
    glColor3f (1.0, 1.0, 1.0);    
	glBegin(GL_POINTS);
		for(int u=0; u<40000;u = u+5){			
			glVertex3f(xcoor[u], ycoor[u], zcoor[u]);
		}
    glEnd();     
    
    glColor3f(1.0, 1.0, 1.0);	
	glBegin(GL_LINES);
    
    for(int i=0;i<tcount;++i){
		int ti = xcolist[i];
		int tj = ycolist[i];
		float dist = 0;
		dist = sqrt((xcoor[ti]-xcoor[tj])*(xcoor[ti]-xcoor[tj])+(ycoor[ti]-ycoor[tj])*(ycoor[ti]-ycoor[tj])+(zcoor[ti]-zcoor[tj])*(zcoor[ti]-zcoor[tj]));
		if(dist < 0.3){
			glVertex3f(xcoor[ti], ycoor[ti], zcoor[ti]);
			glVertex3f(xcoor[tj], ycoor[tj], zcoor[tj]);
		}
	}	
	glEnd();
	   
    glutSwapBuffers();
}

void inputKey(unsigned char c, int x, int y){
    switch (c) {			
			case 'i' : zoom = zoom+ 0.5; break;
			case 'o' : zoom = zoom-0.5; break;
            case 't' : theta++; if(theta > 360) theta = 1; break;
            case 'p' : phi++; if(phi > 360) phi = 1; break;
            case 'T' : theta--; if(theta < 0) theta = 359; break;
            case 'P' : phi--; if(phi < 0) phi = 359; break;
            case 'g' : gluLookAt_On = !gluLookAt_On;; break;
    }
        changeSize(width, height);
}

void init(){
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv){
	int mycount = 0;
	
	for(int i=0;i<10;++i){				// ignore 1st seven lines
		gets(etext);
	}	
	
	float temp1, temp2, temp3;	
	
	for(int i=0;i<n;++i){
		gets(etext);
		if(etext[0] == 'n')	
			continue;		
		
		sscanf(etext, "%f %f %f", &temp1, &temp2, &temp3);
		
		xcoor[mycount] = temp1;
		ycoor[mycount] = temp2;
		zcoor[mycount] = temp3;
		
		if(minx>temp1)
			minx = temp1;
		if(miny>temp2)
			miny = temp2;
		if(minz>temp3)
			minz = temp3;	
		if(maxx<temp1)
			maxx = temp1;
		if(maxy<temp2)
			maxy = temp2;
		if(maxz<temp3)
			maxz = temp3;	
				
		++mycount;
	}	
	
	float meanx = (maxx+minx)/2;
	float meany = (maxy+miny)/2;
	float meanz = (maxz+minz)/2;
	
	printf("PRINTING VALUES mycount =  %d\n", mycount);
	for(int i=0;i< mycount;++i){		
		xcoor[i] = (xcoor[i]-meanx)*1.0/(maxx-minx);
		ycoor[i] = (ycoor[i]-meany)*1.0/(maxy-miny);
		zcoor[i] = (zcoor[i]-meanz)*1.0/(maxz-minz);		
	}
	
	xcoor[0] = -5;	ycoor[0] = -5;
	xcoor[1] = 5; 	ycoor[1] = -5;
	xcoor[2] = 0;  	ycoor[2] = 5;
	
	start = new triangle(0, 1, 2);
	
	unsigned int myseed = time(NULL);
	srand(myseed);
	int thresh = 30;
	
	for(int i=3;i< mycount-100; i = i+1){
		int tt = rand()%10000;
		if(ycoor[i]>0.3)
			thresh = 120;
		else
			thresh = 100;
		if(tt< thresh)
			start->InsertSite(i);
	}
	printf("TRIANGULATION DONE seed = %d\n", myseed);
	//addnodes(start, 0);
	
	xcolist = (int *)malloc(sizeof(int)*(edgecount+100));
	ycolist = (int *)malloc(sizeof(int)*(edgecount+100));
	
	
	for(int i=0;i<40000;++i){
		edgematrix[0][i] = '0';
		edgematrix[i][0] = '0';
		edgematrix[1][i] = '0';
		edgematrix[i][1] = '0';
		edgematrix[2][i] = '0';
		edgematrix[i][2] = '0';
	}
	for(int i=0;i<40000;++i){
		for(int j=i+1;j<40000;++j){
			if(edgematrix[i][j] == '1'){
				xcolist[tcount] = i;
				ycolist[tcount] = j;
				++tcount;
			}
		}
	}
	printf("NODES ADDED EDGECOUNT = %d maxdepth = %d\n", tcount, maxdepth);
	
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(800,600);
    glutCreateWindow("");
    init();
    glutDisplayFunc(renderScene);
    glutIdleFunc(renderScene);
    glutKeyboardFunc(inputKey);
    glutReshapeFunc(changeSize);

    glutMainLoop();
    return 0;
}
