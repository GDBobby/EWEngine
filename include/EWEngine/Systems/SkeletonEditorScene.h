#pragma once


#include "EWEngine/EWEngine.h"
#include "EWEngine/Scene.h"

#include "EWEngine/Imgui/ImguiFileExplorer.h"

#include "LAB/Support/Simple.h"
#include "gltf2cpp/gltf2cpp.hpp"

#include <vector>


namespace EWE{



    struct SplitVertexBufferObject{
        DeviceAddress pos;
        DeviceAddress normal;
        DeviceAddress uv;
        DeviceAddress tangent;
        DeviceAddress skin; //joints and weights
    };
    
    struct SplitVertexData{
        std::optional<Buffer> positions;
        std::optional<Buffer> normal;
        std::optional<Buffer> uv;
        std::optional<Buffer> tangent;

        std::optional<Buffer> skin;

        SplitVertexBufferObject bo;
    };
    
    struct SkinInstanceBufferObject{
        lab::mat4 modelMatrix;
        DeviceAddress bone_matrices; //vector of mat4s
    };

    struct MeshInstanceData{
        lab::Transform3 transform;
        std::size_t offset_into_bone_buffer;
    };

    struct CameraBufferObject{
        lab::mat4 projView;
        lab::mat4 proj;
        lab::mat4 view;
        lab::vec4 camPos;
    };

    struct LightBufferObject{
        Color_RGBA<float> ambientColor;
        lab::vec3 sunDir;
        Color_RGBA<float> sunColor;
        static constexpr std::size_t point_limit = 16;
        uint point_count = 0;
        struct PointLight{
            lab::vec4 pos;
            Color_RGBA<float> color;
        };
        PointLight pointLights[point_limit];
    };

    struct MaterialBufferObject{
        TextureIndex albedo_index;
        Color_RGB<float> albedo;
        
        TextureIndex normal_index;

        TextureIndex rough_index;
        float rough_val;

        TextureIndex metal_index;
        float metal_val;

        int bump_type; //make an enum later
        TextureIndex bump_index;
    };

    struct SkinBufferObject{
        DeviceAddress camera;
        DeviceAddress lights;
        DeviceAddress vertex;
        DeviceAddress material;

        DeviceAddress instances;
    };

    struct SkinPush{
        lab::vec3 cam_forward;
        float fov_radians = lab::DegreesToRadians(90.f);

        CameraBufferObject camera;
        LightBufferObject lights;
        SplitVertexData vert_data;
        MaterialBufferObject material;

        std::vector<MeshInstanceData> instances;

        Buffer cameraBuffer;
        Buffer lightBuffer;
        Buffer vertBuffer;
        Buffer materialBuffer;
        Buffer instanceBuffer;

        SkinBufferObject sbo;

        [[nodiscard]] SkinPush();
        //
    };




    class SkeletonScene : public SceneBase{
        public:
        [[nodiscard]] SkeletonScene();
        ~SkeletonScene();

		void Load() final;
		void Entry() final;
		bool RenderUpdate(double dt) final;
		void Exit() final;


        bool load_open = false;
        ExplorerContext explorer;
        static constexpr uint16_t path_length = 256;
        std::array<char, path_length> root_path;
        std::optional<gltf2cpp::Root> root;

        Command::ObjectPackage skeletonPackage;
        RasterPackage rasterPkg;
		Command::PackageRecord pkgRecord;
		GPUTask gpuTask;
		SubmissionTask subTask;
		RenderGraph renderGraph;

		FullRenderInfo fri;
		PerFlight<DescriptorImageInfo> attachment_diis;
		uint32_t color_attachment_index = 0;

        InstructionPointer<ParamPack<Inst::If>>* ifPack = nullptr;
        InstructionPointer<ParamPack<Inst::Push>>* pushPack = nullptr;
        InstructionPointer<ParamPack<Inst::DrawIndexed>>* drawPack = nullptr;

        SkinPush skinPush;

        void Imgui();

        //void SaveFile();
        void LoadFile();
    };
}