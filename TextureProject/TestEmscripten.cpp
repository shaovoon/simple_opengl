/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL, SDL OpenGL, standard IO, and, strings
#include <SDL.h>
#undef main 

#include "GL/glew.h"
//#include <GL/GLU.h>
//#include <SDL_opengl.h>
#include <SDL_image.h> //Needed for IMG_Load.  If you want to use bitmaps (SDL_LoadBMP), it appears to not be necessary
#include <stdio.h>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_PURE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#define IMG_FILE "yes.png"

struct UserData
{
	UserData() : programObject(0), wmpLoc(-1), positionLoc(-1), texCoordLoc(-1), samplerLoc(-1), textureId(0), images_loaded(0) {}
	// Handle to a program object
	GLuint programObject;

	GLint  wmpLoc;

	// Attribute locations
	GLint  positionLoc;
	GLint  texCoordLoc;

	// Sampler location
	GLint samplerLoc;

	// Texture handle
	GLuint textureId;
	int images_loaded;

};

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//Starts up SDL, creates window, and initializes OpenGL
bool init(UserData *userData);

//Initializes matrices and clear color
bool initGL(UserData* userData);

GLuint LoadShader(GLenum type, const char *shaderSrc);

//Per frame update
void update();

GLuint init_texture(const char * file);
void load_texture(const char * file);
void load_error(const char * file);


void deinit_texture(GLuint texture);

//Renders quad to the screen
void render();
void updatePosition();

void createVBO(GLfloat *vertexBuffer,
	GLushort *indices, GLuint numVertices, GLuint numIndices,
	GLuint& vboId, GLuint& indexId, GLuint numElems, GLenum usage);

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

//Render flag
UserData gUserData;

GLuint gVboId;
GLuint gIndexId;

const float DefaultNearPlaneDistance = 0.01f;
const float DefaultFarPlaneDistance = 1000.0f;

glm::mat4 gViewMatrix;
glm::mat4 gProjectionMatrix;
glm::mat4 gWorldMatrix;

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
			}

			if (gUserData.images_loaded >= 1)
			{
				//Render quad
				render();
			}
			//Update screen
			SDL_GL_SwapWindow(gWindow);
		}
#endif

		deinit_texture(gUserData.textureId);
	}

	//Free resources and close SDL
	close();

	return 0;
}


bool init(UserData *userData)
{
	//Initialization flag
	bool success = true;

#ifdef __EMSCRIPTEN__
	emscripten_set_canvas_element_size("#canvas", SCREEN_WIDTH, SCREEN_HEIGHT);
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.alpha = attr.depth = attr.stencil = attr.antialias = attr.preserveDrawingBuffer = attr.failIfMajorPerformanceCaveat = 0;
	attr.enableExtensionsByDefault = 1;
	attr.premultipliedAlpha = 0;
	attr.majorVersion = 1;
	attr.minorVersion = 0;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
	emscripten_webgl_make_context_current(ctx);

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
#else

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
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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
#endif

	return success;
}

bool initGL(UserData *userData)
{
#ifdef __EMSCRIPTEN__
	emscripten_async_wget("http://localhost:16564/yes.png", IMG_FILE, load_texture, load_error);
#endif

	const char vShaderStr [] =
		"precision mediump float;     \n"
		"uniform mat4 WorldViewProjection;\n"
		"attribute vec3 a_position;   \n"
		"attribute vec2 a_texCoord;   \n"
		"varying vec2 v_texCoord;     \n"
		"void main()                  \n"
		"{                            \n"
		"   gl_Position = WorldViewProjection * vec4(a_position, 1.0); \n"
		"   v_texCoord = a_texCoord;  \n"
		"}                            \n";

	const char fShaderStr [] =
		"precision mediump float;     \n"
		"varying vec2 v_texCoord;                            \n"
		"uniform sampler2D s_texture;                        \n"
		"void main()                                         \n"
		"{                                                   \n"
		"  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
		"}                                                   \n";

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
	userData->positionLoc = glGetAttribLocation(userData->programObject, "a_position");
	userData->texCoordLoc = glGetAttribLocation(userData->programObject, "a_texCoord");

	// Get the sampler location
	userData->samplerLoc = glGetUniformLocation(userData->programObject, "s_texture");

	// Load the texture
#ifndef __EMSCRIPTEN__
	userData->textureId = init_texture(IMG_FILE);
	++gUserData.images_loaded;
#endif

	GLfloat vVertices [] = { -0.5f, 0.5f, 0.0f,  // Position 0
		0.0f, 0.0f,        // TexCoord 0 
		-0.5f, -0.5f, 0.0f,  // Position 1
		0.0f, 1.0f,        // TexCoord 1
		0.5f, -0.5f, 0.0f,  // Position 2
		1.0f, 1.0f,        // TexCoord 2
		0.5f, 0.5f, 0.0f,  // Position 3
		1.0f, 0.0f         // TexCoord 3
	};
	GLushort indices [] = { 0, 1, 2, 0, 2, 3 };

	createVBO(&vVertices[0],
		&indices[0], 4, sizeof(indices) / sizeof(GLushort),
		gVboId, gIndexId, 5, GL_DYNAMIC_DRAW);

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

void update()
{
	//No per frame update needed
}

void render()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexId);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// Use the program object
	glUseProgram(gUserData.programObject);

	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 position(0, 0, 2);

	glm::vec3 target = position + direction;
	gViewMatrix = glm::lookAt(position, target, up);

	gProjectionMatrix = glm::ortho(-1.0f, 1.0f, -0.75f, 0.75f, DefaultNearPlaneDistance, DefaultFarPlaneDistance);

	glm::mat4 wvp = gProjectionMatrix * gViewMatrix * gWorldMatrix;
	glUniformMatrix4fv(gUserData.wmpLoc, 1, GL_FALSE, &wvp[0][0]);

	updatePosition();

	// Load the vertex position
	glVertexAttribPointer(gUserData.positionLoc, 3, GL_FLOAT,
		GL_FALSE, 5 * sizeof(GLfloat), (const void*) 0);
	// Load the texture coordinate
	glVertexAttribPointer(gUserData.texCoordLoc, 2, GL_FLOAT,
		GL_FALSE, 5 * sizeof(GLfloat), (const void*) 12);

	glEnableVertexAttribArray(gUserData.positionLoc);
	glEnableVertexAttribArray(gUserData.texCoordLoc);

	// Bind the texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gUserData.textureId);

	// Set the sampler texture unit to 0
	glUniform1i(gUserData.samplerLoc, 0);

	// Draw 
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void updatePosition()
{
	double total_time = SDL_GetTicks() / 1000.0;
	GLfloat mod2 = fmod(total_time, 4.0);
	GLfloat mod4 = fmod(total_time, 8.0);
	bool to_right = mod4 >= 4.0;
	GLfloat delta = 0.0;
	if (to_right)
		delta = 0.1*(mod2 - 2.0);
	else
		delta = 0.1*((4.0 - mod2) - 2.0);

	GLfloat min_size = 0.4;
	GLfloat scale = 0.5f * delta + min_size;
	GLfloat left = -1.0*scale;
	GLfloat top = 1.0*scale;
	GLfloat right = 1.0*scale;
	GLfloat bottom = -1.0*scale;

	GLfloat vVertices [] = {
		left, top, 0.0f, 0.0f, 0.0f,
		left, bottom, 0.0f, 0.0f, 1.0f,
		right, bottom, 0.0f, 1.0f, 1.0f,
		right, top, 0.0f, 1.0f, 0.0f
	};

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), (const GLvoid *) vVertices);
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

