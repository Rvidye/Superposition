#include<TextureManager.h>
#define STB_IMAGE_IMPLEMENTATION
#include<stb_image.h>
#include<ktx/ktx.h>
//#include<ktx/ktxvulkan.h>
#include<iostream>

namespace AMC {

    std::unordered_map<std::string, GLuint> TextureManager::textureMap;

    // Load a general texture (using stb_image)
    GLuint TextureManager::LoadTexture(const std::string& filename) {
        // Check if the texture is already loaded
        auto it = textureMap.find(filename);
        if (it != textureMap.end()) {
            return it->second;
        }

        // Load the image using stb_image
        int width, height, nchannels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nchannels, 0);
        if (!data) {
            std::cout << "ERROR: Failed to load texture " << filename << std::endl;
            return 0;
        }

        // Determine the image format
        GLenum img_format = GL_RGBA;
        GLenum internal_format = GL_RGBA;

        if (nchannels == 1) {
            img_format = GL_RED;
            internal_format = GL_RED;
        }
        else if (nchannels == 3) {
            img_format = GL_RGB;
            internal_format = GL_RGB;
        }
        else if (nchannels == 4) {
            img_format = GL_RGBA;
            internal_format = GL_RGBA;
        }
        else {
            stbi_image_free(data);
            std::cout << "ERROR: Unsupported texture format in file " << filename << std::endl;
            return 0;
        }

        // Generate and bind the texture
        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // or GL_NEAREST_MIPMAP_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // or GL_NEAREST

        // Upload the texture data
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, img_format, GL_UNSIGNED_BYTE, data);

        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Unbind the texture and free image data
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        textureMap[filename] = texID;
        return texID;
    }

    // Load a general texture (using stb_image)
    GLuint TextureManager::LoadCubeTexture(std::vector<std::string>& faces) {
        // Check if the texture is already loaded
        auto it = textureMap.find(faces[0]);

        if (it != textureMap.end()) {
            return it->second;
        }


        // Load the image using stb_image
        int width, height, nchannels;

        // Determine the image format
        GLenum img_format = GL_RGBA;
        GLenum internal_format = GL_RGBA;

        // Generate and bind the texture
        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        for (unsigned int i = 0; i < faces.size(); i++) {
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nchannels, 0);

            if (nchannels == 1) {
                img_format = GL_RED;
                internal_format = GL_RED;
            }
            else if (nchannels == 3) {
                img_format = GL_RGB;
                internal_format = GL_RGB;
            }
            else if (nchannels == 4) {
                img_format = GL_RGBA;
                internal_format = GL_RGBA;
            }
            else {
                stbi_image_free(data);
                std::cout << "ERROR: Unsupported texture format in file " << faces[i].c_str() << std::endl;
                return 0;
            }

            if (data) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, internal_format, width, height, 0, img_format, GL_UNSIGNED_BYTE, data
                );
                stbi_image_free(data);
            }
            else {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // or GL_NEAREST_MIPMAP_NEAREST
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // or GL_NEAREST

        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // Unbind the texture and free image data
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        textureMap[faces[0]] = texID;
        return texID;
    }

    // Load a KTX2 texture (using KTX library)
    GLuint TextureManager::LoadKTX2Texture(const std::string& filename) {
        // Check if the texture is already loaded
        auto it = textureMap.find(filename);
        if (it != textureMap.end()) {
            return it->second;
        }

        // Load the KTX2 texture
        ktxTexture* kTexture = nullptr;
        KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
        if (result != KTX_SUCCESS) {
            std::cout << "ERROR: Failed to load KTX2 texture " << filename << ": " << ktxErrorString(result) << std::endl;
            return 0;
        }

        GLuint texID = 0;
        GLenum target = GL_INVALID_ENUM, glerror = GL_INVALID_ENUM;
        result = ktxTexture_GLUpload(kTexture, &texID, &target, &glerror);
        if (result != KTX_SUCCESS) {
            std::cout << "ERROR: Failed to upload KTX2 texture " << filename << ": " << ktxErrorString(result) << glerror << std::endl;
            ktxTexture_Destroy(kTexture);
            return 0;
        }

        ktxTexture_Destroy(kTexture);

        // Cache the texture ID
        textureMap[filename] = texID;
        return texID;
    }

    // Unload all textures
    void TextureManager::UnloadTextures() {
        for (auto it = textureMap.begin(); it != textureMap.end(); ++it) {
            GLuint texID = it->second;
            glDeleteTextures(1, &texID);
        }
        textureMap.clear();
    }

};