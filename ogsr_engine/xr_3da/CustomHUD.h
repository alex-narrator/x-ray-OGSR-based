#pragma once

#include "iinputreceiver.h"

ENGINE_API extern Flags32		psHUD_Flags;

enum HUD_Flags: u32 {
	HUD_CROSSHAIR = 1 << 0,
	HUD_CROSSHAIR_DIST = 1 << 1,
	HUD_WEAPON = 1 << 2,
	HUD_INFO = 1 << 3,
	HUD_DRAW = 1 << 4,
	HUD_CROSSHAIR_RT = 1 << 5,
	HUD_WEAPON_RT = 1 << 6,
	HUD_CROSSHAIR_DYNAMIC = 1 << 7,
	HUD_CROSSHAIR_HARD = 1 << 8, // Усложненный режим прицела - оружие от бедра будет стрелять не по центру камеры, а по реальному направлению ствола
	HUD_CROSSHAIR_RT2 = 1 << 9,
	HUD_DRAW_RT = 1 << 10,
	HUD_CROSSHAIR_BUILD = 1 << 11, // старый стиль курсора
	//HUD_SMALL_FONT = 1 << 12, // использовать уменьшенный шрифт
	HUD_STOP_MISSILE_PLAYING = 1 << 13, //отключение анимаций подбрасывания для гранат и болта
	HUD_USE_LUMINOSITY = 1 << 14,
	HUD_TEXTURES_AUTORESIZE = 1 << 15,
};

class CUI;

class ENGINE_API CCustomHUD:
	public DLL_Pure,
	public IEventReceiver	
{
public:
					CCustomHUD				();
	virtual			~CCustomHUD				();

	virtual		void		Load					(){;}
	
	virtual		void		Render_First			(){;}
	virtual		void		Render_Last				(){;}
	virtual		void		Render_Actor_Shadow() = 0;	// added by KD
	
	virtual		void		OnFrame					(){;}
	virtual		void		OnEvent					(EVENT E, u64 P1, u64 P2){;}

	virtual IC	CUI*		GetUI					()=0;
	virtual void			OnScreenRatioChanged	()=0;
	virtual void			OnDisconnected			()=0;
	virtual void			OnConnected				()=0;
	virtual	void			RenderActiveItemUI() = 0;
	virtual	bool			RenderActiveItemUIQuery() = 0;
	virtual void			net_Relcase				(CObject *object) = 0;
};

extern ENGINE_API CCustomHUD* g_hud;

//элементы HUD выводятся по нажатию клавиш
enum EHudLaconicMode
{
	eHudLaconicOff,			//отключено
	eHudLaconicWarning,		//только warning-иконки
	eHudLaconicMotion		//иконка положения персонажа в качестве warning-иконки здоровья
};

extern EHudLaconicMode g_eHudLaconic; //элементы HUD выводятся по нажатию клавиш: 0 - отключено, 1 - только warning-иконки, 2 - иконка положения персонажа в качестве warning-иконки здоровья