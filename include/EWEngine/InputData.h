#pragma once

#include "EightWinds/Window.h"

#include "LAB/Vector.h"

#include <functional>
#include <array>

namespace EWE{
    namespace Input{
        struct Button{
            enum Status : uint8_t{
                Up,
                Pressed,
                Down,
                Released
            };
            Status current = Released;
            void Update(bool pressed) {
                if (pressed) {
                    switch (current) {
                        case Up: 
                        case Released: current = Pressed; break;
                        case Down:
                        case Pressed: current = Down; break;
                    }
                }
                else{
                    switch (current) {
                        case Up: break;
                        case Released: current = Up; break;
                        case Down: 
                        case Pressed: current = Released; break;
                    }
                }
            }
        };

        struct Mouse{
            lab::vec2 current_pos;
            lab::vec2 frame_pos_diff;
            
            int scroll_wheel_diff;

            std::array<bool, 32> buttons; //whats the significance of 32?

            void Update(int button, int action, int mods) {
                if(action == GLFW_PRESS){
                    buttons[button] = true;
                }
                else{
                    buttons[button] = false;
                }
            }

            void UpdatePosition(GLFWwindow* window){
                double x;
                double y;
                glfwGetCursorPos(window, &x, &y);

                frame_pos_diff.x = static_cast<float>(x) - current_pos.x;
                frame_pos_diff.y = static_cast<float>(y) - current_pos.y;
                current_pos.x = static_cast<float>(x);
                current_pos.y = static_cast<float>(y);
            }

            void TakeCallbackControl() {
                buttonCallback = [&](int button, int action, int mods) {
                    this->Update(button, action, mods);
                };
            }

            static std::function<void(int button, int action, int mods)> buttonCallback;

            static void MouseCallback(GLFWwindow* window, int button, int action, int mods);
        };

        struct Keyboard{
            Button::Status buttons[256]; //256? magic?

            void Update(int key, int scancode, int action, int mods) {
                if(action == GLFW_PRESS){
                    buttons[key] = Button::Status::Down;
                }
                else{
                    buttons[key] = Button::Status::Up;
                }
            }

            void TakeCallbackControl() {
                keyCallback = [&](int key, int scancode, int action, int mods) {
                    this->Update(key, scancode, action, mods);
                };
            }

            static std::function<void(int key, int scancode, int action, int mods)> keyCallback;

            static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        };
    } //namespace Input
} //namespace EWE