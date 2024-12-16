#include<common.h>
#include<BSplineUtils.h>
#include<Camera.h>

namespace AMC {

    ShaderProgram* SplineRenderer::m_program = nullptr;
    GLuint SplineRenderer::m_vaoPoint = 0;
    GLuint SplineRenderer::m_vboPoint = 0;
    bool SplineRenderer::m_resourcesInitialized = false;

    SplineRenderer::SplineRenderer(SplineInterpolator* interpolator, float linspace)
        : m_interpolator(interpolator),
        m_nAllPositions(0),
        m_linspace(linspace),
        m_isRenderPoints(false),
        m_points(interpolator->getPoints()) {

        initializeSharedResources();

        glGenVertexArrays(1, &m_vaoSpline);
        glGenBuffers(1, &m_vboSpline);

        loadSplineGeometry();
    }

    void SplineRenderer::initializeSharedResources() {

        if (m_resourcesInitialized) return;

        // Load shared shader program
        m_program = new ShaderProgram({ RESOURCE_PATH("shaders\\color\\color.vert"), RESOURCE_PATH("shaders\\color\\color.frag") });

        // Create VAO and VBO for the cube (used for points)
        glGenVertexArrays(1, &m_vaoPoint);
        glGenBuffers(1, &m_vboPoint);

        const float verts[] = {
            // Cube vertices...
            0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,

            0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, -1.0f,

            0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, -1.0f,
            -1.0f, 0.0f, -1.0f,

            0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,
            -1.0f, 0.0f, 1.0f,

            0.0f, -1.0f, 0.0f,
            1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f,

            0.0f, -1.0f, 0.0f,
            1.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 1.0f,

            0.0f, -1.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,
            1.0f, 0.0f, -1.0f,

            0.0f, -1.0f, 0.0f,
            -1.0f, 0.0f, 1.0f,
            -1.0f, 0.0f, -1.0f,
        };

        glBindVertexArray(m_vaoPoint);
        glBindBuffer(GL_ARRAY_BUFFER, m_vboPoint);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        m_resourcesInitialized = true; // Mark resources as initialized
    }

    void SplineRenderer::loadSplineGeometry() {
        std::vector<glm::vec3> allPositions;

        // Interpolate through the spline
        for (float t = 0.0f; t <= 1.0f; t += m_linspace) {
            allPositions.push_back(m_interpolator->interpolate(t));
            ++m_nAllPositions;
        }

        // Create VAO and VBO for the specific spline
        glBindVertexArray(m_vaoSpline);
        glBindBuffer(GL_ARRAY_BUFFER, m_vboSpline);
        glBufferData(GL_ARRAY_BUFFER, allPositions.size() * sizeof(glm::vec3), allPositions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
    }

    void SplineRenderer::setRenderPoints(bool setting) {
        m_isRenderPoints = setting;
    }

    SplineRenderer::~SplineRenderer() {
        glDeleteBuffers(1, &m_vboSpline);
        glDeleteVertexArrays(1, &m_vaoSpline);
    }

    void SplineRenderer::render(const glm::vec4& lineColor, const glm::vec4& pointColor, const glm::vec4& selectedPointColor, int selected, float scalingFactor) const {

        m_program->use();
        // Pass uniform values
        glUniformMatrix4fv(m_program->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix()));
        glUniform4fv(m_program->getUniformLocation("color"), 1, glm::value_ptr(lineColor));

        // Render the spline as a line strip
        glBindVertexArray(m_vaoSpline);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(m_nAllPositions));

