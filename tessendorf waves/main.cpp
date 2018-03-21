#include <stdio.h>
#include <time.h> 
#include <cmath>

#include "ocean.h"

#include "GL/glew.h"
#include "GL/freeglut.h"
#include "shaderLoader.h"
#include "textureBMP.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

//for mp3-------------------------
#include <Mmsystem.h>
#include <mciapi.h>

#pragma comment(lib, "Winmm.lib") 
//--------------------------------
int screen_width = 1280;
int screen_height = 720;

int fpsMax = 100;
int frames; //help count fps

//tile size
int lx = 2000;
int ly = 2000;

//tile samples (must be power of 2)
int nx = 256;
int ny = 256;

double wind_speed = 50;
double A = 0.000000002; //value regulating wave height

int tiles = 1; //number of tiles in x and y direction

//initial player, camera position and player speed
double camRotX = 115, camRotY = 0;
double playerX = -1000, playerY = -65, playerZ = -1000;
double playerSpeed = 20;

//view range
float cameraFar = 1000.0f;

//mouse is wrapped in wrapX, wrapY position
int wrapX = screen_width / 2;
int wrapY = screen_height / 2;

Ocean *ocean; //Ocean object generate mesh and normals for Tessendorf Waves
float *oceanMesh; //current ocean mesh, new ocean mesh is generated when we change one of parameters e.g. wind speed
float *oceanNorm; //normals of the mesh to calculate reflections
int nOceanMesh; //used to remember number of mesh vertices

bool isSkybox = true; //skybox enable/disable
bool isLineMode = false; //show only mesh
bool isSound = true; //sound on/off

GLuint programId; //shader program id
GLuint vboOcean;
GLuint vboOceanNorm;

