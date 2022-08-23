#pragma once

#include "inventory_item.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem {
	friend class CEatableItemScript;
private:
	typedef CInventoryItem	inherited;

private:
	CPhysicItem		*m_physic_item;

public:
							CEatableItem				();
	virtual					~CEatableItem				();
	virtual	DLL_Pure*		_construct					();
	virtual CEatableItem	*cast_eatable_item			()	{return this;}

	virtual void			Load						(LPCSTR section);
	virtual bool			Useful						() const;

	virtual BOOL			net_Spawn					(CSE_Abstract* DC);
	virtual void			net_Export					(CSE_Abstract* E);

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual	void			UseBy						(CEntityAlive* npc);
			bool			Empty						()	const				{return m_iPortionsNum==0;};
	virtual	void			ZeroAllEffects				();
			void			SetRadiation				(float rad);
			
			LPCSTR			GetUseMenuTip				() const				{ return m_sUseMenuTip; };
protected:	
	//влияние при поедании вещи на параметры игрока
	float					m_fHealthInfluence{};
	float					m_fPowerInfluence{};
	float					m_fMaxPowerUpInfluence{};
	float					m_fSatietyInfluence{};
	float					m_fRadiationInfluence{};
	float					m_fPsyHealthInfluence{};
	float					m_fThirstInfluence{};
	float					m_fAlcoholInfluence{};
	//заживление ран на кол-во процентов
	float					m_fWoundsHealPerc{};

	//количество порций еды, 
	//-1 - порция одна и больше не бывает (чтоб не выводить надпись в меню)
	int						m_iPortionsNum;
	int						m_iStartPortionsNum{};
	bool					m_bUsePortionVolume{};

	//яка доля власної радіоактивності предмета буде передана гравцеві при вживанні
	float					m_fSelfRadiationInfluence{};

	float					GetOnePortionWeight	();
	float					GetOnePortionVolume	();
	u32						GetOnePortionCost	();

	LPCSTR					m_sUseMenuTip{};

public:
	int						GetStartPortionsNum	() const { return m_iStartPortionsNum; };
	int						GetPortionsNum		() const { return m_iPortionsNum; };

	float					GetHealthInfluence		();
	float					GetPowerInfluence		();
	float					GetMaxPowerUpInfluence	();
	float					GetSatietyInfluence		();
	float					GetRadiationInfluence	();
	float					GetPsyHealthInfluence	();
	float					GetThirstInfluence		();
	float					GetAlcoholInfluence		();
	float					GetWoundsHealPerc		();
};

