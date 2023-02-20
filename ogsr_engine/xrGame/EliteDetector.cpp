#include "StdAfx.h"
#include "EliteDetector.h"
#include "player_hud.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "ui/ArtefactDetectorUI.h"

constexpr const char* AF_SIGN = "af_sign";

CEliteDetector::CEliteDetector() { m_artefacts.m_af_rank = 3; }

void CEliteDetector::CreateUI()
{
    R_ASSERT(!m_ui);
    m_ui = xr_new<CUIArtefactDetectorElite>();
    ui().construct(this);
}

CUIArtefactDetectorElite& CEliteDetector::ui() { return *((CUIArtefactDetectorElite*)m_ui); }
void CEliteDetector::UpdateAf()
{
    ui().Clear();
    if (m_artefacts.m_ItemInfos.empty())
        return;

    for (auto& item : m_artefacts.m_ItemInfos){
        auto pAf = item.first;
        if (pAf->H_Parent())
            continue;

        ui().RegisterItemToDraw(pAf->Position(), AF_SIGN);
        TryMakeArtefactVisible(pAf);
    }
}

bool CEliteDetector::render_item_3d_ui_query() { return IsPowerOn() && ui().m_wrk_area != nullptr; }
void CEliteDetector::render_item_3d_ui()
{
    R_ASSERT(HudItemData());
    inherited::render_item_3d_ui();
    ui().Draw();
    //	Restore cull mode
    UIRender->CacheSetCullMode(IUIRender::cmCCW);
}

static void fix_ws_wnd_size(CUIWindow* w, float kx)
{
    Fvector2 p = w->GetWndSize();
    p.x /= kx;
    w->SetWndSize(p);

    p = w->GetWndPos();
    p.x /= kx;
    w->SetWndPos(p);

    for (const auto child_wnd : w->GetChildWndList()){
        fix_ws_wnd_size(child_wnd, kx);
    }
}

void CUIArtefactDetectorElite::construct(CEliteDetector* p)
{
    m_parent = p;
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, "ui_detector_artefact.xml");
    if (xml_result)
    {
        string512 buff;
        xr_strcpy(buff, p->ui_xml_tag());

        CUIXmlInit::InitWindow(uiXml, buff, 0, this);

        m_wrk_area = xr_new<CUIWindow>();

        xr_sprintf(buff, "%s:wrk_area", p->ui_xml_tag());

        CUIXmlInit::InitWindow(uiXml, buff, 0, m_wrk_area);
        m_wrk_area->SetAutoDelete(true);
        AttachChild(m_wrk_area);

        xr_sprintf(buff, "%s", p->ui_xml_tag());
        XML_NODE* pStoredRoot = uiXml.GetLocalRoot();
        uiXml.SetLocalRoot(uiXml.NavigateToNode(buff, 0));

        const int num = uiXml.GetNodesNum(buff, 0, "palette");
        if (num > 0)
        {
            for (int idx = 0; idx < num; ++idx)
            {
                CUIStatic* S = xr_new<CUIStatic>();
                shared_str name = uiXml.ReadAttrib("palette", idx, "id");
                m_palette[name] = S;
                CUIXmlInit::InitStatic(uiXml, "palette", idx, S);
                S->SetAutoDelete(true);
                m_wrk_area->AttachChild(S);
                S->SetCustomDraw(true);
            }
        }
        else
        {
            CUIStatic* S = xr_new<CUIStatic>();
            m_palette[AF_SIGN] = S;
            CUIXmlInit::InitStatic(uiXml, AF_SIGN, 0, S);
            S->SetAutoDelete(true);
            m_wrk_area->AttachChild(S);
            S->SetCustomDraw(true);
        }
        uiXml.SetLocalRoot(pStoredRoot);
    }
    else {
        //R_ASSERT3(xml_result, "xml file not found", "ui_detector_artefact.xml");
        Msg("!![%s] xml file [ui_detector_artefact.xml] not found!", __FUNCTION__);
    }

    Fvector _map_attach_p = pSettings->r_fvector3(m_parent->cNameSect(), "ui_p");
    Fvector _map_attach_r = pSettings->r_fvector3(m_parent->cNameSect(), "ui_r");

    _map_attach_r.mul(PI / 180.f);
    m_map_attach_offset.setHPB(_map_attach_r.x, _map_attach_r.y, _map_attach_r.z);
    m_map_attach_offset.translate_over(_map_attach_p);
}