void load_texture(const char * file)
{
	gUserData.textureId = init_texture(file);
	++gUserData.images_loaded;
}

void load_error(const char * file)
{
	printf("File download failed: %s", file);
}

GLuint init_texture(const char * file) {
	GLuint texture = 0;
	//Load the image from the file into SDL's surface representation
	SDL_Surface* surface = IMG_Load(file);
	if (surface == NULL) { //If it failed, say why and don't continue loading the texture
		printf("Error: \"%s\"\n", SDL_GetError()); return 0;
	}

	//Generate an array of textures.  We only want one texture (one element array), so trick
	//it by treating "texture" as array of length one.
	glGenTextures(1, &texture);
	//Select (bind) the texture we just generated as the current 2D texture OpenGL is using/modifying.
	//All subsequent changes to OpenGL's texturing state for 2D textures will affect this texture.
	glBindTexture(GL_TEXTURE_2D, texture);
	//Specify the texture's data.  This function is a bit tricky, and it's hard to find helpful documentation.  A summary:
	//   GL_TEXTURE_2D:    The currently bound 2D texture (i.e. the one we just made)
	//               0:    The mipmap level.  0, since we want to update the base level mipmap image (i.e., the image itself,
	//                         not cached smaller copies)
	//         GL_RGBA:    The internal format of the texture.  This is how OpenGL will store the texture internally (kinda)--
	//                         it's essentially the texture's type.
	//      surface->w:    The width of the texture
	//      surface->h:    The height of the texture
	//               0:    The border.  Don't worry about this if you're just starting.
	//          GL_RGB:    The format that the *data* is in--NOT the texture!  Our test image doesn't have an alpha channel,
	//                         so this must be RGB.
	//GL_UNSIGNED_BYTE:    The type the data is in.  In SDL, the data is stored as an array of bytes, with each channel
	//                         getting one byte.  This is fairly typical--it means that the image can store, for each channel,
	//                         any value that fits in one byte (so 0 through 255).  These values are to be interpreted as
	//                         *unsigned* values (since 0x00 should be dark and 0xFF should be bright).
	// surface->pixels:    The actual data.  As above, SDL's array of bytes.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	//Set the minification and magnification filters.  In this case, when the texture is minified (i.e., the texture's pixels (texels) are
	//*smaller* than the screen pixels you're seeing them on, linearly filter them (i.e. blend them together).  This blends four texels for
	//each sample--which is not very much.  Mipmapping can give better results.  Find a texturing tutorial that discusses these issues
	//further.  Conversely, when the texture is magnified (i.e., the texture's texels are *larger* than the screen pixels you're seeing
	//them on), linearly filter them.  Qualitatively, this causes "blown up" (overmagnified) textures to look blurry instead of blocky.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Unload SDL's copy of the data; we don't need it anymore because OpenGL now stores it in the texture.
	SDL_FreeSurface(surface);

	return texture;
}
void deinit_texture(GLuint texture) {
	//Deallocate an array of textures.  A lot of people forget to do this.
	glDeleteTextures(1, &texture);
}

