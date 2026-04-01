#include "EWEngine/NodeGraph/Record_NG.h"

#include "EWEngine/Imgui/DragDrop.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace EWE{
namespace Node{
    RecordNodeGraph::RecordNodeGraph()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions.push_back(".ewrg");
    }

    ImNodes::EWE::Node* RecordNodeGraph::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;
        head.payload = reinterpret_cast<void*>(UINT64_MAX);
        head.pos = node_editor_window_pos;
        head.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 

    Inst::Type RecordNodeGraph::GetInstructionFromNode(ImNodes::EWE::Node& node){
        return static_cast<Inst::Type>(reinterpret_cast<std::size_t>(node.payload));
    }

    void RecordNodeGraph::ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const {
        if(node.id != 0){
            auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
            ImGui::Text("node.id[%d] : inst[%s] - pos[%.2f:%.2f]", node.id, Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), temp_pos.x, temp_pos.y);
        }
    }
    void RecordNodeGraph::PrintNode(ImNodes::EWE::Node& node) const{
        Logger::Print<Logger::Debug>("node.id[%d] - type[%s] - node.pin[0].addr[%zu] - node.pin[1].addr[%zu]\n", 
            node.id, 
            Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), 
            node.pins[0]->payload, 
            node.pins[1]->payload
        );
    }
    
    void RecordNodeGraph::RenderNodes() {
        ImNodes::EWE::Editor::RenderNodes();
        Inst::Type* inst_type;
        if(DragDropPtr::Target(inst_type))
        {
            auto temp_min = ImGui::GetItemRectMin();
            auto temp_max =ImGui::GetItemRectMax();
            auto& added_node = CreateRGNode(*inst_type);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            auto window_pos =  ImGui::GetWindowPos();
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& RecordNodeGraph::CreateRGNode(int inst_index) {
        auto& added_node = AddNode();
        added_node.payload = reinterpret_cast<void*>(inst_index);//&emp;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    std::vector<Inst::Type> RecordNodeGraph::CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const{
        std::vector<Inst::Type> ret{};
        
        ImNodes::EWE::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr
        if(headNode->pins[0]->payload != nullptr){
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0]->payload);
            PrintNode(*current_node);
            ret.push_back(GetInstructionFromNode(*current_node));
            if(current_node == limit_node){
                //Logger::Print<Logger::Debug>("early early return - %s\n", __FUNCTION__);
                return ret;
            }

            if(limit_node == nullptr){
                while(current_node->pins[1]->payload != nullptr){
                    PrintNode(*current_node);

                    current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                    auto const current_inst = GetInstructionFromNode(*current_node);
                    ret.push_back(current_inst);
                }
            }
            else{
                while(current_node->pins[1]->payload != nullptr){
                    current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                    auto const current_inst = GetInstructionFromNode(*current_node);
                    ret.push_back(current_inst);
                    if(current_node == limit_node){
                        return ret;
                    }
                }
            }
        }

        if(limit_node != nullptr){
            //if the limit_node wasn't in the chain, an empty vector will be returned
            //if it was in the chain, it was the last node
            if(current_node != limit_node){
                return {};
            }
        }
        return ret;
    }

    void RecordNodeGraph::OpenAddMenu() {
        add_menu_is_open = true;
        static constexpr std::size_t type_count = std::meta::enumerators_of(^^Inst::Type).size();
        if(acceptable_add_instructions.size() != type_count){
            acceptable_add_instructions.clear();
            acceptable_add_instructions.reserve(type_count);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
            template for(constexpr auto inst : std::define_static_array(std::meta::enumerators_of(^^Inst::Type))){
                acceptable_add_instructions.push_back([:inst:]);
            }
#pragma GCC diagnostic pop
        }
    }

    bool RecordNodeGraph::AddInstructionButton(Inst::Type itype) {

        if (filter.PassFilter(Reflect::Enum::ToString(itype).data())){
            if(ImGui::Button(Reflect::Enum::ToString(itype).data())){
                auto& added_node = CreateRGNode(itype);
                if(link_empty_drop_srcNode != nullptr){
                    const uint16_t start_offset = link_empty_drop_srcNode == headNode ? 0 : 1;
                    links.emplace_back(
                        ImNodes::EWE::NodePair{
                            .start = link_empty_drop_srcNode,
                            .start_offset = start_offset,
                            .end = &added_node,
                            .end_offset = 0
                        }
                    );
                }
                return true;
            }
        }
        return false;
    }

    bool RecordNodeGraph::RenderAddMenu() {
        bool wantsClose = false;

        bool window_not_focused;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){

            if(ImGui::BeginListBox("##lb record")){
                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                    filter.Clear();
                }
                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
                filter.Draw("##Filter", -FLT_MIN);

                window_not_focused = !ImGui::IsWindowFocused();
                for(auto const& inst : acceptable_add_instructions){
                    wantsClose |= AddInstructionButton(inst);
                }

                ImGui::EndListBox();
            }
        }
        ImGui::End();
        if(wantsClose | window_not_focused){
            link_empty_drop_srcNode = nullptr;
        }
        return wantsClose | window_not_focused;
    }

    void RecordNodeGraph::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {
        auto const& instructions = CollectInstructionsUpTo(&src_node);

        acceptable_add_instructions = Instruction::GetValidInstructionsAtBackOf(instructions);

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
        link_empty_drop_srcNode = &src_node;
    }

    void RecordNodeGraph::LinkCreated(ImNodes::EWE::NodePair& link) {
        link.start->pins[link.start_offset]->payload = link.end;
        link.end->pins[link.end_offset]->payload = link.start;
    }

    void RecordNodeGraph::RenderNode(ImNodes::EWE::Node& node){
        const std::size_t inst_index = reinterpret_cast<std::size_t>(node.payload);

        ImNodes::BeginNodeTitleBar();

        if(inst_index == UINT64_MAX){
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "head node");
        }
        else if(inst_index >= Reflect::Enum::enum_data<Inst::Type>.size()){
            //if its greater than that, it's a record
            
        }
        else{
            ImGui::Text("%s", Reflect::Enum::ToString(static_cast<Inst::Type>(inst_index)).data());
        }
        ImNodes::EndNodeTitleBar();

        //ImGui::DebugLog("");
        ImGui::Text(""); //empty text just to populate this
    }

    void RecordNodeGraph::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin->local_pos)) {
            //ImGui::Text("pin");
        }
        ImNodes::EndPinAttribute();
    }

    bool RecordNodeGraph::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;
                auto const& collected_instructions = CollectInstructions();
                Logger::Print<Logger::Debug>("collected inst count : %zu\n", collected_instructions.size());
                Command::Record::WriteInstructions(saved_path.string(), collected_instructions);
                explorer.enabled = false;
                explorer.selected_file.reset();
            }
            if(!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)){
                //auto imwin = ImGui::GetCurrentWindow();
                explorer.enabled = false;
            }
        }
        ImGui::End();
        return !explorer.enabled;
    }

    void RecordNodeGraph::CreateFromInstructions(std::span<const Inst::Type> create_instructions){
        nodes.Clear();
        links.clear();
        if(create_instructions.size() == 0){
            return;
        }

        headNode = CreateHeadNode();
        auto& init_node = CreateRGNode(create_instructions[0]);
        headNode->pins[0]->payload = reinterpret_cast<ImNodes::EWE::Node*>(&init_node);
        init_node.pos.x = headNode->pos.x + 100.f;
        init_node.pos.y = headNode->pos.y + 30.f;


        auto* lastNode = &init_node;
        init_node.pins[0]->payload = headNode;

        links.emplace_back(
            ImNodes::EWE::NodePair{
                .start = headNode,
                .start_offset = 0,
                .end = &init_node,
                .end_offset = 0
            }
        );

        for(std::size_t i = 1; i < create_instructions.size(); i++){
            auto& node = CreateRGNode(create_instructions[i]);
            node.pos = lastNode->pos;
            node.pos.x += 100.f;
            node.pos.y += 30.f;

            node.pins[0]->payload = lastNode;
            lastNode->pins[1]->payload = &node;


            links.emplace_back(
                ImNodes::EWE::NodePair{
                    .start = lastNode,
                    .start_offset = 1,
                    .end = &node,
                    .end_offset = 0
                }
            );

            lastNode = &node;
        }

        nodes.ShrinkToFit();
    }

    bool RecordNodeGraph::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;
                //put the record somewhere
                auto const& loaded_instructions = Command::Record::ReadInstructions(load_path.string());

                CreateFromInstructions(std::span{loaded_instructions.Data(), loaded_instructions.Size()});

                Logger::Print<Logger::Debug>("loaded instructions size : %zu\n", loaded_instructions.Size());
                explorer.enabled = false;
                explorer.selected_file.reset();
            }
            if(!ImGui::IsWindowFocused()){
                //auto imwin = ImGui::GetCurrentWindow();
                explorer.enabled = false;
            }
        }
        ImGui::End();
        return !explorer.enabled;
    }
} //namespace Node
} //namespace EWE