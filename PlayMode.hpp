#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	glm::mat4 world_to_clip;
	glm::mat4 set_to_screen;

	struct Character {
		unsigned int id;  // ID handle of the glyph texture
		glm::uvec2 size;       // Size of glyph
		glm::vec2 offset;    // Offset from baseline to left/top of glyph
		glm::vec2 advance;    // Offset to advance to next glyph
	};

	std::shared_ptr< Sound::PlayingSample > rocket_loop;

	GLuint VAO_text, VBO_text = -1U;
	std::string fontfile;
	FT_Face ft_face;
	FT_Library ft_library;
	hb_font_t *hb_font;

	void draw_text_line(std::string text, float x, float y, glm::vec3 color);

	std::string upper_text;
	std::string lower_text;
	struct TextState {
		uint32_t line;
		uint32_t choice = 1;
		std::string text;
		uint32_t upper = false;
		std::vector<uint32_t> next;
	};
	// std::vector<TextState> lines;
	TextState current_state;
	// void build_lines();
	void apply_state(uint32_t location);

	//camera:
	Scene::Camera *camera = nullptr;

};
