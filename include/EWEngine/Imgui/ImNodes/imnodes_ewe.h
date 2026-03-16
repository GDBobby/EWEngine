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
            ImVec2 pos;
            bool snapToGrid = false;

            void* payload = nullptr;
            std::vector<Pin*> pins{}; //i dont want to move the pins from here, hence the pointer

            //negative is doesn't contain
            int CheckPin(PinID globalPinID); 
        };
        struct Link{
            LinkID id;
            PinID start_attr;
            PinID end_attr;
        };

        struct NodePair {
            LinkID id;
            Node* start;
            PinOffset start_offset;
            Node* end;
            PinOffset end_offset;
        };

        struct Editor {
            [[nodiscard]] explicit Editor();
            [[nodiscard]] explicit Editor(bool saveFunc, bool loadFunc);
            std::string name;

            ImNodes::ImNodesEditorContext* context;

            ::EWE::Hive<Node, 32> nodes;
            std::vector<NodePair> links;

            Node& AddNode() {
                return nodes.AddElement();
            }

            void CreateLink(NodePair const& nodePair);

            void RenderNodes();

            virtual void ImGuiNodeDebugPrint(Node& node) const;

            bool attemptingToSave = false;
            bool submissionReady = false;

            ImVec2 node_editor_window_pos{0.f, 0.f};
            ImVec2 node_editor_window_size{0.f, 0.f};

            /*
                i want global controls for the node graph
                something like
                A adds a node, Ctrl+S saves, Ctrl+Shift+S opens up the folder explorer for saving or whatever
            
            */

            virtual void RenderNode(Node&) {}
            virtual void RenderPin(Node&, PinOffset) {}
            //do I want IDs controlled externally or in here? 
            // if I control them internally, there's a lot of assumptions I can't make
            virtual void InitNode(Node&){}

            virtual void LinkEmptyDrop(Node&, PinOffset) {}

            virtual void LinkCreated(NodePair&) {}
            virtual void LinkDestroyed(NodePair& link) {
                const auto ptr_diff = &link - links.data();
                links.erase(links.begin() + ptr_diff);
            }

            virtual void DestroyPressedOnNode(Node& node) {
                Node* node_ptr = &node;
                for (auto& link : links){
                    if(link.start == node_ptr || link.end == node_ptr){
                        LinkDestroyed(link);
                        break;
                    }
                }
                nodes.DestroyElement(node_ptr);

            }
            virtual void DestroyPressedOnPin(Node& node, PinOffset) {
                //i dont want to default this, pins beign deleted is abnormal
            }
            virtual void DestroyPressedOnLink(NodePair& link) {
                LinkDestroyed(link);
            }

            bool add_menu_is_open = false;
            ImVec2 menu_pos;
            virtual void OpenAddMenu() {add_menu_is_open = true;}
            //returning true closes the menu
            virtual bool RenderAddMenu() {return true;}

            virtual void RenderEditorTitle();

            bool has_save_func = false;
            bool has_load_func = false;
            bool save_open = false;
            bool load_open = false;
            //if true is returned, the func will no longer be called
            virtual bool SaveFunc() {return true;}
            virtual bool LoadFunc() {return true;}

        };
    } //namepsacde EWE
} //namespace ImNodes