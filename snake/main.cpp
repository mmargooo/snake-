#include <iostream>
#include <algorithm>
#define GLEW_STATIC
#include <gl/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/functions.hpp>
#include <time.h>"

#include "lodepng.h"
#include "Shader.h"

// game objects
#include "Snake.h"
#include "Food.h"
#include "Obstacle.h"

// models
#include "Cube.h"
#include "Stone.h"
#include "Fruit.h"
#include "Mushroom.h"
#include "Flower.h"

const GLuint WIDTH = 800, HEIGHT = 600;
float aspect = 800.0 / 600.0;

GLuint vbo_vertices, vbo_texture, vbo_normals;
GLuint vbo_stone_vertices, vbo_stone_texture, vbo_stone_normals;
GLuint vbo_fruit_vertices, vbo_fruit_texture, vbo_fruit_normals;
GLuint vbo_mushroom_vertices, vbo_mushroom_texture, vbo_mushroom_normals;
GLuint vbo_flower_vertices, vbo_flower_texture, vbo_flower_normals;

GLuint vao, lightVAO;
GLuint vao_stone;
GLuint vao_fruit;
GLuint vao_mushroom;
GLuint vao_flower;

GLuint tex_snake, tex_grass, tex_stone, tex_fruit, tex_box, tex_mushroom, tex_flower;
Shader * shader, *lampShader;

// left, up, right, down, escape
bool keys[] = {false, false, false, false, false};
bool keyPressed = false;

// timing
float timeInt = 0.25f;

float angle = 0.0f;

int boardWidth = 15;
int boardHeight = 15;
float mapRadius = 6.0f;

// some of the lighting properties
int lightMode = 0;
float dir_diffuse = 0.4f;
float dir_specular = 0.75f;
float point_diffuse = 0.75f;
float point_specular = 0.55f;
float spotilight_diffuse = 0.0f;
float spotilight_specular = 0.0f;

std::vector<Food> food;
std::vector<Obstacle> obstacle;
int numOfFood = 0;

void initOpenGL(GLFWwindow* window);
void prepareObjects();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void clearKeys();
GLuint readTexture(char* filename);
void windowResize(GLFWwindow* window, int width, int height);
void generateObstacles(Snake snake, float mapRadius, int numOfStones, int numOfMushrooms, int numOfFlowers);
void genFood(Snake snake);
void changeLightProperties();

