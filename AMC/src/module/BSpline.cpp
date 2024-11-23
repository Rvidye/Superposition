#include<BSpline.h>

namespace AMC {

    CubicBezierInterpolator::CubicBezierInterpolator(const std::vector<glm::vec3>& inCtrlps) : m_ctrlps(inCtrlps), m_nSplines(inCtrlps.size() / 3) {}

    CubicBezierInterpolator::CubicBezierInterpolator(const Eigen::MatrixX3f& inCtrlps) {
        for (auto& ctrlp : inCtrlps.rowwise()) {
            m_ctrlps.push_back(glm::vec3(ctrlp(0), ctrlp(1), ctrlp(2)));
        }
        m_nSplines = inCtrlps.rows() / 3;
    }
    CubicBezierInterpolator::~CubicBezierInterpolator() {}

    inline float CubicBezierInterpolator::getDistanceOnSpline(float t)
    {
        return t * float(m_nSplines);
    }

    inline const std::vector<glm::vec3>& CubicBezierInterpolator::getPoints(void)
    {
        return m_ctrlps;
    }

    glm::vec3 CubicBezierInterpolator::quadraticBezier(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, float t) {
        glm::vec3 D = glm::mix(A, B, t);
        glm::vec3 E = glm::mix(B, C, t);
        glm::vec3 result = glm::mix(D, E, t);
        return result;
    }

    glm::vec3 CubicBezierInterpolator::cubicBezier(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const glm::vec3& D, float t) {
        glm::vec3 E = glm::mix(A, B, t);
        glm::vec3 F = glm::mix(B, C, t);
        glm::vec3 G = glm::mix(C, D, t);
        return quadraticBezier(E, F, G, t);
    }

    glm::vec3 CubicBezierInterpolator::interpolate(float t) {

        float clampedT = glm::clamp(t, 0.0f, 1.0f);
        float splineLocal = getDistanceOnSpline(clampedT);
        unsigned indexIntoCtrlps = unsigned(splineLocal - 0.00001f);

        glm::vec3 A = m_ctrlps[indexIntoCtrlps * 3 + 0];
        glm::vec3 B = m_ctrlps[indexIntoCtrlps * 3 + 1];
        glm::vec3 C = m_ctrlps[indexIntoCtrlps * 3 + 2];
        glm::vec3 D = m_ctrlps[indexIntoCtrlps * 3 + 3];

        return cubicBezier(A, B, C, D, splineLocal - indexIntoCtrlps);
    }

    BsplineInterpolator::BsplineInterpolator(const std::vector<glm::vec3>& points)
        : m_pointsVec(points), m_cubicBezierInterpolator(nullptr) {
        recalculateSpline();
    }

    inline BsplineInterpolator::~BsplineInterpolator() {
        delete m_cubicBezierInterpolator;
    }

    inline float BsplineInterpolator::getDistanceOnSpline(float t) {
        return m_cubicBezierInterpolator->getDistanceOnSpline(t);
    }

    glm::vec3 BsplineInterpolator::interpolate(float t) {
        return m_cubicBezierInterpolator->interpolate(t);
    }

    inline const std::vector<glm::vec3>& BsplineInterpolator::getPoints(void) {
        return m_pointsVec;
    }

    void BsplineInterpolator::recalculateSpline(void) {
        int row = 0;
        size_t pointVecSize = m_pointsVec.size();
        Eigen::MatrixX3f m_points(pointVecSize, 3);
        for (auto& point : m_pointsVec) {
            m_points.row(row)(0) = point[0];
            m_points.row(row)(1) = point[1];
            m_points.row(row)(2) = point[2];
            ++row;
        }
        /* construct the 1-4-1 matrix and get its inverse */
        size_t nNonEnds = pointVecSize - 2;
        Eigen::MatrixXf cubicBsplineKernel(nNonEnds, nNonEnds);
        cubicBsplineKernel = Eigen::MatrixXf::Identity(nNonEnds, nNonEnds) * 4.0f;
        cubicBsplineKernel.block(0, 1, nNonEnds - 1, nNonEnds - 1).triangularView<Eigen::Upper>().fill(1.0f);
        cubicBsplineKernel.block(0, 2, nNonEnds - 2, nNonEnds - 2).triangularView<Eigen::Upper>().fill(0.0f);
        cubicBsplineKernel.block(1, 0, nNonEnds - 1, nNonEnds - 1).triangularView<Eigen::Lower>().fill(1.0f);
        cubicBsplineKernel.block(2, 0, nNonEnds - 2, nNonEnds - 2).triangularView<Eigen::Lower>().fill(0.0f);
        cubicBsplineKernel = cubicBsplineKernel.inverse();

        /* modify points to use in the 1-4-1 inverse transform equation */
        Eigen::MatrixX3f tempModifiedPoints(nNonEnds, 3);
        tempModifiedPoints.row(0) = 6.0f * m_points.row(1) - m_points.row(0);
        for (int i = 2; i < nNonEnds; i++)
        {
            tempModifiedPoints.row(i - 1) = 6.0f * m_points.row(i);
        }
        tempModifiedPoints.row(nNonEnds - 1) = 6.0f * m_points.row(pointVecSize - 2) - m_points.row(pointVecSize - 1);

        Eigen::MatrixX3f m_bspCtrlps(pointVecSize, 3);
        m_bspCtrlps.row(0) = m_points.row(0);
        m_bspCtrlps.block(1, 0, nNonEnds, 3) = cubicBsplineKernel * tempModifiedPoints;
        m_bspCtrlps.row(pointVecSize - 1) = m_points.row(pointVecSize - 1);

        Eigen::Index nCubicCPs = 2 * (m_bspCtrlps.rows() - 1) + m_points.rows();
        Eigen::MatrixX3f outCtrlps(nCubicCPs, 3);

        int count = 0;
        int i = 0;
        for (; i < m_bspCtrlps.rows() - 1; i++)
        {
            outCtrlps.row(count++) = m_points.row(i); // starts with a knot
            outCtrlps.row(count++) = (2.0f * m_bspCtrlps.row(i) / 3.0f) + (m_bspCtrlps.row(i + 1) / 3.0f);
            outCtrlps.row(count++) = (m_bspCtrlps.row(i) / 3.0f) + (2.0f * m_bspCtrlps.row(i + 1) / 3.0f);
        }
        outCtrlps.row(count++) = m_points.row(i); // ends with a knot

        if (m_cubicBezierInterpolator) {
            delete m_cubicBezierInterpolator;
        }
        m_cubicBezierInterpolator = new CubicBezierInterpolator(outCtrlps);
    }
};
