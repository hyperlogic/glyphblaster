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

#include "../src/glyphblaster.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

enum LoadFileToMemoryResult { CouldNotOpenFile = -1, CouldNotReadFile = -2 };
int LoadFileToMemory(const std::string& filename, unsigned char **result)
{
	int size = 0;
	FILE *f = fopen(filename.c_str(), "rb");
	if (f == NULL)
	{
		*result = NULL;
		return CouldNotOpenFile;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = new unsigned char[(size + 1)];

	int newSize = (int)fread(*result, sizeof(char), size, f);
	if (size != newSize)
	{
		printf("size = %d, newSize = %d\n", size, newSize);
		delete [] (*result);
		return CouldNotReadFile;
	}
	fclose(f);
	(*result)[size] = 0;  // make sure it's null terminated.
	return size;
}

void delete_ttf_memory(void* ptr)
{
    delete [] (uint8_t*)ptr;
}

void harfbuzz_test()
{
    //const char* text = "Hello World";

    // Hêllð WðrlÐ utf8
    // const char text[] = {0x48, 0xc3, 0xaa, 0x6c, 0x6c, 0xc3, 0xb0, 0x20, 0x57, 0xc3, 0xb0, 0x72, 0x6c, 0xc3, 0x90, 0x00};

    const char text[] = "fl";

    // e with accent and macron: U+0065 U+0301 U+0304
    // const char text[] = {0x65, 0xcc, 0x81, 0xcc, 0x84, 0x00};

    int text_len = strlen(text);

    uint8_t* ttf_memory;
    //int size = LoadFileToMemory("Droid-Sans/DroidSans.ttf", &ttf_memory);
    int size = LoadFileToMemory("dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", &ttf_memory);
    //int size = LoadFileToMemory("Zar/XB Zar.ttf", &ttf_memory);
    if (size <= 0) {
        fprintf(stderr, "Error reading font");
        exit(1);
    }

    // referece-chain font -> face -> blob
    hb_blob_t* blob = hb_blob_create((char*)ttf_memory, size, HB_MEMORY_MODE_WRITABLE,
                                     ttf_memory, delete_ttf_memory);
    hb_face_t* face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);
    hb_font_t* font = hb_font_create(face);
    unsigned int upem = hb_face_get_upem(face);
    hb_font_set_scale(font, upem, upem);
    hb_face_destroy(face);
    hb_ft_font_set_funcs(font);

    const int pt_size = 24;

    double scale = double (pt_size) / hb_face_get_upem(hb_font_get_face(font));
    printf("scale = %.5f\n", scale);

    // create a buffer to hold glyph_info's
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_reset(buffer);
    hb_buffer_add_utf8(buffer, text, text_len, 0, text_len);

    // set direct, language and script.
    const char* direction = 0;
    const char* language = 0;
    const char* script = 0;
    hb_buffer_set_direction(buffer, hb_direction_from_string(direction, -1));
    hb_buffer_set_script(buffer, hb_script_from_string (script, -1));
    hb_buffer_set_language(buffer, hb_language_from_string (language, -1));

    // shape
    char **shapers = NULL;
    hb_bool_t shape_success = hb_shape_full(font, buffer, NULL, 0, shapers);

    if (shape_success) {
        fprintf(stderr, "it worked!?\n");
    }

    int num_glyphs = hb_buffer_get_length(buffer);
    hb_glyph_info_t* hb_glyph = hb_buffer_get_glyph_infos(buffer, NULL);
    hb_glyph_position_t* hb_position = hb_buffer_get_glyph_positions(buffer, NULL);

    int num_clusters = num_glyphs ? 1 : 0;
    for (int i = 1; i < num_glyphs; i++) {
        if (hb_glyph[i].cluster != hb_glyph[i-1].cluster)
            num_clusters++;
    }

    printf("num_glyphs = %d\n", num_glyphs);
    printf("num_clusters = %d\n", num_clusters);

    hb_position_t x = 0;
    hb_position_t y = 0;
    int i;
    for (i = 0; i < num_glyphs; i++)
    {
        /*
        l->glyphs[i].index = hb_glyph[i].codepoint;
        l->glyphs[i].x = ( hb_position->x_offset + x) * scale;
        l->glyphs[i].y = (-hb_position->y_offset + y) * scale;
        x +=  hb_position->x_advance;
        y += -hb_position->y_advance;
        */
        char glyph_name[32];
        hb_font_get_glyph_name(font, hb_glyph[i].codepoint, glyph_name, sizeof(glyph_name));
        printf("glyph[%d].glyph_name = %s\n", i, glyph_name);
        printf("glyph[%d].index = 0x%x\n", i, hb_glyph[i].codepoint);
        printf("glyph[%d].x = %.3f\n", i, (hb_position->x_offset + x) * scale);
        printf("glyph[%d].y = %.3f\n", i, (hb_position->y_offset + y) * scale);
        printf("glyph[%d].cluster = %d\n", i, hb_glyph[i].cluster);

        x +=  hb_position->x_advance;
        y += -hb_position->y_advance;

        hb_position++;
    }

    hb_buffer_destroy(buffer);

    hb_font_destroy(font);
}

