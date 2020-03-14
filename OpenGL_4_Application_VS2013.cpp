//
//  main.cpp
//  OpenGL Shadows
//
//  Created by CGIS on 05/12/2019.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC

#include <random>
#include <iostream>
#include "glm/glm.hpp"//core glm functionality  
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include "Model3D.hpp"
#include "Mesh.hpp"

#include <chrono>
#include <thread>
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds


int glWindowWidth = 1920;
int glWindowHeight = 1000;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;


gps::Camera myCamera(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f,0.0f,0.0f));
glm::vec3 defaultStartPosition = glm::vec3(0.0f, 10.0f, 20.0f);
GLfloat cameraSpeed = 1.2f;

bool pressedKeys[1024];


gps::Shader myCustomShader;
gps::Shader clipShader;
gps::Shader shaderNoTexture;
gps::Shader depthMapShader;

float angle = 0.0f;

//for grass carpet
GLfloat vertexData[] = {
	5.0f, 0.0f, 0.0f,
	-5.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 5.0f,

};

GLuint verticesVBO;
GLuint objectVAO;

GLfloat fogDensity;
GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

//animation functions
glm::mat4 modelHeli;
void driveHelicopter(GLfloat x, GLfloat y, GLfloat z) {

	modelHeli = glm::translate(modelHeli, glm::vec3(x, y, z));

}


glm::mat4 modelDog;
void moveDog(GLfloat x, GLfloat y, GLfloat z) {

	modelDog = glm::translate(modelDog, glm::vec3(x, y, z));

}

glm::mat4 modelDogSpot;
void moveDogSpot(GLfloat x, GLfloat y, GLfloat z) {

	modelDogSpot = glm::translate(modelDogSpot, glm::vec3(x, y, z));

}


void rotateHelicopter(GLfloat l){

	modelHeli = glm::rotate(modelHeli, glm::radians(l*1.0f), glm::vec3(0, 0, 1));
}
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

double lastX;
double lastY;

double yaw = 0.0f, pitch = 0.0f;
int state = 0;
bool first = true;
bool onlyAnimation = false;
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (onlyAnimation == false) {
		if (first){

			lastX = xpos;
			lastY = ypos;
			first = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		float sensitivity = 0.6f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 179.0f){
			pitch = 0.0f;
		}
		if (pitch < -179.0f){
			pitch = 0.0f;
		}

		myCamera.rotate(pitch, yaw);
	}
}

bool checkForAdequatePosition(glm::vec3 pos) {

	if (pos.y < 0.7f)
		return false;
	if ((pos.x < 6 && pos.x>-6) && (pos.y < 15 && pos.y>-15) && (pos.z<6 && pos.z>-6))
		return false;

	if ((pos.x < 40 && pos.x>0) && (pos.y <15 && pos.y>0) && (pos.z<90 && pos.z>60))
		return false;
	
	return true;
}

glm::vec3 pointLight;

glm::vec3 lightDir;
glm::vec3 lightDirTr;

GLfloat lightAngle=0;
GLfloat t = 1.0f;

