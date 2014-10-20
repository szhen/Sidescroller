#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

namespace direction {
	enum directions { LEFT, RIGHT };
}

GLuint LoadTexture(const char *image_path, GLuint imageFormat);
bool isSolid(int);
float lerp(float v0, float v1, float t);