#pragma once

#include<glm/glm.hpp>
#include<cmath>
#include<cstdint>

namespace AMC {

    class Compression {
    public:
        static glm::vec3 DecompressUR11G11B10(uint32_t data);
        static uint32_t CompressSR11G11B10(const glm::vec3& data);
        static uint32_t CompressUR11G11B10(const glm::vec3& data);
        static uint32_t CompressUR8G8B8A8(const glm::vec4& data);
        static uint32_t CompressSR8G8B8A8(const glm::vec4& data);
        static glm::vec2 EncodeUnitVec(const glm::vec3& v);

    private:
        static glm::vec2 SignNotZero(const glm::vec2& v);
        static glm::vec2 Abs(const glm::vec2& v);
    };
}