        // Render points if enabled
        if (m_isRenderPoints) {
            glBindVertexArray(m_vaoPoint);

            for (size_t i = 0; i < m_points.size(); ++i) {
                glm::vec4 color = (static_cast<int>(i) == selected) ? selectedPointColor : pointColor;
                glUniform4fv(m_program->getUniformLocation("color"), 1, glm::value_ptr(color));

                // Compute transformation matrix for the point
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_points[i]) * glm::scale(glm::mat4(1.0f), glm::vec3(scalingFactor));
                glUniformMatrix4fv(m_program->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(currentCamera->getProjectionMatrix() * currentCamera->getViewMatrix() * transform));

                glDrawArrays(GL_TRIANGLES, 0, 24);
            }
        }
    }

    SplineAdjuster::SplineAdjuster(BsplineInterpolator* spline)
        :splineInterpolator(spline),
        selectedPoint(0),
        scalingFactor(0.1f),
        isRenderPath(true),
        isRenderPoints(true),
        movementSpeed(0.1f) {
        splineRenderer = new AMC::SplineRenderer(splineInterpolator);
        splineRenderer->setRenderPoints(isRenderPoints);
    }

    inline void SplineAdjuster::setRenderPath(bool setting)
    {
        isRenderPath = setting;
    }

    inline void SplineAdjuster::setRenderPoints(bool setting) {
        splineRenderer->setRenderPoints(setting);
        isRenderPoints = setting;
    }

    inline void SplineAdjuster::setScalingFactor(float iscalingFactor)
    {
        scalingFactor = iscalingFactor;
    }

    void SplineAdjuster::keyboardfunc(char keyPressed, UINT keycode) {

        bool refresh = false;

        switch (keyPressed)
        {
        case 'I':
        case 'i':
            splineInterpolator->m_pointsVec[selectedPoint].z += movementSpeed;
            refresh = true;
            break;
        case 'K':
        case 'k':
            splineInterpolator->m_pointsVec[selectedPoint].z -= movementSpeed;
            refresh = true;
            break;
        case 'L':
        case 'l':
            splineInterpolator->m_pointsVec[selectedPoint].x += movementSpeed;
            refresh = true;
            break;
        case 'J':
        case 'j':
            splineInterpolator->m_pointsVec[selectedPoint].x -= movementSpeed;
            refresh = true;
            break;
        case 'O':
        case 'o':
            splineInterpolator->m_pointsVec[selectedPoint].y += movementSpeed;
            refresh = true;
            break;
        case 'U':
        case 'u':
            splineInterpolator->m_pointsVec[selectedPoint].y -= movementSpeed;
            refresh = true;
            break;
        default:
            break;
        }

        switch (keycode) {
        case VK_OEM_4:
            splineInterpolator->m_pointsVec.insert(splineInterpolator->m_pointsVec.begin() + selectedPoint + 1, splineInterpolator->m_pointsVec[selectedPoint]);
            refresh = true;
            break;
        case VK_OEM_6:
            splineInterpolator->m_pointsVec.erase(splineInterpolator->m_pointsVec.begin() + selectedPoint);
            selectedPoint = selectedPoint % splineInterpolator->m_pointsVec.size();
            refresh = true;
            break;
        }

        if (refresh) {
            splineInterpolator->recalculateSpline();
            delete splineRenderer;
            splineRenderer = new SplineRenderer(splineInterpolator);
            splineRenderer->setRenderPoints(isRenderPoints);
        }
    }

    void SplineAdjuster::render(const glm::vec4 linecolor, const glm::vec4 pointcolor, const glm::vec4 selectedpointcolor) {
        if (isRenderPath) {
            splineRenderer->render(linecolor, pointcolor, selectedpointcolor, selectedPoint, scalingFactor);
        }
    }

    void SplineAdjuster::renderUI() {
    #if defined(_MYDEBUG)
        ImGui::Text("Spline Adjuster Controls");
        ImGui::Checkbox("Render Path", &isRenderPath);
        if (ImGui::Button("Toogle Point Rendering")) {
            isRenderPoints = !isRenderPoints;
            splineRenderer->setRenderPoints(isRenderPoints);
        }
        ImGui::SliderFloat("Scaling Factor", &scalingFactor, 0.1f, 10.0f);
        ImGui::Text("Selected Path Point : %d", selectedPoint);
        if (ImGui::Button("Next Point")) {
            selectedPoint = (selectedPoint == 0) ? (int)splineInterpolator->getPoints().size() - 1 : (int)(selectedPoint - 1) % splineInterpolator->getPoints().size();
        }
        ImGui::SameLine();
        if (ImGui::Button("Previous Point")) {
            selectedPoint = (selectedPoint + 1) % splineInterpolator->getPoints().size();
        }
        if (ImGui::Button("Dump Spline")) {
            dump();
        }
        ImGui::Separator();
        ImGui::Text("To Move Selected Path Point");
        ImGui::Text("I/K = Z path point\nL/J = X path point\nO/U = Y path point");
        ImGui::Separator();
    #endif
    }

    inline BsplineInterpolator* SplineAdjuster::getSpline()
    {
        return splineInterpolator;
    }

    void SplineAdjuster::dump() {

        if (!splineInterpolator) {
            return;
        }

        std::ofstream file("SplineDump.txt", std::ios::app);
        if (!file.is_open()) {
            LOG_ERROR(L"Failed To Open SplineDump.txt");
            return;
        }

        file << "\n// Spline Points Dump\n";
        file << "std::vector<glm::vec3> controlPoints = {\n";
        const auto& points = splineInterpolator->getPoints();
        for (size_t i = 0; i < points.size(); ++i) {
            const auto& point = points[i];
            file << "    {" << std::fixed << std::setprecision(6)
                << point.x << "f, " << point.y << "f, " << point.z << "f}";
            if (i < points.size() - 1) {
                file << ",\n"; // Add a comma except for the last element
            }
            else {
                file << "\n"; // No comma for the last element
            }
        }
        file << "};\n";
        file.close();
        LOG_INFO(L"Successfully appended spline points");
    }

    SplineAdjuster::~SplineAdjuster()
    {
        delete splineRenderer;
    }

};
