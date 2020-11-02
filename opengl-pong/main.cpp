#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>


// initial settings

unsigned int scrWidth = 800;
unsigned int scrHeight = 600;
const char* title = "Pong";
GLuint shaderProgram;

// initialize GLFW
void initGLFW(unsigned int versionMajor, unsigned int versionMinor) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}
// create window

void createWindow(GLFWwindow*& window,
	const char* title, unsigned int width, unsigned int height,
	GLFWframebuffersizefun framebufferSizeCallback) {
	window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!window) {
		return;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	//glViewport(0, 0, width, height);
}



// load GLAD lib
bool loadGlad() {
	return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

std::string readFile(const char* filename) {
	std::ifstream file;
	std::stringstream buf;

	std::string ret = "";

	// open file
	file.open(filename);

	if (file.is_open()) {
		buf << file.rdbuf();
		ret = buf.str();
	}
	else {
		std::cout << "Could not open" << filename << std::endl;
	}

	file.close();
	return ret;
}

int genShader(const char* filepath, GLenum type) {
	std::string shaderSrc = readFile(filepath);
	const GLchar* shader = shaderSrc.c_str();

	// build and compile shader
	int shaderObj = glCreateShader(type);
	glShaderSource(shaderObj, 1, &shader, NULL);
	glCompileShader(shaderObj);

	// check for errors
	int success;
	char infoLog[512];
	glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shaderObj, 512, NULL, infoLog);
		std::cout << "Error in shader compilation" << infoLog << std::endl;
		return -1;
	}
	return shaderObj;
}

int genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
	int shaderProgram = glCreateProgram();
	int vertexShader = genShader(vertexShaderPath, GL_VERTEX_SHADER);
	int fragmentShader = genShader(fragmentShaderPath, GL_FRAGMENT_SHADER);
	if (vertexShader == -1 || fragmentShader == -1) {
		return -1;
	}

	// link shader
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// check for errors
	int success;
	char infoLog[512];
	glGetShaderiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "Error in shader Linking" << infoLog << std::endl;
		return -1;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void bindShader(int shaderProgram) {
	glUseProgram(shaderProgram);
}


void setOrthographicProjection(int shaderProgram,
	float left, float right,
	float bottom, float top,
	float near, float far) {

	float mat[4][4] = {
		{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
		{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
		{0.0f, 0.0f, -2.0f / (far - near), 0.0f},
		{-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f}

	};

	bindShader(shaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &mat[0][0]);
}

void deleteShader(int shaderProgram) {
	glDeleteProgram(shaderProgram);
}

// vertyex Array Object/ Buffer Object Methods

struct VAO {
	GLuint val;
	GLuint posVBO;
	GLuint offsetVBO;
	GLuint sizeVBO;
	GLuint EBO;
};


void genVAO(VAO* vao) {
	glGenVertexArrays(1, &vao->val);
	glBindVertexArray(vao->val);
}
// generate buffer of certain type and set data
template<typename T>
void genBufferObject(GLuint& bo, GLenum type, GLuint noElements, T* data, GLenum usage) {
	glGenBuffers(1, &bo);
	glBindBuffer(type, bo);
	glBufferData(type, noElements * sizeof(T), data, usage);
}


// update data in a buffer object
template<typename T>
void updateData(GLuint& bo, GLintptr offset, GLuint noElements, T* data) {
	glBindBuffer(GL_ARRAY_BUFFER, bo);
	glBufferSubData(GL_ARRAY_BUFFER, offset, noElements * sizeof(T), data);
}

// set attribute pointer
template<typename T>
void setAttPointer(GLuint& bo, GLuint index, GLint size, GLenum tpye, GLuint stride, GLuint offset, GLuint divisor = 0) {
	glBindBuffer(GL_ARRAY_BUFFER, bo);
	glVertexAttribPointer(index, size, tpye, GL_FALSE, stride * sizeof(T), (void*)(offset * sizeof(T)));
	glEnableVertexAttribArray(index);
	if (divisor > 0) {
		// rest index attribute every divisor iteration through instances
		glVertexAttribDivisor(index, divisor);
	}
}

// draw VAO
void draw(VAO vao, GLenum mode, GLuint count, GLenum type, GLint indices, GLuint instanceCount = 1) {
	glBindVertexArray(vao.val);
	glDrawElementsInstanced(mode, count, type, (void*)indices, instanceCount);
}

void unbindBuffer(GLenum type) {
	glBindBuffer(type, 0);
}

void unbindVAO() {
	glBindVertexArray(0);
}

void cleanup(VAO vao) {
	glDeleteBuffers(1, &vao.posVBO);
	glDeleteBuffers(1, &vao.offsetVBO);
	glDeleteBuffers(1, &vao.sizeVBO);
	glDeleteBuffers(1, &vao.EBO);
	glDeleteVertexArrays(1, &vao.val);
}


// callback for window size change
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	scrWidth = width;
	scrHeight = height;

	// update projection matrix
	setOrthographicProjection(shaderProgram, 0, width, 0, height, 0.0f, 1.0f);
}

// process input
void processInput(GLFWwindow* window, float* offset) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		offset[1] += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		offset[0] += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		offset[1] -= 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		offset[0] -= 1.0f;
	}
}