//bool bal_movement_enabled=false;
bool cameraAnimationTrue = false;
bool rainAnimationTrue = false;
bool canMove = false;
bool dogAnimation = false;
void processMovement()
{
	gps::Camera current = myCamera;

	if (onlyAnimation == false) {
		if (pressedKeys[GLFW_KEY_W]) {

			current = myCamera;  
			current.move(gps::MOVE_FORWARD, cameraSpeed); 
			if (checkForAdequatePosition(current.getCameraPosition()) == true){
				myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
			}
		}

		if (pressedKeys[GLFW_KEY_S]) {
			current = myCamera;
			current.move(gps::MOVE_BACKWARD, cameraSpeed);
			if (checkForAdequatePosition(current.getCameraPosition()) == true){
				myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
			}
		}

		if (pressedKeys[GLFW_KEY_A]) {
			current = myCamera;
			current.move(gps::MOVE_LEFT, cameraSpeed);
			if (checkForAdequatePosition(current.getCameraPosition()) == true){
				myCamera.move(gps::MOVE_LEFT, cameraSpeed);
			}
		}

		if (pressedKeys[GLFW_KEY_D]) {
			current = myCamera;
			current.move(gps::MOVE_RIGHT, cameraSpeed);
			if (checkForAdequatePosition(current.getCameraPosition()) == true){
				myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
			}
		}
	}
	if (pressedKeys[GLFW_KEY_Q]) {
		
		lightAngle += 2.0f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDirTr));
		clipShader.useShaderProgram();
		glUniform3fv(glGetUniformLocation(clipShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDirTr));
		shaderNoTexture.useShaderProgram();
		glUniform3fv(glGetUniformLocation(shaderNoTexture.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDirTr));
	}
	//helicopter move up
	if (pressedKeys[GLFW_KEY_U]) {
		driveHelicopter(0, 0.0f, 0.5f);
	}
	//helicopter move dow
	if (pressedKeys[GLFW_KEY_M]) {
		driveHelicopter(0, 0, -0.5f);
	}

	//helicopter move front
	if (pressedKeys[GLFW_KEY_H]) {
		driveHelicopter(0, 0.5f, 0.0f);
	}
	//helicopter move back
	if (pressedKeys[GLFW_KEY_K]) {
		driveHelicopter(0, -0.5f, 0.0f);
	}
	//helicopter rotate right
	if (pressedKeys[GLFW_KEY_L]) {
		rotateHelicopter(2);
	}
	//dog move forward
	if (pressedKeys[GLFW_KEY_P]) {
		/*if (modelDog[3][0] < 117)
		{
			moveDog(0, -20.0f, 0);
		}
		else
		{
			if (modelDog[3][1] > 1)
			{
				moveDog(0, 0, -10.0f);
			}
		} */
		dogAnimation = true;
	}
	if (pressedKeys[GLFW_KEY_O]) {
		rotateHelicopter(1);
	}
	if (pressedKeys[GLFW_KEY_1]) {

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (pressedKeys[GLFW_KEY_2]) {

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (pressedKeys[GLFW_KEY_3]) {

		rainAnimationTrue = true;

	}
	if (pressedKeys[GLFW_KEY_4]) {

		rainAnimationTrue = false;

	}
	if (pressedKeys[GLFW_KEY_5]) {

		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	//try a camera animation
	if (pressedKeys[GLFW_KEY_C]) {
		myCamera = gps::Camera(glm::vec3(10.0f, 3.0f, -50.0f), glm::vec3(10.0f, 3.0f, 0.0f));
		cameraAnimationTrue = true;
	}
	if (pressedKeys[GLFW_KEY_V]) {
		myCamera = gps::Camera(glm::vec3(10.0f, 3.0f, -50.0f), glm::vec3(10.0f, 3.0f, 0.0f));
		cameraAnimationTrue = false;
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "Project", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	//glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

GLuint textureID;
std::vector<const GLchar*> faces;
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;
void initSkyBox() {

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	faces.push_back("skybox/right_.tga");
	faces.push_back("skybox/left_.tga");
	faces.push_back("skybox/gtop.jpg");
	faces.push_back("skybox/bottom_.tga");
	faces.push_back("skybox/back_.tga");
	faces.push_back("skybox/front_.tga");


	mySkyBox.Load(faces);
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));


}

void initGrass() {

	glGenVertexArrays(1, &objectVAO);
	glBindVertexArray(objectVAO);

	glGenBuffers(1, &verticesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	//vertex position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

gps::Model3D castle;
gps::Model3D tree;
gps::Model3D fortress;
gps::Model3D house;
gps::Model3D fence;
gps::Model3D maki;
gps::Model3D prim;
gps::Model3D snow_house;
gps::Model3D grass;
gps::Model3D ground;
gps::Model3D rock;
gps::Model3D tree2;
gps::Model3D tree3;
gps::Model3D tree4;
gps::Model3D heli;
gps::Model3D heli2;
gps::Model3D well;
gps::Model3D tennis;
gps::Model3D balloon;
gps::Model3D balloon2;
gps::Model3D balloon3;
gps::Model3D balloon_under;
gps::Model3D eagle;
gps::Model3D lightCube;
gps::Model3D bridge;
gps::Model3D towers;
gps::Model3D lamp;
gps::Model3D drop;
gps::Model3D dog;
gps::Model3D bed;
gps::Model3D slide;
gps::Model3D dogSpot;

void initModels()
{
	tree = gps::Model3D("objects/mytree/Tree.obj", "objects/mytree/");
	fortress = gps::Model3D("objects/fortress/saintriqT3DS.obj", "objects/fortress/");
	house = gps::Model3D("objects/house/gate_obj.obj", "objects/house/");
	fence = gps::Model3D("objects/fence/objFence.obj", "objects/fence/");
	maki = gps::Model3D("objects/flowers/rose.obj", "objects/flowers/");
	prim = gps::Model3D("objects/flowers/PrimroseP.obj", "objects/flowers/");
	snow_house = gps::Model3D("objects/bad_house/house_obj.obj", "objects/bad_house/");
	grass = gps::Model3D("objects/realgrass/patch2.obj", "objects/realgrass/");
	ground = gps::Model3D("objects/ground/ground.obj", "objects/ground/");
	rock = gps::Model3D("objects/well/rocks_01_model.obj", "objects/well/");
	tree2 = gps::Model3D("objects/mytree/tree2/Tree.obj", "objects/mytree/tree2/");
	tree3 = gps::Model3D("objects/mytree/tree3/Tree.obj", "objects/mytree/tree3/");
	tree4 = gps::Model3D("objects/mytree/tree4/Tree.obj", "objects/mytree/tree4/");
	heli = gps::Model3D("objects/heli/try.obj", "objects/heli/");
	heli2 = gps::Model3D("objects/helicopter/wheel.obj", "objects/helicopter/");
	well = gps::Model3D("objects/well/well.obj", "objects/well/");
	tennis = gps::Model3D("objects/bridge/woood.obj", "objects/bridge/");
	balloon = gps::Model3D("objects/balloon/part1.obj", "objects/balloon/");
	balloon2 = gps::Model3D("objects/balloon/balloon2/part1_green.obj", "objects/balloon/balloon2/");
	balloon3 = gps::Model3D("objects/balloon/balloon3/part1_orange.obj", "objects/balloon/balloon3/");
	balloon_under = gps::Model3D("objects/balloon/part2.obj", "objects/balloon/");
	eagle = gps::Model3D("objects/eagle/hummingbird.obj", "objects/eagle/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");
	bridge = gps::Model3D("objects/bridge/mybridge.obj", "objects/bridge/");
	towers = gps::Model3D("objects/tower/butter.obj", "objects/tower/");
	lamp = gps::Model3D("objects/drone/rv_lamp_post_3.obj", "objects/drone/");
	drop = gps::Model3D("objects/drone/drop.obj", "objects/drone/");
	dog = gps::Model3D("objects/dog/dog.obj", "objects/dog/");
	bed = gps::Model3D("objects/bed/bed.obj", "objects/bed/");
	slide = gps::Model3D("objects/slide/slide.blend", "objects/slide/");
	dogSpot = gps::Model3D("objects/floor/floor.obj", "objects/floor/");
}

gps::Shader lightShader;
gps::Shader rainShader;
void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	clipShader.loadShader("shaders/shaderClip.vert", "shaders/shaderClip.frag");
	shaderNoTexture.loadShader("shaders/shaderNoTexture.vert", "shaders/shaderNoTexture.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
	rainShader.loadShader("shaders/rainShader.vert", "shaders/rainShader.frag");
}

void sendNormalMatrix(gps::Shader shader) {
	shader.useShaderProgram();
	glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	GLuint normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = -300.0f, far_plane = 300.0f;
	glm::mat4 lightProjection = glm::ortho(-300.0f, 300.0f, -300.0f, 300.0f, near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

GLuint shadowMapFBO;
GLuint depthMapTexture;
void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void initUniforms()
{
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");

	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

	lightDir = glm::vec3(0.0f, 10.0f, 20.0f);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));

	pointLight = glm::vec3(10.0f, 1.0f, 60.0f);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "pointLight"), 1, glm::value_ptr(pointLight));  
	clipShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(clipShader.shaderProgram, "pointLight"), 1, glm::value_ptr(pointLight));

	myCustomShader.useShaderProgram();
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	clipShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(clipShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
	glUniform3fv(glGetUniformLocation(clipShader.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));
	glUniform3fv(glGetUniformLocation(clipShader.shaderProgram, "pointLight"), 1, glm::value_ptr(pointLight));
	shaderNoTexture.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(shaderNoTexture.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(shaderNoTexture.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
	glUniform3fv(glGetUniformLocation(shaderNoTexture.shaderProgram, "lightDir"), 1, glm::value_ptr(lightDir));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


GLfloat tx = 0.0f, ty = 0.0f;
float b_rot1 = 0.0f, b_rot2 = 0.0f, b_rot3 = 0.0f;
float max1=10.0f, max2=7.0f, max3=12.0f;

bool ok_to_move_right1 = false , ok_to_move_left1 = false;
bool ok_to_move_right2 = false, ok_to_move_left2 = false;
bool ok_to_move_right3 = false, ok_to_move_left3 = false;
bool first1 = true, first2 = true, first3 = true;
void wait() {

	for (int i = 0; i <= 100; i++) { }
}

float cam_tx = 0, cam_ty = 0, cam_tz = 0, cam_tx2 = 0, cam_tx3 = 0, cam_tz2 = 0;
float cam_rot = 0;

bool goFront(float times) {

	
	if (cam_tx >=times) {
		return true;  
	}
	else {
		onlyAnimation = true;
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_tx++;
		return false;
	}
	
}
bool goFront2(float times) {


	if (cam_tx2 >= times) {
		return true;
	}
	else {
		onlyAnimation = true;
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_tx2++;
		return false;
	}

}
bool goFront3(float times) {


	if (cam_tx3 >= times) {
		return true;
	}
	else {
		onlyAnimation = true;
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_tx3++;
		return false;
	}

}
bool goUp(float times) {
	if (cam_ty>=times) {
		//cam_rot = 0;
		return true;
	}
	else {
		//myCamera.rotate(cam_rot, 0.0f);
		myCamera.rotate(30, 0.0f);
		//cam_rot++;
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_ty=cam_ty+0.4f;
		return false;
	}
}

bool goDown(float times) {
	if (cam_ty >= times) {
		cam_rot = 0;
		return true;
	}
	else {
		myCamera.rotate(360.0f-cam_rot, 0.0f);
		cam_rot++;
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_ty++;
		return false;
	}
}
bool goRight(float times) {

	if (cam_tz2 >= times) {
		cam_rot = 0;
		return true;
	}
	else {
		cam_rot=cam_rot+2;
		myCamera.rotate(0.0f, 90.0f + cam_rot);
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_tz2++;
		return false;
	}
}
bool goLeft(float times) {

	if (cam_tz >= times) {
		cam_rot = 0;
		return true;
	}
	else {
		cam_rot = cam_rot + 2;
		myCamera.rotate(0.0f, 360.0f-cam_rot);
		myCamera.move(gps::MOVE_FORWARD, 1);
		cam_tz++;
		return false;
	}
}
void cameraAnimation() {

	bool front = goFront(95);
	if (front == true) {
		bool left = goLeft(3);
				if (left == true) {
					wait();
					front = goFront2(60);
					if (front == true) {
						bool right = goRight(3);
						if (right == true) {
							bool front = goFront3(50);
							if (front == true) {
								bool up = goUp(20);
								if (up == true)
								{
									myCamera = gps::Camera(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f));
									cam_tx = 0;
									cam_tx2 = 0;
									cam_tx3 = 0;
									cam_ty = 0;
									cam_tz = 0;
									cam_tz2 = 0;
									cameraAnimationTrue = false;
									onlyAnimation = false;
								}
							}
						}
					}
				}
			} 
	
}



void drawDoggo() {
	int y = 0;
	int z = 0;
	GLfloat moveDist = 0;
	GLfloat moveDogSpotDist = 0;
	//myCustomShader.useShaderProgram();
	if (modelDog[3][0] < 117)
	{
		moveDist = moveDist - 15;
		y = moveDist;
		z = 0;
	}
	else if (modelDog[3][1] > 3)
	{
		moveDist = moveDist - 5;
		z = moveDist;
		y = 0;

		moveDogSpotDist = moveDogSpotDist - 5;
	}
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(0,moveDist,0));
	moveDog(0, y, z);
	moveDogSpot(0, moveDogSpotDist, 0);
	//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
	//sendNormalMatrix(myCustomShader);
	//dog.Draw(myCustomShader);
}


struct flowerPos {

	int type;
	int i;
	int j;
};

struct dropPos {

	int  x;
	int  y;
	int  z;
	int ty;
};
flowerPos a[200];

dropPos rainDrops[400];
void generateDropPositions() {

	for (int i = 0; i < 400; i++) {

		rainDrops[i].ty = 0;
		rainDrops[i].x = rand() % 80;
		rainDrops[i].y = rand() % 60 + 20;
		rainDrops[i].z = rand() % 80;
		int sign1 = rand() % 2;
		int sign2 = rand() % 2;
		if (sign1 == 1) rainDrops[i].x = -rainDrops[i].x;
		if (sign2 == 1) rainDrops[i].z = -rainDrops[i].z;  
	}
}


void drawRain() {
	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "tr"), 0.3f);
	for (int i = 0; i < 400; i++) {
		if ((rainDrops[i].y - rainDrops[i].ty) == 0) rainDrops[i].ty = 0;
		else rainDrops[i].ty = rainDrops[i].ty + 2;;
		model = glm::translate(glm::mat4(1.0f), glm::vec3(rainDrops[i].x, rainDrops[i].y-rainDrops[i].ty, rainDrops[i].z));
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));  
		drop.Draw(rainShader);
	}  
}

