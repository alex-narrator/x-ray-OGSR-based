///////////////////////////////////////////////////////////////
// GraviArtifact.cpp
// GraviArtefact - гравитационный артефакт, прыгает на месте
// и неустойчиво парит над землей
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GraviArtifact.h"
#include "PhysicsShell.h"
#include "level.h"
#include "xrmessages.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "phworld.h"
extern CPHWorld*	ph_world;
#define CHOOSE_MAX(x,inst_x,y,inst_y,z,inst_z)\
	if(x>y)\
		if(x>z){inst_x;}\
		else{inst_z;}\
	else\
		if(y>z){inst_y;}\
		else{inst_z;}


CGraviArtefact::CGraviArtefact(void) 
{
	shedule.t_min = 20;
	shedule.t_max = 50;

	m_fEnergy = 1.f;
}

CGraviArtefact::~CGraviArtefact(void) 
{
}

void CGraviArtefact::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_fJumpHeight = READ_IF_EXISTS(pSettings, r_float, section, "jump_height",0.f);
//	m_fEnergy = pSettings->r_float(section,"energy");
}



void CGraviArtefact::UpdateCLChild() 
{

	VERIFY(!ph_world->Processing());
	if (getVisible() && m_pPhysicsShell) {
		if (!fis_zero(GetJumpHeight())) {
			Fvector dir; 
			dir.set(0, -1.f, 0);
			collide::rq_result RQ;
			
			//проверить высоту артифакта
			if(Level().ObjectSpace.RayPick(Position(), dir, /*m_fJumpHeight*/GetJumpHeight(), collide::rqtBoth, RQ, this))
			{
				dir.y = 1.f; 
				m_pPhysicsShell->applyImpulse(dir, 
											  30.f * Device.fTimeDelta * 
											  m_pPhysicsShell->getMass());
			}
		}
	} else 
		if(H_Parent()) 
		{
			XFORM().set(H_Parent()->XFORM());
		};
}