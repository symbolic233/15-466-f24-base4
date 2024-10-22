#include "TextProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< TextProgram > text_program(LoadTagEarly, []() -> TextProgram const * {
	TextProgram *ret = new TextProgram();

	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	return ret;
});

TextProgram::TextProgram() {
	// Vertex and fragment shaders.
    // Much of this is sourced from https://learnopengl.com/In-Practice/Text-Rendering.
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 WORLD_TO_CLIP;\n"
		"layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
        "out vec2 TexCoords;\n"
        "void main() {\n"
            "gl_Position = WORLD_TO_CLIP * vec4(vertex.xy, 0.0, 1.0);\n"
            "TexCoords = vertex.zw;\n"
        "}\n"  
	,
		//fragment shader:
		"#version 330\n"
		"in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n" 
            "vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
            "color = vec4(textColor, 1.0) * sampled;\n"
        "}\n"
	);

	// //look up the locations of vertex attributes:
	// Position_vec4 = glGetAttribLocation(program, "Position");
	// Normal_vec3 = glGetAttribLocation(program, "Normal");
	// Color_vec4 = glGetAttribLocation(program, "Color");
	// TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	WORLD_TO_CLIP_mat4 = glGetUniformLocation(program, "WORLD_TO_CLIP");


	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

TextProgram::~TextProgram() {
	glDeleteProgram(program);
	program = 0;
}

