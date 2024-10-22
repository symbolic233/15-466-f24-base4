#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#define FONT_SIZE 20
#define MARGIN (FONT_SIZE * .5)
#define RESCALE 64
#define MAX_LINE_LENGTH 75
#define LINE_HEIGHT 30

GLuint camera_mesh_program = 0;
Load< MeshBuffer > camera_mesh(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("scene-bg.pnct"));
	camera_mesh_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > camera_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("scene-bg.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = camera_mesh->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = camera_mesh_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > rocket_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("rocket.wav"));
});

Load<std::map<uint32_t, PlayMode::TextState>> script_lines(LoadTagDefault, []() -> std::map<uint32_t, PlayMode::TextState> const * {
	// Some file reading guidance from https://stackoverflow.com/questions/7868936/read-file-line-by-line-using-ifstream-in-c
	std::map<uint32_t, PlayMode::TextState> script;
	
	uint32_t lnum = 1;
    std::ifstream infile(data_path("script.txt"));
	std::string line;
	PlayMode::TextState state;
	while (std::getline(infile, line)) {
		if (lnum % 2 == 1) {
			// jump condition line
			std::istringstream iss(line);
			iss >> state.line >> state.choice;
			uint32_t option;
			for (uint32_t i = 0; i < state.choice; i++) {
				iss >> option;
				state.next.push_back(option);
			}
			iss >> state.upper; // might not go through, but it's ok
		}
		else {
			state.text = line;
			script[state.line] = state;
			state = PlayMode::TextState();
		}
		lnum++;
    }
    return new std::map<uint32_t, PlayMode::TextState>(script);
});

void PlayMode::apply_state(uint32_t location) {
	current_state = (*script_lines).at(location);
	if (current_state.upper) upper_text = current_state.text;
	else lower_text = current_state.text;
}

void PlayMode::draw_text_line(std::string text, float x, float y, glm::vec3 color) {
	// We start by using HarfBuzz and FreeType together to shape the characters.
	// Some of this is adapted from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
	/* Initialize FreeType and create FreeType font face. */
	FT_Error ft_error;
	hb_buffer_t *buf = hb_buffer_create();


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

	std::vector<Character> text_characters;
	/* Print them out as is. */
	for (unsigned int i = 0; i < len; i++) {
		hb_codepoint_t gid   = info[i].codepoint;
		// unsigned int cluster = info[i].cluster; // unused
		float x_advance = (float)pos[i].x_advance / (float)RESCALE;
		float y_advance = (float)pos[i].y_advance / (float)RESCALE;
		float x_offset  = (float)pos[i].x_offset / (float)RESCALE;
		float y_offset  = (float)pos[i].y_offset / (float)RESCALE;

		// Now that we have the shapes of the characters, we have to use FT to get what the glyph looks like.
		// Then we can use GL for the textures.
		// Some of this is adapted from https://learnopengl.com/In-Practice/Text-Rendering
		ft_error = FT_Load_Glyph(ft_face, gid, FT_LOAD_RENDER);

		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		// This is the main actual creation of the texture.
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			ft_face->glyph->bitmap.width,
			ft_face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			ft_face->glyph->bitmap.buffer
		);

		// useful texture settings
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// The original code takes the offset and advance from FT, but we want the HB values.
		Character next = {
			texture, 
			glm::uvec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
			glm::vec2(x_offset + (float)ft_face->glyph->bitmap_left, y_offset + (float)ft_face->glyph->bitmap_top),
			glm::vec2(x_advance, y_advance)
		};
		text_characters.push_back(next);
	}

	// I don't need absolute positions, so we ignore that part of the HB/FT tutorial.
	hb_buffer_destroy(buf);
	
	// Draw the text.
	glUseProgram(text_program->program);

	// set the angles
	glUniform3f(glGetUniformLocation(text_program->program, "textColor"), color.x, color.y, color.z);
	glUniformMatrix4fv(glGetUniformLocation(text_program->program, "WORLD_TO_CLIP"), 1, 0, glm::value_ptr(set_to_screen));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_text);

    // iterate through all characters
	float scale = 1.0f;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (Character ch: text_characters) {
        float xpos = x + ch.offset.x * scale;
        float ypos = y - (ch.size.y - ch.offset.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
		// std::cout << "Drawing at " << xpos << ", " << ypos << std::endl;
        // update VBO for each character
		// This works because you draw two triangles for the entire quad (rectangle) of the text.
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f},            
            { xpos,     ypos,       0.0f, 1.0f},
            { xpos + w, ypos,       1.0f, 1.0f},

            { xpos,     ypos + h,   0.0f, 0.0f},
            { xpos + w, ypos,       1.0f, 1.0f},
            { xpos + w, ypos + h,   1.0f, 0.0f}           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.id);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO_text);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance was divided by rescale earlier)
        x += ch.advance.x * scale;
		// y -= ch.advance.y * scale;

		// Each time we use a texture, it's no longer used later
		glDeleteTextures(1, &ch.id);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0); // so we can draw other stuff
}

