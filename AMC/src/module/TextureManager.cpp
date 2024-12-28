#include<TextureManager.h>
#define STB_IMAGE_IMPLEMENTATION
#include<stb_image.h>
#include<ktx/ktx.h>
//#include<ktx/ktxvulkan.h>
#include<iostream>
#include<common.h>

namespace AMC {

	std::unordered_map<std::string, GLuint> TextureManager::textureMap;
    
    // Load a general texture (using stb_image)
    GLuint TextureManager::LoadTexture(const std::string& filename, GLenum img_format, int desiredchannels, GLenum minFilter, GLenum magFilter, GLenum WRAP_S, GLenum WRAP_T)
    {
        // Check if the texture is already loaded
        auto it = textureMap.find(filename);
        //if (it != textureMap.end())
        //{
        //    return it->second;
        //}

        // Load the image using stb_image
        int width, height, nchannels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nchannels, desiredchannels);
        if (!data)
        {
            std::cout << "ERROR: Failed to load texture " << filename << std::endl;
            return 0;
        }

        if (nchannels == 0)
        {
            stbi_image_free(data);
            std::cout << "ERROR: Unsupported texture format in file " << filename << std::endl;
            return 0;
        }

        GLenum dataFormat;
        switch (desiredchannels)
        {
        case 1: dataFormat = GL_RED; break;
        case 2: dataFormat = GL_RG; break;
        case 3: dataFormat = GL_RGB; break;
        case 4: dataFormat = GL_RGBA; break;
        default:
            std::cout << "ERROR: Unsupported number of channels in texture: " << desiredchannels << std::endl;
            stbi_image_free(data);
            return 0;
        }

        // Generate and bind the texture
        GLuint texID;
        glCreateTextures(GL_TEXTURE_2D, 1, &texID);
        glTextureParameteri(texID, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(texID, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(texID, GL_TEXTURE_WRAP_S, WRAP_S);
        glTextureParameteri(texID, GL_TEXTURE_WRAP_T, WRAP_T);
        glTextureStorage2D(texID, 1, img_format, width, height);
        glTextureSubImage2D(texID, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);

        if (minFilter == GL_LINEAR_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_NEAREST_MIPMAP_NEAREST) {
            glGenerateTextureMipmap(texID);
        }

        stbi_image_free(data);
        textureMap[filename] = texID;
        return texID;
    }

    // Load a KTX2 texture (using KTX library)
    GLuint TextureManager::LoadKTX2Texture(const std::string& filename)
    {
        // Check if the texture is already loaded
        auto it = textureMap.find(filename);
        if (it != textureMap.end())
        {
            return it->second;
        }

        // Load the KTX2 texture
        ktxTexture* kTexture = nullptr;
        KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
        if (result != KTX_SUCCESS)
        {
            std::cout << "ERROR: Failed to load KTX2 texture " << filename << ": " << ktxErrorString(result) << std::endl;
            return 0;
        }

        GLuint texID = 0;
        GLenum target = GL_INVALID_ENUM, glerror = GL_INVALID_ENUM;
        result = ktxTexture_GLUpload(kTexture, &texID, &target, &glerror);
        if (result != KTX_SUCCESS)
        {
            std::cout << "ERROR: Failed to upload KTX2 texture " << filename << ": " << ktxErrorString(result) << glerror <<std::endl;
            ktxTexture_Destroy(kTexture);
            return 0;
        }

        ktxTexture_Destroy(kTexture);

        // Cache the texture ID
        textureMap[filename] = texID;
        return texID;
    }

    // Unload all textures
    void TextureManager::UnloadTextures()
    {
        for (auto it = textureMap.begin(); it != textureMap.end(); ++it)
        {
            GLuint texID = it->second;
            glDeleteTextures(1, &texID);
        }
        textureMap.clear();
    }

};