#pragma once

#include<GL/glew.h>
#include<string>
#include<vector>
#include<map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

namespace AMC {
	class ShaderProgram
	{
		public:

			ShaderProgram(const std::vector<std::string>& shaderFilePaths);
			~ShaderProgram();

			void use() const;

			GLint getUniformLocation(const std::string& name) const;
			GLuint getProgramObject();
		private:
			GLuint program;
			std::map<std::string, GLint> uniforms;

			GLuint compileShader(const std::string& filePath);
			std::string resolveIncludes(const std::string& src);
			//TODO: Look into making it a constexpr for a faster and simpler code path
			GLenum getShaderType(const std::filesystem::path& filePath);

			void linkProgram(const std::string& shaderCombination);
			void queryUniforms();
	};
};