void PlayMode::draw_text_par(std::string text, float x, float y, glm::vec3 color) {
	uint32_t i = 0;
	uint32_t lnum = 0;
	while (i < text.length()) {
		uint32_t length = MAX_LINE_LENGTH;
		while (i + length < text.length() && text[i + length] != ' ') length--;
		std::string line = text.substr(i, length);
		draw_text_line(line, x, y - lnum * (float)LINE_HEIGHT, color);
		i += length + 1; // skip the space
		lnum++;
	}
}

PlayMode::PlayMode() : scene(*camera_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	rocket_loop = Sound::loop(*rocket_sample, 1.0f, 0.0f);

	// build_lines();
	apply_state(1);

	set_to_screen = glm::ortho(0.0f, 960.0f, 0.0f, 540.0f);

	// Do some font setup
	// Some of the code here is adapted from the same two links from draw_text_line.
	fontfile = data_path("OpenSans-Regular.ttf");

	FT_Init_FreeType(&ft_library);
	if ((FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face))) {
		throw std::runtime_error("did not find fontfile");
	}
	if ((FT_Set_Char_Size(ft_face, FONT_SIZE*RESCALE, FONT_SIZE*RESCALE, 0, 0))) {
		throw std::runtime_error("could not set char size");
	}

	/* Create hb-ft font. */
	hb_font = hb_ft_font_create (ft_face, NULL);

	// Do some text texture setup
	// Setup the textures
	glUseProgram(text_program->program);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glGenVertexArrays(1, &VAO_text);
	glGenBuffers(1, &VBO_text);
	glBindVertexArray(VAO_text);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_text);
	// Note: sizeof(Character) should be sizeof(float) * 7
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glUseProgram(0); // so we can draw other stuff
}

PlayMode::~PlayMode() {
	FT_Done_Face (ft_face);
	FT_Done_FreeType (ft_library);
	hb_font_destroy(hb_font);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (current_state.choice == 1) {
		// mouse to progress
		if (evt.type == SDL_MOUSEBUTTONDOWN) {
			apply_state(current_state.next[0]);
			return true;
		}
	}
	else {
		if (evt.type == SDL_KEYDOWN) {
			uint32_t keychoice = 0;
			if (evt.key.keysym.sym == SDLK_1) keychoice = 1;
			if (evt.key.keysym.sym == SDLK_2) keychoice = 2;
			if (evt.key.keysym.sym == SDLK_3) keychoice = 3;
			if (!keychoice || (current_state.choice < keychoice)) return false;
			apply_state(current_state.next[keychoice - 1]);
			return true;
		}
	}

	set_to_screen = glm::ortho(0.0f, float(window_size.x), 0.0f, float(window_size.y));

	return false;
}

void PlayMode::update(float elapsed) {

	if (current_state.upper) apply_state(current_state.next[0]); // don't prompt upper text

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	world_to_clip = camera->make_projection() * glm::mat4(camera->transform->make_world_to_local());
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);
	draw_text_par(upper_text, 10.0f, 360.0f, glm::vec3{0.0f, 0.0f, 1.0f});
	draw_text_par(lower_text, 10.0f, 180.0f, glm::vec3{});
	GL_ERRORS();
}
