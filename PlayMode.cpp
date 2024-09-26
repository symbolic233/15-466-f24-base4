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

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)
#define RESCALE 64

GLuint camera_mesh_program = 0;
Load< MeshBuffer > camera_mesh(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	camera_mesh_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > camera_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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

void PlayMode::draw_text_line(glm::vec3 color) {
	// We start by using HarfBuzz and FreeType together to shape the characters.
	// Much of this is adapted from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
	FT_Library ft_library;
	FT_Init_FreeType(&ft_library);

	hb_buffer_t *buf = hb_buffer_create();

	std::string fontfile;
  	std::string text;

	fontfile = data_path("opensans-regular.ttf");
	text = "Sample text part 1";

	/* Initialize FreeType and create FreeType font face. */
	FT_Face ft_face;
	FT_Error ft_error;

	if ((FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face))) {
		throw std::runtime_error("did not find fontfile");
	}
	if ((FT_Set_Char_Size(ft_face, FONT_SIZE*RESCALE, FONT_SIZE*RESCALE, 0, 0))) {
		throw std::runtime_error("could not set char size");
	}

	/* Create hb-ft font. */
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (ft_face, NULL);

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
		float x_advance = pos[i].x_advance / (float)RESCALE;
		float y_advance = pos[i].y_advance / (float)RESCALE;
		float x_offset  = pos[i].x_offset / (float)RESCALE;
		float y_offset  = pos[i].y_offset / (float)RESCALE;

		// Now that we have the shapes of the characters, we have to use FT to get what the glyph looks like.
		// Then we can use GL for the textures.
		// Much of this part of the code is adapted from https://learnopengl.com/In-Practice/Text-Rendering
		ft_error = FT_Load_Glyph(ft_face, gid, FT_LOAD_RENDER);
		// if (ft_face->glyph->bitmap.buffer != nullptr)
		// 	std::cout << ft_face->glyph->bitmap.buffer << std::endl;
		// else std::cout << "<blank>" << std::endl;

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
			glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
			glm::ivec2(x_offset, y_offset),
			glm::ivec2(x_advance, y_advance)
		};
		text_characters.push_back(next);
	}

	// I don't need absolute positions, so we ignore that part of the HB/FT tutorial.

	hb_buffer_destroy(buf);
	hb_font_destroy (hb_font);
	FT_Done_Face (ft_face);
	FT_Done_FreeType (ft_library);

	// Draw the text.
	glUseProgram(text_program->program);

	assert(camera->transform);
	glm::mat4 world_to_clip = camera->make_projection() * glm::mat4(camera->transform->make_world_to_local());

	// set the angles
	glUniform3f(glGetUniformLocation(text_program->program, "textColor"), color.x, color.y, color.z);
	glUniformMatrix4fv(glGetUniformLocation(text_program->program, "WORLD_TO_CLIP"), 1, 0, glm::value_ptr(world_to_clip));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_text);

    // iterate through all characters
	float x = 0.0f;
	float y = 0.0f;
	float scale = 1.0f;
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
		y += ch.advance.y * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0); // so we can draw other stuff
}

PlayMode::PlayMode() : scene(*camera_scene) {

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	rocket_loop = Sound::loop(*rocket_sample, 1.0f, 0.0f);

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
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	glUseProgram(lit_color_texture_program->program);
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);
	draw_text_line(glm::vec3{});

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Nothing here",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	}
	GL_ERRORS();
}