void generateFlowerPositions() {

	for (int k = 0; k < 200; k++) {

		a[k].type = rand()%2;
		a[k].i = rand() % 30;
		a[k].j = rand() % 20;
		int sign1 = rand() % 2;
		int sign2 = rand() % 2;
		if (sign1 == 0) a[k].i = -a[k].i;
		if (sign2 == 0) a[k].j = -a[k].j;
	}
}

void drawAlotOfFlowers() {

	myCustomShader.useShaderProgram();
	for (int i = 0; i < 200; i++) {
			model = glm::translate(glm::mat4(1.0f), glm::vec3(a[i].i, 0, a[i].j));
			glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
			if (a[i].type == 0) {
				model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				sendNormalMatrix(myCustomShader);
				maki.Draw(myCustomShader);
			}
			else
				model = glm::scale(model, glm::vec3(2.1f, 2.1f,2.1f));
			      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				  sendNormalMatrix(myCustomShader);
				prim.Draw(myCustomShader);  
		}
}



void drawFenceForSpring() {

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(11, 0, 20));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-11, 0, 20));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(22, 0, 20));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-22, 0, 20));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(28, 0, 14));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-28, 0, 14));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(28, 0, 2));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-28, 0, 2));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(28, 0, -10));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-28, 0, -10));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	model = glm::rotate(model, -1.57f, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(11, 0, -16));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-11, 0, -16));
	//model = glm::scale(model, glm::vec3(2, 2, 2));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(22, 0, -16));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-22, 0, -16));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);

	//draw fence
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -16));
	//model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fence.Draw(myCustomShader);
}

