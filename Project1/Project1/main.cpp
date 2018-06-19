#include <cstdlib> //for rand() function (random velocities and colors for the particles)
#include <GL/glut.h>
#include <GL/GL.h>
#include "AntTweakBar.h" //UI
//the fowllowing is for testing outputs in the console
//#include <iostream>
//using namespace std;

//global variables and user inputs:
bool _start = false;
bool _reset = false;
bool _updateGrav = false;
bool _updateDamping = false;
bool _updateWind = false;
bool _updateSource = false;
float gGravity = -0.01f;  //gravity, downward
float gFriction = 0.7f;   //friction factor in tangent directions, 1 when there is no friciton
float gCoeffRes = 0.7f;   //coefficient of restitution in normal direction, 1 when the plane is totally elastic
float gWind[3] = { 0.0f, 0.0f, 0.0f }; //in 6 directions
float gCamera[3] = { 0.0f, 13.0f, 50.0f }; // camera position

float gTimestep = 1.0f;
float gMass = 1.0f;
float gSource[3] = { 14.0f, 16.0f, -3.0f }; //particle source/emitter position
float plane_sizeX = 50.0f; //plane size
float plane_sizeY = 0.01f;
float plane_sizeZ = 50.0f;
float plane_color[6][3] = { { 0.9f, 0.9f, 0.9f },{ 0.7f, 0.7f, 0.7f },{ 0.5f, 0.5f, 0.5f },{ 0.2f, 0.2f, 1.0f },{ 0.7f, 0.7f, 0.7f },{ 0.5f, 0.5f, 0.5f } };
float plane_center[3] = { 0.0f, -3.0f, 0.0f };


class Particle {
private:
	float position[3];
	float velocity[3];  //with direction
	float color[3];
	float mass;
	int life;           //life=1000, a particle will last for 1000 updates
	
public:
	//constructor of the class
	Particle(float pos[3], float vel[3], float col[3], float mas) {
		mass = mas;
		life = 1000;  //this parameter can control the number of particles in the system

		for (int i = 0; i <= 2; i++) {
			position[i] = pos[i];
			velocity[i] = vel[i];
			color[i] = col[i];
		}
	}
	
	//-----functions only used in updateParticles():
	void setPosition(float newPosition[3]) {
		for (int i = 0; i <= 2; i++) {
			position[i] = newPosition[i];
		}
	}

	//for every update, life -1
	//this can also control the number of particles in the system
	void updateLife() {
		life--;
	}

	//check the life before get rid off the died particles
	float getLife() {
		return life;
	}

	//get the velocities before apply the forces
	float getVelocity(int i) {
		return velocity[i];
	}

	//the following are functions of the simple physics:
	//gravity is along Y axis
	void addGravity(float gravity) {
		velocity[1] += (gravity / mass) * gTimestep;
	}

	//after collision, the velocity on Y will be inverted
	void invertNormal() {
		velocity[1] *= -1;
	}

	//factor of friction in tangent directions of the plane
	void addFriction(float friction) {
			velocity[0] *= friction;
			velocity[2] *= friction;
	}

	//factor of restitution in normal directions of the plane
	void addCoeffRes(float coeffres) {
		velocity[1] *= coeffres;
	}

	//the wind can be applied in all 3 directions
	void addWind(float wind[3]) {
		for (int i = 0; i < 3; i++) {
			velocity[i] += (wind[i] / mass) * gTimestep;
		}
	}
	//-----functions only used in updateParticles() end

	//used in drawParticles() and updateParticles()
	float getPosition(int i) {
		return position[i];
	}

	//only used in drawParticles()
	float getColor(int i) {
		return color[i];
	}

};

class ParticleSystem {
private:
	
	float partSize = 0.3f; //size of particles
	float factorVel = 0.5f; //velocity magnitude controller, this factor is to be multiplied to the velocity
	
	float position[3];
	float gravity; 
	float friction; //factor of friction
	float coeffRes;	//coefficient of restitution
	float wind[3];
	
	float minX, maxX, 
		  topY, 
		  minZ,  maxZ; //plane boundaries, for detecting collisions	

	Particle* particle;    //pointer to a particle

	//to store the particle system, we use a doubly linked list
	//every node stores a particle element
	struct node {
		Particle* element;
		node* next;
		node* prev;
	};

	node* head; //head of the list, head->prev = NULL
	node* tail; //tail of the list, tail->next = NULL
	
	node* lastOne;  //pointer to the last added element
	node* newOne;   //pointer to a new node to be added to the list
	node* current;  //the current pointer

public:	
	int counter; //to count the number of particles in the list

