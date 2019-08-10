/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL, SDL OpenGL, standard IO, and, strings
#include <SDL.h>
#undef main 

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "GL/glew.h"
#include <stdio.h>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_PURE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/transform.hpp"

struct UserData
{
	UserData() : programObject(0), wmpLoc(-1), positionLoc(-1), colorLoc(-1) {}
	// Handle to a program object
	GLuint programObject;

	GLint  wmpLoc;

	// Attribute locations
	GLint  positionLoc;
	GLint  colorLoc;
};

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//Starts up SDL, creates window, and initializes OpenGL
bool init(UserData *userData);

//Initializes matrices and clear color
bool initGL(UserData* userData);

GLuint LoadShader(GLenum type, const char *shaderSrc);

//Input handler
void handleKeys(unsigned char key, int x, int y);

//Renders quad to the screen
void render();
void updatePosition(glm::mat4x4& mat);
void updateColor();

void createVBO(GLfloat *vertexBuffer,
	GLushort *indices, GLuint numVertices, GLuint numIndices,
	GLuint& vboId, GLuint& indexId, GLuint numElems, GLenum usage);

void createVBOArray(GLfloat *vertexBuffer, GLuint numVertices,
	GLuint& vboId, GLuint numElems, GLenum usage);

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

SDL_Renderer* renderer = NULL;

//OpenGL context
SDL_GLContext gContext;

//Render flag
bool gRenderQuad = true;
UserData gUserData;

GLuint gVboId;
GLuint gIndexId;
GLuint gVboColorId;
GLuint gIndexColorId;

const float DefaultNearPlaneDistance = 0.01f;
const float DefaultFarPlaneDistance = 1000.0f;

glm::mat4 gViewMatrix;
glm::mat4 gProjectionMatrix;
glm::mat4 gWorldMatrix;

#define VERT_POS_INDEX 0
#define COLOR_INDEX 1

int main()
{
	//Start up SDL and create window
	if (!init(&gUserData))
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Main loop flag
		bool quit = false;

		//Event handler
		SDL_Event e;

		//Enable text input
		SDL_StartTextInput();
#ifdef __EMSCRIPTEN__
		emscripten_set_main_loop(render, 0, 0);
#else
		//While application is running
		while (!quit)
		{
			//Handle events on queue
			while (SDL_PollEvent(&e) != 0)
			{
				//User requests quit
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
				//Handle keypress with current mouse position
				else if (e.type == SDL_TEXTINPUT)
				{
					int x = 0, y = 0;
					SDL_GetMouseState(&x, &y);
					handleKeys(e.text.text[0], x, y);
				}
			}

			render();

			//Update screen
			SDL_GL_SwapWindow(gWindow);
		}
#endif
		//Disable text input
		SDL_StopTextInput();

	}

	//Free resources and close SDL
	close();

	return 0;
}


