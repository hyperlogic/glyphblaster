#include <stdio.h>
#include <string>
#include <memory>
#include "SDL2/SDL.h"

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


#include "../src/context.h"
#include "../src/font.h"
#include "../src/text.h"

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

static uint32_t MakeColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    return alpha << 24 | blue << 16 | green << 8 | red;
}

void DrawTexturedQuad(uint32_t gl_tex, Vector2f const& origin, Vector2f const& size,
                      Vector2f const& uv_origin, Vector2f const& uv_size,
                      uint32_t color)
{
    // assume texture is enabled.
    glBindTexture(GL_TEXTURE_2D, gl_tex);

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

    uint32_t colors[8];
    for (int i = 0; i < 8; i++)
        colors[i] = color;

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    glActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

#if 0
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
                         MakeColor(255, 255, 255, 255));
        y += texture_size + sheet_gap;
    }
}

#endif

void TextRenderFunc(const std::vector<gb::Quad>& quadVec)
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

    static int count = 1;  // set to 0 to enable dumping
    int i = 0;
    for (auto &quad : quadVec) {

        if (count == 0) {
            printf("quad[%d]\n", i++);
            printf("    origin = [%d, %d]\n", quad.origin.x, quad.origin.y);
            printf("    size = [%d, %d]\n", quad.size.x, quad.size.y);
            printf("    uv_origin = [%.3f, %.3f]\n", quad.uvOrigin.x, quad.uvOrigin.y);
            printf("    uv_size = [%.3f, %.3f]\n", quad.uvSize.x, quad.uvSize.y);
            printf("    gl_tex_obj = %u\n", quad.glTexObj);
            printf("    user_data = %p\n", quad.userData);
        }

        DrawTexturedQuad(quad.glTexObj,
                         Vector2f(quad.origin.x, quad.origin.y),
                         Vector2f(quad.size.x, quad.size.y),
                         Vector2f(quad.uvOrigin.x, quad.uvOrigin.y),
                         Vector2f(quad.uvSize.x, quad.uvSize.y),
                         *((uint32_t*)quad.userData));
    }

    count++;
}

int main(int argc, char* argv[])
{
    s_config = new Config(false, false, 800, 600);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_Window* displayWindow;
    SDL_Renderer* displayRenderer;
    SDL_CreateWindowAndRenderer(s_config->width, s_config->height, SDL_WINDOW_OPENGL, &displayWindow, &displayRenderer);

    atexit(SDL_Quit);

    // clear to white
    Vector4f clearColor(0, 0, 1, 1);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapWindow(displayWindow);

    // create the context
    gb::Context::Init(4096, 3, gb::TextureFormat_Alpha);

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
    const char* lorem = (char*)mmap(0, s.st_size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (lorem < 0) {
        fprintf(stderr, "mmap failed\n errno = %d\n", errno);
        exit(1);
    }
    // we are lazy, don't unmap the file.

    // create a font
    std::shared_ptr<gb::Font> mainFont(new gb::Font("Droid-Sans/DroidSans.ttf", 12, gb::FontRenderOption_Normal, gb::FontHintOption_ForceAuto));
    //err = GB_FontMake(gb, "Droid-Sans/DroidSans.ttf", 20, GB_RENDER_NORMAL, GB_HINT_FORCE_AUTO, &mainFont);
    //err = GB_FontMake(gb, "Arial.ttf", 48, GB_RENDER_NORMAL, GB_HINT_DEFAULT, &mainFont);
    //err = GB_FontMake(gb, "Ayuthaya.ttf", 16, GB_RENDER_NORMAL, GB_HINT_DEFAULT, &mainFont);
    //std::shared_ptr<gb::Font> mainFont(new gb::Font("dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 15, gb::FontRenderOption_Normal, gb::FontHintOption_Default));
    //err = GB_FontMake(gb, "Zar/XB Zar.ttf", 48, GB_RENDER_NORMAL, GB_HINT_DEFAULT, &mainFont);
    //err = GB_FontMake(gb, "Times New Roman.ttf", 20, GB_RENDER_NORMAL, GB_HINT_FORCE_AUTO, &mainFont);

    /*
    std::shared_ptr<gb::Font> arabicFont;
    // create an arabic font
    err = GB_FontMake(gb, "Zar/XB Zar.ttf", 48, &arabicFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    */

    // create a text
    gb::IntPoint origin = {0, 0};
    gb::IntPoint size = {s_config->width - 1, s_config->height};
    uint32_t textColor = MakeColor(255, 255, 255, 255);
    uint32_t* userData = (uint32_t*)malloc(sizeof(uint32_t));
    *userData = textColor;

    std::shared_ptr<gb::Text> helloText(new gb::Text(lorem, mainFont, userData, origin, size,
                                                     gb::TextHorizontalAlign_Left,
                                                     gb::TextVerticalAligh_Center,
                                                     gb::TextOptionFlags_None));

    //GB_TextRelease(gb, helloText);

    /*
    // أبجد hello
    const char abjad[] = {0xd8, 0xa3, 0xd8, 0xa8, 0xd8, 0xac, 0xd8, 0xaf, 0x00};
    char XXX[1024];
    sprintf(XXX, "%s hello", abjad);
    err = GB_TextMake(gb, XXX, mainFont, 0xffffffff, origin, size,
                      GB_HORIZONTAL_ALIGN_CENTER, GB_VERTICAL_ALIGN_CENTER, &helloText);
    */

    gb::Context::Get().SetRenderFunc(TextRenderFunc);

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
            glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //DebugDrawGlyphCache(gb, config);

            helloText->Draw();
            SDL_GL_SwapWindow(displayWindow);
        }
    }

#if 0
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
#endif

    return 0;
}
