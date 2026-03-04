#pragma once

#include "EightWinds/Data/Hive.h"

#include "EWEngine/Imgui/ImNodes/imnodes.h"

//#include "LAB/Vector.h"

#include <vector>
#include <functional>
#include <cstdint>

namespace ImNodes{
    //struct ImNodes::ImNodesEditorContext; //forward declare

    namespace EWE{
        using NodeID = int;
        using PinID = int;
        using PinOffset = uint16_t; //this is the pin id within the node
        using LinkID = int;
        
        struct Pin{
            ImVec2 local_pos;
            void* payload;
        };
        struct Node{
            NodeID id; //changed every render, internal usage only
            ImVec2 nodePos;
            bool snapToGrid = false;

            void* payload = nullptr;
            std::vector<Pin*> pins{}; //i dont want to move the pins from here, hence the pointer
        };
        struct Link{
            LinkID id;
            PinID start_attr;
            PinID end_attr;
        };

        struct NodePair {
            Node* start;
            PinOffset start_offset;
            Node* end;
            PinOffset end_offset;
        };

        struct Editor {
            [[nodiscard]] explicit Editor();
            std::string name;

            ImNodes::ImNodesEditorContext* context;
            ::EWE::Hive<Node, 32> nodes;
            std::vector<NodePair> links;

            Node& AddNode() {
                return nodes.AddElement();
            }

            void CreateLink(NodePair const& nodePair);

            void RenderNodes();

            bool attemptingToSave = false;
            bool submissionReady = false;

            ImVec2 node_editor_window_pos{0.f, 0.f};
            ImVec2 node_editor_window_size{0.f, 0.f};

            /*
                i want global controls for the node graph
                something like
                A adds a node, Ctrl+S saves, Ctrl+Shift+S opens up the folder explorer for saving or whatever
            
            */

            std::function<void(Node&)> node_title_renderer = nullptr;
            std::function<void(Node&)> node_renderer = nullptr;
            std::function<void(Node&, PinOffset pin_index)> pin_renderer = nullptr;
            std::function<void()> save = nullptr;
            //do I want IDs controlled externally or in here? 
            // if I control them internally, there's a lot of assumptions I can't make
            std::function<void(Node&)> init_node = nullptr; 

            //returning true closes the menu
            bool add_menu_is_open = false;
            ImVec2 menu_pos;
            std::function<bool(ImVec2)> render_add_menu = nullptr;

            std::function<void()> title_extension = nullptr;
        };
    } //namepsacde EWE
} //namespace ImNodes