#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <string>

#include "data_path.hpp"

//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)
#define RESCALE 64

int main(int argc, char **argv) {
	FT_Library ft_library;
	FT_Init_FreeType(&ft_library);

	hb_buffer_t *buf = hb_buffer_create();

	std::string fontfile;
  	std::string text;

	fontfile = data_path("dist/opensans-regular.ttf");
	text = "Sample text 1";

	/* Initialize FreeType and create FreeType font face. */
	FT_Face ft_face;
	// FT_Error ft_error;

	std::cout << "setup..." << std::endl;

	if ((FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face))) {
		std::cout << "did not find fontfile" << std::endl;
		throw std::runtime_error("did not find fontfile");
	}
	if ((FT_Set_Char_Size(ft_face, FONT_SIZE*RESCALE, FONT_SIZE*RESCALE, 0, 0))) {
		throw std::runtime_error("could not set char size");
	}

	/* Create hb-ft font. */
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (ft_face, NULL);

	std::cout << "initialization done" << std::endl;

	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (hb_buffer, text.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties (hb_buffer);

	/* Shape it! */
	hb_shape (hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	unsigned int len = hb_buffer_get_length (hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

	/* Print them out as is. */
	std::cout << "Raw buffer contents:" << std::endl;
	for (unsigned int i = 0; i < len; i++) {
		hb_codepoint_t gid   = info[i].codepoint;
		unsigned int cluster = info[i].cluster;
		float x_advance = pos[i].x_advance / (float)RESCALE;
		float y_advance = pos[i].y_advance / (float)RESCALE;
		float x_offset  = pos[i].x_offset / (float)RESCALE;
		float y_offset  = pos[i].y_offset / (float)RESCALE;

		char glyphname[32];
		hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

		std::cout << "glpyh='" << glyphname << "' ";
		std::cout << "cluster=" << cluster << "  ";
		std::cout << "advance=(" << x_advance << "," << y_advance << ")" << " ";
		std::cout << "offset=(" << x_offset << "," << y_offset << ")" << " ";
		std::cout << std::endl;
	}

	std::cout << "Converted to absolute positions:" << std::endl;
	/* And converted to absolute positions. */
	{
		float current_x = 0;
		float current_y = 0;
		for (unsigned int i = 0; i < len; i++) {
			hb_codepoint_t gid   = info[i].codepoint;
			unsigned int cluster = info[i].cluster;
			float x_position = current_x + pos[i].x_offset / (float)RESCALE;
			float y_position = current_y + pos[i].y_offset / (float)RESCALE;

			char glyphname[32];
			hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

			std::cout << "glpyh='" << glyphname << "' ";
			std::cout << "cluster=" << cluster << "  ";
			std::cout << "position=(" << x_position << "," << y_position << ")" << " ";
			std::cout << std::endl;

			current_x += pos[i].x_advance / (float)RESCALE;
			current_y += pos[i].y_advance / (float)RESCALE;
		}
	}

	hb_buffer_destroy(buf);
	hb_font_destroy (hb_font);
	FT_Done_Face (ft_face);
	FT_Done_FreeType (ft_library);

	std::cout << "It worked?" << std::endl;
}
