#pragma once

#include<common.h>
#include<ShaderProgram.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
#include<VulkanHelperClasses.h>

#define MAX_BONE_COUNT 125
#define MAX_BONE_INFLUENCE 4

#define MESHLET_MAX_VERTEX_COUNT 128
#define MESHLET_MAX_TRIANGLE_COUNT 252

namespace AMC {

	enum TextureType
	{
		TextureTypeDiffuse = 0,
		TextureTypeNormalMap,
		TextureTypeMetallicRoughnessMap,
		TextureTypeEmissive,
		TextureTypeSpecularMap,
		TextureTypeAmbient,
		TextureTypeEnd
	};

	enum AnimationType {
		SKELETALANIM = 0,
		KEYFRAMEANIM,
		MORPHANIM
	};

	struct ModelTexture {
		GLuint texture;
		TextureType type;
	};

	struct AABB {
		glm::vec3 mMin;
		glm::vec3 mMax;
	};

	struct GpuMeshlet {
		uint32_t VertexOffset;
		uint32_t IndicesOffset;
		uint32_t VertexCount;
		uint32_t TriangleCount;
	};

	struct GpuMeshletInfo {
		glm::vec3 Min; float _pad0;
		glm::vec3 Max; float _pad1;
	};

	class Material {
		public:
			glm::vec3 albedo;
			float metallic;
			float roughness;
			float emissiveFactor;
			glm::vec3 emission;
			float alpha;
			GLuint textureFlag;
			std::vector<ModelTexture> textures;

			Material();
			void Apply(ShaderProgram* program);
			void ReleseTextures();
			void LoadMaterialTexturesFromFile(const std::string& path, TextureType type);
			void LoadMaterialTexturesFromMemory(const aiTexture* t, TextureType type);
			//void Bind(ShaderProgram* program);
	};

	__declspec(align(16)) struct Vertex {
		glm::vec4 position; // 16 bytes
		GLuint normal; // 4 bytes
		GLuint tangent; // 4 bytes
		glm::vec2 texCoords; // 8 bytes
		glm::ivec4 boneIDs; // 16 bytes
		glm::vec4 weights; // 16 bytes
	};

	static_assert(sizeof(Vertex) == 64, "Vertex Struct must be 112 bytes");
	static_assert(offsetof(Vertex, position) == 0, "Position offset incorrect");
	static_assert(offsetof(Vertex, normal) == 16, "Normal offset incorrect");
	static_assert(offsetof(Vertex, tangent) == 20, "Bitangent offset incorrect");
	static_assert(offsetof(Vertex, texCoords) == 24, "Tangent offset incorrect");
	static_assert(offsetof(Vertex, boneIDs) == 32, "BoneIDs offset incorrect");
	static_assert(offsetof(Vertex, weights) == 48, "Weights offset incorrect");

	class Mesh {
	public:
		GLuint vao,vbo,ibo,outVbo;
		UINT mTriangleCount;
		UINT mVertexCount;
		UINT mMaterial;
		glm::mat4 nodeTransform = glm::mat4(1.0f); // accumulated node-hierarchy transform for this mesh
		VkAccelerationStructureGeometryKHR geomConfig;
		VkAccelerationStructureKHR blas;
		uint32_t meshletCount = 0;
		uint32_t meshletOffset = 0;
		uint32_t baseVertex = 0;
	};

	struct BoneInfo {
		INT id;
		glm::mat4 offset;
	};

	struct AssimpNodeData {
		glm::mat4 transformation;
		std::string name;
		INT childCount;
		std::vector<AssimpNodeData> children;
	};

	struct KeyPosition {
		glm::vec3 position;
		FLOAT time;
	};

	struct KeyRotation {
		glm::quat orientation;
		FLOAT time;
	};

	struct KeyScale {
		glm::vec3 scale;
		FLOAT time;
	};

	struct Bone {
		INT id;
		std::string name;
		std::vector<KeyPosition> positions;
		std::vector<KeyRotation> rotations;
		std::vector<KeyScale> scales;
		glm::mat4 localTransform;
	};

	struct SkeletonAnimator {
		GLuint boneSSBO;
		std::vector<glm::mat4> finalBoneMatrices;
		FLOAT duration;
		INT ticksPerSecond;
		std::vector<Bone> bones;
		AssimpNodeData rootNode;
		FLOAT currentTime;
	};

	//
	struct NodeAnimation {
		std::vector<KeyPosition> positions;
		std::vector<KeyRotation> rotations;
		std::vector<KeyScale> scales;
	};

