#pragma once

#include<BSpline.h>
#include<ShaderProgram.h>
//#include<Camera.h>

namespace AMC {

    class SplineRenderer {
    public:
        SplineRenderer(SplineInterpolator* interpolator,const float linspace = 0.01f);
        ~SplineRenderer();

        void render(const glm::vec4& lineColor, const glm::vec4& pointColor, const glm::vec4& selectedPointColor, int selected, float scalingFactor) const;
        void setRenderPoints(bool setting);

    private:
        void initializeSharedResources();
        void loadSplineGeometry();

        SplineInterpolator* m_interpolator;
        std::vector<glm::vec3> m_points;

        GLuint m_vaoSpline;
        GLuint m_vboSpline;

        size_t m_nAllPositions;
        float m_linspace;
        bool m_isRenderPoints;

        static ShaderProgram* m_program;
        static GLuint m_vaoPoint;         
        static GLuint m_vboPoint;         
        static bool m_resourcesInitialized;
    };

    class SplineAdjuster {
    private:
        BsplineInterpolator* splineInterpolator;
        SplineRenderer* splineRenderer;
        bool isRenderPath;
        bool isRenderPoints;
        int selectedPoint;
        float scalingFactor;
        float movementSpeed;
    public:
        SplineAdjuster(BsplineInterpolator* spline);
        void setRenderPath(bool setting);
        void setRenderPoints(bool setting);
        void setScalingFactor(float scalingFactor);
        void keyboardfunc(char keyPressed, UINT keycode);
        void render(const glm::vec4 linecolor, const glm::vec4 pointcolor, const glm::vec4 selectedpointcolor);
        void renderUI();
        BsplineInterpolator* getSpline();
        void dump();
        ~SplineAdjuster();
    };
};
