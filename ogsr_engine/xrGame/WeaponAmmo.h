#pragma once
#include "inventory_item_object.h"

class CCartridge 
{
public:
	CCartridge();
	void Load(LPCSTR section, u8 LocalAmmoType);
	virtual float Weight() const;

	shared_str	m_ammoSect;
	enum{
		cfTracer				= (1<<0),
		cfRicochet				= (1<<1),
		cfCanBeUnlimited		= (1<<2),
		cfExplosive				= (1<<3),
	};
	float	m_kDist, m_kDisp, m_kHit, m_kImpulse, m_kPierce, m_kAP, m_kAirRes, m_kSpeed;
	int		m_buckShot;
	float	m_impair;
	float	fWallmarkSize{};
	
	u8		m_u8ColorID{};
	u8		m_LocalAmmoType{};

	
	u16		bullet_material_idx;
	Flags8	m_flags;

	shared_str	m_InvShortName;
	RStringVec	m_ExplodeParticles;
	//вероятность осечки
	float		m_misfireProbability;
};

class CWeaponAmmo :	public CInventoryItemObject {
	typedef CInventoryItemObject		inherited;
public:
									CWeaponAmmo			(void);
	virtual							~CWeaponAmmo		(void);

	virtual CWeaponAmmo				*cast_weapon_ammo	()	{return this;}
	virtual void					Load				(LPCSTR section);
	virtual BOOL					net_Spawn			(CSE_Abstract* DC);
	virtual void					net_Destroy			();
	virtual void					net_Export			( CSE_Abstract* E );
	virtual void					OnH_B_Chield		();
	virtual void					OnH_B_Independent	(bool just_before_destroy);
	virtual void					UpdateCL			();
	virtual void					renderable_Render	();

	virtual bool					Useful				() const;
	virtual float					Weight				() const;

	virtual u32						Cost				() const;
	bool							Get					(CCartridge &cartridge);

	float		m_kDist, m_kDisp, m_kHit, m_kImpulse, m_kPierce, m_kAP, m_kAirRes, m_kSpeed;
	int			m_buckShot;
	float		m_impair;
	float		fWallmarkSize;
	u8			m_u8ColorID;

	u16			m_boxSize;
	u16			m_boxCurr;
	bool		m_tracer;
	//
	shared_str	m_ammoSect, m_EmptySect;
	shared_str	m_InvShortName;
	//вероятность осечки от патрона
	float		m_misfireProbability;
	//вероятность осечки от магазина
	float		m_misfireProbabilityBox;

	xr_vector<shared_str>		m_ammoTypes;
	xr_vector<shared_str>		m_magTypes;
	virtual bool IsBoxReloadable		() const;
	virtual bool IsBoxReloadableEmpty	() const;
	void ReloadBox				(LPCSTR ammo_sect);
	void UnloadBox				();
	void SpawnAmmo				(u32 boxCurr = 0xffffffff, LPCSTR ammoSect = NULL, u32 ParentID = 0xffffffff);

public:
	virtual CInventoryItem *can_make_killing	(const CInventory *inventory) const;

protected:
	ref_sound				sndLoad;
	ref_sound				sndUnload;
};
