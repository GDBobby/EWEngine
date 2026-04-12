#include "EWEngine/Imgui/ImNodes/Graph/InstructionPackage_NG.h"
#include "EWEngine/Global.h"
#include "EWEngine/Imgui/ImNodes/imnodes_ewe.h"
#include "EWEngine/Imgui/Objects.h"
#include "EWEngine/Imgui/Params.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/ObjectRasterConfig.h"
#include "EightWinds/Preprocessor.h"
#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/VulkanHeader.h"

namespace EWE{
namespace Node{

    InstructionPackage_NG::InstructionPackage_NG()
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions = Global::assetManager->instPkg.files.acceptable_extensions;
        acceptable_add_instructions = std::vector<Inst::Type>{
            Command::InstructionPackage::allowed_instructions.begin(), 
            Command::InstructionPackage::allowed_instructions.end()
        }; 
        package_payload = nullptr;
    }

    InstructionPackage_NG::InstructionPackage_NG(Command::InstructionPackage& pkg)
    : ImNodes::EWE::Editor{true, true},
        explorer{std::filesystem::current_path()},
        headNode{CreateHeadNode()}
    {
        explorer.acceptable_extensions = Global::assetManager->instPkg.files.acceptable_extensions;
        InitFromObject(pkg);
    }

    ImNodes::EWE::Node* InstructionPackage_NG::CreateHeadNode(){
        auto& head = AddNode();
        head.snapToGrid = true;
        auto* node_payload = new InstNodePayload();
        node_payload->type = InstNodePayload::Head;
        node_payload->distanceFromHead = -1; //head isn['t connected to head
        head.payload = node_payload;
        head.pos = node_editor_window_pos;
        head.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});
        return &head;
    } 

    Inst::Type InstructionPackage_NG::GetInstructionFromNode(ImNodes::EWE::Node& node){
        return reinterpret_cast<InstNodePayload*>(node.payload)->iType;
    }

    void InstructionPackage_NG::ImGuiNodeDebugPrint(ImNodes::EWE::Node& node) const {
        if(node.id != 0){
            auto temp_pos = ImNodes::GetNodeScreenSpacePos(node.id);
            ImGui::Text("node.id[%d] : inst[%s] - pos[%.2f:%.2f]", node.id, Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), temp_pos.x, temp_pos.y);
        }
    }

    void InstructionPackage_NG::PrintNode(ImNodes::EWE::Node& node) const{
        Logger::Print<Logger::Debug>("node.id[%d] - type[%s] - node.pin[0].addr[%zu] - node.pin[1].addr[%zu]\n", 
            node.id, 
            Reflect::Enum::ToString(GetInstructionFromNode(node)).data(), 
            node.pins[0].payload, 
            node.pins[1].payload
        );
    }

    void InstructionPackage_NG::RenderEditorTitle(){
        ImGui::BulletText("param data - inst count[%zu] : param heap size[%zu] : param count[%zu]", paramPool.instructions.size(), paramPool.params[0].Size(), paramPool.param_data.size());
        ImNodes::EWE::Editor::RenderEditorTitle();
        if(packageType == Command::InstructionPackage::Object){
            ObjectRasterConfig& rasterConfig = reinterpret_cast<Command::ObjectPackage::Payload*>(package_payload)->config;
            //ImGui::SameLine();
            if(ImGui::TreeNode("object config")){
                ImguiExtension::Imgui(rasterConfig);
                ImGui::TreePop();
            }
        }
    }

    void InstructionPackage_NG::SetPackageType(Command::InstructionPackage::Type pkg_type){
        packageType = pkg_type;
        if(previous_package_type != pkg_type){
            if(package_payload != nullptr){
                switch(previous_package_type){
                    case Command::InstructionPackage::Base: 
                        break;
                    case Command::InstructionPackage:: Object:
                        delete reinterpret_cast<Command::ObjectPackage::Payload*>(package_payload);
                        break;
                    default: EWE_UNREACHABLE;
                }
            }

            switch(pkg_type){
                case Command::InstructionPackage::Base:
                    acceptable_add_instructions = std::vector<Inst::Type>{
                        Command::InstructionPackage::allowed_instructions.begin(), 
                        Command::InstructionPackage::allowed_instructions.end()
                    };
                    explorer.acceptable_extensions = Global::assetManager->instPkg.files.acceptable_extensions;
                    break;
                case Command::InstructionPackage::Object:
                    acceptable_add_instructions = std::vector<Inst::Type>{
                        Command::ObjectPackage::allowed_instructions.begin(), 
                        Command::ObjectPackage::allowed_instructions.end()
                    };
                    package_payload = new Command::ObjectPackage::Payload();
                    explorer.acceptable_extensions = Global::assetManager->objPkg.files.acceptable_extensions;
                    break;
                default: break;
            }
        }
        previous_package_type = packageType;
    }

    void InstructionPackage_NG::RenderNodes() {
        if(previous_package_type != packageType){
            SetPackageType(packageType);
        }

        ImNodes::EWE::Editor::RenderNodes();
        Inst::Type* inst_type;
        if(DragDropPtr::Target(inst_type)) {
            auto& added_node = CreateRGNode(*inst_type);
            auto temp_mouse_pos =ImGui::GetIO().MousePos;
            added_node.pos = temp_mouse_pos;// - ImNodes::EditorContextGetPanning();// - (temp_min - window_pos);
        }
    }

    ImNodes::EWE::Node& InstructionPackage_NG::CreateRGNode(Inst::Type iType) {
        auto& added_node = AddNode();
        auto* node_payload = new InstNodePayload();
        node_payload->type = InstNodePayload::Instruction;
        node_payload->iType = iType;

        added_node.payload = node_payload;
        added_node.pos = menu_pos;
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{0.f, 0.5f}, .payload{nullptr}});
        added_node.pins.emplace_back(ImNodes::EWE::Pin{.local_pos{1.f, 0.5f}, .payload{nullptr}});

        return added_node;
    }

    std::vector<Inst::Type> InstructionPackage_NG::CollectInstructionsUpTo(ImNodes::EWE::Node* limit_node) const{
        std::vector<Inst::Type> ret{};
        
        ImNodes::EWE::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr
        if(headNode->pins[0].payload != nullptr){
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);
            PrintNode(*current_node);
            ret.push_back(GetInstructionFromNode(*current_node));
            if(current_node == limit_node){
                //Logger::Print<Logger::Debug>("early early return - %s\n", __FUNCTION__);
                return ret;
            }

            if(limit_node == nullptr){
                while(current_node->pins[1].payload != nullptr){
                    PrintNode(*current_node);

                    current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
                    auto const current_inst = GetInstructionFromNode(*current_node);
                    ret.push_back(current_inst);
                }
            }
            else{
                while(current_node->pins[1].payload != nullptr){
                    current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
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

    void InstructionPackage_NG::InsertNodeToParamPool(ImNodes::EWE::Node* inserted_node){
        UpdateNodeOffsets();
        auto* node_payload = reinterpret_cast<InstNodePayload*>(inserted_node->payload);
        if(node_payload->distanceFromHead >= 0){
            paramPool.Insert(static_cast<std::size_t>(node_payload->distanceFromHead), node_payload->iType);
        }
    }

    void InstructionPackage_NG::UpdateNodeOffsets() {

        InstNodePayload* current_payload = nullptr;
        for(auto& node : nodes){
            current_payload = reinterpret_cast<InstNodePayload*>(node.payload);
            current_payload->distanceFromHead = -1;
        }
        
        ImNodes::EWE::Node* current_node = nullptr;
        //the pin payload is going to be a pointer to the node, unless nullptr

        std::size_t distance = 0;
        if(headNode->pins[0].payload != nullptr){
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);
            if(current_node == nullptr){
                //Logger::Print<Logger::Debug>("early early return - %s\n", __FUNCTION__);
                return;
            }
            current_payload = reinterpret_cast<InstNodePayload*>(current_node->payload);
            current_payload->distanceFromHead = distance++;
            //PrintNode(*current_node);

            while(current_node->pins[1].payload != nullptr){
                PrintNode(*current_node);

                current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
                current_payload = reinterpret_cast<InstNodePayload*>(current_node->payload);
                current_payload->distanceFromHead = distance++;
            }
            
        }
    }

    void InstructionPackage_NG::OpenAddMenu() {
        add_menu_is_open = true;
    }

    bool InstructionPackage_NG::InsertLink(ImNodes::EWE::Node& added_node, ImNodes::EWE::Node* otherNode){
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
        added_node.pins[0].payload = existing_link->start.node;
        added_node.pins[1].payload = existing_link->end.node;
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

    bool InstructionPackage_NG::AddInstructionButton(Inst::Type itype){
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

    bool InstructionPackage_NG::RenderAddMenu() {
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

    void InstructionPackage_NG::LinkEmptyDrop(ImNodes::EWE::Node& src_node, ImNodes::EWE::PinOffset pin_offset) {
        //auto const& instructions = CollectInstructionsUpTo(&src_node);

        menu_pos = ImGui::GetMousePos();
        add_menu_is_open = true;
        link_empty_drop.node = &src_node;
        link_empty_drop.offset = pin_offset;
    }
    void InstructionPackage_NG::LinkCreated(ImNodes::EWE::NodePair& link) {
        if(link.start.node->pins[link.start.offset].payload != nullptr 
            || link.end.node->pins[link.end.offset].payload != nullptr
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
        link.start.node->pins[link.start.offset].payload = link.end.node;
        link.end.node->pins[link.end.offset].payload = link.start.node;
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
    void InstructionPackage_NG::LinkDestroyed(ImNodes::EWE::NodePair& link) {
        Logger::Print<Logger::Debug>("ipng::link destroyed\n");
        
        link.start.node->pins[link.start.offset].payload = nullptr;
        link.end.node->pins[link.end.offset].payload = nullptr;
        auto* end_payload = reinterpret_cast<InstNodePayload*>(link.end.node->payload);
        //auto* start_payload = reinterpret_cast<InstNodePayload*>(link.start.node->payload); 
        if(end_payload->distanceFromHead >= 0){
            Logger::Print<Logger::Debug>("ipng : erase necessary\n");
            paramPool.ShrinkToSize(end_payload->distanceFromHead);
            UpdateNodeOffsets();
        }
        ImNodes::EWE::Editor::LinkDestroyed(link);
    }

    void InstructionPackage_NG::RenderNode(ImNodes::EWE::Node& node) {
        auto node_payload = reinterpret_cast<InstNodePayload*>(node.payload);

        ImNodes::BeginNodeTitleBar();

        if(node_payload->type == InstNodePayload::Type::Head){
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "head node");
            if(changing_package_type_allowed){
                ImGui::SetNextItemWidth(150.f);
                auto prev_pkg_type = packageType;
                Reflect::Enum::Imgui_Combo_Selectable("package type", packageType);
                if(prev_pkg_type != packageType){
                    if(package_payload != nullptr){
                        switch(prev_pkg_type){
                            case Command::InstructionPackage::Object: 
                                delete reinterpret_cast<Command::ObjectPackage::Payload*>(package_payload); 
                                break;
                            //case Command::InstructionPackage::Base: EWE_UNREACHABLE;
                            default:EWE_UNREACHABLE;
                        }
                    }
                    switch(packageType){
                        case Command::InstructionPackage::Object: {
                            auto* temp_payload = new Command::ObjectPackage::Payload();
                            temp_payload->shaders.fill(nullptr);
                            package_payload = temp_payload;
                            break;
                        }
                        default: break;

                    }
                }
            }
            ImNodes::EndNodeTitleBar();
            ImGui::Text(" "); //nodes need some abritrary filler
            return;
        }
        else if(node_payload->type == InstNodePayload::Package){
            //need to peek contents
            //if 
            
        }
        else{
            ImGui::Text("%s[%d]", Reflect::Enum::ToString(node_payload->iType).data(), node.id);
        }
        ImNodes::EndNodeTitleBar();

        //ImGui::DebugLog("");
        ImGui::Text("distance from head : %d", node_payload->distanceFromHead); //empty text just to populate this
        const std::size_t param_size = Inst::GetParamSize(node_payload->iType);
        ImGui::Text("param size : %zu", param_size);
        if(node_payload->distanceFromHead >= 0){
            if(param_size > 0){
                const std::size_t pack_index = paramPool.GetPackIndex(node_payload->distanceFromHead);
                ImguiExpandInstruction(reinterpret_cast<void*>(paramPool.param_data[pack_index].data[0]), paramPool.instructions[node_payload->distanceFromHead]);
            }
        }
    }

    void InstructionPackage_NG::RenderPin(ImNodes::EWE::Node& node, ImNodes::EWE::PinOffset pin_index) {
        auto& pin = node.pins[pin_index];

        if (ImNodes::BeginPinAttribute(node.id + pin_index + 1, pin.local_pos)) {
            //ImGui::Text("pin");
            
        }
        if(node.pins[pin_index].payload != nullptr){
            ImGui::Text("payload id : %d", reinterpret_cast<ImNodes::EWE::Node*>(node.pins[pin_index].payload)->id);
        }
        ImNodes::EndPinAttribute();
    }

    bool InstructionPackage_NG::SaveFunc() {
        explorer.enabled = save_open;
        explorer.state = ExplorerContext::State::Save;
        if(ImGui::Begin("file save")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path saved_path = *explorer.selected_file;
                const std::filesystem::path proximate = std::filesystem::proximate(saved_path, Global::assetManager->root_directory);
                Asset::WriteToInstPkgFile(paramPool, package_payload, packageType, proximate);
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

    void InstructionPackage_NG::RecreateLinks(){
        links.clear();

        if(headNode->pins[0].payload == nullptr){
            return;
        }
        ImNodes::EWE::Node* current_node = reinterpret_cast<ImNodes::EWE::Node*>(headNode->pins[0].payload);

        links.emplace_back(
            ImNodes::EWE::NodePair{
                .start = ImNodes::EWE::NodeAndPin{
                    .node = headNode,
                    .offset = 0
                },
                .end = ImNodes::EWE::NodeAndPin{
                    .node = current_node,
                    .offset = 0
                }
            }
        );
        while(current_node->pins[1].payload != nullptr){
            links.emplace_back(
                ImNodes::EWE::NodePair{
                    .start = ImNodes::EWE::NodeAndPin{
                        .node = current_node,
                        .offset = 1
                    },
                    .end = ImNodes::EWE::NodeAndPin{
                        .node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload),
                        .offset = 0
                    }
                }
            );
            current_node = reinterpret_cast<ImNodes::EWE::Node*>(current_node->pins[1].payload);
        }

        UpdateNodeOffsets();
    }

    void InstructionPackage_NG::InitFromFile(Command::ParamPool const& pp, void* payload, Command::InstructionPackage::Type pkg_type){
        nodes.Clear();
        links.clear();


        try{
            paramPool = pp;
        }
        catch(std::exception& e){
            Logger::Print<Logger::Error>("failed to copy operator param pool : %s\n", e.what());
        }

        headNode = CreateHeadNode();
        if(pp.instructions.size() == 0){
            return;
        }
        auto& init_node = CreateRGNode(paramPool.instructions[0]);
        reinterpret_cast<InstNodePayload*>(init_node.payload)->distanceFromHead = 0;
        headNode->pins[0].payload = &init_node;
        init_node.pos.x = headNode->pos.x + 100.f;
        init_node.pos.y = headNode->pos.y + 30.f;

        auto* lastNode = &init_node;
        //InstNodePayload* last_payload = lastNode->payload;
        init_node.pins[0].payload = headNode;

        for(std::size_t i = 1; i < pp.instructions.size(); i++){
            auto& node = CreateRGNode(pp.instructions[i]);
            node.pos = lastNode->pos;
            node.pos.x += 100.f;
            node.pos.y += 30.f;

            node.pins[0].payload = lastNode;
            reinterpret_cast<InstNodePayload*>(node.payload)->distanceFromHead = reinterpret_cast<InstNodePayload*>(lastNode->payload)->distanceFromHead + 1;
            lastNode->pins[1].payload = &node;

            lastNode = &node;
        }

        nodes.ShrinkToFit();

        SetPackageType(pkg_type);
        RecreateLinks();
    }
    void InstructionPackage_NG::InitFromObject(Command::InstructionPackage& pkg){
        packageType = Command::InstructionPackage::Base;
        switch(pkg.type){
            case Command::InstructionPackage::Base: 
                explorer.acceptable_extensions = Global::assetManager->instPkg.files.acceptable_extensions;
                InitFromFile(pkg.paramPool, nullptr, pkg.type);
                break;
            case Command::InstructionPackage::Object:
                explorer.acceptable_extensions = Global::assetManager->objPkg.files.acceptable_extensions;
                InitFromFile(pkg.paramPool, &reinterpret_cast<Command::ObjectPackage&>(pkg).payload, pkg.type);
                break;
            default: 
                Logger::Print<Logger::Warning>("attempting to init node graph from invalid pkg type\n"); 
                break;
        }
        return;
    }

    bool InstructionPackage_NG::LoadFunc() {
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;
                //put the record somewhere
                
                const auto proximate_path = std::filesystem::proximate(load_path, Global::assetManager->root_directory);

                switch(packageType){
                    case Command::InstructionPackage::Base:{
                        auto& pkg = EWE::Global::assetManager->instPkg.Get(proximate_path);
                        InitFromObject(pkg);
                        Logger::Print<Logger::Debug>("loaded pkg instructions size - %zu : %zu\n", pkg.paramPool.instructions.size(), paramPool.instructions.size());
                        break;
                    }
                    case Command::InstructionPackage::Object:{
                        auto& pkg = EWE::Global::assetManager->objPkg.Get(proximate_path);
                        InitFromObject(pkg);
                        Logger::Print<Logger::Debug>("loaded pkg instructions size - %zu : %zu\n", pkg.paramPool.instructions.size(), paramPool.instructions.size());
                        break;
                    }
                    default: EWE_UNREACHABLE;
                }

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