#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

//Shader program that draws transformed, lit, textured vertices tinted with vertex colors:
struct TextProgram {
	TextProgram();
	~TextProgram();

	GLuint program = 0;

	//Uniform (per-invocation variable) locations:
	GLuint WORLD_TO_CLIP_mat4 = -1U;
	
	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< TextProgram > text_program;
