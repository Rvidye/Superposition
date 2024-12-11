#include<Log.h>
#include <ShaderProgram.h>

namespace AMC {

	AMC::ShaderProgram::ShaderProgram(const std::vector<std::string>& shaderFilePaths)
	{
		program = glCreateProgram();

		for (const auto& filepath : shaderFilePaths) 
		{
			GLuint shader = compileShader(filepath);
			if (shader != 0) {
				glAttachShader(program, shader);
				glDeleteShader(shader);
			}
		}

		linkProgram();
		queryUniforms();
	}

	ShaderProgram::~ShaderProgram()
	{
		if (program) {
			glDeleteProgram(program);
		}
	}

	GLuint ShaderProgram::compileShader(const std::string& filePath)
	{
		//freopen("CONOUT$", "w", stdout);
		std::cout << "Loading Shader : " << filePath << std::endl;
		//const std::string& filePath;
		//LOG_INFO(L"Loading Shader : %s\n", filePath);

		GLenum shaderType;
		std::filesystem::path fileName = std::filesystem::path(filePath).filename();
		bool isSpv = fileName.extension() == ".spv";
		if (isSpv) {
			fileName = fileName.replace_extension();
		}
		shaderType = getShaderType(fileName);

		GLuint shader = glCreateShader(shaderType);

		// Read shader source from file
		std::ios_base::openmode openFlags = 0;
		if (isSpv)
			openFlags = std::ios::binary;
		std::ifstream shaderFile(filePath, openFlags);
		if (!shaderFile) 
		{
			LOG(AMC::LogLevel::LOG_ERROR);
			std::cout << "Error opening shader file: " << filePath << std::endl;
			return 0;
		}

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		std::string shaderSource = shaderStream.str();

		// Resolve includes
		std::string finalSource = resolveIncludes(shaderSource);

		// Compile shader
		const char* sourceCStr = finalSource.c_str();
		if (isSpv) {
			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderSource.c_str(), shaderSource.length());
			glSpecializeShader(shader, "main", 0, nullptr, nullptr);
		}
		else {
			glShaderSource(shader, 1, &sourceCStr, nullptr);
			glCompileShader(shader);
		}
		// Check compilation status
		GLint compileStatus;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
		if (!compileStatus) 
		{
			GLint infoLogLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

			std::vector<char> infoLog(infoLogLength);
			glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
			LOG(AMC::LogLevel::LOG_ERROR);
			std::cout << "Shader compilation error in " << filePath << ":\n" << infoLog.data() << std::endl;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	std::string ShaderProgram::resolveIncludes(const std::string& src) 
	{
		std::string result = src;
		std::regex includePattern(R"(#include\s*<(.+?)>)");
		std::smatch match;

		// Continue resolving includes until none are left
		while (std::regex_search(result, match, includePattern)) 
		{
			std::string includePath = match[1];
			std::cout << "Including file: " << includePath << std::endl;

			// Read included file
			std::ifstream includeFile(includePath);
			if (!includeFile) 
			{
				LOG(AMC::LogLevel::LOG_ERROR);
				std::cout << "Error opening include file: " << includePath << std::endl;
				// Remove the include directive to prevent infinite loop
				result.replace(match.position(0), match.length(0), "");
				continue;
			}

			std::stringstream includeStream;
			includeStream << includeFile.rdbuf();
			std::string includeSrc = includeStream.str();

			// Recursively resolve includes in the included file
			includeSrc = resolveIncludes(includeSrc);

			// Replace the include directive with the included source
			result.replace(match.position(0), match.length(0), includeSrc);
		}
		return result;
	}

	GLenum ShaderProgram::getShaderType(const std::filesystem::path& filePath) 
	{
		const std::filesystem::path extension = filePath.extension();
		if (extension == ".vert") {
			return GL_VERTEX_SHADER;
		}
		else if (extension == ".frag") {
			return GL_FRAGMENT_SHADER;
		}
		else if (extension == ".geom") {
			return GL_GEOMETRY_SHADER;
		}
		else if (extension == ".tesc") {
			return GL_TESS_CONTROL_SHADER;
		}
		else if (extension == ".tese") {
			return GL_TESS_EVALUATION_SHADER;
		}
		else if (extension == ".comp") {
			return GL_COMPUTE_SHADER;
		}
		else {
			throw std::runtime_error("Unsupported shader type in file: " + filePath.string());
		}
	}

	void ShaderProgram::linkProgram() 
	{
		glLinkProgram(program);

		// Check link status
		GLint linkStatus;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (!linkStatus) 
		{
			GLint infoLogLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0)
			{
				std::vector<char> infoLog(infoLogLength);
				glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
				//LOG_ERROR(L"Program link error:\n : %s\n", infoLog.data());
				LOG(AMC::LogLevel::LOG_ERROR);
				std::cout << "Program link error:\n" << infoLog.data() << std::endl;
			}
		}
	}

	void ShaderProgram::queryUniforms()
	{
		GLint numUniforms = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);

		for (GLint i = 0; i < numUniforms; ++i) 
		{
			char nameBuffer[256];
			GLsizei length;
			GLint size;
			GLenum type;

			glGetActiveUniform(program, i, sizeof(nameBuffer), &length, &size, &type, nameBuffer);
			std::string name(nameBuffer, length);
			GLint location = glGetUniformLocation(program, name.c_str());
			uniforms[name] = location;
		}
	}

	void ShaderProgram::use() const {
		glUseProgram(program);
	}

	GLint ShaderProgram::getUniformLocation(const std::string& name) const 
	{
		auto it = uniforms.find(name);
		if (it != uniforms.end()) 
		{
			return it->second;
		}
		else 
		{
			//LOG_WARNING(L"Uniform '%s' not found.\n",name.c_str());
			std::cout << "Warning: Uniform '" << name << "' not found." << std::endl;
			return -1;
		}
	}

};
