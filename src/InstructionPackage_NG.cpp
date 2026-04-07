#include "EWEngine/NodeGraph/InstructionPackage_NG.h"
#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "EWEngine/Imgui/Params.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/VulkanHeader.h"

namespace EWE{
namespace Node{

    InstructionPackageNodeGraph::InstructionPackageNodeGraph()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions.push_back(".eip");
        acceptable_add_instructions = std::vector<Inst::Type>{
            Command::InstructionPackage::allowed_instructions.begin(), 
            Command::InstructionPackage::allowed_instructions.end()
        }; 
    }

    ImNodes::EWE::Node* InstructionPackageNodeGraph::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;
        auto* node_payload = new InstNodePayload();
        node_payload->type = InstNodePayload::Head;
        node_payload->distanceFromHead = -1; //head isn['t connected to head
        head.payload = node_payload;
        head.pos = node_editor_window_pos;
        head.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 

    Inst::Type InstructionPackageNodeGraph::GetInstructionFromNode(ImNodes::EWE::Node& node){
        return reinterpret_cast<InstNodePayload*>(node.payload)->iType;
    }

    void InstructionPackageNodeGraph::ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const {
        if(node.id != 0){
            auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
            ImGui::Text("node.id[%d] : inst[%s] - pos[%.2f:%.2f]", node.id, Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), temp_pos.x, temp_pos.y);
        }
    }

    void InstructionPackageNodeGraph::PrintNode(ImNodes::EWE::Node& node) const{
        Logger::Print<Logger::Debug>("node.id[%d] - type[%s] - node.pin[0].addr[%zu] - node.pin[1].addr[%zu]\n", 
            node.id, 
            Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), 
            node.pins[0]->payload, 
            node.pins[1]->payload
        );
    }


    void InstructionPackageNodeGraph::RenderNodes() {
        ImGui::BulletText("param instruction count : %zu", paramPool.instructions.size());
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

    ImNodes::EWE::Node& InstructionPackageNodeGraph::CreateRGNode(Inst::Type iType) {
        auto& added_node = AddNode();
        auto* node_payload = new InstNodePayload();
        node_payload->type = InstNodePayload::Instruction;
        node_payload->iType = iType;

        added_node.payload = node_payload;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(new ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    std::vector<Inst::Type> InstructionPackageNodeGraph::CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const{
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

    void InstructionPackageNodeGraph::InsertNodeToParamPool(ImNodes::EWE::Node* inserted_node){
        UpdateNodeOffsets();
        auto* node_payload = reinterpret_cast<InstNodePayload*>(inserted_node->payload);
        if(node_payload->distanceFromHead >= 0){
            paramPool.Insert(static_cast<std::size_t>(node_payload->distanceFromHead), node_payload->iType);
        }
    }

    void InstructionPackageNodeGraph::UpdateNodeOffsets() {

        InstNodePayload* current_payload = nullptr;
        for(auto& node : nodes){
            current_payload = reinterpret_cast<InstNodePayload*>(node.payload);
            current_payload->distanceFromHead = -1;
        }
        
        ImNodes::EWE::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr

        std::size_t distance = 0;
        if(headNode->pins[0]->payload != nullptr){
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0]->payload);
            if(current_node == nullptr){
                //Logger::Print<Logger::Debug>("early early return - %s\n", __FUNCTION__);
                return;
            }
            current_payload = reinterpret_cast<InstNodePayload*>(current_node->payload);
            current_payload->distanceFromHead = distance++;
            //PrintNode(*current_node);

            while(current_node->pins[1]->payload != nullptr){
                PrintNode(*current_node);

                current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1]->payload);
                current_payload = reinterpret_cast<InstNodePayload*>(current_node->payload);
                current_payload->distanceFromHead = distance++;
            }
            
        }
    }

    void InstructionPackageNodeGraph::OpenAddMenu() {
        add_menu_is_open = true;
    }

    bool InstructionPackageNodeGraph::InsertLink(ImNodes::EWE::Node& added_node, ImNodes::EWE::Node* otherNode){
        //printf("creating in between\n");
        //this is added an inst in between two connected nodes
        //first, find the link that connects those two
        ImNodes::EWE::NodePair* existing_link = nullptr;
        for(auto& link : links){
            if((link.start == link_empty_drop || link.end == link_empty_drop) && (link.start.node == otherNode || link.end.node == otherNode)){
                existing_link = &link;
                break;
            }
        }
        EWE_ASSERT(existing_link != nullptr);
        
        //offsets should already be at 1 and 0 respectively
        existing_link->start.GetPin()->payload = &added_node;
        existing_link->end.GetPin()->payload = &added_node;
        added_node.pins[0]->payload = existing_link->start.node;
        added_node.pins[1]->payload = existing_link->end.node;
        ImNodes::EWE::NodePair created_link{
            .start = ImNodes::EWE::NodeAndPin{
                .node = &added_node,
                .offset = 1 - existing_link->end.offset
            },
            .end = ImNodes::EWE::NodeAndPin{
                .node = existing_link->end.node,
                .offset = existing_link->end.offset
            }
        };
        existing_link->end.node = &added_node;
        links.push_back(created_link); //i want this to make a copy, may need todebug to make sure it does
        link_empty_drop.node = nullptr;

        UpdateNodeOffsets();
        InsertNodeToParamPool(&added_node);

        return true;
    }

    bool InstructionPackageNodeGraph::AddInstructionButton(Inst::Type itype){
        if (filter.PassFilter(Reflect::Enum::ToString(itype).data())){ 
            if(ImGui::Button(Reflect::Enum::ToString(itype).data())){
                auto& added_node = CreateRGNode(itype);
                if(link_empty_drop.node != nullptr){
                    ImNodes::EWE::Node* otherNode = reinterpret_cast<ImNodes::EWE::Node*>(link_empty_drop.GetPin()->payload);
                    if(otherNode != nullptr) {
                        return InsertLink(added_node, otherNode);
                    }

                    reinterpret_cast<InstNodePayload*>(added_node.payload)->distanceFromHead = reinterpret_cast<InstNodePayload*>(link_empty_drop.node->payload)->distanceFromHead + 1;

                    if(link_empty_drop.node == headNode){
                        LinkCreated(
                            links.emplace_back(
                                ImNodes::EWE::NodePair{
                                    .start = ImNodes::EWE::NodeAndPin{
                                        .node = &added_node,
                                        .offset = 0
                                    },
                                    .end = ImNodes::EWE::NodeAndPin{
                                        .node = headNode,
                                        .offset = 0
                                    }
                                }
                            )
                        );
                    }
                    else if(link_empty_drop.offset == 1){
                        LinkCreated(
                            links.emplace_back(
                                ImNodes::EWE::NodePair{
                                    .start = ImNodes::EWE::NodeAndPin{
                                        .node = &added_node,
                                        .offset = 1 - link_empty_drop.offset
                                    },
                                    .end = link_empty_drop
                                }
                            )
                        );
                    }
                    else{
                        LinkCreated(
                            links.emplace_back(
                                ImNodes::EWE::NodePair{
                                    .start = link_empty_drop,
                                    .end = ImNodes::EWE::NodeAndPin{
                                        .node = &added_node,
                                        .offset = 1 - link_empty_drop.offset
                                    }
                                }
                            )
                        );
                    }
                }
                link_empty_drop.node = nullptr;
                return true;
            }
        }
        return false;
    }

    bool InstructionPackageNodeGraph::RenderAddMenu() {
        bool wantsClose = false;

        bool window_not_focused;

        ImGui::SetNextWindowPos(menu_pos);
        if(ImGui::Begin("add menu")){

            if (ImGui::BeginListBox("##add inst listbox")) {
                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                    filter.Clear();
                }
                ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);
                filter.Draw("##Filter", -FLT_MIN);

                window_not_focused = !ImGui::IsWindowFocused();
                //printf("acceptable add size : %zu\n", acceptable_add_instructions.size());
                for(auto const& inst : acceptable_add_instructions){
                    wantsClose |= AddInstructionButton(inst);
                }
                ImGui::EndListBox();
            }
        }
        ImGui::End();
        if(wantsClose | window_not_focused){
            if(link_empty_drop.node != nullptr){
                //check if its linked, if so, delete the link
                if(link_empty_drop.GetPin()->payload != nullptr){
                    auto* otherNode = reinterpret_cast<ImNodes::EWE::Node*>(link_empty_drop.GetPin()->payload);
                    ImNodes::EWE::NodePair* existing_link = nullptr;
                    for(auto& link : links){
                        if(
                            (link.start.node == link_empty_drop.node || link.end.node == link_empty_drop.node)
                            && (link.start.node == otherNode || link.end.node == otherNode)
                        ){
                            existing_link = &link;
                            break;
                        }
                    }
                    if(existing_link != nullptr){
                        LinkDestroyed(*existing_link);
                    }
                }
            }
            link_empty_drop.node = nullptr;
        }
        return wantsClose | window_not_focused;
    }

    void InstructionPackageNodeGraph::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {
        //auto const& instructions = CollectInstructionsUpTo(&src_node);

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
        link_empty_drop.node = &src_node;
        link_empty_drop.offset = pin_offset;
    }
    void InstructionPackageNodeGraph::LinkCreated(ImNodes::EWE::NodePair& link) {
        if(link.start.node->pins[link.start.offset]->payload != nullptr 
            || link.end.node->pins[link.end.offset]->payload != nullptr
        ){
            ImNodes::EWE::Editor::LinkDestroyed(link);
            return;
        }
        if(link.start.node == headNode || link.end.node == headNode){
            if(link.start.offset != 0 || link.end.offset != 0){
                ImNodes::EWE::Editor::LinkDestroyed(link);
                return;
            }
            if(link.end.node == headNode){
                std::swap(link.end, link.start);
            }
        }
        else {
            if(link.start.offset == link.end.offset){
                ImNodes::EWE::Editor::LinkDestroyed(link);
                return;
            }
            if(link.start.offset == 0){
                std::swap(link.start, link.end);
            }
        }

        Logger::Print<Logger::Debug>("ipng::link created\n");
        link.start.node->pins[link.start.offset]->payload = link.end.node;
        link.end.node->pins[link.end.offset]->payload = link.start.node;
        if(reinterpret_cast<InstNodePayload*>(link.start.node->payload)->distanceFromHead >= 0 || link.start.node == headNode){
            Logger::Print<Logger::Debug>("ipng : push back necessary\n");
            auto* end_payload = reinterpret_cast<InstNodePayload*>(link.end.node->payload);
            paramPool.PushBack(end_payload->iType);
            if(end_payload->iType == Inst::Push){
                for(uint8_t frame = 0; frame < max_frames_in_flight; frame++){
                    GlobalPushConstant_Raw* push = reinterpret_cast<GlobalPushConstant_Raw*>(paramPool.param_data.back().data[frame]);
                    for(uint8_t i = 0; i < GlobalPushConstant_Raw::buffer_count; i++){
                        push->buffer_addr[i] = null_buffer;
                    }
                    for(uint8_t i = 0; i < GlobalPushConstant_Raw::texture_count; i++){
                        push->texture_indices[i] = null_texture;
                    }
                }
            }
            UpdateNodeOffsets();
        }
    }
    void InstructionPackageNodeGraph::LinkDestroyed(ImNodes::EWE::NodePair& link) {
        Logger::Print<Logger::Debug>("ipng::link destroyed\n");
        if(link.start.node->pins[link.start.offset] != nullptr){
            link.start.node->pins[link.start.offset]->payload = nullptr;
        }
        link.end.node->pins[link.end.offset]->payload = nullptr;
        auto* end_payload = reinterpret_cast<InstNodePayload*>(link.end.node->payload);
        //auto* start_payload = reinterpret_cast<InstNodePayload*>(link.start.node->payload); 
        if(end_payload->distanceFromHead >= 0){
            Logger::Print<Logger::Debug>("ipng : erase necessary\n");
            paramPool.ShrinkToSize(end_payload->distanceFromHead);
            UpdateNodeOffsets();
        }
        ImNodes::EWE::Editor::LinkDestroyed(link);
    }

    void InstructionPackageNodeGraph::RenderNode(ImNodes::EWE::Node& node) {
        auto node_payload = reinterpret_cast<InstNodePayload*>(node.payload);

        ImNodes::BeginNodeTitleBar();

        if(node_payload->type == InstNodePayload::Type::Head){
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "head node");
        }
        else if(node_payload->type == InstNodePayload::Package){
            //if its greater than that, it's a record
            
        }
        else{
            ImGui::Text("%s[%d]", Reflect::Enum::ToString(node_payload->iType).data(), node.id);
        }
        ImNodes::EndNodeTitleBar();

        //ImGui::DebugLog("");
        ImGui::Text("distance from head : %d", node_payload->distanceFromHead); //empty text just to populate this
        const std::size_t param_size = Instruction::GetParamSize(node_payload->iType);
        ImGui::Text("param size : %zu", param_size);
        if(node_payload->distanceFromHead >= 0){
            if(param_size > 0){
                const std::size_t pack_index = paramPool.GetPackIndex(node_payload->distanceFromHead);
                ImguiExpandInstruction(reinterpret_cast<void*>(paramPool.param_data[pack_index].data[0]), paramPool.instructions[node_payload->distanceFromHead]);
            }
        }
    }

    void InstructionPackageNodeGraph::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin->local_pos)) {
            //ImGui::Text("pin");
            
        }
        if(node.pins[pin_index]->payload != nullptr){
            ImGui::Text("payload id : %d", reinterpret_cast<ImNodes::EWE::Node*>(node.pins[pin_index]->payload)->id);
        }
        ImNodes::EndPinAttribute();
    }

    bool InstructionPackageNodeGraph::SaveFunc() {
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

    void InstructionPackageNodeGraph::CreateFromInstructions(std::span<const Inst::Type> create_instructions){
        nodes.Clear();
        links.clear();
        if(create_instructions.size() == 0){
            return;
        }

        headNode = CreateHeadNode();
        auto& init_node = CreateRGNode(create_instructions[0]);
        reinterpret_cast<InstNodePayload*>(init_node.payload)->distanceFromHead = 0;
        headNode->pins[0]->payload = reinterpret_cast<ImNodes::EWE::Node*>(&init_node);
        init_node.pos.x = headNode->pos.x + 100.f;
        init_node.pos.y = headNode->pos.y + 30.f;


        auto* lastNode = &init_node;
        lastNode->payload = new InstNodePayload();
        init_node.pins[0]->payload = headNode;

        
        LinkCreated(
            links.emplace_back(
                ImNodes::EWE::NodePair{
                    .start = ImNodes::EWE::NodeAndPin{
                        .node = headNode,
                        .offset = 0
                    },
                    .end = ImNodes::EWE::NodeAndPin{
                        .node = &init_node,
                        .offset = 0
                    }
                }
            )
        );

        for(std::size_t i = 1; i < create_instructions.size(); i++){
            auto& node = CreateRGNode(create_instructions[i]);
            node.pos = lastNode->pos;
            node.pos.x += 100.f;
            node.pos.y += 30.f;

            node.pins[0]->payload = lastNode;
            reinterpret_cast<InstNodePayload*>(node.payload)->distanceFromHead = reinterpret_cast<InstNodePayload*>(lastNode->payload)->distanceFromHead + 1;
            lastNode->pins[1]->payload = &node;

            LinkCreated(
                links.emplace_back(
                    ImNodes::EWE::NodePair{
                        .start = ImNodes::EWE::NodeAndPin{
                            .node = lastNode,
                            .offset = 1
                        },
                        .end = ImNodes::EWE::NodeAndPin{
                            .node = &node,
                            .offset = 0
                        }
                    }
                )
            );
            lastNode = &node;
        }

        nodes.ShrinkToFit();
    }

    bool InstructionPackageNodeGraph::LoadFunc() {
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