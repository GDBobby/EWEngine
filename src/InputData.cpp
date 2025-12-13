#include "EWEngine/InputData.h"

namespace EWE {
    namespace Input {
        std::function<void(int button, int action, int mods)> Mouse::buttonCallback;

        void Mouse::MouseCallback(GLFWwindow* window, int button, int action, int mods) {
            buttonCallback(button, action, mods);
        }

        std::function<void(int key, int scancode, int action, int mods)> Keyboard::keyCallback;

        void Keyboard::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
            keyCallback(key, scancode, action, mods);
        }
    }
}