void CUIArtefactDetectorElite::update()
{
    inherited::update();
    CUIWindow::Update();
}

void CUIArtefactDetectorElite::Draw()
{
    if (!m_wrk_area)
        return;

    Fmatrix LM;
    GetUILocatorMatrix(LM);

    IUIRender::ePointType bk = UI()->m_currentPointType;

    UI()->m_currentPointType = IUIRender::pttLIT;

    UIRender->CacheSetXformWorld(LM);
    UIRender->CacheSetCullMode(IUIRender::cmNONE);

    CUIWindow::Draw();

    //.	Frect r						= m_wrk_area->GetWndRect();
    Fvector2 wrk_sz = m_wrk_area->GetWndSize();
    Fvector2 rp;
    m_wrk_area->GetAbsolutePos(rp);

    Fmatrix M{}, Mc{};
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    Mc.setHPB(h, 0, 0);
    Mc.c.set(Device.vCameraPosition);
    M.invert(Mc);

    UI()->ScreenFrustumLIT().CreateFromRect(Frect().set(rp.x, rp.y, wrk_sz.x, wrk_sz.y));

    //auto it = m_items_to_draw.cbegin();
    //auto it_e = m_items_to_draw.cend();
    for (const auto& item : m_items_to_draw){
        Fvector p = item.pos;
        Fvector pt3d;
        M.transform_tiny(pt3d, p);
        float kz = wrk_sz.y / m_parent->m_fAfDetectRadius;
        pt3d.x *= kz;
        pt3d.z *= kz;

        pt3d.x += wrk_sz.x / 2.0f;
        pt3d.z -= wrk_sz.y;

        Fvector2 pos{};
        pos.set(pt3d.x, -pt3d.z);
        pos.sub(rp);
        if (1 /* r.in(pos)*/)
        {
            item.pStatic->SetWndPos(pos);
            item.pStatic->Draw();
        }
    }

    UI()->m_currentPointType = bk;
}

void CUIArtefactDetectorElite::GetUILocatorMatrix(Fmatrix& _m)
{
    Fmatrix trans = m_parent->HudItemData()->m_item_transform;
    u16 bid = m_parent->HudItemData()->m_model->LL_BoneID("cover");
    Fmatrix cover_bone = m_parent->HudItemData()->m_model->LL_GetTransform(bid);
    _m.mul(trans, cover_bone);
    _m.mulB_43(m_map_attach_offset);
}

void CUIArtefactDetectorElite::Clear() { m_items_to_draw.clear(); }
void CUIArtefactDetectorElite::RegisterItemToDraw(const Fvector& p, const shared_str& palette_idx)
{
    auto it = m_palette.find(palette_idx);
    if (it == m_palette.end()){
        Msg("! RegisterItemToDraw. static not found for [%s]", palette_idx.c_str());
        return;
    }
    CUIStatic* S = m_palette[palette_idx];
    SDrawOneItem itm(S, p);
    m_items_to_draw.push_back(itm);
}

CScientificDetector::CScientificDetector() { m_artefacts.m_af_rank = 3; }
CScientificDetector::~CScientificDetector() {}

void CScientificDetector::UpdateWork()
{
    ui().Clear();

    for (auto& item : m_artefacts.m_ItemInfos){
        auto pAf = item.first;
        if (pAf->H_Parent())
            continue;

        ui().RegisterItemToDraw(pAf->Position(), pAf->cNameSect());

        TryMakeArtefactVisible(pAf);
    }

    for (auto& item : m_zones.m_ItemInfos){
        auto pZone = item.first;
        ui().RegisterItemToDraw(pZone->Position(), pZone->cNameSect());
    }

    m_ui->update();
}
