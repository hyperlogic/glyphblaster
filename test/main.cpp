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
#include "../src/cache.h"

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

void DrawLine(Vector2f p0, Vector2f p1, uint32_t color)
{
    float verts[4] = {p0.x, p0.y, p1.x, p1.y};
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glEnableClientState(GL_VERTEX_ARRAY);

    uint32_t colors[4] = {color, color, color, color};
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    static uint16_t indices[] = {0, 1};
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_SHORT, indices);
}

void DrawTexturedQuad(uint32_t gl_tex, Vector2f origin, Vector2f size,
                      Vector2f uv_origin, Vector2f uv_size,
                      uint32_t color)
{
    // assume texture is enabled.
    glBindTexture(GL_TEXTURE_2D, gl_tex);

    float verts[8] = {origin.x, origin.y,
                      origin.x + size.x, origin.y,
                      origin.x, origin.y + size.y,
                      origin.x + size.x, origin.y + size.y};

    glVertexPointer(2, GL_FLOAT, 0, verts);
    glEnableClientState(GL_VERTEX_ARRAY);

    float uvs[8] = {uv_origin.x, uv_origin.y,
                    uv_origin.x + uv_size.x, uv_origin.y,
                    uv_origin.x, uv_origin.y + uv_size.y,
                    uv_origin.x + uv_size.x, uv_origin.y + uv_size.y};

    uint32_t colors[8] = {color, color, color, color,
                          color, color, color, color};

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    glActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

// for debug draw only
void DebugDrawGlyphCache(const Config& config)
{
    // note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, config.width, config.height, 0, -10, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gb::Context& context = gb::Context::Get();
    const gb::Cache& cache = context.GetCache();

    const int sheetGap = 2;
    const int textureSize = cache.GetTextureSize();
    std::vector<uint32_t> texObjVec;
    cache.GetTextureObjects(texObjVec);
    int y = 0;
    for (auto texObj : texObjVec)
    {
        glEnable(GL_TEXTURE_2D);
        DrawTexturedQuad(texObj, Vector2f(0, y),
                         Vector2f(textureSize, textureSize),
                         Vector2f(0, 0), Vector2f(1, 1),
                         MakeColor(255, 255, 255, 255));

        glDisable(GL_TEXTURE_2D);
        Vector2f corners[4] = {Vector2f(0, y),
                               Vector2f(0, textureSize),
                               Vector2f(textureSize, textureSize),
                               Vector2f(textureSize, y)};

        for (int i = 0, p = 3; i < 4; p = i, i++)
        {
            DrawLine(corners[p], corners[i], MakeColor(0, 255, 0, 255));
        }

        y += textureSize + sheetGap;
    }
}

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
    for (auto &quad : quadVec)
    {
        if (count == 0)
        {
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
    gb::Context::Init(512, 1, gb::TextureFormat_Alpha);

    // load lorem.txt
    int fd = open("lorem.txt", O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "open failed\n");
        exit(1);
    }
    struct stat s;
    if (fstat(fd, &s) < 0)
    {
        fprintf(stderr, "fstat failed\n errno = %d\n", errno);
        exit(1);
    }
    const char* lorem = (char*)mmap(0, s.st_size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (lorem < 0)
    {
        fprintf(stderr, "mmap failed\n errno = %d\n", errno);
        exit(1);
    }
    // we are lazy, don't unmap the file.


    // create a font
    auto droidSans = std::make_shared<gb::Font>("Droid-Sans/DroidSans.ttf", 15,
                                                gb::FontRenderOption_Normal,
                                                gb::FontHintOption_ForceAuto);

    /*
    auto dejaVuSans = std::make_shared<gb::Font>("dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 30,
                                                 gb::FontRenderOption_Normal,
                                                 gb::FontHintOption_Default);
    */

    /*
    auto dejaVuSans = std::make_shared<gb::Font>("dejavu-fonts-ttf-2.33/ttf/DejaVuSansMono.ttf", 20,
                                                 gb::FontRenderOption_Normal,
                                                 gb::FontHintOption_Default);
    */

    /*
    auto arial = std::make_shared<gb::Font>("Arial.ttf", 20,
                                            gb::FontRenderOption_Normal,
                                            gb::FontHintOption_Default);
    */


    /*
    auto zar = std::make_shared<gb::Font>("Zar/XB Zar.ttf", 48,
                                          gb::FontRenderOption_Normal,
                                          gb::FontHintOption_Default);
    */


    // create a text
    gb::IntPoint origin = {0, s_config->height / 4};
    gb::IntPoint size = {s_config->width - 1, s_config->height};
    uint32_t textColor = MakeColor(255, 255, 255, 255);
    uint32_t* userData = (uint32_t*)malloc(sizeof(uint32_t));
    *userData = textColor;

    // create a text object
    auto loremText = std::make_shared<gb::Text>(lorem, droidSans, userData, origin, size,
                                                gb::TextHorizontalAlign_Left,
                                                gb::TextVerticalAlign_Center);
                                                /*gb::TextOptionFlags_DirectionRightToLeft, "Hebr");*/
                                                /*gb::TextOptionFlags_DirectionRightToLeft, "Arab");*/

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

            DebugDrawGlyphCache(*s_config);

            loremText->Draw();

            SDL_GL_SwapWindow(displayWindow);
        }
    }

    return 0;
}