int main()
{
	srand(time(NULL));

	glfwInit();
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Snake", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	Snake snake;

	generateObstacles(snake, mapRadius, 3, 3, 2);
	for (int i = 0; i < 3; i++) {
		genFood(snake);
	}

	initOpenGL(window);
	glfwSetTime(0);

	// game loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// time calculation
		float currentFrame = glfwGetTime();
		float ratio = currentFrame / timeInt;

		// point light movement
		float x = sin(angle) * 7.0f;
		float z = cos(angle) * 7.0f;
		angle += 0.002;

		glm::vec3 lightPos(x, 5.0f, z);
		glm::vec3 cameraPos(0.0f, 15.0f, 10.0f);

		shader->use();
		shader->setUnifVec3("directionalLight.direction", glm::vec3(5.0f, 10.0f, 5.0f));
		shader->setUnifVec3("directionalLight.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
		shader->setUnifVec3("directionalLight.diffuse", glm::vec3(dir_diffuse, dir_diffuse, dir_diffuse));
		shader->setUnifVec3("directionalLight.specular", glm::vec3(dir_specular, dir_specular, dir_specular));

		shader->setUnifVec3("pointLight.position", lightPos);
		shader->setUnifVec3("pointLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		shader->setUnifVec3("pointLight.diffuse", glm::vec3(point_diffuse, point_diffuse, point_diffuse));
		shader->setUnifVec3("pointLight.specular", glm::vec3(point_specular, point_specular, point_specular));

		shader->setUnifVec3("spotLight.position", snake.getHead() + glm::vec3(0.0f, 10.0f, 0.0f));
		shader->setUnifVec3("spotLight.direction", -glm::vec3(0.0f, 10.0f, 0.0f));
		shader->setUnifVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		shader->setUnifVec3("spotLight.diffuse", glm::vec3(spotilight_diffuse, spotilight_diffuse, spotilight_diffuse));
		shader->setUnifVec3("spotLight.specular", glm::vec3(spotilight_specular, spotilight_specular, spotilight_specular));
		glUniform1f(shader->getUniformLocation("spotLight.cutOff"), glm::cos(glm::radians(1.0f)));
		glUniform1f(shader->getUniformLocation("spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));

		glUniform1i(shader->getUniformLocation("textureMap"), 0);
		glUniform1f(shader->getUniformLocation("shininess"), 16);

		glm::mat4 P = glm::perspective(50.0f * 3.1415f / 180.0f, aspect, 1.0f, 50.0f);
		glm::mat4 V = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 M = glm::mat4(1.0f);
		shader->setUnifMat4("P", P);
		shader->setUnifMat4("V", V);
		shader->setUnifMat4("M", M);

		if (glfwGetTime() > timeInt) {
			snake.changeDirection(keys);
			snake.checkCollision(mapRadius, &food, &obstacle, &numOfFood);
			if (numOfFood < 3) {
				genFood(snake);
			}
			snake.move();
			clearKeys();
			changeLightProperties();
			glfwSetTime(0);
		}

		// draw background
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_grass);
		glBindVertexArray(vao);
		for (int i = 0; i < boardWidth; i++) {
			for (int j = 0; j < boardHeight; j++) {
				M = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f*(-boardWidth / 2 + i), 0.0f, 1.0f*(-boardHeight / 2 + j)));
				shader->setUnifMat4("M", M);
				glDrawArrays(GL_TRIANGLES, 0, cube_numOfTriangles);
			}
		}
		glBindVertexArray(0);

		// draw boxes
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_box);
		glBindVertexArray(vao);
		for (int i = 0; i < boardWidth; i++) {
			for (int j = 0; j < boardHeight; j++) {
				if (i == 0 || j == 0 || i == boardWidth - 1 || j == boardHeight - 1) {
					M = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f*(-boardWidth / 2 + i), 1.0f, 1.0f*(-boardHeight / 2 + j)));
					shader->setUnifMat4("M", M);
					glDrawArrays(GL_TRIANGLES, 0, cube_numOfTriangles);
				}
			}
		}

		// draw snake
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_snake);
		glBindVertexArray(vao);
		for (int i = 0; i < snake.elem.size(); i++) {
			glm::vec3 adj = snake.getElem(i);
			M = glm::translate(glm::mat4(1.0f), snake.getElem(i));
			shader->setUnifMat4("M", M);
			glDrawArrays(GL_TRIANGLES, 0, cube_numOfTriangles);
		}
		glBindVertexArray(0);

		// draw food
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_fruit);
		glBindVertexArray(vao_fruit);
		for (int i = 0; i < food.size(); i++) {
			food[i].angle += 0.005;
			M = glm::translate(glm::mat4(1.0f), food[i].position);
			M = glm::scale(M, glm::vec3(0.35f, 0.35f, 0.35f));
			M = glm::rotate(M, food[i].angle, glm::vec3(0.0f, 1.0f, 0.0f));
			M = glm::translate(M, glm::vec3(0.0f, (sin(food[i].angle) - 0.5f)*0.25f, 0.0f));
			shader->setUnifMat4("M", M);
			glDrawArrays(GL_TRIANGLES, 0, fruit_numOfTriangles);
		}
		glBindVertexArray(0);

		// draw stones
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_stone);
		glBindVertexArray(vao_stone);
		for (int i = 0; i < obstacle.size(); i++) {
			if (obstacle[i].type == 0) {
				M = glm::translate(glm::mat4(1.0f), obstacle[i].position);
				M = glm::rotate(M, obstacle[i].angle, glm::vec3(0.0f, 1.0f, 0.0f));
				M = glm::translate(M, glm::vec3(0.0f, -0.5f, 0.0f));
				M = glm::scale(M, glm::vec3(0.85f, 0.85f, 0.85f));
				shader->setUnifMat4("M", M);
				glDrawArrays(GL_TRIANGLES, 0, stone_numOfTriangles);
			}
		}
		glBindVertexArray(0);

		// draw mushroom
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_mushroom);
		glBindVertexArray(vao_mushroom);
		for (int i = 0; i < obstacle.size(); i++) {
			if (obstacle[i].type == 1) {
				M = glm::translate(glm::mat4(1.0f), (glm::vec3(0.0f, 0.0f, 0.0f) - obstacle[i].position) * glm::vec3(-0.95f, -0.95f, -0.95f));
				M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.175f));
				shader->setUnifMat4("M", M);
				glDrawArrays(GL_TRIANGLES, 0, mushroom_numOfTriangles);
			}
		}
		glBindVertexArray(0);

		// draw flower
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_flower);
		glBindVertexArray(vao_flower);
		for (int i = 0; i < obstacle.size(); i++) {
			if (obstacle[i].type == 2) {
				M = glm::translate(glm::mat4(1.0f), (glm::vec3(0.0f, 0.0f, 0.0f) - obstacle[i].position) * glm::vec3(-0.95f, -0.95f, -0.95f));
				M = glm::rotate(M, obstacle[i].angle, glm::vec3(0.0f, 1.0f, 0.0f));
				M = glm::scale(M, glm::vec3(1.5f, 1.5f, 1.5f));
				shader->setUnifMat4("M", M);
				glDrawArrays(GL_TRIANGLES, 0, flower_numOfTriangles);
			}
		}
		glBindVertexArray(0);

		/*lampShader->use();
		M = glm::translate(glm::mat4(1.0f), lightPos);
		M = glm::scale(M, glm::vec3(0.2f));
		lampShader->setUnifMat4("P", P);
		lampShader->setUnifMat4("V", V);
		lampShader->setUnifMat4("M", M);*/

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, cube_numOfTriangles);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteVertexArrays(1, &vao_stone);
	glDeleteVertexArrays(1, &vao_fruit);
	glDeleteVertexArrays(1, &vao_mushroom);
	glDeleteVertexArrays(1, &vao_flower);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &vbo_texture);
	glDeleteBuffers(1, &vbo_normals);
	glDeleteBuffers(1, &vbo_texture);
	glDeleteBuffers(1, &vbo_stone_texture);
	glDeleteBuffers(1, &vbo_stone_normals);
	glDeleteBuffers(1, &vbo_stone_texture);
	glDeleteBuffers(1, &vbo_fruit_texture);
	glDeleteBuffers(1, &vbo_fruit_normals);
	glDeleteBuffers(1, &vbo_fruit_texture);
	glDeleteBuffers(1, &vbo_mushroom_texture);
	glDeleteBuffers(1, &vbo_mushroom_normals);
	glDeleteBuffers(1, &vbo_mushroom_texture);
	glDeleteBuffers(1, &vbo_flower_texture);
	glDeleteBuffers(1, &vbo_flower_normals);
	glDeleteBuffers(1, &vbo_flower_texture);

	glfwTerminate();

	return EXIT_SUCCESS;
}