void drawGrass() {

	for (int i = -100; i <= 200; i=i+10)
	for (int j = -100; j <= 200; j =j+10) {

		model = glm::translate(glm::mat4(1.0f), glm::vec3(i, 0, j));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		sendNormalMatrix(myCustomShader);
		ground.Draw(myCustomShader);
	}
}

void drawOutterTrees( glm::mat4 view, gps::Shader shader) {

	shader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(40, 0, 20));
	model = glm::scale(model, glm::vec3(3, 3, 3));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(50, 0, 32));
	model = glm::scale(model, glm::vec3(3, 3, 3));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-50, 0, 43));
	model = glm::scale(model, glm::vec3(3, 3, 3));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-32, 0, 43));
	model = glm::scale(model, glm::vec3(3, 3, 3));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-50, 0, -30));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree3.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(35, 0, 60));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(38, 0, -40));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree3.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(45, 0, -30));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree4.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(50, 0, 60));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree4.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-39, 0, -47));
	model = glm::scale(model, glm::vec3(5, 5, 5));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree4.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(18,0,-50));
	model = glm::scale(model, glm::vec3(5, 5, 5));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree2.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(3, 0, 44));
	model = glm::scale(model, glm::vec3(5, 5, 5));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree3.Draw(shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-2, 0, -75));
	model = glm::scale(model, glm::vec3(5, 5, 5));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(shader);
	tree3.Draw(shader);

}

