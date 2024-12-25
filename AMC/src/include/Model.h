#pragma once

#include<common.h>
#include<ShaderProgram.h>
#include<assimp/scene.h>
#include<assimp/postprocess.h>
#include<VulkanHelperClasses.h>

#define MAX_BONE_COUNT 100
#define MAX_BONE_INFLUENCE 4

namespace AMC {

	enum TextureType
	{
		TextureTypeDiffuse = 0,
		TextureTypeNormalMap,
		TextureTypeMetallicRoughnessMap,
		TextureTypeSpecularMap,
		TextureTypeAmbient,
		TextureTypeEmissive,
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

	class Material {
		public:
			glm::vec3 albedo;
			float metallic;
			float roughness;
			float ao;
			glm::vec3 emission;
			float alpha;
			GLuint textureFlag;
			std::vector<ModelTexture> textures;

			Material();
			void Apply(ShaderProgram* program);
			void LoadMaterialTexturesFromFile(const std::string& path, TextureType type);
			void LoadMaterialTexturesFromMemory(const aiTexture* t, TextureType type);
			//void Bind(ShaderProgram* program);
	};

	__declspec(align(16)) struct Vertex {
		glm::vec4 position;
		glm::vec4 normal;
		glm::vec4 texCoords;
		glm::vec4 tangent;
		glm::vec4 bitangent;
		glm::ivec4 boneIDs;
		glm::vec4 weights;
	};

	static_assert(sizeof(Vertex) == 112, "Vertex Struct must be 112 bytes");
	static_assert(offsetof(Vertex, position) == 0, "Position offset incorrect");
	static_assert(offsetof(Vertex, normal) == 16, "Normal offset incorrect");
	static_assert(offsetof(Vertex, texCoords) == 32, "Tangent offset incorrect");
	static_assert(offsetof(Vertex, tangent) == 48, "Bitangent offset incorrect");
	static_assert(offsetof(Vertex, bitangent) == 64, "TexCoords offset incorrect");
	static_assert(offsetof(Vertex, boneIDs) == 80, "BoneIDs offset incorrect");
	static_assert(offsetof(Vertex, weights) == 96, "Weights offset incorrect");

	class Mesh {
	public:
		GLuint vao,vbo,ibo,outVbo;
		UINT mTriangleCount;
		UINT mVertexCount;
		UINT mMaterial;
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
		std::vector<UINT> meshIndices;
		std::vector<NodeData> children;
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

			VkAccelerationStructureKHR blas;
			AABB aabb;
			std::vector<Mesh*> meshes;
			std::vector<Material*> materials;
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
		private:

			void drawNodes(const NodeData& node, const glm::mat4& parentTransform, ShaderProgram* program, UINT iNumInstance = 1, bool iUseMaterial = true);
			void ComputeSkin();
	};
};