void changeLightProperties() {
	if (lightMode == 0) {
		dir_diffuse = 0.4f;
		dir_specular = 0.75;
		point_diffuse = 0.75f;
		point_specular = 0.55f;
		spotilight_diffuse = 0.0f;
		spotilight_specular = 0.0f;
	}
	else if (lightMode == 1) {
		// dir
		dir_diffuse -= 0.01;
		dir_diffuse = std::max(dir_diffuse, 0.0f);
		dir_specular -= 0.01;
		dir_specular = std::max(dir_specular, 0.0f);
		// point
		point_diffuse -= 0.01;
		point_diffuse = std::max(point_diffuse, 0.0f);
		point_specular -= 0.01;
		point_specular = std::max(point_specular, 0.0f);
		// spotlight
		spotilight_diffuse += 0.01;
		spotilight_diffuse = std::min(spotilight_diffuse, 0.5f);
		spotilight_specular += 0.01;
		spotilight_specular = std::min(spotilight_specular, 0.35f);
	}
	else if (lightMode == 2) {
		// dir
		dir_diffuse += 0.01;
		dir_diffuse = std::min(dir_diffuse, 0.4f);
		dir_specular += 0.01;
		dir_specular = std::min(dir_specular, 0.75f);
		// point
		point_diffuse += 0.01;
		point_diffuse = std::min(point_diffuse, 0.75f);
		point_specular += 0.01;
		point_specular = std::min(point_specular, 0.55f);
		// spotlight
		spotilight_diffuse -= 0.01;
		spotilight_diffuse = std::max(spotilight_diffuse, 0.0f);
		spotilight_specular -= 0.01;
		spotilight_specular = std::max(spotilight_specular, 0.0f);
	}
}