float rot = 0.0f;

GLfloat fogDen;

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	processMovement();
	double xpos, ypos;
	glfwGetCursorPos(glWindow, &xpos, &ypos);
	mouseCallback(glWindow, xpos, ypos);

	depthMapShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	/*for (int i = -100; i <= 200; i = i + 10)
	for (int j = -100; j <= 200; j = j + 10) {

		model = glm::translate(glm::mat4(1.0f), glm::vec3(i, 0, j));
		glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		//sendNormalMatrix(depthMapShader);
		//ground.Draw(depthMapShader);
	}*/

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.07, 0.07, 0.07));
	model = glm::translate(model, glm::vec3(-800, 0, 1));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),1, GL_FALSE, glm::value_ptr(model));
	rock.Draw(depthMapShader);

	

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDog));
	dog.Draw(depthMapShader);

	drawOutterTrees(view, depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	clipShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	if (cameraAnimationTrue) {

		cameraAnimation();
	}

	view = myCamera.getViewMatrix();
	myCustomShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));

	//compute light direction transformation matrix
	glm::mat3 lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	clipShader.useShaderProgram();
	glUniformMatrix3fv(glGetUniformLocation(clipShader.shaderProgram, "lightDirMatrix"), 1, GL_FALSE, glm::value_ptr(lightDirMatrix));
	myCustomShader.useShaderProgram();
	glUniformMatrix3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix"), 1, GL_FALSE, glm::value_ptr(lightDirMatrix));


	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

	clipShader.useShaderProgram();
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(clipShader.shaderProgram, "shadowMap"), 3);

	/*DRAW GRASS*/
	//draw grass
	myCustomShader.useShaderProgram();
	glm::mat4 view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));

	model = glm::translate(model, glm::vec3(0, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-7, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(14, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(7, 0, 5));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(7, 0, 5));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-14, 0, -10));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-7, 0, -5));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-7, 0, -5));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-7, 0, -5));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(model, glm::vec3(-7, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(7, 0, 5));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(7, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-7, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-14, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-14, 0, 0));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+14, 0, -5));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+14, 0, -10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+21, 0, -10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+21, 0, -5));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+21, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(+21, 0, 5));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-21, 0, 10));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-21, 0, 5));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	grass.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::mat4(1.0f);
	myCustomShader.useShaderProgram();
	//send fog once
	fogDen = 0.008;
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram,"den"), fogDen);


	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	drawGrass();

	//draw rocks
	fogDen = 0.004;
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "den"), fogDen); 

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.07, 0.07, 0.07));
	model = glm::translate(model, glm::vec3(-800, 0, 1));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	rock.Draw(myCustomShader);  


	//draw helicopter
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram,"model"), 1, GL_FALSE, glm::value_ptr(modelHeli));
	sendNormalMatrix(myCustomShader);
	heli.Draw(myCustomShader);
	model = glm::rotate(modelHeli, (GLfloat)glm::radians(rot), glm::vec3(0, 0, 1));
	rot+=20;
	if (rot == 360) rot = 0;
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	//heli2.Draw(myCustomShader);

	//draw well
	myCustomShader.useShaderProgram();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(40, 0, 50));
	model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	well.Draw(myCustomShader);

