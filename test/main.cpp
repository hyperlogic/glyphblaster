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

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

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

Config* s_config = 0;

void DrawTexturedQuad(uint32_t gl_tex, Vector2f const& origin, Vector2f const& size,
                      Vector2f const& uv_origin, Vector2f const& uv_size,
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

    float uvs[8];
    uvs[0] = uv_origin.x;
    uvs[1] = uv_origin.y;
    uvs[2] = uv_origin.x + uv_size.x;
    uvs[3] = uv_origin.y;
    uvs[4] = uv_origin.x;
    uvs[5] = uv_origin.y + uv_size.y;
    uvs[6] = uv_origin.x + uv_size.x;
    uvs[7] = uv_origin.y + uv_size.y;

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
    // note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, config.width, config.height, 0, -10, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    const int sheet_gap = 2;
    const int texture_size = gb->cache->texture_size;
    GB_Cache* cache = gb->cache;
    int y = 0;
    for (uint32_t i = 0; i < cache->num_sheets; i++) {
        DrawTexturedQuad(cache->sheet[i].gl_tex_obj, Vector2f(0, y),
                         Vector2f(texture_size, texture_size),
                         Vector2f(0, 0), Vector2f(1, 1),
                         Vector4f(0.7, 0.7, 0.7, 1));
        y += texture_size + sheet_gap;
    }
}

void TextRenderFunc(GB_GlyphQuad* quads, uint32_t num_quads)
{
    // note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, s_config->width, s_config->height, 0, -10, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    for (uint32_t i = 0; i < num_quads; ++i) {

        /*
        printf("quad[%d]\n", i);
        printf("    origin = [%d, %d]\n", quads[i].origin[0], quads[i].origin[1]);
        printf("    size = [%d, %d]\n", quads[i].size[0], quads[i].size[1]);
        printf("    uv_origin = [%.3f, %.3f]\n", quads[i].uv_origin[0], quads[i].uv_origin[1]);
        printf("    uv_size = [%.3f, %.3f]\n", quads[i].uv_size[0], quads[i].uv_size[1]);
        */

        DrawTexturedQuad(quads[i].gl_tex_obj,
                         Vector2f(quads[i].origin[0], quads[i].origin[1]),
                         Vector2f(quads[i].size[0], quads[i].size[1]),
                         Vector2f(quads[i].uv_origin[0], quads[i].uv_origin[1]),
                         Vector2f(quads[i].uv_size[0], quads[i].uv_size[1]),
                         Vector4f(0, 0, 0, 1));
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
    s_config = new Config(false, false, 768/2, 1024/2);
    Config& config = *s_config;
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
    err = GB_ContextMake(64, 3, &gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }

    // load lorem.txt
    int fd = open("lorem.txt", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open failed\n");
        exit(1);
    }
    struct stat s;
    if (fstat(fd, &s) < 0) {
        fprintf(stderr, "fstat failed\n errno = %d\n", errno);
        exit(1);
    }
    const uint8_t* lorem = (uint8_t*)mmap(0, s.st_size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (lorem < 0) {
        fprintf(stderr, "mmap failed\n errno = %d\n", errno);
        exit(1);
    }
    // we are lazy, don't unmap the file.

    // create a font
    GB_Font* mainFont = NULL;
    //err = GB_FontMake(gb, "Droid-Sans/DroidSans.ttf", 16, &mainFont);
    //err = GB_FontMake(gb, "Arial.ttf", 10, &mainFont);
    err = GB_FontMake(gb, "dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 9, &mainFont);
    //err = GB_FontMake(gb, "Zar/XB Zar.ttf", 16, &mainFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    GB_Font* arabicFont = NULL;
    /*
    // create an arabic font
    err = GB_FontMake(gb, "Zar/XB Zar.ttf", 48, &arabicFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    */

    // create a text
    uint32_t origin[2] = {0, 10};
    uint32_t size[2] = {videoInfo->current_w, videoInfo->current_h};
    GB_Text* helloText = NULL;

    err = GB_TextMake(gb, lorem, mainFont, 0xffffffff, origin, size,
                      GB_HORIZONTAL_ALIGN_CENTER, GB_VERTICAL_ALIGN_CENTER, &helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    //GB_TextRelease(gb, helloText);

    /*
    // أبجد hello
    const char abjad[] = {0xd8, 0xa3, 0xd8, 0xa8, 0xd8, 0xac, 0xd8, 0xaf, 0x00};
    char XXX[1024];
    sprintf(XXX, "%s hello", abjad);
    err = GB_TextMake(gb, XXX, mainFont, 0xffffffff, origin, size,
                      GB_HORIZONTAL_ALIGN_CENTER, GB_VERTICAL_ALIGN_CENTER, &helloText);
    */

    GB_ContextSetTextRenderFunc(gb, TextRenderFunc);

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

            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            DebugDrawGlyphCache(gb, config);

            err = GB_TextDraw(gb, helloText);
            if (err != GB_ERROR_NONE) {
                fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
                exit(1);
            }
            SDL_GL_SwapBuffers();
        }
    }

    err = GB_TextRelease(gb, helloText);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseText Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_FontRelease(gb, mainFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    if (arabicFont) {
        err = GB_FontRelease(gb, arabicFont);
        if (err != GB_ERROR_NONE) {
            fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
            exit(1);
        }
    }
    err = GB_ContextRelease(gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    return 0;
}
