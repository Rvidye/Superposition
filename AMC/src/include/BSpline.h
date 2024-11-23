#pragma once

#include<glm/glm.hpp>
#include<iostream>
#include<vector>
#include<eigen/LU>
#include<stdexcept>

namespace AMC {

    class SplineInterpolator {
    public:
        virtual glm::vec3 interpolate(float t) = 0;
        virtual const std::vector<glm::vec3>& getPoints(void) = 0;
        virtual ~SplineInterpolator() = default;
    };

    class CubicBezierInterpolator : public SplineInterpolator {
    private:
        std::vector<glm::vec3> m_ctrlps;
        size_t m_nSplines;

        glm::vec3 quadraticBezier(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, float t);
        glm::vec3 cubicBezier(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const glm::vec3& D, float t);

    public:
        CubicBezierInterpolator(const std::vector<glm::vec3>& inCtrlps);
        CubicBezierInterpolator(const Eigen::MatrixX3f& inCtrlps);
        ~CubicBezierInterpolator();
        float getDistanceOnSpline(float t);
        glm::vec3 interpolate(float t) override;
        const std::vector<glm::vec3>& getPoints(void) override;
    };

    class BsplineInterpolator : public SplineInterpolator {
    private:
        std::vector<glm::vec3> m_pointsVec;
        CubicBezierInterpolator* m_cubicBezierInterpolator;
        friend class SplineAdjuster;
    public:
        BsplineInterpolator(const std::vector<glm::vec3>& points);
        ~BsplineInterpolator();
        float getDistanceOnSpline(float t);
        void recalculateSpline(void);
        glm::vec3 interpolate(float t) override;
        const std::vector<glm::vec3>& getPoints(void) override;
    };
};