	//system constructor
	ParticleSystem(float pos[3], float grav, float fric, float coeff, float wd[3]) {
		gravity = grav;
		friction = fric;
		coeffRes = coeff;

		for (int i = 0; i <= 2; i++) {
			position[i] = pos[i];
			wind[i] = wd[i];
		}
	}
	
	//create and add the particle to the list
	void createParticle() {

		float mass = gMass;

		//set the color of the particle, the values are between 0 and 1
		float newColor[3] = { rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX };

		//set the velocity of the particle, the values are between -1 and 1
		//-1 and 1 means 2 opposite directions
		//if only one side / direction, use rand()/(float)RAND_MAX (or then * -1.0f)
		float velocity[3] = { 2.0f * (rand() / (float)RAND_MAX) - 1.0f,
							2.0f * (rand() / (float)RAND_MAX) - 1.0f,
							2.0f * (rand() / (float)RAND_MAX) - 1.0f };

		//create a new particle element, and let the pointer particle point to it
		particle = new Particle(position, velocity, newColor, mass); 
		//create a new node in the list and let newOne point to it
		newOne = new node;
		//put the particle into the new node
		newOne->element = particle;

		//add new node to the list at the end
		if (lastOne != NULL && lastOne->element != NULL) { //if the list is not empty
			lastOne->next = newOne; 
			newOne->prev = lastOne;
		}
		else { //if lastOne = NULL, the list is empty
			newOne->prev = NULL; 
			head = newOne;
		}

		newOne->next = NULL;  //this is the last node of the list, so let the next points to NULL
		lastOne = newOne; //pointer lastOne points to the newly added element
		tail = lastOne;   //move the pointer tail to the newly added element too
	}

	//draw particles
	void drawParticles() {
		current = head; //be prepared to iterate from the very first node

		while (current != NULL && current->element != NULL) { //when list is not empty or not reaching the end			
			glPushMatrix();
			//use getColor()
			glColor3f(current->element->getColor(0), current->element->getColor(1), current->element->getColor(2));
			//use getPosition()
			glTranslatef(current->element->getPosition(0), current->element->getPosition(1), current->element->getPosition(2));
			//draw a sphere for one particle
			glutWireSphere(partSize, 5, 5); // (radius, lines of longitude, lines of latitude)
			glPopMatrix();
			current = current->next; //go to the next node
		}

	}

	//update the particles
	void updateParticles() {
		float newPosition[3]; //new position to be updated into every particle		
		current = head; //be prepared to iterate from the very first node
		counter = 0;    //the number of particle in the list, set to 0

		while (current != NULL && current->element != NULL) { //when list is not empty or not reaching the end
			current->element->updateLife(); //update the life of the current particle
			//if the particle died i.e. life negative
			if (current->element->getLife() < 0) {
				//set the current node to NULL
				current->element = NULL; 
				//go to the next node
				current = current->next;
				//free the memory
				free(head);
				//move the list head
				head = current;
				continue;
			}
			//set newPosition:
			for (int i = 0; i <= 2; i++) {
				//initialize
				newPosition[i] = current->element->getPosition(i);
				//update the position (without other forces)
				newPosition[i] += current->element->getVelocity(i) * factorVel * gTimestep;  
			}
			//add gravity
			current->element->addGravity(gravity);
			//add wind
			current->element->addWind(wind);
			//add collision and damping:
			//conditions for collision detection
			if (newPosition[1] < (topY + partSize / 2.0f) 
				&& newPosition[0] > minX && newPosition[0] < maxX
				&& newPosition[2] > minZ && newPosition[2] < maxZ) {
				    //invert the velocity in Y
					current->element->invertNormal(); 
					//bring the particle on the plane
					newPosition[1] = topY + partSize / 2.0f;
					//add friction in tangent directions
					current->element->addFriction(friction);
					//add factor of restitution of the plane in normal direction
					current->element->addCoeffRes(coeffRes);
			}		
			//set the postion
			current->element->setPosition(newPosition);		
			//go to the next node
			current = current->next;
			//update the counter
			counter++;
		} //while end
		//cout << "number of particles: "<<counter << endl;
	}

	void collisionBoundary(float center[3], float sizeX, float sizeY, float sizeZ) {
		minX = -sizeX / 2 + center[0];
		maxX = sizeX / 2  + center[0];
		topY = sizeY / 2 + center[1];
		minZ = -sizeZ / 2 + center[2];
		maxZ = sizeZ / 2 + center[2];
	}