glm::mat4 M,V,P; //model view perspective for player movement
//===========================================================
void movePlayer(float angle) {
	playerX += -sin((camRotX + angle)*0.0174532925)*playerSpeed;
	playerZ += cos((camRotX + angle)*0.0174532925)*playerSpeed;
}
//-----------------------------------------------------------
void mouse(int x, int y)
{
	//calculate camera rotation
	camRotX += (x - wrapX)*0.01;
	camRotY += (y - wrapY)*0.01;

	//if camera angle is out of range <-180,180> recalculate
	if (camRotX > 180) camRotX -= 360;
	else if (camRotX < -180) camRotX += 360;

	if (camRotY > 90) camRotY = 90;
	else if (camRotY < -90) camRotY = -90;
}
//-----------------------------------------------------------
void resize(int width, int height)
{
	//save new screen width, height and cursor wrapped position
	screen_width = width;
	screen_height = height;
	wrapX = width / 2;
	wrapY = height / 2;

	//clear open gl buffers, set viewport and set new perspective matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screen_width, screen_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)screen_width / (GLfloat)screen_height, 0.1f, cameraFar);

	P = glm::perspective(glm::radians(45.0f), (GLfloat)screen_width / (GLfloat)screen_height, 1.0f, cameraFar);

	glutPostRedisplay();
}
//-----------------------------------------------------------
void keyboard(GLubyte key, int x, int y)
{
	switch (key) {

	case 27: //Esc
		delete ocean;
		delete[] oceanMesh;

		exit(1);
		break;

	//player movement
	case 'w':
		movePlayer(0);
		break;
	case 'a':
		movePlayer(-90);
		break;
	case 's':
		movePlayer(-180);
		break;
	case 'd':
		movePlayer(90);
		break;
	case 'q': //down
		playerY += 10;
		break;
	case 'e': //up
		playerY -= 10;
		break;

	//skybox on/off
	case '1':
		isSkybox = !isSkybox;
		break;

	//display only mesh on/off
	case '2':
		isLineMode = !isLineMode;
		break;

	//enable/disable sound
	case '3':
		isSound = !isSound;
		break;

	//decrease wind speed and generate new mesh
	case '4':
		if (wind_speed > 10) {
			wind_speed -= 10;
			delete ocean;
			delete[] oceanMesh;
			ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
			oceanMesh = ocean->generateMesh(&nOceanMesh);
		}
		break;

	//increase wind speed and generate new mesh
	case '5':
		wind_speed += 10;
		delete ocean;
		delete[] oceanMesh;
		ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
		oceanMesh = ocean->generateMesh(&nOceanMesh);
		break;

	//decrease wave height and generate new mesh
	case '6':
		if (A > 0.000000002) {
			A -= 0.000000001;
			delete ocean;
			delete[] oceanMesh;
			ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
			oceanMesh = ocean->generateMesh(&nOceanMesh);
		}
		break;

	//increase wave height and generate new mesh
	case '7':
		A += 0.000000001;
		delete ocean;
		delete[] oceanMesh;
		ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
		oceanMesh = ocean->generateMesh(&nOceanMesh);
		break;

	//decrease wave samples (quality), must be power of 2
	case '9':
		if (nx > 2 && ny > 2) {
			nx /= 2;
			ny /= 2;
			delete ocean;
			delete[] oceanMesh;
			ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
			oceanMesh = ocean->generateMesh(&nOceanMesh);
		}
		break;

	//increase wave samples (quality)
	case '0':
		nx *= 2;
		ny *= 2;
		delete ocean;
		delete[] oceanMesh;
		ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
		oceanMesh = ocean->generateMesh(&nOceanMesh);
		break;

	//decrease/increase view range
	case '-':
		cameraFar -= 100;
		resize(screen_width, screen_height);
		break;
	case '=':
		cameraFar += 100;
		resize(screen_width, screen_height);
		break;
	}
}
//-----------------------------------------------------------
void drawSkybox() {
	//open gl 2.0 usage
	//reset model view matrix
	//skybox is always in center
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix();
	
	//set proper camera position
	glRotatef(camRotY, 1, 0, 0);
	glRotatef(camRotX, 0, 1, 0);

	//align the sun to horizon
	glTranslatef(0, -0.22, 0);

	float sbSize = 2; //real skybox edge length is sbSize * 2
	glDepthMask(GL_FALSE); //disable z buffer
	glEnable(GL_TEXTURE_2D);

	//front side
	glBindTexture(GL_TEXTURE_2D, 102);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(-sbSize, -sbSize, -sbSize);
	glTexCoord2d(1, 0); glVertex3f(sbSize, -sbSize, -sbSize);
	glTexCoord2d(1, 1); glVertex3f(sbSize, sbSize, -sbSize);
	glTexCoord2d(0, 1); glVertex3f(-sbSize, sbSize, -sbSize);
	glEnd();
	//left
	glBindTexture(GL_TEXTURE_2D, 101);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(-sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 0); glVertex3f(-sbSize, -sbSize, -sbSize);
	glTexCoord2d(1, 1); glVertex3f(-sbSize, sbSize, -sbSize);
	glTexCoord2d(0, 1); glVertex3f(-sbSize, sbSize, sbSize);
	glEnd();
	//right
	glBindTexture(GL_TEXTURE_2D, 103);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(sbSize, -sbSize, -sbSize);
	glTexCoord2d(1, 0); glVertex3f(sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 1); glVertex3f(sbSize, sbSize, sbSize);
	glTexCoord2d(0, 1); glVertex3f(sbSize, sbSize, -sbSize);
	glEnd();
	//back
	glBindTexture(GL_TEXTURE_2D, 104);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 0); glVertex3f(-sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 1); glVertex3f(-sbSize, sbSize, sbSize);
	glTexCoord2d(0, 1); glVertex3f(sbSize, sbSize, sbSize);
	glEnd();
	//top
	glBindTexture(GL_TEXTURE_2D, 100);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(-sbSize, sbSize, -sbSize);
	glTexCoord2d(1, 0); glVertex3f(sbSize, sbSize, -sbSize);
	glTexCoord2d(1, 1); glVertex3f(sbSize, sbSize, sbSize);
	glTexCoord2d(0, 1); glVertex3f(-sbSize, sbSize, sbSize);
	glEnd();
	//bottom
	glBindTexture(GL_TEXTURE_2D, 105);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0); glVertex3f(-sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 0); glVertex3f(sbSize, -sbSize, sbSize);
	glTexCoord2d(1, 1); glVertex3f(sbSize, -sbSize, -sbSize);
	glTexCoord2d(0, 1); glVertex3f(-sbSize, -sbSize, -sbSize);
	glEnd();
	glPopMatrix();

	glDepthMask(GL_TRUE); //enable z buffer
}
//-----------------------------------------------------------
void draw()
{
	glutWarpPointer(wrapX, wrapY); //wrap mouse to window center
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(isSound) //play or pause sound
		mciSendString("resume mp3", NULL, 0, NULL);
	else
		mciSendString("pause mp3", NULL, 0, NULL);

	glUseProgram(0); //use open gl 2.0 default program
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if(isSkybox)
		drawSkybox();

	glUseProgram(programId); //use shader program to render tessnedorf waves
	if (isLineMode) { //set fragment shader to render only mesh lines
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		GLuint colorId = glGetUniformLocation(programId, "isColor");
		glUniform1i(colorId, 1);
	}
	else { //set fragment shader to render mesh with reflections
		GLuint colorId = glGetUniformLocation(programId, "isColor");
		glUniform1i(colorId, 0);
	}

	//set model view matrix to compute waves position relative to player position and camera rotation
	V = glm::mat4(1.0f);
	V = glm::rotate(V, (float)glm::radians(camRotY), glm::vec3(1, 0, 0));
	V = glm::rotate(V, (float)glm::radians(camRotX), glm::vec3(0, 1, 0));

	M = glm::mat4(1.0f);
	M = glm::translate(M, glm::vec3(playerX, playerY, playerZ));

	//send to fragment shader player/camera position
	GLuint camPosId = glGetUniformLocation(programId, "cameraPosition");
	glUniform3f(camPosId, -playerX, -playerY, -playerZ);
	
	//set cube map skybox in fragment shader to compute reflection on waves
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 106);
	GLuint cubeMapId = glGetUniformLocation(programId, "CubeMap");
	glUniform1i(cubeMapId, 0);

	//generate mesh height in particular time and pass it to shader
	ocean->setMeshHeight(oceanMesh, 0.6*glutGet(GLUT_ELAPSED_TIME)/1000.);
	int t = glutGet(GLUT_ELAPSED_TIME)/1000 % 60;
	glBindBuffer(GL_ARRAY_BUFFER, vboOcean);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * nOceanMesh, oceanMesh, GL_DYNAMIC_DRAW);

	//generate mesh normals in particular time and pass it to shader
	float *oceanNorm = ocean->generateNorm(oceanMesh);
	glBindBuffer(GL_ARRAY_BUFFER, vboOceanNorm);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * nOceanMesh, oceanNorm, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboOcean);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vboOceanNorm);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//get uniform location of mvp
	GLuint mId = glGetUniformLocation(programId, "M");
	GLuint vId = glGetUniformLocation(programId, "V");
	GLuint pId = glGetUniformLocation(programId, "P");
	
	//calculate every tile position and render it
	for (int x = 0; x < tiles; x++) {
		for (int y = 0; y < tiles; y++) {
			glm::mat4 M2 = glm::translate(M, glm::vec3(x*lx, 0, y*ly));
			
			glUniformMatrix4fv(mId, 1, GL_FALSE, &(M2[0][0]));
			glUniformMatrix4fv(vId, 1, GL_FALSE, &(V[0][0]));
			glUniformMatrix4fv(pId, 1, GL_FALSE, &(P[0][0]));

			for (int i = 0; i < ny; i++) {
				int n = nOceanMesh / (ny);
				glDrawArrays(GL_TRIANGLE_STRIP, i*n, n);
			}
			//glDrawArrays(GL_TRIANGLE_STRIP, 0, nOceanMesh);
		}
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	//wave mesh change only its height so we don't delete it but normals are not needed
	delete[] oceanNorm;

	glFlush();
	glutSwapBuffers();

}
//-----------------------------------------------------------
void setFps() { //calculate actual fps and reset frames counter
	static int timebase = 0;
	int t = glutGet(GLUT_ELAPSED_TIME);
	char buf[100];
	sprintf(buf, "%d", 1000 * frames / (t - timebase));
	glutSetWindowTitle(buf);
	timebase = t;
	frames = 0;
}
//-----------------------------------------------------------
void fps(int value) { //render frames with specified fpsMax speed
	draw();
	if(++frames == 10) setFps(); //show actual fps every 10 frames
	glutTimerFunc(1000 / fpsMax, fps, 0);
}

