#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"

class ENGINE_API CMotionDef;

//размер очереди считается бесконечность
//заканчиваем стрельбу, только, если кончились патроны
constexpr auto WEAPON_ININITE_QUEUE = -1;

class CBinocularsVision;

class CWeaponMagazined: public CWeapon
{
	friend class CWeaponScript;
private:
	typedef CWeapon inherited;
protected:
	// Media :: sounds
	HUD_SOUND		sndShow;
	HUD_SOUND		sndHide;
	HUD_SOUND		sndShot;
	HUD_SOUND		sndEmptyClick;
	HUD_SOUND		sndReload, sndReloadPartly;
	bool			sndReloadPartlyExist{};
	HUD_SOUND		sndFireModes;
	HUD_SOUND		sndZoomChange;
	//
	HUD_SOUND		sndShutter, sndUnload;
	HUD_SOUND		sndZoomIn;
	HUD_SOUND		sndZoomOut;
	HUD_SOUND		sndNightVisionOn;
	HUD_SOUND		sndNightVisionOff;
	HUD_SOUND		sndNightVisionIdle;
	HUD_SOUND		sndNightVisionBroken;

	HUD_SOUND		sndLaserSwitch;
	HUD_SOUND		sndFlashlightSwitch;
	HUD_SOUND		sndAimStart, sndAimEnd;

	//звук текущего выстрела
	HUD_SOUND*		m_pSndShotCurrent{};

	virtual void	StopHUDSounds		();

	//дополнительная информация о глушителе
	LPCSTR			m_sSilencerFlameParticles{};
	LPCSTR			m_sSilencerSmokeParticles{};
	HUD_SOUND		sndSilencerShot;

	ESoundTypes		m_eSoundShow;
	ESoundTypes		m_eSoundHide;
	ESoundTypes		m_eSoundShot;
	ESoundTypes		m_eSoundEmptyClick;
	ESoundTypes		m_eSoundReload;
	ESoundTypes		m_eSoundShutter;

	// General
	//кадр момента пересчета UpdateSounds
	u32				dwUpdateSounds_Frame{};
protected:
	virtual void	OnMagazineEmpty	();

	virtual void	switch2_Idle	();
	virtual void	switch2_Fire	();
	virtual void	switch2_Fire2	(){}
	virtual void	switch2_Empty	();
	virtual void	switch2_Reload	();
	virtual void	switch2_Hiding	();
	virtual void	switch2_Hidden	();
	virtual void	switch2_Showing	();
	
	virtual void	OnShot			();	
	
	virtual void	OnEmptyClick	();

	virtual void	OnAnimationEnd	(u32 state);
	virtual void	OnStateSwitch	(u32 S, u32 oldState);

	virtual void	UpdateSounds	();

	bool			TryReload		();

protected:
	virtual void	ReloadMagazine		();
			void	ApplySilencerParams	();
			void	ApplySilencerKoeffs	();
			void	ApplyStockParams	();
			void	ApplyForendParams	();
			void	DetachWForend		();

