#pragma once

namespace EWE {

	typedef uint8_t SceneKey;

	class SceneBase {
	public:
		std::filesystem::path name;

		virtual ~SceneBase() = default;
		//this is called immediately after the last scene's exit, the loading screen will be running during this function
		virtual void Load() = 0;
		//this is called immediately after load, the loading screen will be running during this function
		//when this function is finished, the loading screen will cut
		virtual void Entry() = 0;

		//returns true if the window was resized, skips 1 render if it was resized
		virtual bool RenderUpdate(double dt) = 0;

		//the loading screen will be called before this function
		virtual void Exit() = 0;

	};
}