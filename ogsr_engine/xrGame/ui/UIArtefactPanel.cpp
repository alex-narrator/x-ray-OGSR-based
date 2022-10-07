#include "StdAfx.h"
#include "UIArtefactPanel.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"
#include "inventory_item.h"
#include "Actor.h"
#include "Inventory.h"
#include "string_table.h"

void CUIArtefactPanel::InitFromXML	(CUIXml& xml, LPCSTR path, int index)
{
	CUIXmlInit::InitWindow		(xml, path, index, this);
	m_cell_size.x				= xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y				= xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale					= xml.ReadAttribFlt(path, index, "scale");
}

void CUIArtefactPanel::Update()
{
	m_si.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	for(const auto& _itm : inv.m_belt){
		if (_itm) {
			m_vRects.push_back( &(_itm->m_icon_params ) );
		}
	}
}

void CUIArtefactPanel::Draw(){
	const float iIndent = 1.0f;
	      float x{};
		  float y{};
		  float iHeight;
		  float iWidth;

	Frect				rect;
	GetAbsoluteRect		(rect);
	x					= rect.left;
	y					= rect.top;	
	
	float _s			= m_cell_size.x/m_cell_size.y;

	for ( const auto& params : m_vRects )
	{
		params->set_shader( &m_si );
		const auto& r = params->original_rect();
		iHeight = m_fScale*(r.bottom - r.top);
		iWidth  = _s*m_fScale*(r.right - r.left);

		m_si.SetRect(0, 0, iWidth, iHeight);

		m_si.SetPos(x, y);
		x = x + iIndent + iWidth;

        m_si.Render();
	}

	CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////SLOT PANEL//////////////////////
/////////////////////////////////////////////////
void CUISlotPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index)
{
	CUIXmlInit::InitWindow(xml, path, index, this);
	m_cell_size.x = xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y = xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale = xml.ReadAttribFlt(path, index, "scale");
}

void CUISlotPanel::Update()
{
	m_st.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();
	m_action_key.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	for (u32 i = 0; i < inv.m_slots.size(); ++i) {
		const auto& _itm = inv.m_slots[i].m_pIItem;
		if (!_itm) continue;

		switch (i)
		{
		case QUICK_SLOT_0:
		case QUICK_SLOT_1:
		case QUICK_SLOT_2:
		case QUICK_SLOT_3:
		{
			if (_itm) {
				string16	slot_key{};
				if (_itm->cast_eatable_item() || _itm->cast_hud_item()) {
					sprintf_s(slot_key, "ui_use_slot_%d", _itm->GetSlot());
				}
				m_action_key.push_back(CStringTable().translate(slot_key).c_str());
				m_vRects.push_back(&(_itm->m_icon_params));
			}
		}
		default:
			break;
		}
	}
}

void CUISlotPanel::Draw() {
	const float iIndent = 1.0f;
	float x{};
	float y{};
	float iHeight;
	float iWidth;

	Frect				rect;
	GetAbsoluteRect(rect);
	x = rect.left;
	y = rect.top;

	float _s = m_cell_size.x / m_cell_size.y;

	for (int i = 0; i < m_vRects.size(); i++)
	{
		const auto& params = m_vRects[i];

		params->set_shader(&m_st);
		const auto& r = params->original_rect();
		iHeight = m_fScale * (r.bottom - r.top);
		iWidth = _s * m_fScale * (r.right - r.left);

		m_st.SetWndRect(0, 0, iWidth, iHeight);

		m_st.SetWndPos(x, y);
		x = x + iIndent + iWidth;

		m_st.SetText(m_action_key[i].c_str());

		m_st.Draw();
	}

	CUIWindow::Draw();
}

/////////////////////////////////////////////////
/////////////////VEST PANEL//////////////////////
/////////////////////////////////////////////////
void CUIVestPanel::InitFromXML(CUIXml& xml, LPCSTR path, int index)
{
	CUIXmlInit::InitWindow(xml, path, index, this);
	m_cell_size.x	= xml.ReadAttribFlt(path, index, "cell_width");
	m_cell_size.y	= xml.ReadAttribFlt(path, index, "cell_height");
	m_fScale		= xml.ReadAttribFlt(path, index, "scale");
}

void CUIVestPanel::Update()
{
	m_st.SetShader(InventoryUtilities::GetEquipmentIconsShader());
	m_vRects.clear();

	auto pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	auto& inv = pActor->inventory();

	for (u32 i = 0; i < inv.m_slots.size(); ++i) {
		const auto& _itm = inv.m_slots[i].m_pIItem;
		if (!_itm) continue;

		switch (i)
		{
		case VEST_POUCH_1:
		case VEST_POUCH_2:
		case VEST_POUCH_3:
		case VEST_POUCH_4:
		case VEST_POUCH_5:
		case VEST_POUCH_6:
		case VEST_POUCH_7:
		case VEST_POUCH_8:
		case VEST_POUCH_9:
		case VEST_POUCH_10:
		{
			if (_itm) {
				m_vRects.push_back(&(_itm->m_icon_params));
			}
		}
		default:
			break;
		}
	}
}

void CUIVestPanel::Draw() {
	const float iIndent = 1.0f;
	float x{};
	float y{};
	float iHeight;
	float iWidth;

	Frect				rect;
	GetAbsoluteRect(rect);
	x = rect.left;
	y = rect.top;

	float _s = m_cell_size.x / m_cell_size.y;

	for (int i = 0; i < m_vRects.size(); i++)
	{
		const auto& params = m_vRects[i];

		params->set_shader(&m_st);
		const auto& r = params->original_rect();
		iHeight = m_fScale * (r.bottom - r.top);
		iWidth = _s * m_fScale * (r.right - r.left);

		m_st.SetRect(0, 0, iWidth, iHeight);

		m_st.SetPos(x, y);
		x = x + iIndent + iWidth;

		m_st.Render();
	}

	CUIWindow::Draw();
}