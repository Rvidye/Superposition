#pragma once

#include<GL/glew.h>
#include<unordered_map>
#include<string>

namespace AMC {

	class TextureManager {

        public:
            static GLuint LoadTexture(const std::string& filename, GLenum img_format = GL_RGBA8, int desiredchannels = 4,GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR ,GLenum WRAP_S = GL_REPEAT, GLenum WRAP_T = GL_REPEAT);
            static GLuint LoadKTX2Texture(const std::string& filename);
            static void UnloadTextures();

        private:
            static std::unordered_map<std::string, GLuint> textureMap;
	};
};
