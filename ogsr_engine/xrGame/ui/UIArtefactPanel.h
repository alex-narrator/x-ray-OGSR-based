#pragma once
#include "UIWindow.h"
#include "../UIStaticItem.h"
#include "UIIconParams.h"

class CUIXml;
//artefact panel
class CUIArtefactPanel : public CUIWindow
{
public:
	CUIArtefactPanel() = default;
	~CUIArtefactPanel() = default;

	virtual void Update			();
	virtual void Draw			();
			void InitFromXML	(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStaticItem               m_si;
};
//quick slot panel
class CUISlotPanel : public CUIWindow
{
public:
	CUISlotPanel() = default;
	~CUISlotPanel() = default;

	virtual void Update			();
	virtual void Draw			();
			void InitFromXML	(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStatic					m_st;
	xr_vector<shared_str>		m_action_key;
};
//vest slot panel
class CUIVestPanel : public CUIWindow
{
public:
	CUIVestPanel() = default;
	~CUIVestPanel() = default;

	virtual void Update		();
	virtual void Draw		();
			void InitFromXML(CUIXml& xml, LPCSTR path, int index);

protected:
	float						m_fScale;
	Fvector2					m_cell_size;
	xr_vector<CIconParams*>      m_vRects;
	CUIStaticItem				m_st;
};