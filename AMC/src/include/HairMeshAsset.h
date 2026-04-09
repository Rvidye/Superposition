#pragma once

#include<common.h>
#include<Model.h>
#include<array>
#include<string>
#include<vector>

namespace AMC {
	namespace HairBindings {
		inline constexpr GLuint HairAssetUBO = 24;
		inline constexpr GLuint HairBundleSSBO = 25;
		inline constexpr GLuint HairRootSampleSSBO = 26;
		inline constexpr GLuint HairSimulationParamsUBO = 27;
		inline constexpr GLuint HairSimulationReadSSBO = 28;
		inline constexpr GLuint HairSimulationWriteSSBO = 29;
		inline constexpr GLuint HairSimulationRestSSBO = 30;
	}

	inline constexpr float HairSentinel = 1.0f / 255.0f;
	inline constexpr int HairLayerCount = 2;
	inline constexpr int HairStrandSegmentCount = 16;
	inline constexpr int HairRootSampleCount = 64;
	inline constexpr int HairMeshSliceCount = HairLayerCount * 4 + 1;
	inline constexpr int HairStyleSliceCount = HairLayerCount + 1;
	inline constexpr int HairPatchQuadCount = 4;
	inline constexpr float HairRootHalfExtent = 0.28f;
	inline constexpr float HairLayerHalfExtent = 0.18f;
	inline constexpr float HairLayerHeight = 0.9f;
	inline constexpr float HairRibbonHalfWidth = 0.006f;

	enum class HairBundleTopology : int {
		Quad = 0,
		Triangle = 1
	};

	struct HairAssetGpuData {
		glm::ivec4 Counts = glm::ivec4(0);
		glm::vec4 LODParams = glm::vec4(0.0f);
		glm::vec4 FuzzParams = glm::vec4(0.0f);
		glm::vec4 FrizzParams = glm::vec4(0.0f);
		glm::vec4 KinkParams = glm::vec4(0.0f);
		glm::vec4 CurlParams = glm::vec4(0.0f);
		glm::vec4 StyleExtraParams = glm::vec4(0.0f);
		glm::vec4 ClumpParams = glm::vec4(0.0f);
		GLuint64 HairMeshTexture = 0;
		GLuint64 UVTexture = 0;
		GLuint64 WTexture = 0;
		GLuint64 UTexture = 0;
		GLuint64 VTexture = 0;
	};

	struct HairStyleParameters {
		float FuzzAmplitude = 0.0035f;
		float FuzzAmplitudeExponent = 1.35f;
		float FuzzDistributionExponent = 1.1f;

		float FrizzAmplitude = 0.0075f;
		float FrizzAmplitudeExponent = 1.2f;
		float FrizzFrequency = 8.0f;

		float KinkAmplitude = 0.0100f;
		float KinkAmplitudeExponent = 1.1f;
		float KinkUVFrequency = 6.0f;
		float KinkWFrequency = 10.0f;

		float CurlAmplitude = 0.0180f;
		float CurlAmplitudeExponent = 1.0f;
		float CurlDistributionExponent = 1.35f;
		float CurlFrequency = 3.5f;
		float CurlRotations = 2.25f;

		float ClumpThickness = 0.0120f;
		float ClumpExponent = 1.25f;
		float ClumpNoiseAmount = 0.1800f;
		float ClumpNoiseFrequency = 4.0f;
		float ClumpGridResolution = 5.0f;
		float ClumpPercentage = 0.8500f;
		float ClumpVariation = 0.3500f;
	};

	struct HairSimulationParams {
		bool Enabled = false;
		glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
		glm::vec3 WindDirection = glm::vec3(1.0f, 0.0f, 0.0f);
		float WindStrength = 0.45f;
		float WindPulseFrequency = 0.85f;
		float Stiffness = 28.0f;
		float Damping = 0.22f;
		float TipInfluence = 1.0f;
		float MaxDisplacement = 0.18f;
		float TimeScale = 1.0f;
	};

	struct HairSimulationGpuData {
		glm::vec4 GravityDeltaTime = glm::vec4(0.0f);
		glm::vec4 WindDirectionTime = glm::vec4(0.0f);
		glm::vec4 Dynamics = glm::vec4(0.0f);
		glm::vec4 Limits = glm::vec4(0.0f);
	};

	struct HairBundleDesc {
		glm::vec3 BoundsMin = glm::vec3(0.0f);
		float _pad0 = 0.0f;
		glm::vec3 BoundsMax = glm::vec3(0.0f);
		float _pad1 = 0.0f;
		glm::vec4 AtlasRect = glm::vec4(0.0f);
		glm::ivec4 SliceInfo = glm::ivec4(0);
		glm::ivec4 RootInfo = glm::ivec4(0);
	};