    //the following are for updating the parameters in the UI when system runs
	void updateGravity(float grav) {
		gravity = grav;
	}

	void updateFriction(float fric) {
		friction = fric;
	}

	void updateCoeffRes(float coeff) {
		coeffRes = coeff;
	}

	void updateWind(float newWind[3]) {
		for (int i = 0; i <= 2; i++) {
			wind[i] = newWind[i];
		}
	}

};

//draw the plane
class Plane {
private:
	void polygon(int a, int b, int c, int d, float v[8][3]) {
		glBegin(GL_POLYGON);
		glVertex3fv(v[a]);
		glVertex3fv(v[b]);
		glVertex3fv(v[c]);
		glVertex3fv(v[d]);
		glEnd();
	}

	void cube(float v[8][3], float colors[6][3]) {
		glColor3fv(colors[0]);
		polygon(0, 4, 7, 3, v);
		glColor3fv(colors[1]);
		polygon(0, 3, 2, 1, v);
		glColor3fv(colors[2]);
		polygon(0, 1, 5, 4, v);
		glColor3fv(colors[3]);
		polygon(5, 1, 2, 6, v);
		glColor3fv(colors[4]);
		polygon(2, 3, 7, 6, v);
		glColor3fv(colors[5]);
		polygon(4, 5, 6, 7, v);
	}

public:
	void plane(float* center, float sizeX, float sizeY, float sizeZ, float colors[6][3]) {
		//8 vertices of the cube
		float vertices[8][3] = { { center[0] - sizeX / 2, center[1] - sizeY / 2, center[2] + sizeZ/ 2 },
		{ center[0] - sizeX / 2, center[1] + sizeY / 2, center[2] + sizeZ/ 2 },
		{ center[0] + sizeX / 2, center[1] + sizeY / 2, center[2] + sizeZ/ 2 },
		{ center[0] + sizeX / 2, center[1] - sizeY / 2, center[2] + sizeZ/ 2 },
		{ center[0] - sizeX / 2, center[1] - sizeY / 2, center[2] - sizeZ/ 2 },
		{ center[0] - sizeX / 2, center[1] + sizeY / 2, center[2] - sizeZ/ 2 },
		{ center[0] + sizeX / 2, center[1] + sizeY / 2, center[2] - sizeZ/ 2 },
		{ center[0] + sizeX / 2, center[1] - sizeY / 2, center[2] - sizeZ/ 2 } };
		cube(vertices, colors);
	}
};

//draw a plane
Plane drawPlane;
//create a particle system
ParticleSystem particleSystem(gSource, gGravity, gFriction, gCoeffRes, gWind);

//for doing reset function on the UI
void initParams() {
	gGravity = -0.01f; 
	gFriction = 0.7f; 
	gCoeffRes = 0.7f;
	gWind[0] = 0.0f;
	gWind[1] = 0.0f;
	gWind[2] = 0.0f; 
	gCamera[0] = 0.0f;
	gCamera[1] = 13.0f;
	gCamera[2] = 50.0f;
	plane_sizeX = 50;
	plane_sizeY = 0.01;
	plane_sizeZ = 50;
}

void init() {
	particleSystem.collisionBoundary(plane_center, plane_sizeX, plane_sizeY, plane_sizeZ);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE); 

	glClearColor(0, 0, 0, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1, 1, 200);  //(fovy, aspect, zNear, zFar)

}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(gCamera[0], gCamera[1], gCamera[2], 0, 0, 0, 0, 1, 0);

	glPushMatrix();
	//draw the plane
	drawPlane.plane(plane_center, plane_sizeX, plane_sizeY, plane_sizeZ, plane_color);

	glPushMatrix();
	glTranslatef(gSource[0], gSource[1], gSource[2]);
	glColor3f(1.0f,1.0f,0.0f); //yellow
	//draw the source of the particle
	glutWireSphere(1, 10, 10); 
	glPopMatrix();

	//draw the particle systems
	particleSystem.drawParticles();
	glPopMatrix(); 

	//draw UI bar
	TwDraw(); 
	glutSwapBuffers();
}

//for UI bar
void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	TwWindowSize(width, height);
}

void Terminate() {
	TwTerminate();
}

