#pragma once

#include <cstdint>
#include <concepts>

#include "EightWinds/Logger.h"

namespace EWE{
	template<typename T>
	struct Color_RGB {
		T r;
		T g;
		T b;

		operator lab::Vector<T, 3>() const {
			return lab::Vector<T, 3>{
				.x = r,
				.y = g,
				.z = b
			};
		}
	};
	template<typename T>
	struct Color_RGBA {
		T r;
		T g;
		T b;
		T a;

		operator lab::Vector<T, 3>() const {
			return lab::Vector<T, 3>{
				.x = r,
				.y = g,
				.z = b,
				.w = a
			};
		}
	};

	template<typename RetType, typename InputType>
	constexpr RetType ConvertColorType(InputType const input){
		if constexpr(std::is_same_v<InputType, float>){
			if constexpr(std::is_same_v<RetType, uint8_t>){
				return static_cast<uint8_t>(input * 255.f);
			}
			else{
				Log::Error("unsupported type\n");
			}
		}
		else if constexpr(std::is_same_v<InputType, uint8_t>){
			if constexpr(std::is_same_v<RetType, float>){
				return static_cast<float>(input) / 255.f;
			}
			else{
				Log::Error("unsupported type\n");
			}
		}
	}

	template<typename RetType, typename InputType>
	requires(!std::is_same_v<RetType, InputType>)
	constexpr Color_RGB<RetType> ConvertColor(Color_RGB<InputType> const& input){
		return Color_RGB<RetType>{
			.r = ConvertColorType<RetType, InputType>(input.r),
			.g = ConvertColorType<RetType, InputType>(input.g),
			.b = ConvertColorType<RetType, InputType>(input.b)
		};
	}
	template<typename RetType, typename InputType>
	requires(!std::is_same_v<RetType, InputType>)
	constexpr Color_RGBA<RetType> ConvertColor(Color_RGBA<InputType> const& input){
		return Color_RGB<RetType>{
			.r = ConvertColorType<RetType, InputType>(input.r),
			.g = ConvertColorType<RetType, InputType>(input.g),
			.b = ConvertColorType<RetType, InputType>(input.b),
			.a = ConvertCOlorType<RetType, InputType>(input.a)
		};
	}


}