	struct HairRootSample {
		glm::vec4 UVSeed = glm::vec4(0.0f);
	};

	struct HairSimulationPointGpuData {
		glm::vec4 Position = glm::vec4(0.0f);
		glm::vec4 Velocity = glm::vec4(0.0f);
	};

	struct HairSimulationRestPointGpuData {
		glm::vec4 Position = glm::vec4(0.0f);
		glm::vec4 UVW = glm::vec4(0.0f);
	};

	struct HairAssetDefinition {
		std::array<std::vector<glm::vec3>, HairStyleSliceCount> Layers;
		std::array<std::vector<glm::vec3>, HairStyleSliceCount> UDirections;
		std::array<std::vector<glm::vec3>, HairStyleSliceCount> VDirections;
		std::vector<glm::vec2> UVCoordinates;
		HairSimulationParams SimulationParams{};
	};

	class HairMeshAsset {
	public:
		HairMeshAsset();
		explicit HairMeshAsset(const std::string& assetPath);
		~HairMeshAsset();

		bool IsValid() const;
		void Bind() const;
		GLuint GetBundleCount() const;
		void SetStyleParameters(const HairStyleParameters& params);
		const HairStyleParameters& GetStyleParameters() const;
		void SetSimulationParams(const HairSimulationParams& params);
		const HairSimulationParams& GetSimulationParams() const;
		void UpdateSimulationUniforms(float deltaTime, float timeSeconds);
		void ResetSimulation();
		bool IsSimulationEnabled() const;
		bool HasSimulationState() const;
		void BindSimulation() const;
		void SwapSimulationState();
		GLuint GetSimulationReadBuffer() const;
		GLuint GetSimulationWriteBuffer() const;
		GLuint GetSimulationRestBuffer() const;
		GLuint GetSimulationParamsBuffer() const;
		GLuint GetHairMeshTexture() const;
		GLuint GetUTexture() const;
		GLuint GetVTexture() const;
		GLuint GetAtlasWidth() const;
		GLuint GetAtlasHeight() const;
		GLuint GetControlPointCount() const;
		const std::vector<glm::vec4>& GetDebugLineVertices() const;

		AABB bounds{};

		static std::vector<glm::vec4> BuildDebugLineVertices();
		static const HairStyleParameters& GetGlobalStyleParameters();
		static void SetGlobalStyleParameters(const HairStyleParameters& params);
		static const HairSimulationParams& GetGlobalSimulationParams();
		static void SetGlobalSimulationParams(const HairSimulationParams& params);

	private:
		void InitializeDefaultAsset();
		bool InitializeFromJson(const std::string& assetPath);
		void InitializeFromDefinition(const HairAssetDefinition& definition);
		void UpdateConservativeBounds();
		void UploadAssetData() const;
		void ApplyStyleParameters();
		void UploadSimulationParams() const;
		void ApplySimulationParams();
		void UploadBundleData() const;

		HairAssetGpuData m_gpuData{};
		HairStyleParameters m_styleParams{};
		HairSimulationParams m_simulationParams{};
		HairSimulationGpuData m_simulationGpuData{};
		GLuint m_assetUBO = 0;
		GLuint m_bundleSSBO = 0;
		GLuint m_rootSampleSSBO = 0;
		GLuint m_simulationParamsUBO = 0;
		GLuint m_simulationStateBuffers[2] = { 0, 0 };
		GLuint m_simulationRestSSBO = 0;

		GLuint m_hairMeshTexture = 0;
		GLuint m_uvTexture = 0;
		GLuint m_wTexture = 0;
		GLuint m_uTexture = 0;
		GLuint m_vTexture = 0;

		GLuint64 m_hairMeshHandle = 0;
		GLuint64 m_uvHandle = 0;
		GLuint64 m_wHandle = 0;
		GLuint64 m_uHandle = 0;
		GLuint64 m_vHandle = 0;

		GLuint m_bundleCount = 0;
		GLuint m_controlPointCount = 0;
		GLuint m_atlasWidth = 0;
		GLuint m_atlasHeight = 0;
		GLuint m_simulationReadIndex = 0;
		AABB m_baseBounds{};
		std::vector<HairBundleDesc> m_bundles;
		std::vector<AABB> m_baseBundleBounds;
		std::vector<glm::vec4> m_debugLines;
		std::vector<HairSimulationPointGpuData> m_initialSimulationPoints;
		std::vector<HairSimulationRestPointGpuData> m_restSimulationPoints;
	};
}