bool init(UserData *userData)
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 2.1
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				GLenum err = glewInit();
				if (GLEW_OK != err)
				{
					printf("GLEW init failed: %s!\n", glewGetErrorString(err));
					success = false;
				}

				//Initialize OpenGL
				if (!initGL(userData))
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL(UserData *userData)
{
	const char vShaderStr [] =
		"precision mediump float;     \n"
		"uniform mat4 WorldViewProjection;\n"
		"attribute vec3 a_position;   \n"
		"attribute vec4 a_color;   \n"
		"varying vec4 v_color;   \n"
		"void main()                  \n"
		"{  v_color = a_color;                          \n"
		"   gl_Position = WorldViewProjection * vec4(a_position, 1.0); \n"
		"}                            \n";

	const char fShaderStr [] =
		"precision mediump float;     \n"
		"varying vec4 v_color;   \n"
		"void main()                                  \n"
		"{                                            \n"
		"  gl_FragColor = v_color;      \n"
		"}                                            \n";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	// Create the program object
	programObject = glCreateProgram();

	if (programObject == 0)
		return 0;

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Bind vPosition to attribute 0   
	//glBindAttribLocation(programObject, 0, "vPosition");

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = (char*) malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			printf("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return false;
	}

	// Store the program object
	userData->programObject = programObject;

	// Get the wmp location
	userData->wmpLoc = glGetUniformLocation(userData->programObject, "WorldViewProjection");

	// Get the attribute locations
	//userData->positionLoc = glGetAttribLocation(userData->programObject, "a_position");

	// Get the attribute locations
	//userData->colorLoc = glGetAttribLocation(userData->programObject, "a_color");

	GLfloat vVertices [] = { -1.0f, 1.0f, 0.0f,  // Position 0
		-1.0f, -1.0f, 0.0f,  // Position 1
		1.0f, -1.0f, 0.0f,  // Position 2
		1.0f, 1.0f, 0.0f,  // Position 3
	};
	GLushort indices [] = { 0, 1, 2, 0, 2, 3 };

	GLfloat vColors [] = { 1.0, 1.0, 0.0, 0.0, 
		1.0, 1.0, 0.0, 0.0, 
		1.0, 1.0, 0.0, 0.0, 
		1.0, 1.0, 0.0, 0.0 };

	createVBO(&vVertices[0],
		&indices[0], 4, sizeof(indices) / sizeof(GLushort),
		gVboId, gIndexId, 3, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(VERT_POS_INDEX);
	glBindAttribLocation(gUserData.programObject, VERT_POS_INDEX, "a_position");

	createVBOArray(&vColors[0],
		4, gVboColorId, 4, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(COLOR_INDEX);
	glBindAttribLocation(gUserData.programObject, COLOR_INDEX, "a_color");

	// Set the viewport
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	return true;
}

void createVBO(GLfloat *vertexBuffer,
	GLushort *indices, GLuint numVertices, GLuint numIndices,
	GLuint& vboId, GLuint& indexId, GLuint numElems, GLenum usage)
{
	// vertex
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, numVertices*(sizeof(GLfloat) * numElems),
		vertexBuffer, usage);

	// bind buffer object for element indices
	glGenBuffers(1, &indexId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		numIndices * sizeof(GLushort), indices,
		usage);
}

void createVBOArray(GLfloat *vertexBuffer, GLuint numVertices, 
	GLuint& vboId, GLuint numElems, GLenum usage)
{
	// vertex
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, numVertices*(sizeof(GLfloat) * numElems),
		vertexBuffer, usage);
}

void handleKeys(unsigned char key, int x, int y)
{
	//Toggle quad
	if (key == 'q')
	{
		gRenderQuad = !gRenderQuad;
	}
}

void render()
{
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// Use the program object
	glUseProgram(gUserData.programObject);

	glEnableVertexAttribArray(VERT_POS_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, gVboId);

	glVertexAttribPointer(VERT_POS_INDEX, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof(GLfloat), (const void*) 0);

	glEnableVertexAttribArray(COLOR_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, gVboColorId);

	glVertexAttribPointer(COLOR_INDEX, 4, GL_FLOAT,
		GL_FALSE, 4*sizeof(GLfloat), (const void*) 0);

	updateColor();

	glm::vec3 eye(0, 0, 2);
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 target = eye + direction;
	gViewMatrix = glm::lookAt(eye, target, up);

	//gProjectionMatrix = glm::ortho(-1.0f, 1.0f, -0.75f, 0.75f, DefaultNearPlaneDistance, DefaultFarPlaneDistance);
	gProjectionMatrix = glm::perspective(30.0f, 1.0f / 0.75f, 1.0f, 20.0f);

	glm::mat4x4 mat;
	updatePosition(mat);
	glm::mat4 wvp = gProjectionMatrix * gViewMatrix * mat;
	glUniformMatrix4fv(gUserData.wmpLoc, 1, GL_FALSE, &wvp[0][0]);

	// Draw 
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void updatePosition(glm::mat4x4& mat)
{
	double total_time = SDL_GetTicks() / 1000.0;

	GLfloat half_PI = static_cast<GLfloat>(M_PI) / 2.0f;
	GLfloat total_animated_time = 4.0f;
	GLfloat mod4 = fmod(total_time, total_animated_time);

	GLfloat y_angle = half_PI * mod4;
	mat = glm::rotate(y_angle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void updateColor()
{
	GLfloat vColors [] = { 1.0, 0.0, 0.0, 0.0, 
			1.0, 0.0, 0.0, 0.0, 
			1.0, 0.0, 0.0, 0.0, 
			1.0, 0.0, 0.0, 0.0 };
	
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vColors), (const GLvoid *) vColors);
}

void close()
{
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}


GLuint LoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	// Create the shader object
	shader = glCreateShader(type);

	if (shader == 0)
		return 0;

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader
	glCompileShader(shader);

	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = (char*) malloc(sizeof(char) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			printf("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}


