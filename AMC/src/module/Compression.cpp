#include<Compression.h>
#include<cmath>
#include<algorithm>

namespace AMC {

    glm::vec3 Compression::DecompressUR11G11B10(uint32_t data) {
        float r = static_cast<float>(data >> 0 & ((1u << 11) - 1));
        float g = static_cast<float>(data >> 11 & ((1u << 11) - 1));
        float b = static_cast<float>(data >> 22 & ((1u << 10) - 1));

        r *= 1.0f / ((1u << 11) - 1);
        g *= 1.0f / ((1u << 11) - 1);
        b *= 1.0f / ((1u << 10) - 1);

        return glm::vec3(r, g, b);
    }

    uint32_t Compression::CompressSR11G11B10(const glm::vec3& data) {
        return CompressUR11G11B10(data * 0.5f + glm::vec3(0.5f));
    }

    uint32_t Compression::CompressUR11G11B10(const glm::vec3& data) {
        uint32_t r = static_cast<uint32_t>(std::round(data.x * ((1u << 11) - 1)));
        uint32_t g = static_cast<uint32_t>(std::round(data.y * ((1u << 11) - 1)));
        uint32_t b = static_cast<uint32_t>(std::round(data.z * ((1u << 10) - 1)));

        return (b << 22) | (g << 11) | (r << 0);
    }

    uint32_t Compression::CompressUR8G8B8A8(const glm::vec4& data) {
        uint32_t r = static_cast<uint32_t>(std::round(data.x * ((1u << 8) - 1)));
        uint32_t g = static_cast<uint32_t>(std::round(data.y * ((1u << 8) - 1)));
        uint32_t b = static_cast<uint32_t>(std::round(data.z * ((1u << 8) - 1)));
        uint32_t a = static_cast<uint32_t>(std::round(data.w * ((1u << 8) - 1)));

        return (a << 24) | (b << 16) | (g << 8) | (r << 0);
    }

    uint32_t Compression::CompressSR8G8B8A8(const glm::vec4& data) {
        return CompressUR8G8B8A8(data * 0.5f + glm::vec4(0.5f));
    }

    glm::vec2 Compression::EncodeUnitVec(const glm::vec3& v) {
        glm::vec2 p = glm::vec2(v.x, v.y) * (1.0f / (std::abs(v.x) + std::abs(v.y) + std::abs(v.z)));
        return (v.z <= 0.0f) ? ((glm::vec2(1.0f) - Abs(glm::vec2(p.y, p.x))) * SignNotZero(p)) : p;
    }

    glm::vec2 Compression::SignNotZero(const glm::vec2& v) {
        return glm::vec2((v.x >= 0.0f) ? 1.0f : -1.0f, (v.y >= 0.0f) ? 1.0f : -1.0f);
    }

    glm::vec2 Compression::Abs(const glm::vec2& v) {
        return glm::vec2(std::abs(v.x), std::abs(v.y));
    }
};