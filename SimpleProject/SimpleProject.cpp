//Using SDL, SDL OpenGL, standard IO, and, strings
#include <SDL.h>
#undef main 

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "GL/glew.h"
#include <cstdio>
#include <string>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//Starts up SDL, creates window, and initializes OpenGL
bool init();

//Renders quad to the screen
void render();

//Frees media and shuts down SDL
void close();

SDL_Window* gWindow = NULL;
SDL_GLContext gContext;

int main()
{
	//Start up SDL and create window
	if (!init())
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

			render();

			//Update screen
			SDL_GL_SwapWindow(gWindow);
		}
#endif
	}

	//Free resources and close SDL
	close();

	return 0;
}

bool init()
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

				// Set the viewport
				glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				return true;

			}
		}
	}

	return success;
}

void render() // all gl calls fails in this render function
{
	// Clear the color buffer
	//glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void close()
{
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}