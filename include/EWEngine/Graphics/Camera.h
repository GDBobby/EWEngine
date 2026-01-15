#pragma once

#include "EightWinds/Buffer.h"
#include "EightWinds/Data/PerFlight.h"

#include "EWEngine/Global.h"

#include <LAB/Camera.h>

#include <vector>
#include <array>


namespace EWE {

	struct alignas(16) CameraBufferObject {
		lab::mat4 projView;
		lab::mat4 proj;//ection
		lab::mat4 view; //i dont think i need view and proj, one or the other
		lab::vec4 frustumPlanes[6];
		lab::vec3 cameraPos{ 1.f };
	};

	class Camera {
	public:
		[[nodiscard]] explicit Camera();

		void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void SetPerspectiveProjection(float fovy, float aspect, float near, float far);

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void ViewDirection(const lab::vec3 position, const lab::vec3 direction, const lab::vec3 up = lab::vec3{ 0.0f,1.0f, 0.0f }) {
			lab::ViewDirection<CoordinateSystem>(data.view, position, direction);
			data.projView = data.proj * data.view;
			data.cameraPos = position;
		}

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void ViewRotation(const lab::vec3 position, const lab::vec3 rotation) {
			lab::ViewRotation<CoordinateSystem>(data.view, position, rotation);
			data.projView = data.proj * data.view;
			data.cameraPos = position;
		}

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void UpdateCamera() {
			if (dataHasBeenUpdated == 0) {
				return;
			}
			dataHasBeenUpdated--;
			const lab::vec3 forward = lab::Normalized(target - position);
			ViewDirection<CoordinateSystem>(position, forward, cameraUp);
			memcpy(buffers[Global::frameIndex].mapped, &data, sizeof(CameraBufferObject));
			buffers[Global::frameIndex].Flush();
		}

		lab::mat4 const& GetProjection() const { return data.proj; }
		lab::mat4 const& GetView() const { return data.view; }

		void UpdateBothBuffers();
		void UpdateBuffer(uint8_t frameIndex);
		void UpdateBuffer();

		void SetBuffers();
		void UpdateViewData(lab::vec3 const& position, lab::vec3 const& target, lab::vec3 const& cameraUp = lab::vec3{ 0.f,1.f,0.f });

		std::array<lab::vec4, 6> GetFrustumPlanes();
		std::array<lab::vec4, 6> GetConservativeFrustumPlanes(const lab::vec3 position, const lab::vec3 rotation);

		lab::mat4 conservativeProjection;

		lab::vec3 position;
		lab::vec3 target;
		lab::vec3 cameraUp{ 0.f, 1.f, 0.f };
	private:
		CameraBufferObject data;
		PerFlight<Buffer> buffers;

		uint8_t dataHasBeenUpdated = 0;

	};
}