void nothing() {} //glutDisplayFunc need function which I don't use
//-----------------------------------------------------------
int main(int argc, char **argv)
{
	//open gl and window init
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(screen_width, screen_height);
	glutInitWindowPosition(0, 0);
	//glutEnterGameMode();
	glutCreateWindow("Tessendorf waves");

	glewInit();

	//set function to render and resize
	glutDisplayFunc(nothing);
	glutReshapeFunc(resize);
	glutTimerFunc(0, fps, 0);

	//set keyboard and mouse event function
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouse);

	glEnable(GL_DEPTH_TEST);
	glClearColor(1, 1, 1, 1);

	//hide cursor and wrap it in the center
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(wrapX, wrapY);

	//VAO
	GLuint VertexArrayId;
	glGenVertexArrays(1, &VertexArrayId);
	glBindVertexArray(VertexArrayId);

	//create program to render tessendorf waves
	programId = loadShaders("vertex_shader.glsl", "fragment_shader.glsl");
	glBindAttribLocation(programId, 0, "pos");
	glBindAttribLocation(programId, 1, "normal");
	glLinkProgram(programId);
	
	//create ocean and generate mesh without its height
	ocean = new Ocean(lx, ly, nx, ny, wind_speed, 0.1, A);
	oceanMesh = ocean->generateMesh(&nOceanMesh);

	//ocean mesh and norm VBO
	glGenBuffers(1, &vboOcean);
	glGenBuffers(1, &vboOceanNorm);
	
	//load texture for skybox
	textureBMP("skybox/skybox_top.bmp", 100);
	textureBMP("skybox/skybox_left.bmp", 101);
	textureBMP("skybox/skybox_front.bmp", 102);
	textureBMP("skybox/skybox_right.bmp", 103);
	textureBMP("skybox/skybox_back.bmp", 104);
	textureBMP("skybox/skybox_bottom.bmp", 105);

	//load texture of skybox to create reflection on waves
	textureBMP("skybox/skybox_bottom.bmp", 106, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
	textureBMP("skybox/skybox_left.bmp", 106, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
	textureBMP("skybox/skybox_back.bmp", 106, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	textureBMP("skybox/skybox_right.bmp", 106, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	textureBMP("skybox/skybox_front.bmp", 106, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
	textureBMP("skybox/skybox_top.bmp", 106, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

	//play mp3 sound
	mciSendString("open \"Seagull sounds.mp3\" type mpegvideo alias mp3", NULL, 0, NULL);
	mciSendString("play mp3 repeat", NULL, 0, NULL);

	glutMainLoop();
	return(0);
}