/*	//draw tennis
	myCustomShader.useShaderProgram();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(50, 1, 50));
	//model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	tennis.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-50, 1, 30));
	//model = glm::scale(model, glm::vec3(4, 4, 4));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	tennis.Draw(myCustomShader); */


	/*moveBalloon(balloon, balloon_under, glm::vec3(75, 0,50), ok_to_move_right1, ok_to_move_left1, b_rot1, max1, 0.1f, first1);

	moveBalloon(balloon2, balloon_under, glm::vec3(-80, 0, 55), ok_to_move_right2, ok_to_move_left2, b_rot2, max2, 0.15f, first2);

	moveBalloon(balloon3, balloon_under, glm::vec3(80, 0, -50), ok_to_move_right3, ok_to_move_left3, b_rot3, max3, 0.2f, first3); */
	//draw outter trees

	drawOutterTrees(view, clipShader);  


	/*PART1: DRAW SPRING SCENE*/
	//draw fortresss
	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(1.5f, 1.2f, 1.2f));
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	fortress.Draw(myCustomShader);


	//draw tree1
	clipShader.useShaderProgram();

	model = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
	model = glm::translate(model, glm::vec3(3, -0.5, -3));
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	sendNormalMatrix(clipShader);
	tree.Draw(clipShader);

	model = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
	model = glm::translate(model, glm::vec3(-4, -0.5, 1));
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(clipShader);
	tree.Draw(clipShader);

	model = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
	model = glm::translate(model, glm::vec3(-2, -0.5, -3));
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(clipShader);
	tree.Draw(clipShader);
	model = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
	model = glm::translate(model, glm::vec3(5, -0.5, 1));
	glUniformMatrix4fv(glGetUniformLocation(clipShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(clipShader);
	tree.Draw(clipShader);

	//draw entry
	myCustomShader.useShaderProgram();
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 20));
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	house.Draw(myCustomShader);

	//draw dog
	myCustomShader.useShaderProgram();
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
	sendNormalMatrix(myCustomShader);
	dog.Draw(myCustomShader); 

	

	/*//draw slide
	myCustomShader.useShaderProgram();
	model = glm::translate(glm::mat4(1.0f), glm::vec3(120, 40, 100));
	//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	slide.Draw(myCustomShader); */

	//draw floor
	//myCustomShader.useShaderProgram();
	clipShader.useShaderProgram();
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDogSpot));
	sendNormalMatrix(clipShader);
	dogSpot.Draw(clipShader);

	/*//draw bed
	//myCustomShader.useShaderProgram();
	model = glm::translate(glm::mat4(1.0f), glm::vec3(120, 0, 100));
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	//model = glm::rotate(model, (GLfloat)glm::radians(90.0f), glm::vec3(0, 0, 1));
	//model = glm::rotate(model, (GLfloat)glm::radians(90.0f), glm::vec3(1, 0, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	bed.Draw(myCustomShader);
	*/

	//draw flowers
	drawAlotOfFlowers();

	//draw fence
	drawFenceForSpring();

	
	model = glm::translate(glm::mat4(1.0f), glm::vec3(20, 0, 80));
	model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
	model = glm::rotate(model, (GLfloat)glm::radians(180.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	sendNormalMatrix(myCustomShader);
	snow_house.Draw(myCustomShader);


	shaderNoTexture.useShaderProgram();
	model = glm::translate(glm::mat4(1.0f), glm::vec3(20,0, 10));
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	model = glm::rotate(model, glm::radians(rot), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(shaderNoTexture.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	towers.Draw(shaderNoTexture);

	myCustomShader.useShaderProgram();
	//DRAW BRIDGE
	model = glm::translate(glm::mat4(1.0f), glm::vec3(20, 0, 100));
	//model = glm::scale(model, glm::vec3(0.03f, 0.03f, 0.03f));
	//model = glm::rotate(model, (GLfloat)glm::radians(180.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	bridge.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(60, 0, 100));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	bridge.Draw(myCustomShader);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-20, 0, 100));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	bridge.Draw(myCustomShader);

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::translate(model, glm::vec3(10*lightDir.x, 10*lightDir.y, 10*lightDir.z));
	//model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	lightCube.Draw(lightShader); 

	/*rainShader.useShaderProgram();
	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(pointLight));
	glUniformMatrix4fv(glGetUniformLocation(rainShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1f(glGetUniformLocation(rainShader.shaderProgram, "tr"), 0.5f);
	drop.Draw(rainShader);*/

	if (rainAnimationTrue == true) {

		drawRain();
	}

	if (dogAnimation == true) {
		drawDoggo();
	}

	/*PART 3: DRAW SKY BOX*/
	skyboxShader.useShaderProgram();
	mySkyBox.Draw(skyboxShader, view, projection);
}



void initHeli() {

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(20, 30, 0));
	model = glm::rotate(model, (GLfloat)glm::radians(-90.0f), glm::vec3(1, 0, 0));
	model = glm::scale(model, glm::vec3(2, 2, 2));
	modelHeli = model;
}

void initDog() {
	model = glm::translate(glm::mat4(1.0f), glm::vec3(-108, 41.3f, 100));
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	model = glm::rotate(model, (GLfloat)glm::radians(90.0f), glm::vec3(0, 0, 1));
	model = glm::rotate(model, (GLfloat)glm::radians(90.0f), glm::vec3(0, 1, 0));
	modelDog = model;
}

void initDogSpot() {
	model = glm::translate(glm::mat4(1.0f), glm::vec3(117, 41, 100));
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	modelDogSpot = model;
}

/*void initFog() {
	fogDensity = 0.005f;
	myCustomShader.useShaderProgram();
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "dens"), fogDensity);
	clipShader.useShaderProgram();
	glUniform1f(glGetUniformLocation(clipShader.shaderProgram, "dens"), fogDensity);
}*/

int main(int argc, const char * argv[]) {

	glClearColor(0.5, 0.5, 0.5, 1.0);

	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initHeli();
	initDog();
	initDogSpot();
	initModels();
	initShaders();
	initUniforms();
	initSkyBox();
	glCheckError();
	lightDirTr = lightDir;
	generateFlowerPositions();
	generateDropPositions();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}