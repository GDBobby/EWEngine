#pragma once

#include <cstdint>
#include "LAB/Vector.h"

namespace EWE{
	namespace UI{
		constexpr lab::vec2 Position_ScreenToVulkan(int x, int y, float screen_width, float screen_height){
			
			//if input == 0,0, output == -1,1
			//if input == screen_width,screen_height, output = 1, -1
			return lab::vec2{
				((static_cast<float>(x) / screen_width) - 0.5f) * 2.f,
				((static_cast<float>(y) / screen_height) - 0.5f) * -2.f
			};
		}
		constexpr lab::vec2 Position_ScreenToVulkan(lab::ivec2 input, float screen_width, float screen_height){
			return Position_ScreenToVulkan(input.x, input.y, screen_width, screen_height);
		}
		constexpr lab::vec2 Position_Difference(int x, int y, float screen_width, float screen_height){
			const lab::vec2 raw = Position_ScreenToVulkan(x, y, screen_width, screen_height);
			return lab::vec2{
				raw.x + 1.f,
				raw.y - 1.f
			};
		}
		constexpr lab::vec2 Position_Difference(lab::ivec2 screen_diff, float screen_width, float screen_height){
			return Position_Difference(screen_diff.x, screen_diff.y, screen_width, screen_height);
		}
		constexpr lab::ivec2 Position_VulkanToScreen(float x, float y, float screen_width, float screen_height){
			return lab::ivec2{
				static_cast<int>((x + 1.0f) / 2.f * screen_width),
				static_cast<int>(((-y / 2.f) + 0.5f) * screen_height)
			};
		}
		constexpr lab::ivec2 Position_VulkanToScreen(lab::vec2 input, float screen_width, float screen_height){
			return Position_VulkanToScreen(input.x, input.y, screen_width, screen_height);	
		}
		
		constexpr lab::vec2 CoordWithinQuad(lab::ivec2 screen_pos, lab::vec2 quad_pos, lab::vec2 quad_scale, float screen_width, float screen_height){
			//top left corner
			//pos 0 with scale 1 means the quad covers the entire screen
			const lab::vec2 top_left_corner = quad_pos - quad_scale;
			const lab::vec2 bottom_right_corner = quad_pos + quad_scale;
			
			const lab::vec2 target_pos = Position_ScreenToVulkan(screen_pos, screen_width, screen_height);
			
			return lab::vec2{
				(target_pos.x - top_left_corner.x) / (quad_scale.x * 2.f),
				1.f - ((target_pos.y - top_left_corner.y) / (quad_scale.y * 2.f)) //the 1.f - Y means the top is 0.f, and bottom is 1.f
			};
		}
	} //namespace UI
} //namespace EWE