void clearScreen() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void newFrame(GLFWwindow* window) {
	glfwSwapBuffers(window);
	glfwPollEvents();
}


void cleanup() {
	glfwTerminate();
}

int main() {
	std::cout << "Hello World!" << std::endl;

	// timing
	double dt = 0.0;
	double lastFrame = 0.0;


	// initialization
	initGLFW(3, 3);

	GLFWwindow* window = nullptr;
	createWindow(window, title, scrWidth, scrHeight, framebufferSizeCallback);
	if (!window) {
		std::cout << "Could not create window" << std::endl;
		cleanup();
		return -1;
	}

	if (!loadGlad()) {
		std::cout << "Could not init GLAD" << std::endl;
		cleanup();
		return -1;
	}

	// shaders 
	shaderProgram = genShaderProgram("main.vs", "main.fs");
	setOrthographicProjection(shaderProgram, 0, scrWidth, 0, scrHeight, 0.0f, 1.0f);

	float vertices[] = {
		// x	y
		0.5, 0.5f,
	   -0.5f, 0.5f,
	   -0.5f, -0.5f,
		0.5f, -0.5f
	};

	// setup index data
	unsigned int indices[] = {
		0, 1, 2, // top left triangle
		2, 3, 0, // bottom right triangle
	};

	float offsets[] = {
		200.0f, 200.0f
	};

	float sizes[] = {
		50.0f, 50.0f
	};

	// setup VAO/VBO
	VAO vao;
	genVAO(&vao);

	// pos VBO
	genBufferObject<float>(vao.posVBO, GL_ARRAY_BUFFER, 2 * 4, vertices, GL_STATIC_DRAW);
	setAttPointer<float>(vao.posVBO, 0, 2, GL_FLOAT, 2, 0);

	// offset VBO
	genBufferObject<float>(vao.offsetVBO, GL_ARRAY_BUFFER, 1 * 2, offsets, GL_DYNAMIC_DRAW);
	setAttPointer<float>(vao.offsetVBO, 1, 2, GL_FLOAT, 2, 0, 1);

	// size VBO
	genBufferObject<float>(vao.sizeVBO, GL_ARRAY_BUFFER, 1 * 2, offsets, GL_DYNAMIC_DRAW);
	setAttPointer<float>(vao.sizeVBO, 2, 2, GL_FLOAT, 2, 0, 1);

	// EBO
	genBufferObject<unsigned int>(vao.EBO, GL_ELEMENT_ARRAY_BUFFER, 3 * 2, indices, GL_STATIC_DRAW);

	unbindBuffer(GL_ARRAY_BUFFER);
	unbindVAO();

	// render loop
	while (!glfwWindowShouldClose(window)) {
		// update time
		dt = glfwGetTime() - lastFrame;
		lastFrame += dt;

		// input
		processInput(window, offsets);

		// clear screen for new frame
		clearScreen();

		// update
		updateData<float>(vao.offsetVBO, 0, 1 * 2, offsets);

		// render object
		bindShader(shaderProgram);
		draw(vao, GL_TRIANGLES, 3 * 2, GL_UNSIGNED_INT, 0);

		// swap frames
		newFrame(window);
	}

	// cleanup memory
	cleanup(vao);
	deleteShader(shaderProgram);
	cleanup();

	return 0;
}