// TODO: use this one!
void harfbuzz_test2()
{
    //const char* text = "Hello World";

    // Hêllð WðrlÐ utf8
    // const char text[] = {0x48, 0xc3, 0xaa, 0x6c, 0x6c, 0xc3, 0xb0, 0x20, 0x57, 0xc3, 0xb0, 0x72, 0x6c, 0xc3, 0x90, 0x00};

    // const char text[] = "florida";

    // e with accent and macron: U+0065 U+0301 U+0304
    const char text[] = {0x65, 0xcc, 0x81, 0xcc, 0x84, 0x00};

    //const char* text = "finalae";
    char glyph_name[30];
    size_t i;

    FT_Library library;
    FT_Init_FreeType(&library);
    FT_Face face;
    FT_New_Face(library, "dejavu-fonts-ttf-2.33/ttf/DejaVuSans.ttf", 0, &face);
    FT_Set_Char_Size(face, (int)(32*64), 0, 72, 72);

    hb_font_t * font = hb_ft_font_create(face, 0);
    hb_ft_font_set_funcs(font);

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_add_utf8(buffer, text, strlen(text), 0, strlen(text));
    hb_shape(font, buffer, NULL, 0);
    unsigned int num_glyphs = hb_buffer_get_length(buffer);
    hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(buffer, 0);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, 0);
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, NULL);

    for (i = 0; i < num_glyphs; ++i)
    {
        if (i)
            printf( "|" );
        else
            printf( "<" );

        if (!FT_Get_Glyph_Name(face, glyphs[i].codepoint, glyph_name, sizeof(glyph_name)))
            printf("%s", glyph_name);
        else
            printf("%u", info->codepoint);

        printf("=%u", info->cluster);
        if (pos->x_offset || pos->y_offset) {
            printf("@");
            if (pos->x_offset)
                printf("%d", pos->x_offset);
            if (pos->y_offset)
                printf(",%d", pos->y_offset);
        }
        if (pos->x_advance || pos->y_advance) {
            printf("+");
            if (pos->x_advance)
                printf ("%d", pos->x_advance);
            if (pos->y_advance)
                printf (",%d", pos->y_advance);
        }
        ++pos;
        ++info;
    }
    printf( ">\n" );

    hb_font_destroy(font);
    FT_Done_Face( face );
    FT_Done_FreeType( library );
}

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

    harfbuzz_test2();

    // create the context
    GB_ERROR err;
    GB_CONTEXT* gb;
    err = GB_ContextMake(&gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }

    // create a font
    GB_Font* droidFont = NULL;
    err = GB_FontMake(gb, "Droid-Sans/DroidSans.ttf", 32, &droidFont);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    // create a text
    uint32_t origin[2] = {0, 0};
    uint32_t size[2] = {videoInfo->current_w, videoInfo->current_h};
    GB_Text* helloText = NULL;
    err = GB_TextMake(gb, "Hello World", droidFont, 0xffffffff, origin, size, GB_HORIZONTAL_ALIGN_CENTER,
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
            /*
            err = GB_DrawText(gb, helloText);
            if (err != GB_ERROR_NONE) {
                fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
                exit(1);
            }
            */
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
    err = GB_ContextRelease(gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    return 0;
}