	// seems redundant but don't have time to optimize for now
	struct NodeData {
		std::string name;
		glm::mat4 transformation;
		glm::mat4 globalTransform;
		glm::mat4 prevGlobalTransform = glm::mat4(1.0f);
		std::vector<UINT> meshIndices;
		std::vector<NodeData> children;
	};

	// Flattened node representation for the new pooled-buffer architecture.
	// Produced at load time from the recursive NodeData tree.
	// Stored in pre-order (parent index < child index) so that transform
	// propagation can be done in a single forward pass.
	struct FlatNode {
		int32_t     parentIndex;       // -1 for root, local to this asset
		glm::mat4   localTransform;    // rest-pose local transform
		std::string name;              // for animation channel lookup
		std::vector<uint32_t> meshIndices; // indices into Model::meshes[]
	};

	struct NodeAnimator {
		FLOAT duration;
		INT ticksPerSecond;
		NodeData rootNode;
		FLOAT currentTime;
		std::string name; // Optional: Name of the animation
		std::unordered_map<std::string, NodeAnimation> nodeAnimations;
	};

	struct MorphTargetAnimator
	{
		std::string meshName;
		std::vector<float> times;                   // Keyframe times
		std::vector<std::vector<float>> weights;    // Morph target weights at each keyframe
		std::vector<std::vector<unsigned int>> indices; // Morph target indices at each keyframe
		FLOAT duration;
		INT ticksPerSecond;
		FLOAT currentTime;
	};

	class Model {

		public:
			
			Model(std::string path, int iAssimpFlags, const AMC::VkContext* ctx = nullptr);
			~Model();

			void draw(ShaderProgram* program,UINT iNumInstance = 1, bool iUseMaterial = true);
			void update(float dt);
			void lerpAnimation(float t);
			void setActiveAnimation(int indel = 0);

			AABB aabb;
			std::vector<Mesh*> meshes;
			GLuint materialSSBO;
			//std::vector<Material*> materials;
			NodeData rootNode;

			BOOL haveAnimation = FALSE;
			INT CurrentAnimation = 0;
			AnimationType animType;
			INT BoneCounter = 0;

			static ShaderProgram* programGPUSkin;

			// Animation Data
			std::vector<SkeletonAnimator> skeletonAnimator;
			std::vector<NodeAnimator> nodeAnimator;
			std::vector<MorphTargetAnimator> morphAnimator;
			std::unordered_map<std::string, std::vector<float>> currentMorphWeights;
			std::unordered_map<std::string, BoneInfo> BoneInfoMap;

			// DEPRECATED: Per-model meshlet SSBOs replaced by global pools in ModelAssetManager.
			// Kept for backward compatibility with legacy drawMeshShader path.
			GLuint meshletSSBO = 0;
			GLuint meshletInfoSSBO = 0;
			GLuint meshletVertexSSBO = 0;
			GLuint meshletLocalSSBO = 0;
			GLuint vertexDataSSBO = 0;
			bool hasMeshletData = false;

			// CPU-side data retained from loading for meshlet generation
			std::vector<Vertex> cpuVertices;
			std::vector<uint32_t> cpuIndices;
			std::vector<uint32_t> cpuMeshVertexOffsets;
			std::vector<uint32_t> cpuMeshIndexOffsets;

			void generateMeshlets();
			void drawMeshShader(ShaderProgram* program);
			void drawMeshShaderFlat(ShaderProgram* program);

			// === New pooled-buffer architecture fields (Milestone 1) ===

			// Flattened node hierarchy (set by ModelAssetManager::RegisterModel)
			std::vector<FlatNode> flatNodes;

			// Asset ID assigned by ModelAssetManager (-1 = not registered)
			int32_t assetId = -1;

			// Instance ID assigned by SceneInstanceManager (-1 = not instanced)
			int32_t instanceId = -1;

			// Global pool offsets (set by ModelAssetManager::RegisterModel)
			uint32_t globalVertexBase = 0;
			uint32_t globalMeshletBase = 0;
			uint32_t globalNodeBase = 0;
			uint32_t globalMeshBase = 0;
			uint32_t globalMaterialBase = 0;

			// Number of materials pushed to the global pool
			uint32_t globalMaterialCount = 0;

			// Whether this model uses global pools (set after UploadToGPU)
			bool useGlobalPools = false;

		private:

			void drawNodes(const NodeData& node, const glm::mat4& parentTransform, const glm::mat4& prevParentTransform, ShaderProgram* program, UINT iNumInstance = 1, bool iUseMaterial = true);
			void drawNodesMeshShader(const NodeData& node, const glm::mat4& parentTransform, const glm::mat4& prevParentTransform, ShaderProgram* program);
			void ComputeSkin();
	};
};
