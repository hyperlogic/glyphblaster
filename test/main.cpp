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

#include "../src/gb_context.h"
#include "../src/gb_font.h"
#include "../src/gb_text.h"

#include "abaci.h"

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

void DrawTexturedQuad(uint32_t gl_tex, Vector2f const& origin, Vector2f const& size,
                      Vector4f const& color)
{
    // assume texture is enabled.
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glColor4f(color.x, color.y, color.z, color.w);

    float verts[8];
    verts[0] = origin.x;
    verts[1] = origin.y;
    verts[2] = origin.x + size.x;
    verts[3] = origin.y;
    verts[4] = origin.x;
    verts[5] = origin.y + size.y;
    verts[6] = origin.x + size.x;
    verts[7] = origin.y + size.y;

    glVertexPointer(2, GL_FLOAT, 0, verts);
    glEnableClientState(GL_VERTEX_ARRAY);

    static float uvs[] = {0, 0, 1, 0, 0, 1, 1, 1};
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    glActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

// for debug draw only
#include "../src/gb_cache.h"

void DebugDrawGlyphCache(GB_Context* gb, const Config& config)
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /// note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, config.width, config.height, 0, -10, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    GB_Cache* cache = gb->cache;
    int y = 0;
    for (uint32_t i = 0; i < cache->num_sheets; i++) {
        DrawTexturedQuad(cache->sheet[i].gl_tex_obj, Vector2f(0, y),
                         Vector2f(GB_TEXTURE_SIZE, GB_TEXTURE_SIZE + y),
                         Vector4f(0, 0, 0, 1));
        y += GB_TEXTURE_SIZE;
    }
}

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

    // create the context
    GB_ERROR err;
    GB_Context* gb;
    err = GB_ContextMake(&gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }

    // create a font
    GB_Font* droidFont = NULL;
    err = GB_FontMake(gb, "Droid-Sans/DroidSans.ttf", 12, &droidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    // create a bigger font
    GB_Font* bigDroidFont = NULL;
    err = GB_FontMake(gb, "Droid-Sans/DroidSans.ttf", 48, &bigDroidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    // create a text
    uint32_t origin[2] = {0, 0};
    uint32_t size[2] = {videoInfo->current_w, videoInfo->current_h};
    GB_Text* helloText = NULL;
    err = GB_TextMake(gb, "abcdefg", droidFont, 0xffffffff, origin, size,
                      GB_HORIZONTAL_ALIGN_CENTER, GB_VERTICAL_ALIGN_CENTER, &helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    GB_TextRelease(gb, helloText);
    err = GB_TextMake(gb, "ABCDEFG", bigDroidFont, 0xffffffff, origin, size,
                      GB_HORIZONTAL_ALIGN_CENTER, GB_VERTICAL_ALIGN_CENTER, &helloText);

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
            /*
            err = GB_DrawText(gb, helloText);
            if (err != GB_ERROR_NONE) {
                fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
                exit(1);
            }
            */
            DebugDrawGlyphCache(gb, config);
            SDL_GL_SwapBuffers();
        }
    }

    err = GB_TextRelease(gb, helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_FontRelease(gb, droidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_FontRelease(gb, bigDroidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_ContextRelease(gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    return 0;
}
