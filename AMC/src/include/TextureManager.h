#pragma once

#include<GL/glew.h>
#include<unordered_map>
#include<string>

namespace AMC {

	class TextureManager {

        public:
            static GLuint LoadTexture(const std::string& filename);
            static GLuint LoadKTX2Texture(const std::string& filename);
            static void UnloadTextures();

        private:
            static std::unordered_map<std::string, GLuint> textureMap;
	};
};
