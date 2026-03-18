#pragma once


#include "EWEngine/Reflect/Enum.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"
#include "EightWinds/RenderGraph/RasterTask.h"

#include "EWEngine/Assets/FileResource.h"

#include "EWEngine/Global.h"

#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "imgui.h"

/*
namespace EWE{
    namespace Node{

        struct RasterPackageHeader{
            enum RasterType{
                Vert,
                Index,
                Mesh,
                VertIndirect,
                IndexIndirect,
                MeshIndirect,
                //also counts, not currently
            };   
            RasterType rasterType;
            std::array<std::string, Shader::Stage::COUNT> shaders{};
            ObjectRasterConfig config;
            GlobalPushConstant_Abstract push;
        };
        template<typename T>
        struct RasterPackage{
            using Type = T;

            RasterPackageHeader head;
            T draw;

            void Imgui(){
                template for(constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^Type, std::meta::access_context::current()))){
                    ImGui::Text("%s : %d", Reflect::GetName<mem>().data(), draw.[:mem:]);
                    //ImGui::DragInt(Reflect::GetName<mem>().data(), &[:mem:], 0, 10, 100);
                }
            }
        };

        using VertexRaster = RasterPackage<ParamPack::VertexDraw>;
        using IndexedRaster = RasterPackage<IndexedDrawData>;
        using MeshRaster = RasterPackage<ParamPack::DrawMeshTasks>;
        using VertexIndirectRaster = RasterPackage<ParamPack::DrawIndirect>;
        using IndexedIndirectRaster = RasterPackage<ParamPack::DrawIndexedIndirect>;
        using MeshIndirectRaster = RasterPackage<ParamPack::DrawMeshTasksIndirect>;
        //using VertexIndirectCountRaster = RasterPackage<VertexIndirectCountDrawData>;

        struct RasterTaskNodeGraph{
            ImNodes::EWE::Editor editor;

            TaskRasterConfig taskConfig;

            [[nodiscard]] explicit RasterTaskNodeGraph()
            : editor{},
                taskConfig{},
                draw_data{},
                indexed_data{},
                mesh_data{},
                vert_indirect_data{},
                indexed_indirect_data{}
            {
                taskConfig.SetDefaults();
                SetFunctionHooks();
            }

            Hive<VertexRaster, 32> draw_data;
            Hive<IndexedRaster, 32> indexed_data;
            Hive<MeshRaster, 32> mesh_data;
            Hive<VertexIndirectRaster, 32> vert_indirect_data;
            Hive<IndexedIndirectRaster, 32> indexed_indirect_data;

            bool AddNodeMenu(ImVec2 menu_pos){
                bool wantsClose = false;

                ImGui::SetNextWindowPos(menu_pos);
                if(ImGui::Begin("add menu")){
                    if(ImGui::Button("Vertex Draw")){
                        auto& emp = draw_data.AddElement();
                        emp.head.config.SetDefaults();
                        emp.head.rasterType = RasterPackageHeader::RasterType::Vert;
                        auto& added_node = editor.AddNode();
                        added_node.payload = &emp;
                        added_node.pos = menu_pos;
                        wantsClose = true;
                    }
                    if(ImGui::Button("Indexed Draw")){
                        auto& emp = indexed_data.AddElement();
                        emp.head.config.SetDefaults();
                        emp.head.rasterType = RasterPackageHeader::RasterType::Index;
                        auto& added_node = editor.AddNode();
                        added_node.payload = &emp;
                        added_node.pos = menu_pos;
                        wantsClose = true;
                    }
                    if(ImGui::Button("Mesh Draw")){
                        auto& emp = mesh_data.AddElement();
                        emp.head.config.SetDefaults();
                        emp.head.rasterType = RasterPackageHeader::RasterType::Mesh;
                        auto& added_node = editor.AddNode();
                        added_node.payload = &emp;
                        added_node.pos = menu_pos;
                        wantsClose = true;
                    }
                    if(ImGui::Button("Vertex Indirect")){
                        auto& emp = vert_indirect_data.AddElement();
                        emp.head.config.SetDefaults();
                        emp.head.rasterType = RasterPackageHeader::RasterType::VertIndirect;
                        auto& added_node = editor.AddNode();
                        added_node.payload = &emp;
                        added_node.pos = menu_pos;
                        wantsClose = true;
                    }
                    if(ImGui::Button("Indexed Indirect")){
                        auto& emp = indexed_indirect_data.AddElement();
                        emp.head.config.SetDefaults();
                        emp.head.rasterType = RasterPackageHeader::RasterType::IndexIndirect;
                        auto& added_node = editor.AddNode();
                        added_node.payload = &emp;
                        added_node.pos = menu_pos;
                        wantsClose = true;
                    }
                }
                ImGui::End();
                return wantsClose;
            }

            void RenderNode(ImNodes::EWE::Node& node){

                auto& payload_header = *reinterpret_cast<RasterPackageHeader*>(node.payload);
                ImNodes::BeginNodeTitleBar();
                ImGui::Text("Raster type : %d", Reflect::enum_to_string(payload_header.rasterType));
                ImNodes::EndNodeTitleBar();

                for(uint8_t i = 0; i < Shader::Stage::COUNT; i++){
                    ImGui::Text("%s : %s", Reflect::enum_to_string(static_cast<Shader::Stage::Bits>(i)).data(), payload_header.shaders[i].c_str());
                    if(ImGui::BeginDragDropTarget()){
                        if (const ImGuiPayload* dragdrop_payload = ImGui::AcceptDragDropPayload("SHADER")){
                            payload_header.shaders[i] = reinterpret_cast<const char*>(dragdrop_payload->Data);
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
                for(uint8_t i = 0; i < GlobalPushConstant_Raw::buffer_count; i++){
                    if(payload_header.push.buffers[i]){
                        ImGui::Text("buffer[%u] : %s", i, payload_header.push.buffers[i]->name);
                    }
                    else{
                        ImGui::Text("buffer[%u]", i);
                    }
                    if(ImGui::BeginDragDropTarget()){
                        if (const ImGuiPayload* dragdrop_payload = ImGui::AcceptDragDropPayload("BUFFER")){
                            payload_header.push.buffers[i] = reinterpret_cast<Buffer*>(dragdrop_payload->Data);
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
                for(uint8_t i = 0; i < GlobalPushConstant_Raw::texture_count; i++){
                    if(payload_header.push.textures[i]){
                        ImGui::Text("tex index[%u] : %s", i, payload_header.push.textures[i]->view.name.c_str());
                    }
                    else{
                        ImGui::Text("tex index[%u]", i);
                    }
                    if(ImGui::BeginDragDropTarget()){
                        if (const ImGuiPayload* dragdrop_payload = ImGui::AcceptDragDropPayload("TEXTURE")){
                            payload_header.push.textures[i] = reinterpret_cast<DescriptorImageInfo*>(dragdrop_payload->Data);
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                switch(payload_header.rasterType){
                    case RasterPackageHeader::Vert: reinterpret_cast<VertexRaster*>(node.payload)->Imgui(); break;
                    case RasterPackageHeader::Index: reinterpret_cast<IndexedRaster*>(node.payload)->Imgui(); break;
                    case RasterPackageHeader::Mesh: reinterpret_cast<MeshRaster*>(node.payload)->Imgui(); break;
                    case RasterPackageHeader::VertIndirect: reinterpret_cast<VertexIndirectRaster*>(node.payload)->Imgui(); break;
                    case RasterPackageHeader::IndexIndirect: reinterpret_cast<IndexedIndirectRaster*>(node.payload)->Imgui(); break;
                    case RasterPackageHeader::MeshIndirect: reinterpret_cast<MeshIndirectRaster*>(node.payload)->Imgui(); break;
                }
            }

            void PinRender(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index){

            }

            void SetFunctionHooks(){
                editor.render_add_menu = [&](ImVec2 menu_pos){
                    return AddNodeMenu(menu_pos);
                };
                //editor.title_extension = [&]{
                //    TitleExtension();
                //};
                editor.node_renderer = [&](ImNodes::EWE::Node& node){
                    RenderNode(node);
                };
                editor.pin_renderer = [&](ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index){
                    PinRender(node, pin_index);
                };
            }
        };
    } 
} //namespace EWE
 */