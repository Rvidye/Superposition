#include<common.h>
#include<HairMeshAsset.h>

#include<algorithm>
#include<array>
#include<cctype>
#include<cmath>
#include<cstdlib>
#include<fstream>
#include<numeric>
#include<sstream>
#include<vector>

namespace AMC {
	namespace {
		constexpr int kBundleTexelSize = 2;
		constexpr int kPatchVertexCount = HairPatchQuadCount + 1;
		constexpr int kAtlasWidth = HairPatchQuadCount * kBundleTexelSize;
		constexpr int kAtlasHeight = HairPatchQuadCount * kBundleTexelSize;
		constexpr int kBlueNoiseCandidateCount = 256;

		using LayerGrid = std::vector<glm::vec3>;
		using UVGrid = std::vector<glm::vec2>;

		struct HairCageData {
			std::array<LayerGrid, HairStyleSliceCount> layers;
			std::array<LayerGrid, HairStyleSliceCount> uDirections;
			std::array<LayerGrid, HairStyleSliceCount> vDirections;
			UVGrid uvCoordinates;
		};

		struct HairAuthoringParameters {
			float RootHalfExtent = HairRootHalfExtent;
			float TipHalfExtent = HairLayerHalfExtent;
			float LayerHeight = HairLayerHeight;
			float CrownHeight = 0.035f;
			glm::vec2 SideSweep = glm::vec2(0.03f, 0.02f);
			glm::vec2 ForwardSweep = glm::vec2(0.01f, -0.065f);
			glm::vec3 Lean = glm::vec3(0.0f);
			float Twist = 0.0f;
			float Bulge = 0.015f;
			// Quadratic-in-layerT bend: shifts the tip away from straight up so the
			// rest pose has non-vertical tangents. Without this the simulation has
			// nothing to rotate against and gravity gets normalized away by the
			// length constraint.
			glm::vec3 Bend = glm::vec3(0.0f);
			float RadialBend = 0.0f;
			HairSimulationParams Simulation{};
			HairStyleParameters Style{};
			bool HasStyleOverride = false;
		};

		HairStyleParameters g_globalStyleParameters{};
		HairSimulationParams g_globalSimulationParameters{};
		std::vector<HairMeshAsset*> g_registeredAssets;

		void DeleteBuffer(GLuint& buffer) {
			if (buffer != 0) {
				glDeleteBuffers(1, &buffer);
				buffer = 0;
			}
		}

		void DeleteTexture(GLuint& texture) {
			if (texture != 0) {
				glDeleteTextures(1, &texture);
				texture = 0;
			}
		}

