#pragma once

#include<GL/glew.h>
#include<string>
#include<vector>
#include<map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

namespace AMC {
	class ShaderProgram
	{
		public:

			ShaderProgram(const std::vector<std::string>& shaderFilePaths);
			~ShaderProgram();

			void use() const;

			GLint getUniformLocation(const std::string& name) const;

		private:
			GLuint program;
			std::map<std::string, GLint> uniforms;

			GLuint compileShader(const std::string& filePath);
			std::string resolveIncludes(const std::string& src);
			GLenum getShaderType(const std::string& filePath);

			void linkProgram();
			void queryUniforms();
	};
};

