#ifndef SGP_FPS_H
#define SGP_FPS_H

struct SDL_Renderer;
class SGPVObject;

namespace FPS
{
	using GameLoopFunc_t  = void (*)();
	using RenderPresent_t = void (*)(SDL_Renderer *);

	extern GameLoopFunc_t  GameLoopPtr;
	extern RenderPresent_t RenderPresentPtr;

	void Init(GameLoopFunc_t gameLoop, SGPVObject * FPSDisplayFont);
	void ToggleOnOff();
}

#endif
