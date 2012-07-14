#include <stdio.h>
#include <string>
#include "SDL/SDL.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if defined DARWIN

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#endif

#include "../include/glyphblaster.h"

struct Config
{
    Config(bool fullscreenIn, bool msaaIn, int widthIn, int heightIn) :
        fullscreen(fullscreenIn), msaa(msaaIn), msaaSamples(4), width(widthIn), height(heightIn) {};

	bool fullscreen;
    bool msaa;
    int msaaSamples;
    int width;
    int height;
    std::string title;
};

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        fprintf(stderr, "Couldn't init SDL!\n");

    atexit(SDL_Quit);

	// Get the current desktop width & height
	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();

	// TODO: get this from config file.
    Config config(false, false, 768/2, 1024/2);
    config.title = "glyphblaster test";

    // msaa
    if (config.msaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.msaaSamples);
    }

	SDL_Surface* screen;
	if (config.fullscreen)
	{
		int width = videoInfo->current_w;
		int height = videoInfo->current_h;
		int bpp = videoInfo->vfmt->BitsPerPixel;
		screen = SDL_SetVideoMode(width, height, bpp,
                                  SDL_HWSURFACE | SDL_OPENGL | SDL_FULLSCREEN);
	}
	else
	{
        screen = SDL_SetVideoMode(config.width, config.height, 32, SDL_HWSURFACE | SDL_OPENGL);
	}

    SDL_WM_SetCaption(config.title.c_str(), config.title.c_str());

    if (!screen)
        fprintf(stderr, "Couldn't create SDL screen!\n");

    // clear to white
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();

    GB_ERROR err;
    GB_GB* gb;
    err = GB_Init(&gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }
    GB_Font* droidFont = NULL;
    err = GB_MakeFont(gb, "Droid-Sans/DroidSans.ttf", 32, &droidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    uint32_t min[2] = {0, 0};
    uint32_t max[2] = {videoInfo->current_w, videoInfo->current_h};
    GB_Text* helloText = NULL;
    err = GB_MakeText(gb, "Hello World", droidFont, 0xffffffff, min, max, GB_HORIZONTAL_ALIGN_CENTER,
                      GB_VERTICAL_ALIGN_CENTER, &helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    int done = 0;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                done = 1;
                break;

            case SDL_MOUSEMOTION:
                if (event.motion.state & SDL_BUTTON(1)) {
                    // move touch
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // start touch
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // end touch
                }
                break;

			case SDL_JOYAXISMOTION:
				// stick move
				break;

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				// joy pad press
				break;
            }
        }

        if (!done)
        {
            err = GB_Update(gb);
            if (err != GB_ERROR_NONE) {
                fprintf(stderr, "GB_Update Error %s\n", GB_ErrorToString(err));
                exit(1);
            }
            err = GB_DrawText(gb, helloText);
            if (err != GB_ERROR_NONE) {
                fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
                exit(1);
            }
            SDL_GL_SwapBuffers();
        }
    }

    err = GB_ReleaseText(gb, helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_ReleaseFont(gb, droidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_Shutdown(gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    return 0;
}
