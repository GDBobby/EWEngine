#pragma once

#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/RenderGraph/RasterPackage.h"
#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/Shader.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EightWinds/Data/PerFlight.h"

#include "EWEngine/Data/Timing.h"

#include "LAB/Transform.h"

#include <random>
#include <functional>

//angF = angularFrequency * timestep
//EllRatio = ratio of minor to major axis in ellipse
//rotRatio = ratio of elliptical oscillation to rotation of leaf itself

//credit to Martin Pazout, his masters thesis on Falling Leaves Simulation
//idk how to cite so im just gonna put the school too
//czech technical university in prague


namespace EWE {

	struct LeafSystem {
	public:
		[[nodiscard]] LeafSystem(std::filesystem::path const& filepath);
		~LeafSystem();
		void RenderLoop(std::function<bool()> cancel_func);
	private:
		static constexpr int LEAF_COUNT = 256;
		static constexpr float WIND_SPEED{4.f};
		static constexpr float gravity = -1.06566f; //idk why gravity is such a weird value

		enum LeafFallMotions {
			LF_Steady,
			LF_Tumbling,
			LF_Fluttering,
			LF_Chaotic, //only diff between chaotic and tumbling is directrion of rotation
			LF_Helix,
			LF_Spiral, //no diff between helix and spiral, just variable values
		};

		struct LeafStruct {
			lab::Transform3 transform{};
			lab::vec3 origin{ 0.f };
			LeafFallMotions fallMotion{ LF_Steady };
			lab::vec3 averageVelocity{ 0 };
			float angF{ 0.f };
			float ellRatio{ 0.f };
			float rotRatio{ 0.f };
			float ellOsc{ 0.f };
			float time = 0.f;
			float fallAmplitude{ 0.f };
			float swingAmplitude{ 0.f };
			float rotSpeed{ 0.f };
		};


		//helix -> ellRatio~1  rotRatio~1
		//spiral -> elLRatio~0 rotRatio~4

		//i could use simd to significantly speed this up
		void LeafPhysicsInitialization();
		void FallCalculation(float timeStep);

		////this should be a graphics queue command buffer
		void InitData(std::filesystem::path const& filepath);

		void LoadLeafModel(std::filesystem::path const& filepath);
		void LoadLeafTexture();

		void Record();

		struct RandomSubObject{ 
			//this is just for grouping
			//it splits up the constructor a bit
			std::random_device dev;
			std::mt19937 gen;
			std::uniform_real_distribution<float> ellipseRatio;
			std::uniform_real_distribution<float> rotRatio;
			std::uniform_int_distribution<int> motion;
			std::uniform_real_distribution<float> ellipseOsc;
			std::uniform_real_distribution<float> angularFrequency;
			std::uniform_real_distribution<float> initTime;
			std::uniform_real_distribution<float> depthVariance;
			std::uniform_real_distribution<float> widthVariance;
			std::uniform_real_distribution<float> fallSwingVariance;
			std::uniform_real_distribution<float> initHeightVariance;
			std::uniform_real_distribution<float> variance;
			std::uniform_real_distribution<float> rockDist;

			[[nodiscard]] RandomSubObject();
		};
		RandomSubObject random;
		Buffer vertices;
		Buffer indices;
		PerFlight<Buffer> sceneBuffer;

		struct WorldData {
			lab::mat4 projView;
			lab::vec4 cameraPos;

			//light buffer, merged into 1 buffer
			lab::vec4 ambientColor;
			lab::vec4 sunDir; //w for sun power
			lab::vec4 sunColor;
		};
		struct SceneBufferObject{
			WorldData worldData;
			//point lights removed for the moment
			lab::mat4 transforms[LEAF_COUNT];
		};
		PerFlight<SceneBufferObject> sceneData;
		Shader vertShader;
		Shader fragShader;

		RenderGraph renderGraph; //the render graph is necessary, it handles STCs

		Command::ObjectPackage obj_pkg;
		RasterPackage raster_pkg;
		Command::PackageRecord pkg_record;
		GPUTask gpuTask;
		SubmissionTask subTask;

		InstructionPointer<ParamPack<Inst::Push>>* pushPacks;

		//this is going to generate a color image even though the swap image is going to be used
		//and i dont feel like fixing it right now
		FullRenderInfo renderInfo;

		uint16_t present_img_att_index = 0; //guaranteed to be 0 currently
		
		std::vector<LeafStruct> leafs{};

		//float FrictionPerp = 5.f;
		//float leafWeight = 1.f; //grams
		//float leafDensity = 0.1f;
		//float leafKA = 4.f;

		/*
		//POSITION:
			yVelocity = 200; //pixels per second
			oscFreq = 1.5; //oscillations per second
			oscDepth = 35; //oscillation depth (pixels)
			drift = 25; // drift (wind?) (pixels per second: - = left, + = right)

			value + [oscDepth * Math.sin(oscFreq * Math.PI * 2 * time) + drift * time,
				yVelocity * time, 0]

		//Z ROTATION :

			seed_random(index, true);
			random(360);

		//Y ROTATION :

			oscFreq = 1.5;
			maxTilt = 15; //degrees

			maxTilt* Math.cos(oscFreq* Math.PI * 2 * time)
		*/
	};
}