//set up the UI bar
void init_gui() {
	TwBar *operbar = TwNewBar("ControlBar");
	TwDefine(" GLOBAL help='CubeAnimation' ");
	TwDefine(" ControlBar position='16 16' size='235 420' color='96 216 224' ");
	TwAddVarRO(operbar, "# Particles", TW_TYPE_INT16, &particleSystem.counter, NULL);
	TwAddSeparator(operbar, "", "");
	TwAddVarRW(operbar, "Start/Stop", TW_TYPE_BOOL8, &_start, "help='start the animation'");
	TwAddVarRW(operbar, "Reset to Init", TW_TYPE_BOOL8, &_reset, "help='reset to the initial status'");
	TwAddSeparator(operbar, "", "");
	TwAddButton(operbar, "comment1", NULL, NULL, " label='click UPDATE to update ' ");
	TwAddSeparator(operbar, "", "");
	TwAddVarRW(operbar, "Gravity", TW_TYPE_FLOAT, &gGravity, "min=-5 max=5 step=0.01");
	TwAddVarRW(operbar, "UPDATE (Grav.)", TW_TYPE_BOOL8, &_updateGrav, "help='update the gravity'");
	TwAddSeparator(operbar, "", "");
	TwAddVarRW(operbar, "X left-right", TW_TYPE_FLOAT, &gWind[0], "group='Wind' min=-5 max=5 step=0.01");
	TwAddVarRW(operbar, "Y down-up", TW_TYPE_FLOAT, &gWind[1], "group='Wind' min=-5 max=5 step=0.01");
	TwAddVarRW(operbar, "Z fore-back", TW_TYPE_FLOAT, &gWind[2], "group='Wind' min=-5 max=5 step=0.01");
	TwAddVarRW(operbar, "UPDATE (Wind)", TW_TYPE_BOOL8, &_updateWind, "group='Wind' help='update the wind'");
	TwAddSeparator(operbar, "", "");
	TwAddVarRW(operbar, "tangent: Friction", TW_TYPE_FLOAT, &gFriction, "group='Damping' min=0 max=1 step=0.1");
	TwAddVarRW(operbar, "normal: CoeffRes.", TW_TYPE_FLOAT, &gCoeffRes, "group='Damping' min=0 max=1 step=0.1");
	TwAddVarRW(operbar, "UPDATE (Damp.)", TW_TYPE_BOOL8, &_updateDamping, "group='Damping' help='update the damping factors'");
	TwAddSeparator(operbar, "", "");
	TwAddVarRW(operbar, "X", TW_TYPE_FLOAT, &gCamera[0], "group='Camera' min=-100 max=100 step=1");
	TwAddVarRW(operbar, "Y", TW_TYPE_FLOAT, &gCamera[1], "group='Camera' min=-100 max=100 step=1");
	TwAddVarRW(operbar, "Z", TW_TYPE_FLOAT, &gCamera[2], "group='Camera' min=-100 max=100 step=1");
}

void timer(int value) {
	//10 msc per update
	glutTimerFunc(10, timer, 0);

	if (_start) { //start to create particles
		particleSystem.createParticle(); //create one particle per update
	}

	if (_reset) { //reset parameters to the initial values
		initParams();
		particleSystem.updateGravity(gGravity);
		particleSystem.updateFriction(gFriction);
		particleSystem.updateCoeffRes(gCoeffRes);
		particleSystem.updateWind(gWind);
		_reset = false;
	}

	if (_updateGrav) { //update gravity value on UI
		particleSystem.updateGravity(gGravity);
		_updateGrav = false;
	}

	if (_updateDamping) { //update damping factors on UI
		particleSystem.updateFriction(gFriction);
		particleSystem.updateCoeffRes(gCoeffRes);
		_updateDamping = false;
	}

	if (_updateWind) { //update wind values on UI
		particleSystem.updateWind(gWind);
		_updateWind = false;
	}

	particleSystem.updateParticles(); //update the list of the particles

	glutPostRedisplay(); //call display function
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutInitWindowSize(900, 900);
	glutInitWindowPosition(300, 0);

	glutCreateWindow("Particle System");
	TwInit(TW_OPENGL, NULL);

	glutDisplayFunc(display);

	//Directly redirect GLUT mouse button events to AntTweakBar
	glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	//Directly redirect GLUT mouse motion events to AntTweakBar
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	//Directly redirect GLUT mouse "passive" motion events to AntTweakBar (same as MouseMotion)
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	//Directly redirect GLUT key events to AntTweakBar
	glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
	//Directly redirect GLUT special key events to AntTweakBar
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	//Send 'glutGetModifers' function pointer to AntTweakBar;
	TwGLUTModifiersFunc(glutGetModifiers);

	glutTimerFunc(1, timer, 0);
	init();

	glutReshapeFunc(reshape);
	init_gui();

	//starts the event glutMainLoop
	glutMainLoop();				
	atexit(Terminate);
	return(0);
}