void genFood(Snake snake) {
	int mr = (int)mapRadius;
	bool checkOthers;

	int x, z;
	do {
		checkOthers = false;

		x = -mr + (rand() % static_cast<int>(mr - (-mr) + 1));
		z = -mr + (rand() % static_cast<int>(mr - (-mr) + 1));
		glm::vec3 pos = glm::vec3((float)x, 1.0f, (float)z);
		for (int j = 0; j < snake.elem.size(); j++) {
			if (snake.elem[j] == pos)
				checkOthers = true;
		}
		for (int j = 0; j < obstacle.size(); j++) {
			if (obstacle[j].position == pos)
				checkOthers = true;
		}
		for (int j = 0; j < food.size(); j++) {
			glm::vec3 foodPos = food[j].position;
			foodPos.y = 1.0f;
			if (foodPos == pos)
				checkOthers = true;
		}
	} while (checkOthers);

	food.push_back(Food(glm::vec3((float)x, 1.0f, (float)z)));
	numOfFood++;
}

void generateObstacles(Snake snake, float mapRadius, int numOfStones, int numOfMushroms, int numOfFlowers) {
	int numOfObstacles = numOfStones + numOfMushroms + numOfFlowers;
	int mr = (int)mapRadius;
	for (int i = 0; i < numOfObstacles; i++) {
		bool checkOthers = false;

		int x, z;
		do {
			checkOthers = false;

			do {
				x = -mr + (rand() % static_cast<int>(mr - (-mr) + 1));
			} while (abs(x) < 2);
			
			do {
				z = -mr + (rand() % static_cast<int>(mr - (-mr) + 1));
			} while (abs(z) < 2);
			
			glm::vec3 pos = glm::vec3((float)x, 1.0f, (float)z);
			for (int j = 0; j < snake.elem.size(); j++) {
				if (snake.elem[j] == pos)
					checkOthers = true;
			}
			for (int j = 0; j < obstacle.size(); j++) {
				if (obstacle[j].position == pos)
					checkOthers = true;
			}
		} while (checkOthers);

		int type;
		if (i < numOfStones)
			type = 0;
		else if (i < numOfStones + numOfMushroms)
			type = 1;
		else if (i < numOfStones + numOfMushroms + numOfFlowers)
			type = 2;
		obstacle.push_back(Obstacle(type, glm::vec3((float)x, 1.0f, (float)z)));
	}
}

void windowResize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	if (height != 0) {
		aspect = (float)width / (float)height;
	}
	else {
		aspect = 1;
	}
}

void initOpenGL(GLFWwindow* window) {
	glClearColor(0.1f, 0.1f, 0.2f, 1);
	glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, windowResize);

	shader = new Shader("./resources/shaders/vertex.vs", "", "./resources/shaders/fragment.fs");
	lampShader = new Shader("./resources/shaders/lamp.vs", "", "./resources/shaders/lamp.fs");

	GLint textureUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);

	prepareObjects();

	tex_snake = readTexture("./resources/textures/metal.png");
	tex_grass = readTexture("./resources/textures/grass.png");
	tex_stone = readTexture("./resources/textures/rock-texture.png");
	tex_fruit = readTexture("./resources/textures/mandarin.png");
	tex_box = readTexture("./resources/textures/box.png");
	tex_mushroom = readTexture("./resources/textures/mushroom.png");
	tex_flower = readTexture("./resources/textures/flower.png");
}

GLuint makeBuffer(void *data, int vertexCount, int vertexSize) {
	GLuint handle;

	glGenBuffers(1, &handle);
	glBindBuffer(GL_ARRAY_BUFFER, handle);
	glBufferData(GL_ARRAY_BUFFER, vertexCount*vertexSize, data, GL_STATIC_DRAW);

	return handle;
}