		void SkipJsonWhitespace(const std::string& src, size_t& pos) {
			while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])) != 0) {
				++pos;
			}
		}

		bool FindJsonValueStart(const std::string& src, const std::string& key, size_t& pos) {
			const std::string needle = "\"" + key + "\"";
			pos = src.find(needle);
			if (pos == std::string::npos) {
				return false;
			}
			pos = src.find(':', pos + needle.size());
			if (pos == std::string::npos) {
				return false;
			}
			++pos;
			SkipJsonWhitespace(src, pos);
			return pos < src.size();
		}

		bool TryParseJsonNumber(const std::string& src, const std::string& key, float& value) {
			size_t pos = 0;
			if (!FindJsonValueStart(src, key, pos)) {
				return false;
			}

			char* endPtr = nullptr;
			value = std::strtof(src.c_str() + pos, &endPtr);
			return endPtr != src.c_str() + pos;
		}

		bool TryParseJsonBool(const std::string& src, const std::string& key, bool& value) {
			size_t pos = 0;
			if (!FindJsonValueStart(src, key, pos)) {
				return false;
			}

			if (src.compare(pos, 4, "true") == 0) {
				value = true;
				return true;
			}
			if (src.compare(pos, 5, "false") == 0) {
				value = false;
				return true;
			}
			return false;
		}

		bool TryParseJsonVec2(const std::string& src, const std::string& key, glm::vec2& value) {
			size_t pos = 0;
			if (!FindJsonValueStart(src, key, pos) || src[pos] != '[') {
				return false;
			}

			++pos;
			SkipJsonWhitespace(src, pos);
			char* endPtr = nullptr;
			value.x = std::strtof(src.c_str() + pos, &endPtr);
			if (endPtr == src.c_str() + pos) {
				return false;
			}
			pos = static_cast<size_t>(endPtr - src.c_str());
			pos = src.find(',', pos);
			if (pos == std::string::npos) {
				return false;
			}
			++pos;
			SkipJsonWhitespace(src, pos);
			value.y = std::strtof(src.c_str() + pos, &endPtr);
			return endPtr != src.c_str() + pos;
		}

		bool TryParseJsonVec3(const std::string& src, const std::string& key, glm::vec3& value) {
			size_t pos = 0;
			if (!FindJsonValueStart(src, key, pos) || src[pos] != '[') {
				return false;
			}

			++pos;
			SkipJsonWhitespace(src, pos);
			char* endPtr = nullptr;
			value.x = std::strtof(src.c_str() + pos, &endPtr);
			if (endPtr == src.c_str() + pos) {
				return false;
			}
			pos = static_cast<size_t>(endPtr - src.c_str());
			pos = src.find(',', pos);
			if (pos == std::string::npos) {
				return false;
			}
			++pos;
			SkipJsonWhitespace(src, pos);
			value.y = std::strtof(src.c_str() + pos, &endPtr);
			if (endPtr == src.c_str() + pos) {
				return false;
			}
			pos = static_cast<size_t>(endPtr - src.c_str());
			pos = src.find(',', pos);
			if (pos == std::string::npos) {
				return false;
			}
			++pos;
			SkipJsonWhitespace(src, pos);
			value.z = std::strtof(src.c_str() + pos, &endPtr);
			return endPtr != src.c_str() + pos;
		}

		int VertexIndex(int x, int z) {
			return z * kPatchVertexCount + x;
		}

		float Hash01(uint32_t value) {
			value ^= value >> 17u;
			value *= 0xed5ad4bbu;
			value ^= value >> 11u;
			value *= 0xac4c1b51u;
			value ^= value >> 15u;
			value *= 0x31848babu;
			value ^= value >> 14u;
			return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x01000000u);
		}

		glm::vec2 R2SequencePoint(uint32_t index) {
			const float x = std::fmod(0.5f + (static_cast<float>(index) + 1.0f) * 0.7548776662f, 1.0f);
			const float y = std::fmod(0.5f + (static_cast<float>(index) + 1.0f) * 0.5698402910f, 1.0f);
			return glm::vec2(x, y);
		}

		float ToroidalDistanceSquared(const glm::vec2& a, const glm::vec2& b) {
			glm::vec2 delta = glm::abs(a - b);
			delta = glm::min(delta, glm::vec2(1.0f) - delta);
			return glm::dot(delta, delta);
		}

		std::vector<glm::vec2> BuildProgressiveBlueNoisePoints(uint32_t sampleCount) {
			std::vector<glm::vec2> candidates;
			candidates.reserve(kBlueNoiseCandidateCount);
			for (uint32_t i = 0; i < kBlueNoiseCandidateCount; ++i) {
				candidates.push_back(R2SequencePoint(i));
			}

			std::vector<int> active(candidates.size());
			std::iota(active.begin(), active.end(), 0);
			std::vector<int> eliminated;
			eliminated.reserve(candidates.size());

			while (active.size() > 1) {
				float worstDistance = FLT_MAX;
				size_t worstIndex = 0;

				for (size_t activeIndex = 0; activeIndex < active.size(); ++activeIndex) {
					float nearestDistance = FLT_MAX;
					for (size_t otherIndex = 0; otherIndex < active.size(); ++otherIndex) {
						if (activeIndex == otherIndex) {
							continue;
						}

						nearestDistance = std::min(
							nearestDistance,
							ToroidalDistanceSquared(candidates[active[activeIndex]], candidates[active[otherIndex]])
						);
					}

					if (nearestDistance < worstDistance) {
						worstDistance = nearestDistance;
						worstIndex = activeIndex;
					}
				}

				eliminated.push_back(active[worstIndex]);
				active.erase(active.begin() + static_cast<std::ptrdiff_t>(worstIndex));
			}

			std::vector<glm::vec2> progressive;
			progressive.reserve(candidates.size());
			progressive.push_back(candidates[active.front()]);
			for (auto it = eliminated.rbegin(); it != eliminated.rend(); ++it) {
				progressive.push_back(candidates[*it]);
			}
			progressive.resize(sampleCount);
			return progressive;
		}

		const std::vector<HairRootSample>& GetSharedBlueNoiseSamples() {
			static const std::vector<HairRootSample> samples = []() {
				std::vector<HairRootSample> out(HairRootSampleCount);
				const std::vector<glm::vec2> points = BuildProgressiveBlueNoisePoints(HairRootSampleCount);
				for (uint32_t i = 0; i < HairRootSampleCount; ++i) {
					out[i].UVSeed = glm::vec4(
						points[i],
						Hash01(i * 9781u + 17u),
						Hash01(i * 6271u + 101u)
					);
				}
				return out;
			}();
			return samples;
		}

		glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback) {
			const float len2 = glm::dot(value, value);
			if (len2 <= 1e-8f) {
				return fallback;
			}
			return value * glm::inversesqrt(len2);
		}

		glm::vec2 EvaluatePatchUV(int x, int z) {
			return glm::vec2(
				static_cast<float>(x) / static_cast<float>(HairPatchQuadCount),
				static_cast<float>(z) / static_cast<float>(HairPatchQuadCount)
			);
		}

		glm::vec3 EvaluatePatchPosition(int x, int z, int layerIndex) {
			const float layerT = static_cast<float>(layerIndex) / static_cast<float>(HairLayerCount);
			const glm::vec2 uv = EvaluatePatchUV(x, z);
			const glm::vec2 signedUV = uv * 2.0f - 1.0f;

			const float halfExtent = glm::mix(HairRootHalfExtent, HairLayerHalfExtent, layerT);
			glm::vec2 xz = glm::mix(glm::vec2(-halfExtent), glm::vec2(halfExtent), uv);

			const float crown = (1.0f - glm::clamp(glm::dot(signedUV, signedUV), 0.0f, 1.0f)) * 0.035f;
			const glm::vec2 sweep = glm::vec2(
				0.030f * layerT * signedUV.x,
				-0.065f * layerT * layerT + 0.020f * layerT * signedUV.y
			);

			return glm::vec3(
				xz.x + sweep.x,
				HairLayerHeight * layerT + crown,
				xz.y + sweep.y
			);
		}

		float CornerAngle(const glm::vec3& center, const glm::vec3& edge0, const glm::vec3& edge1) {
			const glm::vec3 v0 = SafeNormalize(edge0 - center, glm::vec3(1.0f, 0.0f, 0.0f));
			const glm::vec3 v1 = SafeNormalize(edge1 - center, glm::vec3(0.0f, 0.0f, 1.0f));
			return std::acos(glm::clamp(glm::dot(v0, v1), -1.0f, 1.0f));
		}

		void AccumulateTriangleFrame(
			const LayerGrid& positions,
			const UVGrid& uvCoordinates,
			int i0,
			int i1,
			int i2,
			LayerGrid& accumulatedU,
			LayerGrid& accumulatedNormal
		) {
			const glm::vec3& p0 = positions[i0];
			const glm::vec3& p1 = positions[i1];
			const glm::vec3& p2 = positions[i2];
			const glm::vec2& uv0 = uvCoordinates[i0];
			const glm::vec2& uv1 = uvCoordinates[i1];
			const glm::vec2& uv2 = uvCoordinates[i2];

			const glm::vec3 edge01 = p1 - p0;
			const glm::vec3 edge02 = p2 - p0;
			const glm::vec2 duv01 = uv1 - uv0;
			const glm::vec2 duv02 = uv2 - uv0;

			const float determinant = duv01.x * duv02.y - duv01.y * duv02.x;
			if (std::abs(determinant) <= 1e-6f) {
				return;
			}

			const float inverseDeterminant = 1.0f / determinant;
			const glm::vec3 dPdu = (edge01 * duv02.y - edge02 * duv01.y) * inverseDeterminant;
			const glm::vec3 faceNormal = SafeNormalize(glm::cross(edge02, edge01), glm::vec3(0.0f, 1.0f, 0.0f));
			const float area = glm::length(glm::cross(edge01, edge02)) * 0.5f;

			const std::array<int, 3> indices = { i0, i1, i2 };
			const std::array<float, 3> weights = {
				CornerAngle(p0, p1, p2) * area,
				CornerAngle(p1, p2, p0) * area,
				CornerAngle(p2, p0, p1) * area
			};

			for (size_t corner = 0; corner < indices.size(); ++corner) {
				accumulatedU[indices[corner]] += dPdu * weights[corner];
				accumulatedNormal[indices[corner]] += faceNormal * weights[corner];
			}
		}

		HairCageData BuildProceduralHairCage() {
			HairCageData cage{};
			cage.uvCoordinates.resize(kPatchVertexCount * kPatchVertexCount);
			for (auto& layer : cage.layers) {
				layer.resize(kPatchVertexCount * kPatchVertexCount);
			}
			for (auto& layer : cage.uDirections) {
				layer.resize(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
			}
			for (auto& layer : cage.vDirections) {
				layer.resize(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
			}

			for (int z = 0; z < kPatchVertexCount; ++z) {
				for (int x = 0; x < kPatchVertexCount; ++x) {
					const int vertexIndex = VertexIndex(x, z);
					cage.uvCoordinates[vertexIndex] = EvaluatePatchUV(x, z);
					for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
						cage.layers[layerIndex][vertexIndex] = EvaluatePatchPosition(x, z, layerIndex);
					}
				}
			}

			for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
				LayerGrid accumulatedU(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
				LayerGrid accumulatedNormal(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));

				for (int z = 0; z < HairPatchQuadCount; ++z) {
					for (int x = 0; x < HairPatchQuadCount; ++x) {
						const int v00 = VertexIndex(x, z);
						const int v10 = VertexIndex(x + 1, z);
						const int v01 = VertexIndex(x, z + 1);
						const int v11 = VertexIndex(x + 1, z + 1);

						AccumulateTriangleFrame(cage.layers[layerIndex], cage.uvCoordinates, v00, v01, v10, accumulatedU, accumulatedNormal);
						AccumulateTriangleFrame(cage.layers[layerIndex], cage.uvCoordinates, v01, v11, v10, accumulatedU, accumulatedNormal);
					}
				}

				for (int vertexIndex = 0; vertexIndex < kPatchVertexCount * kPatchVertexCount; ++vertexIndex) {
					const glm::vec3 wAxis = SafeNormalize(accumulatedNormal[vertexIndex], glm::vec3(0.0f, 1.0f, 0.0f));
					glm::vec3 uAxis = accumulatedU[vertexIndex] - wAxis * glm::dot(accumulatedU[vertexIndex], wAxis);
					uAxis = SafeNormalize(
						uAxis,
						(std::abs(wAxis.y) < 0.99f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f)
					);
					glm::vec3 vAxis = SafeNormalize(glm::cross(wAxis, uAxis), glm::vec3(0.0f, 0.0f, 1.0f));
					uAxis = SafeNormalize(glm::cross(vAxis, wAxis), uAxis);

					cage.uDirections[layerIndex][vertexIndex] = uAxis;
					cage.vDirections[layerIndex][vertexIndex] = vAxis;
				}
			}

			return cage;
		}

		glm::vec3 EvaluatePatchPosition(const HairAuthoringParameters& params, int x, int z, int layerIndex) {
			const float layerT = static_cast<float>(layerIndex) / static_cast<float>(HairLayerCount);
			const glm::vec2 uv = EvaluatePatchUV(x, z);
			const glm::vec2 signedUV = uv * 2.0f - 1.0f;
			const float halfExtent = std::lerp(params.RootHalfExtent, params.TipHalfExtent, layerT);
			glm::vec2 xz = glm::mix(glm::vec2(-halfExtent), glm::vec2(halfExtent), uv);

			const float twistAngle = params.Twist * layerT;
			const float c = std::cos(twistAngle);
			const float s = std::sin(twistAngle);
			xz = glm::vec2(c * xz.x - s * xz.y, s * xz.x + c * xz.y);

			const float radialFalloff = 1.0f - glm::clamp(glm::dot(signedUV, signedUV), 0.0f, 1.0f);
			const float crown = radialFalloff * params.CrownHeight * (1.0f - 0.35f * layerT);
			const glm::vec2 sweep = glm::vec2(
				params.SideSweep.x * layerT * signedUV.x + params.SideSweep.y * layerT * layerT,
				params.ForwardSweep.x * layerT * signedUV.y + params.ForwardSweep.y * layerT * layerT
			);
			const glm::vec3 lean = params.Lean * layerT;

			// Quadratic bend: the tangent at the root stays +Y (so the strand emerges
			// vertically from the scalp), then progressively curls toward params.Bend
			// plus a radial outward component as layerT grows. The vertical growth is
			// reduced by the magnitude of the horizontal bend to keep approximate arc
			// length conservation, otherwise heavily-bent presets would feel stretched.
			const float bendT = layerT * layerT;
			const glm::vec3 bend = params.Bend * bendT
				+ glm::vec3(signedUV.x, 0.0f, signedUV.y) * (params.RadialBend * bendT);
			const float bendHorizontalLen = std::sqrt(bend.x * bend.x + bend.z * bend.z);
			const float verticalShortening = bendHorizontalLen * 0.35f;

			return glm::vec3(
				xz.x + sweep.x + params.Bulge * layerT * signedUV.x + lean.x + bend.x,
				params.LayerHeight * layerT + crown + lean.y + bend.y - verticalShortening,
				xz.y + sweep.y + params.Bulge * layerT * signedUV.y + lean.z + bend.z
			);
		}

		HairCageData BuildAuthoredHairCage(const HairAuthoringParameters& params) {
			HairCageData cage{};
			cage.uvCoordinates.resize(kPatchVertexCount * kPatchVertexCount);
			for (auto& layer : cage.layers) {
				layer.resize(kPatchVertexCount * kPatchVertexCount);
			}
			for (auto& layer : cage.uDirections) {
				layer.resize(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
			}
			for (auto& layer : cage.vDirections) {
				layer.resize(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
			}

			for (int z = 0; z < kPatchVertexCount; ++z) {
				for (int x = 0; x < kPatchVertexCount; ++x) {
					const int vertexIndex = VertexIndex(x, z);
					cage.uvCoordinates[vertexIndex] = EvaluatePatchUV(x, z);
					for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
						cage.layers[layerIndex][vertexIndex] = EvaluatePatchPosition(params, x, z, layerIndex);
					}
				}
			}

			for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
				LayerGrid accumulatedU(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));
				LayerGrid accumulatedNormal(kPatchVertexCount * kPatchVertexCount, glm::vec3(0.0f));

				for (int z = 0; z < HairPatchQuadCount; ++z) {
					for (int x = 0; x < HairPatchQuadCount; ++x) {
						const int v00 = VertexIndex(x, z);
						const int v10 = VertexIndex(x + 1, z);
						const int v01 = VertexIndex(x, z + 1);
						const int v11 = VertexIndex(x + 1, z + 1);

						AccumulateTriangleFrame(cage.layers[layerIndex], cage.uvCoordinates, v00, v01, v10, accumulatedU, accumulatedNormal);
						AccumulateTriangleFrame(cage.layers[layerIndex], cage.uvCoordinates, v01, v11, v10, accumulatedU, accumulatedNormal);
					}
				}

				for (int vertexIndex = 0; vertexIndex < kPatchVertexCount * kPatchVertexCount; ++vertexIndex) {
					const glm::vec3 wAxis = SafeNormalize(accumulatedNormal[vertexIndex], glm::vec3(0.0f, 1.0f, 0.0f));
					glm::vec3 uAxis = accumulatedU[vertexIndex] - wAxis * glm::dot(accumulatedU[vertexIndex], wAxis);
					uAxis = SafeNormalize(
						uAxis,
						(std::abs(wAxis.y) < 0.99f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f)
					);
					glm::vec3 vAxis = SafeNormalize(glm::cross(wAxis, uAxis), glm::vec3(0.0f, 0.0f, 1.0f));
					uAxis = SafeNormalize(glm::cross(vAxis, wAxis), uAxis);

					cage.uDirections[layerIndex][vertexIndex] = uAxis;
					cage.vDirections[layerIndex][vertexIndex] = vAxis;
				}
			}

			return cage;
		}

		bool LoadAuthoringParametersFromJson(const std::string& assetPath, HairAuthoringParameters& params) {
			std::ifstream input(assetPath, std::ios::binary);
			if (!input.is_open()) {
				return false;
			}

			std::stringstream stream;
			stream << input.rdbuf();
			const std::string json = stream.str();

			params.Simulation = g_globalSimulationParameters;
			params.Style = g_globalStyleParameters;
			TryParseJsonNumber(json, "root_half_extent", params.RootHalfExtent);
			TryParseJsonNumber(json, "tip_half_extent", params.TipHalfExtent);
			TryParseJsonNumber(json, "layer_height", params.LayerHeight);
			TryParseJsonNumber(json, "crown_height", params.CrownHeight);
			TryParseJsonVec2(json, "side_sweep", params.SideSweep);
			TryParseJsonVec2(json, "forward_sweep", params.ForwardSweep);
			TryParseJsonVec3(json, "lean", params.Lean);
			TryParseJsonNumber(json, "twist", params.Twist);
			TryParseJsonNumber(json, "bulge", params.Bulge);
			TryParseJsonVec3(json, "bend", params.Bend);
			TryParseJsonNumber(json, "radial_bend", params.RadialBend);
			TryParseJsonBool(json, "simulation_enabled", params.Simulation.Enabled);
			TryParseJsonVec3(json, "gravity", params.Simulation.Gravity);
			TryParseJsonVec3(json, "wind_direction", params.Simulation.WindDirection);
			TryParseJsonNumber(json, "wind_strength", params.Simulation.WindStrength);
			TryParseJsonNumber(json, "wind_pulse_frequency", params.Simulation.WindPulseFrequency);
			TryParseJsonNumber(json, "stiffness", params.Simulation.Stiffness);
			TryParseJsonNumber(json, "damping", params.Simulation.Damping);
			TryParseJsonNumber(json, "tip_influence", params.Simulation.TipInfluence);
			TryParseJsonNumber(json, "max_displacement", params.Simulation.MaxDisplacement);
			TryParseJsonNumber(json, "time_scale", params.Simulation.TimeScale);

			// Procedural styling parameters from Bhokare et al. 2024 supplemental
			// (fuzz, frizz, kink, curl, clumping). Any field present in the JSON
			// flips HasStyleOverride and is propagated to the asset's style state
			// after the cage is built.
			auto tryStyleField = [&](const char* key, float& target) {
				if (TryParseJsonNumber(json, key, target)) {
					params.HasStyleOverride = true;
				}
			};
			tryStyleField("fuzz_amplitude", params.Style.FuzzAmplitude);
			tryStyleField("fuzz_amplitude_exponent", params.Style.FuzzAmplitudeExponent);
			tryStyleField("fuzz_distribution_exponent", params.Style.FuzzDistributionExponent);
			tryStyleField("frizz_amplitude", params.Style.FrizzAmplitude);
			tryStyleField("frizz_amplitude_exponent", params.Style.FrizzAmplitudeExponent);
			tryStyleField("frizz_frequency", params.Style.FrizzFrequency);
			tryStyleField("kink_amplitude", params.Style.KinkAmplitude);
			tryStyleField("kink_amplitude_exponent", params.Style.KinkAmplitudeExponent);
			tryStyleField("kink_uv_frequency", params.Style.KinkUVFrequency);
			tryStyleField("kink_w_frequency", params.Style.KinkWFrequency);
			tryStyleField("curl_amplitude", params.Style.CurlAmplitude);
			tryStyleField("curl_amplitude_exponent", params.Style.CurlAmplitudeExponent);
			tryStyleField("curl_distribution_exponent", params.Style.CurlDistributionExponent);
			tryStyleField("curl_frequency", params.Style.CurlFrequency);
			tryStyleField("curl_rotations", params.Style.CurlRotations);
			tryStyleField("clump_thickness", params.Style.ClumpThickness);
			tryStyleField("clump_exponent", params.Style.ClumpExponent);
			tryStyleField("clump_noise_amount", params.Style.ClumpNoiseAmount);
			tryStyleField("clump_noise_frequency", params.Style.ClumpNoiseFrequency);
			tryStyleField("clump_grid_resolution", params.Style.ClumpGridResolution);
			tryStyleField("clump_percentage", params.Style.ClumpPercentage);
			tryStyleField("clump_variation", params.Style.ClumpVariation);
			return true;
		}

		std::vector<glm::vec4> BuildDebugLines(const HairCageData& cage) {
			std::vector<glm::vec4> lines;
			lines.reserve(512);

			auto appendSegment = [&lines](const glm::vec3& a, const glm::vec3& b) {
				lines.emplace_back(a, 1.0f);
				lines.emplace_back(b, 1.0f);
			};

			for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
				const LayerGrid& layer = cage.layers[layerIndex];

				for (int z = 0; z < kPatchVertexCount; ++z) {
					for (int x = 0; x < HairPatchQuadCount; ++x) {
						appendSegment(layer[VertexIndex(x, z)], layer[VertexIndex(x + 1, z)]);
					}
				}

				for (int x = 0; x < kPatchVertexCount; ++x) {
					for (int z = 0; z < HairPatchQuadCount; ++z) {
						appendSegment(layer[VertexIndex(x, z)], layer[VertexIndex(x, z + 1)]);
					}
				}
			}

			for (int layerIndex = 0; layerIndex < HairLayerCount; ++layerIndex) {
				const LayerGrid& lower = cage.layers[layerIndex];
				const LayerGrid& upper = cage.layers[layerIndex + 1];
				for (int z = 0; z < kPatchVertexCount; ++z) {
					for (int x = 0; x < kPatchVertexCount; ++x) {
						appendSegment(lower[VertexIndex(x, z)], upper[VertexIndex(x, z)]);
					}
				}
			}

			return lines;
		}

		HairAssetDefinition BuildDefinitionFromCage(const HairCageData& cage, const HairSimulationParams& simulationParams) {
			HairAssetDefinition definition{};
			for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
				definition.Layers[layerIndex] = cage.layers[layerIndex];
				definition.UDirections[layerIndex] = cage.uDirections[layerIndex];
				definition.VDirections[layerIndex] = cage.vDirections[layerIndex];
			}
			definition.UVCoordinates = cage.uvCoordinates;
			definition.SimulationParams = simulationParams;
			return definition;
		}

		AABB ComputeBounds(const HairCageData& cage) {
			AABB out{};
			out.mMin = glm::vec3(FLT_MAX);
			out.mMax = glm::vec3(-FLT_MAX);

			for (const LayerGrid& layer : cage.layers) {
				for (const glm::vec3& point : layer) {
					out.mMin = glm::min(out.mMin, point);
					out.mMax = glm::max(out.mMax, point);
				}
			}

			return out;
		}

		std::array<glm::vec3, 5> BuildSegmentSlices(
			const glm::vec3& previous,
			const glm::vec3& current,
			const glm::vec3& next,
			const glm::vec3& after
		) {
			const glm::vec3 cubic0 = current;
			const glm::vec3 cubic1 = current + (next - previous) / 6.0f;
			const glm::vec3 cubic2 = next - (after - current) / 6.0f;
			const glm::vec3 cubic3 = next;

			const auto evaluateBezier = [](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
				const float u = 1.0f - t;
				return (u * u * u) * p0
					+ (3.0f * u * u * t) * p1
					+ (3.0f * u * t * t) * p2
					+ (t * t * t) * p3;
			};

			const glm::vec3 mid = evaluateBezier(cubic0, cubic1, cubic2, cubic3, 0.5f);
			const glm::vec3 quarter = evaluateBezier(cubic0, cubic1, cubic2, cubic3, 0.25f);
			const glm::vec3 threeQuarter = evaluateBezier(cubic0, cubic1, cubic2, cubic3, 0.75f);

			const glm::vec3 leftControl = 2.0f * quarter - 0.5f * (cubic0 + mid);
			const glm::vec3 rightControl = 2.0f * threeQuarter - 0.5f * (mid + cubic3);

			return {
				cubic0,
				leftControl,
				mid,
				rightControl,
				cubic3
			};
		}

		std::array<glm::vec3, HairMeshSliceCount> BuildSliceCurve(const std::array<glm::vec3, HairStyleSliceCount>& controlPoints) {
			std::array<glm::vec3, HairMeshSliceCount> slices{};
			int writeIndex = 0;

			for (int segmentIndex = 0; segmentIndex < HairLayerCount; ++segmentIndex) {
				const glm::vec3 previous = (segmentIndex > 0)
					? controlPoints[segmentIndex - 1]
					: controlPoints[segmentIndex] - (controlPoints[segmentIndex + 1] - controlPoints[segmentIndex]);
				const glm::vec3 current = controlPoints[segmentIndex];
				const glm::vec3 next = controlPoints[segmentIndex + 1];
				const glm::vec3 after = (segmentIndex + 2 < HairStyleSliceCount)
					? controlPoints[segmentIndex + 2]
					: controlPoints[segmentIndex + 1] + (controlPoints[segmentIndex + 1] - controlPoints[segmentIndex]);

				const auto segmentSlices = BuildSegmentSlices(previous, current, next, after);
				for (size_t sliceIndex = 0; sliceIndex < segmentSlices.size(); ++sliceIndex) {
					if (segmentIndex > 0 && sliceIndex == 0) {
						continue;
					}
					slices[writeIndex++] = segmentSlices[sliceIndex];
				}
			}

			return slices;
		}

		void MakeTextureResident(GLuint texture, GLuint64& handle) {
			handle = glGetTextureHandleARB(texture);
			if (handle != 0) {
				glMakeTextureHandleResidentARB(handle);
			}
		}

		void MakeTextureNonResident(GLuint64 handle) {
			if (handle != 0) {
				glMakeTextureHandleNonResidentARB(handle);
			}
		}

		void UnregisterHairAsset(HairMeshAsset* asset) {
			g_registeredAssets.erase(
				std::remove(g_registeredAssets.begin(), g_registeredAssets.end(), asset),
				g_registeredAssets.end()
			);
		}
	}

	HairMeshAsset::HairMeshAsset() {
		m_styleParams = g_globalStyleParameters;
		m_simulationParams = g_globalSimulationParameters;
		InitializeDefaultAsset();
		g_registeredAssets.push_back(this);
	}

	HairMeshAsset::HairMeshAsset(const std::string& assetPath)
		: HairMeshAsset() {
		if (!assetPath.empty()) {
			InitializeFromJson(assetPath);
		}
	}

	void HairMeshAsset::InitializeDefaultAsset() {
		InitializeFromDefinition(BuildDefinitionFromCage(BuildProceduralHairCage(), m_simulationParams));
	}

	bool HairMeshAsset::InitializeFromJson(const std::string& assetPath) {
		HairAuthoringParameters params;
		if (!LoadAuthoringParametersFromJson(assetPath, params)) {
			return false;
		}

		InitializeFromDefinition(BuildDefinitionFromCage(BuildAuthoredHairCage(params), params.Simulation));
		if (params.HasStyleOverride) {
			SetStyleParameters(params.Style);
		}
		return true;
	}

	void HairMeshAsset::InitializeFromDefinition(const HairAssetDefinition& definition) {
		HairCageData cage{};
		cage.uvCoordinates = definition.UVCoordinates;
		for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
			cage.layers[layerIndex] = definition.Layers[layerIndex];
			cage.uDirections[layerIndex] = definition.UDirections[layerIndex];
			cage.vDirections[layerIndex] = definition.VDirections[layerIndex];
		}

		MakeTextureNonResident(m_hairMeshHandle);
		MakeTextureNonResident(m_uvHandle);
		MakeTextureNonResident(m_wHandle);
		MakeTextureNonResident(m_uHandle);
		MakeTextureNonResident(m_vHandle);
		m_hairMeshHandle = 0;
		m_uvHandle = 0;
		m_wHandle = 0;
		m_uHandle = 0;
		m_vHandle = 0;

		DeleteBuffer(m_assetUBO);
		DeleteBuffer(m_bundleSSBO);
		DeleteBuffer(m_rootSampleSSBO);
		DeleteBuffer(m_simulationParamsUBO);
		DeleteBuffer(m_simulationStateBuffers[0]);
		DeleteBuffer(m_simulationStateBuffers[1]);
		DeleteBuffer(m_simulationRestSSBO);
		DeleteTexture(m_hairMeshTexture);
		DeleteTexture(m_uvTexture);
		DeleteTexture(m_wTexture);
		DeleteTexture(m_uTexture);
		DeleteTexture(m_vTexture);

		m_simulationReadIndex = 0;
		m_bundleCount = 0;
		m_atlasWidth = kAtlasWidth;
		m_atlasHeight = kAtlasHeight;
		m_controlPointCount = static_cast<GLuint>(HairStyleSliceCount * kPatchVertexCount * kPatchVertexCount);
		m_bundles.clear();
		m_baseBundleBounds.clear();
		m_initialSimulationPoints.clear();
		m_restSimulationPoints.clear();
		m_debugLines = BuildDebugLines(cage);
		m_baseBounds = ComputeBounds(cage);
		bounds = m_baseBounds;

		std::vector<glm::vec4> hairMeshTexels(static_cast<size_t>(kAtlasWidth * kAtlasHeight * HairMeshSliceCount), glm::vec4(0.0f));
		std::vector<glm::vec2> uvTexels(static_cast<size_t>(kAtlasWidth * kAtlasHeight), glm::vec2(0.0f));
		std::vector<float> wTexels(static_cast<size_t>(kAtlasWidth * kAtlasHeight * HairStyleSliceCount), 0.0f);
		std::vector<glm::vec4> uTexels(static_cast<size_t>(kAtlasWidth * kAtlasHeight * HairStyleSliceCount), glm::vec4(0.0f));
		std::vector<glm::vec4> vTexels(static_cast<size_t>(kAtlasWidth * kAtlasHeight * HairStyleSliceCount), glm::vec4(0.0f));
		m_bundles.reserve(HairPatchQuadCount * HairPatchQuadCount);
		m_baseBundleBounds.reserve(HairPatchQuadCount * HairPatchQuadCount);

		for (int bundleZ = 0; bundleZ < HairPatchQuadCount; ++bundleZ) {
			for (int bundleX = 0; bundleX < HairPatchQuadCount; ++bundleX) {
				const int baseTexelX = bundleX * kBundleTexelSize;
				const int baseTexelY = bundleZ * kBundleTexelSize;
				const std::array<int, 4> vertexIndices = {
					VertexIndex(bundleX, bundleZ),
					VertexIndex(bundleX + 1, bundleZ),
					VertexIndex(bundleX, bundleZ + 1),
					VertexIndex(bundleX + 1, bundleZ + 1)
				};

				HairBundleDesc bundle{};
				bundle.BoundsMin = glm::vec3(FLT_MAX);
				bundle.BoundsMax = glm::vec3(-FLT_MAX);
				bundle.AtlasRect = glm::vec4(static_cast<float>(baseTexelX), static_cast<float>(baseTexelY), 2.0f, 2.0f);
				bundle.SliceInfo = glm::ivec4(0, HairMeshSliceCount - 1, HairMeshSliceCount, HairStyleSliceCount);
				bundle.RootInfo = glm::ivec4(0, HairRootSampleCount, static_cast<int>(HairBundleTopology::Quad), 0);

				for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
					const float wCoord = static_cast<float>(layerIndex) / static_cast<float>(HairLayerCount);
					for (int localY = 0; localY < kBundleTexelSize; ++localY) {
						for (int localX = 0; localX < kBundleTexelSize; ++localX) {
							const int cornerIndex = localY * kBundleTexelSize + localX;
							const int vertexIndex = vertexIndices[cornerIndex];
							const size_t texelIndex = static_cast<size_t>(layerIndex) * kAtlasWidth * kAtlasHeight
								+ static_cast<size_t>(baseTexelY + localY) * kAtlasWidth
								+ static_cast<size_t>(baseTexelX + localX);

							wTexels[texelIndex] = wCoord;
							uTexels[texelIndex] = glm::vec4(cage.uDirections[layerIndex][vertexIndex], 0.0f);
							vTexels[texelIndex] = glm::vec4(cage.vDirections[layerIndex][vertexIndex], 0.0f);
						}
					}
				}

				for (int localY = 0; localY < kBundleTexelSize; ++localY) {
					for (int localX = 0; localX < kBundleTexelSize; ++localX) {
						const int cornerIndex = localY * kBundleTexelSize + localX;
						const int vertexIndex = vertexIndices[cornerIndex];
						const size_t texelIndex = static_cast<size_t>(baseTexelY + localY) * kAtlasWidth + static_cast<size_t>(baseTexelX + localX);
						uvTexels[texelIndex] = cage.uvCoordinates[vertexIndex];

						std::array<glm::vec3, HairStyleSliceCount> controlPoints{};
						for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
							controlPoints[layerIndex] = cage.layers[layerIndex][vertexIndex];
						}

						const auto curveSlices = BuildSliceCurve(controlPoints);
						for (int sliceIndex = 0; sliceIndex < HairMeshSliceCount; ++sliceIndex) {
							const size_t sliceTexelIndex = static_cast<size_t>(sliceIndex) * kAtlasWidth * kAtlasHeight
								+ static_cast<size_t>(baseTexelY + localY) * kAtlasWidth
								+ static_cast<size_t>(baseTexelX + localX);
							hairMeshTexels[sliceTexelIndex] = glm::vec4(curveSlices[sliceIndex], 1.0f);
							bundle.BoundsMin = glm::min(bundle.BoundsMin, curveSlices[sliceIndex]);
							bundle.BoundsMax = glm::max(bundle.BoundsMax, curveSlices[sliceIndex]);
						}
					}
				}

				m_bundles.push_back(bundle);
				m_baseBundleBounds.push_back(AABB{ bundle.BoundsMin, bundle.BoundsMax });
			}
		}

		m_bundleCount = static_cast<GLuint>(m_bundles.size());
		m_initialSimulationPoints.resize(m_controlPointCount);
		m_restSimulationPoints.resize(m_controlPointCount);

		for (int layerIndex = 0; layerIndex < HairStyleSliceCount; ++layerIndex) {
			const float wCoord = static_cast<float>(layerIndex) / static_cast<float>(HairLayerCount);
			for (int z = 0; z < kPatchVertexCount; ++z) {
				for (int x = 0; x < kPatchVertexCount; ++x) {
					const int vertexIndex = VertexIndex(x, z);
					const int controlIndex = layerIndex * kPatchVertexCount * kPatchVertexCount + vertexIndex;
					const glm::vec3 point = cage.layers[layerIndex][vertexIndex];
					m_initialSimulationPoints[controlIndex].Position = glm::vec4(point, 1.0f);
					m_initialSimulationPoints[controlIndex].Velocity = glm::vec4(0.0f);
					m_restSimulationPoints[controlIndex].Position = glm::vec4(point, 1.0f);
					m_restSimulationPoints[controlIndex].UVW = glm::vec4(cage.uvCoordinates[vertexIndex], wCoord, 0.0f);
				}
			}
		}

		glCreateTextures(GL_TEXTURE_3D, 1, &m_hairMeshTexture);
		glTextureParameteri(m_hairMeshTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_hairMeshTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_hairMeshTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_hairMeshTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_hairMeshTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureStorage3D(m_hairMeshTexture, 1, GL_RGBA16F, kAtlasWidth, kAtlasHeight, HairMeshSliceCount);
		glTextureSubImage3D(m_hairMeshTexture, 0, 0, 0, 0, kAtlasWidth, kAtlasHeight, HairMeshSliceCount, GL_RGBA, GL_FLOAT, hairMeshTexels.data());

		glCreateTextures(GL_TEXTURE_2D, 1, &m_uvTexture);
		glTextureParameteri(m_uvTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_uvTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_uvTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_uvTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureStorage2D(m_uvTexture, 1, GL_RG16F, kAtlasWidth, kAtlasHeight);
		glTextureSubImage2D(m_uvTexture, 0, 0, 0, kAtlasWidth, kAtlasHeight, GL_RG, GL_FLOAT, uvTexels.data());

		glCreateTextures(GL_TEXTURE_3D, 1, &m_wTexture);
		glTextureParameteri(m_wTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_wTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_wTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_wTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_wTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureStorage3D(m_wTexture, 1, GL_R16F, kAtlasWidth, kAtlasHeight, HairStyleSliceCount);
		glTextureSubImage3D(m_wTexture, 0, 0, 0, 0, kAtlasWidth, kAtlasHeight, HairStyleSliceCount, GL_RED, GL_FLOAT, wTexels.data());

		glCreateTextures(GL_TEXTURE_3D, 1, &m_uTexture);
		glTextureParameteri(m_uTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_uTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_uTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_uTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_uTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureStorage3D(m_uTexture, 1, GL_RGBA16F, kAtlasWidth, kAtlasHeight, HairStyleSliceCount);
		glTextureSubImage3D(m_uTexture, 0, 0, 0, 0, kAtlasWidth, kAtlasHeight, HairStyleSliceCount, GL_RGBA, GL_FLOAT, uTexels.data());

		glCreateTextures(GL_TEXTURE_3D, 1, &m_vTexture);
		glTextureParameteri(m_vTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_vTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_vTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_vTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_vTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureStorage3D(m_vTexture, 1, GL_RGBA16F, kAtlasWidth, kAtlasHeight, HairStyleSliceCount);
		glTextureSubImage3D(m_vTexture, 0, 0, 0, 0, kAtlasWidth, kAtlasHeight, HairStyleSliceCount, GL_RGBA, GL_FLOAT, vTexels.data());

		MakeTextureResident(m_hairMeshTexture, m_hairMeshHandle);
		MakeTextureResident(m_uvTexture, m_uvHandle);
		MakeTextureResident(m_wTexture, m_wHandle);
		MakeTextureResident(m_uTexture, m_uHandle);
		MakeTextureResident(m_vTexture, m_vHandle);

		m_gpuData.Counts = glm::ivec4(HairLayerCount, HairPatchQuadCount, HairRootSampleCount, HairStrandSegmentCount);
		m_gpuData.LODParams = glm::vec4(HairRibbonHalfWidth, 15.0f, 0.35f, 0.125f);
		m_gpuData.HairMeshTexture = m_hairMeshHandle;
		m_gpuData.UVTexture = m_uvHandle;
		m_gpuData.WTexture = m_wHandle;
		m_gpuData.UTexture = m_uHandle;
		m_gpuData.VTexture = m_vHandle;
		ApplyStyleParameters();

		glCreateBuffers(1, &m_assetUBO);
		glNamedBufferData(m_assetUBO, sizeof(HairAssetGpuData), &m_gpuData, GL_DYNAMIC_DRAW);

		glCreateBuffers(1, &m_bundleSSBO);
		glNamedBufferData(m_bundleSSBO, static_cast<GLsizeiptr>(m_bundles.size() * sizeof(HairBundleDesc)), m_bundles.data(), GL_DYNAMIC_DRAW);

		const auto& rootSamples = GetSharedBlueNoiseSamples();
		glCreateBuffers(1, &m_rootSampleSSBO);
		glNamedBufferData(m_rootSampleSSBO, static_cast<GLsizeiptr>(rootSamples.size() * sizeof(HairRootSample)), rootSamples.data(), GL_STATIC_DRAW);

		glCreateBuffers(1, &m_simulationParamsUBO);
		glNamedBufferData(m_simulationParamsUBO, sizeof(HairSimulationGpuData), nullptr, GL_DYNAMIC_DRAW);
		glCreateBuffers(2, m_simulationStateBuffers);
		glNamedBufferData(m_simulationStateBuffers[0], static_cast<GLsizeiptr>(m_initialSimulationPoints.size() * sizeof(HairSimulationPointGpuData)), m_initialSimulationPoints.data(), GL_DYNAMIC_DRAW);
		glNamedBufferData(m_simulationStateBuffers[1], static_cast<GLsizeiptr>(m_initialSimulationPoints.size() * sizeof(HairSimulationPointGpuData)), m_initialSimulationPoints.data(), GL_DYNAMIC_DRAW);
		glCreateBuffers(1, &m_simulationRestSSBO);
		glNamedBufferData(m_simulationRestSSBO, static_cast<GLsizeiptr>(m_restSimulationPoints.size() * sizeof(HairSimulationRestPointGpuData)), m_restSimulationPoints.data(), GL_STATIC_DRAW);

		m_simulationParams = definition.SimulationParams;
		ApplySimulationParams();
	}

	HairMeshAsset::~HairMeshAsset() {
		UnregisterHairAsset(this);

		MakeTextureNonResident(m_hairMeshHandle);
		MakeTextureNonResident(m_uvHandle);
		MakeTextureNonResident(m_wHandle);
		MakeTextureNonResident(m_uHandle);
		MakeTextureNonResident(m_vHandle);

		DeleteBuffer(m_assetUBO);
		DeleteBuffer(m_bundleSSBO);
		DeleteBuffer(m_rootSampleSSBO);
		DeleteBuffer(m_simulationParamsUBO);
		DeleteBuffer(m_simulationStateBuffers[0]);
		DeleteBuffer(m_simulationStateBuffers[1]);
		DeleteBuffer(m_simulationRestSSBO);

		DeleteTexture(m_hairMeshTexture);
		DeleteTexture(m_uvTexture);
		DeleteTexture(m_wTexture);
		DeleteTexture(m_uTexture);
		DeleteTexture(m_vTexture);
	}

	bool HairMeshAsset::IsValid() const {
		return m_assetUBO != 0
			&& m_bundleSSBO != 0
			&& m_rootSampleSSBO != 0
			&& m_hairMeshHandle != 0
			&& m_uvHandle != 0
			&& m_wHandle != 0
			&& m_uHandle != 0
			&& m_vHandle != 0
			&& m_bundleCount > 0;
	}

	void HairMeshAsset::Bind() const {
		glBindBufferBase(GL_UNIFORM_BUFFER, HairBindings::HairAssetUBO, m_assetUBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HairBindings::HairBundleSSBO, m_bundleSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HairBindings::HairRootSampleSSBO, m_rootSampleSSBO);
	}

	GLuint HairMeshAsset::GetBundleCount() const {
		return m_bundleCount;
	}

	void HairMeshAsset::SetStyleParameters(const HairStyleParameters& params) {
		m_styleParams = params;
		ApplyStyleParameters();
		UpdateConservativeBounds();
		UploadBundleData();
		UploadAssetData();
	}

	const HairStyleParameters& HairMeshAsset::GetStyleParameters() const {
		return m_styleParams;
	}

	void HairMeshAsset::UploadAssetData() const {
		if (m_assetUBO != 0) {
			glNamedBufferSubData(m_assetUBO, 0, sizeof(HairAssetGpuData), &m_gpuData);
		}
	}

	void HairMeshAsset::ApplyStyleParameters() {
		m_gpuData.FuzzParams = glm::vec4(
			m_styleParams.FuzzAmplitude,
			m_styleParams.FuzzAmplitudeExponent,
			m_styleParams.FuzzDistributionExponent,
			0.0f
		);
		m_gpuData.FrizzParams = glm::vec4(
			m_styleParams.FrizzAmplitude,
			m_styleParams.FrizzAmplitudeExponent,
			m_styleParams.FrizzFrequency,
			0.0f
		);
		m_gpuData.KinkParams = glm::vec4(
			m_styleParams.KinkAmplitude,
			m_styleParams.KinkAmplitudeExponent,
			m_styleParams.KinkUVFrequency,
			m_styleParams.KinkWFrequency
		);
		m_gpuData.CurlParams = glm::vec4(
			m_styleParams.CurlAmplitude,
			m_styleParams.CurlAmplitudeExponent,
			m_styleParams.CurlDistributionExponent,
			m_styleParams.CurlFrequency
		);
		m_gpuData.StyleExtraParams = glm::vec4(
			m_styleParams.CurlRotations,
			m_styleParams.ClumpThickness,
			m_styleParams.ClumpExponent,
			m_styleParams.ClumpNoiseAmount
		);
		m_gpuData.ClumpParams = glm::vec4(
			m_styleParams.ClumpNoiseFrequency,
			m_styleParams.ClumpGridResolution,
			m_styleParams.ClumpPercentage,
			m_styleParams.ClumpVariation
		);
	}

	void HairMeshAsset::SetSimulationParams(const HairSimulationParams& params) {
		m_simulationParams = params;
		ApplySimulationParams();
	}

	const HairSimulationParams& HairMeshAsset::GetSimulationParams() const {
		return m_simulationParams;
	}

	void HairMeshAsset::UpdateSimulationUniforms(float deltaTime, float timeSeconds) {
		const glm::vec3 windDirection = SafeNormalize(m_simulationParams.WindDirection, glm::vec3(1.0f, 0.0f, 0.0f));
		m_simulationGpuData.GravityDeltaTime = glm::vec4(m_simulationParams.Gravity, deltaTime * m_simulationParams.TimeScale);
		m_simulationGpuData.WindDirectionTime = glm::vec4(windDirection, timeSeconds);
		m_simulationGpuData.Dynamics = glm::vec4(
			m_simulationParams.WindStrength,
			m_simulationParams.Stiffness,
			m_simulationParams.Damping,
			m_simulationParams.TipInfluence
		);
		m_simulationGpuData.Limits = glm::vec4(
			m_simulationParams.MaxDisplacement,
			m_simulationParams.WindPulseFrequency,
			m_simulationParams.Enabled ? 1.0f : 0.0f,
			static_cast<float>(m_controlPointCount)
		);
		UploadSimulationParams();
	}

	void HairMeshAsset::ResetSimulation() {
		m_simulationReadIndex = 0;
		if (!m_initialSimulationPoints.empty()) {
			const GLsizeiptr byteSize = static_cast<GLsizeiptr>(m_initialSimulationPoints.size() * sizeof(HairSimulationPointGpuData));
			glNamedBufferSubData(m_simulationStateBuffers[0], 0, byteSize, m_initialSimulationPoints.data());
			glNamedBufferSubData(m_simulationStateBuffers[1], 0, byteSize, m_initialSimulationPoints.data());
		}
	}

	bool HairMeshAsset::IsSimulationEnabled() const {
		return m_simulationParams.Enabled;
	}

	bool HairMeshAsset::HasSimulationState() const {
		return m_simulationParamsUBO != 0
			&& m_simulationStateBuffers[0] != 0
			&& m_simulationStateBuffers[1] != 0
			&& m_simulationRestSSBO != 0
			&& m_controlPointCount > 0;
	}

	void HairMeshAsset::BindSimulation() const {
		glBindBufferBase(GL_UNIFORM_BUFFER, HairBindings::HairAssetUBO, m_assetUBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, HairBindings::HairSimulationParamsUBO, m_simulationParamsUBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HairBindings::HairSimulationReadSSBO, m_simulationStateBuffers[m_simulationReadIndex]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HairBindings::HairSimulationWriteSSBO, m_simulationStateBuffers[1u - m_simulationReadIndex]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HairBindings::HairSimulationRestSSBO, m_simulationRestSSBO);
	}

	void HairMeshAsset::SwapSimulationState() {
		m_simulationReadIndex = 1u - m_simulationReadIndex;
	}

	GLuint HairMeshAsset::GetSimulationReadBuffer() const {
		return m_simulationStateBuffers[m_simulationReadIndex];
	}

	GLuint HairMeshAsset::GetSimulationWriteBuffer() const {
		return m_simulationStateBuffers[1u - m_simulationReadIndex];
	}

	GLuint HairMeshAsset::GetSimulationRestBuffer() const {
		return m_simulationRestSSBO;
	}

	GLuint HairMeshAsset::GetSimulationParamsBuffer() const {
		return m_simulationParamsUBO;
	}

	GLuint HairMeshAsset::GetHairMeshTexture() const {
		return m_hairMeshTexture;
	}

	GLuint HairMeshAsset::GetUTexture() const {
		return m_uTexture;
	}

	GLuint HairMeshAsset::GetVTexture() const {
		return m_vTexture;
	}

	GLuint HairMeshAsset::GetAtlasWidth() const {
		return m_atlasWidth;
	}

	GLuint HairMeshAsset::GetAtlasHeight() const {
		return m_atlasHeight;
	}

	GLuint HairMeshAsset::GetControlPointCount() const {
		return m_controlPointCount;
	}

	const std::vector<glm::vec4>& HairMeshAsset::GetDebugLineVertices() const {
		return m_debugLines;
	}

	void HairMeshAsset::UpdateConservativeBounds() {
		const float stylePadding =
			m_styleParams.FuzzAmplitude +
			m_styleParams.FrizzAmplitude +
			m_styleParams.KinkAmplitude +
			m_styleParams.CurlAmplitude +
			m_styleParams.ClumpThickness;
		const float simulationPadding = m_simulationParams.Enabled ? m_simulationParams.MaxDisplacement : 0.0f;
		const float totalPadding = HairRibbonHalfWidth * 2.5f + stylePadding + simulationPadding;
		const glm::vec3 padding(totalPadding);

		bounds.mMin = m_baseBounds.mMin - padding;
		bounds.mMax = m_baseBounds.mMax + padding;

		for (size_t bundleIndex = 0; bundleIndex < m_bundles.size() && bundleIndex < m_baseBundleBounds.size(); ++bundleIndex) {
			m_bundles[bundleIndex].BoundsMin = m_baseBundleBounds[bundleIndex].mMin - padding;
			m_bundles[bundleIndex].BoundsMax = m_baseBundleBounds[bundleIndex].mMax + padding;
		}
	}

	void HairMeshAsset::UploadSimulationParams() const {
		if (m_simulationParamsUBO != 0) {
			glNamedBufferSubData(m_simulationParamsUBO, 0, sizeof(HairSimulationGpuData), &m_simulationGpuData);
		}
	}

	void HairMeshAsset::ApplySimulationParams() {
		UpdateConservativeBounds();
		UploadBundleData();
		UpdateSimulationUniforms(0.0f, 0.0f);
	}

	void HairMeshAsset::UploadBundleData() const {
		if (m_bundleSSBO != 0 && !m_bundles.empty()) {
			glNamedBufferSubData(m_bundleSSBO, 0, static_cast<GLsizeiptr>(m_bundles.size() * sizeof(HairBundleDesc)), m_bundles.data());
		}
	}

	std::vector<glm::vec4> HairMeshAsset::BuildDebugLineVertices() {
		return BuildDebugLines(BuildProceduralHairCage());
	}

	const HairStyleParameters& HairMeshAsset::GetGlobalStyleParameters() {
		return g_globalStyleParameters;
	}

	void HairMeshAsset::SetGlobalStyleParameters(const HairStyleParameters& params) {
		g_globalStyleParameters = params;
		for (HairMeshAsset* asset : g_registeredAssets) {
			if (asset != nullptr) {
				asset->SetStyleParameters(params);
			}
		}
	}

	const HairSimulationParams& HairMeshAsset::GetGlobalSimulationParams() {
		return g_globalSimulationParameters;
	}

	void HairMeshAsset::SetGlobalSimulationParams(const HairSimulationParams& params) {
		g_globalSimulationParameters = params;
		for (HairMeshAsset* asset : g_registeredAssets) {
			if (asset != nullptr) {
				asset->SetSimulationParams(params);
			}
		}
	}
}