	virtual void	state_Fire		(float dt);
	virtual void	state_MagEmpty	(float dt);
	virtual void	state_Misfire	(float dt);
public:
					CWeaponMagazined	(LPCSTR name="AK74",ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazined	();

	virtual void	Load			(LPCSTR section);
	virtual CWeaponMagazined*cast_weapon_magazined	()		 {return this;}

	virtual void	SetDefaults		();
	virtual void	FireStart		();
	virtual void	FireEnd			();
	virtual void	Reload			();
	

	virtual	void	UpdateCL		();
	virtual BOOL	net_Spawn		(CSE_Abstract* DC);
	virtual void	net_Destroy		();
	virtual void	net_Export		( CSE_Abstract* E );

	virtual void	OnH_A_Chield	();

	virtual bool	Attach			(PIItem pIItem, bool b_send_event);
	virtual bool	Detach			(const char* item_section_name, bool b_spawn_item, float item_condition = 1.f);
	virtual bool	CanAttach		(PIItem pIItem);
	virtual bool	CanDetach		(const char* item_section_name);

	virtual void	InitAddons();
//	virtual void	InitZoomParams	(LPCSTR section, bool useTexture);

	virtual bool	Action			(s32 cmd, u32 flags);
	virtual void	UnloadMagazine	(bool spawn_ammo = true);

	virtual void	GetBriefInfo				(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count);


	//////////////////////////////////////////////
	// для стрельбы очередями или одиночными
	//////////////////////////////////////////////
public:
	virtual bool	SwitchMode				();
	virtual bool	SingleShotMode			()			{return 1 == m_iQueueSize;}
	virtual void	SetQueueSize			(int size);
	IC		int		GetQueueSize			() const	{return m_iQueueSize;};
	virtual bool	StopedAfterQueueFired	()			{return m_bStopedAfterQueueFired; }
	virtual void	StopedAfterQueueFired	(bool value){m_bStopedAfterQueueFired = value; }

protected:
	//максимальный размер очереди, которой можно стрельнуть
	int				m_iQueueSize = WEAPON_ININITE_QUEUE;
	//количество реально выстреляных патронов
	int				m_iShotNum{};
	//  [7/20/2005]
	//после какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-зи Абакана)
	int				m_iShootEffectorStart{};
	Fvector			m_vStartPos{}, m_vStartDir{};
	//  [7/20/2005]
	//флаг того, что мы остановились после того как выстреляли
	//ровно столько патронов, сколько было задано в m_iQueueSize
	bool			m_bStopedAfterQueueFired{};
	//флаг того, что хотя бы один выстрел мы должны сделать
	//(даже если очень быстро нажали на курок и вызвалось FireEnd)
	bool			m_bFireSingleShot{};
	//режимы стрельбы
	bool			m_bHasDifferentFireModes{};
	xr_vector<int>	m_aFireModes;
	int				m_iCurFireMode{};
	string16		m_sCurFireMode;
	int				m_iPrefferedFireMode{ -1 };
	u32				m_fire_zoomout_time = u32(-1);

	//у оружия есть патронник
	bool			m_bHasChamber{ true };
	//присоединён ли магазин
	bool			m_bIsMagazineAttached{ true };

	//переменная блокирует использование
	//только разных типов патронов
	bool			m_bLockType{};

	const char*		m_str_count_tmpl{};

	bool			m_bShowAmmoCounter{};

	// режим выделения рамкой противников
protected:
	bool					m_bVision{};
	CBinocularsVision*		m_binoc_vision{};

	//////////////////////////////////////////////
	// режим приближения
	//////////////////////////////////////////////
public:
	virtual void	OnZoomIn			();
	virtual void	OnZoomOut			(bool = false);
	virtual void	OnZoomChanged		();
	virtual	void	OnNextFireMode		();
	virtual	void	OnPrevFireMode		();
	virtual bool	HasFireModes		() { return m_bHasDifferentFireModes; };
	virtual	int		GetCurrentFireMode	() const { return m_bHasDifferentFireModes ? m_aFireModes[m_iCurFireMode] : 1; };
	virtual LPCSTR	GetCurrentFireModeStr	() {return m_sCurFireMode;};
	virtual shared_str	GetAmmoElapsedStr	() const;

	virtual bool	ShowAmmoCounter		() const;

	virtual void	save				(NET_Packet &output_packet);
	virtual void	load				(IReader &input_packet);

protected:
	virtual bool	AllowFireWhileWorking() {return false;}

	//виртуальные функции для проигрывания анимации HUD
	virtual void	PlayAnimShow		();
	virtual void	PlayAnimHide		();
	virtual void	PlayAnimReload		();
	virtual void	PlayAnimIdle		();

private:
	string64 guns_aim_anm{};
protected:
	const char* GetAnimAimName();

	virtual void	PlayAnimAim			();
	virtual void	PlayAnimShoot		();
	virtual void	PlayReloadSound		();

	virtual	int		ShotsFired			() { return m_iShotNum; }
	virtual float	GetWeaponDeterioration	()  const;
	//для хранения состояния присоединённого прицела
	float			m_fAttachedScopeCondition{ 1.f };
	//для хранения состояния присоединённого гранатомёта
	float			m_fAttachedGrenadeLauncherCondition{ 1.f };
	//для хранения состояния присоединённого глушителя
	float			m_fAttachedSilencerCondition{ 1.f };
	//износ самого глушителя при стрельбе
	virtual float	GetSilencerDeterioration();
	virtual void	DeteriorateSilencerAttachable(float);

	virtual void	OnDrawUI();
	virtual void	net_Relcase(CObject *object);

//  bool ScopeRespawn( PIItem );
public:
	// Real Wolf.20.01.15
	virtual					bool TryToGetAmmo(u32);
	//
	LPCSTR					m_NightVisionSect{};
	bool					m_bNightVisionOn{};
	void					SwitchNightVision(bool, bool);
	void					UpdateSwitchNightVision();
	void					SwitchNightVision();

	virtual float			GetConditionMisfireProbability() const;

	//оружие использует отъёмный магазин
	virtual bool	HasDetachableMagazine	(bool = false) const;
	virtual bool	IsMagazineAttached		() const;
	//у оружия есть патронник
	virtual bool	HasChamber				() { return m_bHasChamber; };
	//разрядить кол-во патронов
	virtual void	UnloadAmmo(int unload_count, bool spawn_ammo = true, bool detach_magazine = false);
	//
	int				GetMagazineCount() const;
	//
	virtual bool	IsSingleReloading	();
	virtual bool	AmmoTypeIsMagazine	(u32 type) const;
	virtual LPCSTR	GetMagazineEmptySect(bool = false) const;
	virtual LPCSTR	GetCurrentMagazine_ShortName(bool = false);

	//действие передёргивания затвора
	virtual void	ShutterAction();
	//сохранение типа патрона в патроннике при смешанной зарядке
	virtual void	HandleCartridgeInChamber();

	virtual float	Weight() const;
	virtual float	Volume() const;

	virtual void	LoadZoomParams		(LPCSTR);
	virtual void	LoadLaserParams		(LPCSTR);
	virtual void	LoadFlashlightParams(LPCSTR);
	//
	LPCSTR			binoc_vision_sect{};
	//
	virtual void	ChangeAttachedSilencerCondition			(float);
	virtual void	ChangeAttachedScopeCondition			(float);
	virtual void	ChangeAttachedGrenadeLauncherCondition	(float);

	virtual bool	IsSilencerBroken		() const;
	virtual bool	IsScopeBroken			() const;
	virtual bool	IsGrenadeLauncherBroken	() const;

	virtual	void	Hit(SHit* pHDS);
	virtual bool	IsHitToAddon(SHit* pHDS);

	virtual bool	IsNightVisionEnabled	() const { return m_bNightVisionEnabled; };
	virtual bool	IsVisionPresent			() const { return m_bVision; };

	virtual void	SwitchLaser				(bool on);
	virtual void	SwitchFlashlight		(bool on);

	virtual void	UnloadWeaponFull		(bool = false);

	virtual void	UnloadAndDetachAllAddons(bool = false);
	virtual void	PrepairItem				();
protected:
	bool			m_bNightVisionEnabled{};
	bool			m_bNightVisionSwitchedOn{ true };
	//передёргивание затвора
	virtual void	OnShutter();
	virtual void	switch2_Shutter();
	virtual void	PlayAnimShutter();
	virtual void	PlayAnimFiremodes();
	virtual void	switch2_Unload();
	virtual void	PlayAnimUnload();
	virtual void	OnKick();
	virtual void	switch2_Kick();
};