void prepareObjects() {
	vbo_vertices = makeBuffer(cube, cube_numOfTriangles, sizeof(float) * 4);
	vbo_texture = makeBuffer(cube_texture, cube_numOfTriangles, sizeof(float) * 2);
	vbo_normals = makeBuffer(cube_normals, cube_numOfTriangles, sizeof(float) * 4);

	vbo_stone_vertices = makeBuffer(stone, stone_numOfTriangles, sizeof(float) * 4);
	vbo_stone_texture = makeBuffer(stone_texture, stone_numOfTriangles, sizeof(float) * 2);
	vbo_stone_normals = makeBuffer(stone_normals, stone_numOfTriangles, sizeof(float) * 4);

	vbo_fruit_vertices = makeBuffer(fruit, fruit_numOfTriangles, sizeof(float) * 4);
	vbo_fruit_texture = makeBuffer(fruit_texture, fruit_numOfTriangles, sizeof(float) * 2);
	vbo_fruit_normals = makeBuffer(fruit_normals, fruit_numOfTriangles, sizeof(float) * 4);

	vbo_mushroom_vertices = makeBuffer(mushroom, mushroom_numOfTriangles, sizeof(float) * 4);
	vbo_mushroom_texture = makeBuffer(mushroom_texture, mushroom_numOfTriangles, sizeof(float) * 2);
	vbo_mushroom_normals = makeBuffer(mushroom_normals, mushroom_numOfTriangles, sizeof(float) * 4);

	vbo_flower_vertices = makeBuffer(flower, flower_numOfTriangles, sizeof(float) * 4);
	vbo_flower_texture = makeBuffer(flower_texture, flower_numOfTriangles, sizeof(float) * 2);
	vbo_flower_normals = makeBuffer(flower_normals, flower_numOfTriangles, sizeof(float) * 4);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	shader->setAttrVec4("vertex", vbo_vertices);
	shader->setAttrVec4("normal", vbo_normals);
	shader->setAttrVec2("texture", vbo_texture);
	glBindVertexArray(0);

	// --- STONE ---
	glGenVertexArrays(1, &vao_stone);
	glBindVertexArray(vao_stone);
	shader->setAttrVec4("vertex", vbo_stone_vertices);
	shader->setAttrVec4("normal", vbo_stone_normals);
	shader->setAttrVec2("texture", vbo_stone_texture);
	glBindVertexArray(0);

	// --- FRUIT ---
	glGenVertexArrays(1, &vao_fruit);
	glBindVertexArray(vao_fruit);
	shader->setAttrVec4("vertex", vbo_fruit_vertices);
	shader->setAttrVec4("normal", vbo_fruit_normals);
	shader->setAttrVec2("texture", vbo_fruit_texture);
	glBindVertexArray(0);

	// --- MUSHROOM ---
	glGenVertexArrays(1, &vao_mushroom);
	glBindVertexArray(vao_mushroom);
	shader->setAttrVec4("vertex", vbo_mushroom_vertices);
	shader->setAttrVec4("normal", vbo_mushroom_normals);
	shader->setAttrVec2("texture", vbo_mushroom_texture);
	glBindVertexArray(0);

	// --- FLOWER ---
	glGenVertexArrays(1, &vao_flower);
	glBindVertexArray(vao_flower);
	shader->setAttrVec4("vertex", vbo_flower_vertices);
	shader->setAttrVec4("normal", vbo_flower_normals);
	shader->setAttrVec2("texture", vbo_flower_texture);
	glBindVertexArray(0);

	// --- LIGHT SHADER ---
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	lampShader->setAttrVec4("vertex", vbo_vertices);
	glBindVertexArray(0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (!keyPressed) {
			keyPressed = true;
			if (key == GLFW_KEY_LEFT) keys[0] = true;
			else if (key == GLFW_KEY_UP) keys[1] = true;
			else if (key == GLFW_KEY_RIGHT) keys[2] = true;
			else if (key == GLFW_KEY_DOWN) keys[3] = true;
			else if (key == GLFW_KEY_R) keys[4] = true;
			else if (key == GLFW_KEY_0) lightMode = 0;
			else if (key == GLFW_KEY_1) lightMode = 1;
			else if (key == GLFW_KEY_2) lightMode = 2;
		}
	}
}

void clearKeys() {
	keyPressed = false;
	for (int i = 0; i < 5; i++) {
		keys[i] = false;
	}
}

GLuint readTexture(char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}