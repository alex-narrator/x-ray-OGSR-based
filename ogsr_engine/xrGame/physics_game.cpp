#include "stdafx.h"
#include "ParticlesObject.h"
#include "../xr_3da/gamemtllib.h"
#include "level.h"
#include "gamepersistent.h"
#include "Extendedgeom.h"
#include "PhysicsGamePars.h"
#include "PhysicsCommon.h"
#include "PhSoundPlayer.h"
#include "PhysicsShellHolder.h"
#include "PHCommander.h"
#include "MathUtils.h"
#include "PHWorld.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
static const float PARTICLE_EFFECT_DIST=70.f;
static const float SOUND_EFFECT_DIST=70.f;
//////////////////////////////////////////////////////////////////////////////////
static const float SQUARE_PARTICLE_EFFECT_DIST=PARTICLE_EFFECT_DIST*PARTICLE_EFFECT_DIST;
static const float SQUARE_SOUND_EFFECT_DIST=SOUND_EFFECT_DIST*SOUND_EFFECT_DIST;
class CPHParticlesPlayCall :
		public CPHAction
{
LPCSTR ps_name;
dContactGeom c;
public:
	CPHParticlesPlayCall(const dContactGeom &contact,bool invert_n,LPCSTR psn)
	{
		ps_name=psn;
		c=contact;
		if(invert_n)
		{
			c.normal[0]=-c.normal[0];c.normal[1]=-c.normal[1];c.normal[2]=-c.normal[2];
		}
	}
	virtual void 			run								()
	{
		CParticlesObject* ps = CParticlesObject::Create(ps_name,TRUE);

		Fmatrix pos{};
		Fvector zero_vel{};
		pos.k.set(*((Fvector*)c.normal));
		Fvector::generate_orthonormal_basis(pos.k, pos.j, pos.i);
		pos.c.set(*((Fvector*)c.pos));

		ps->UpdateParent(pos,zero_vel);
		GamePersistent().ps_needtoplay.push_back(ps);
	};
	virtual bool 			obsolete						()const{return false;}
};


class CPHWallMarksCall :
	public CPHAction
{
	wm_shader pWallmarkShader;
	Fvector pos;
	CDB::TRI* T;
public:
	CPHWallMarksCall(const Fvector &p, CDB::TRI* Tri, const wm_shader &s)
	{
		pWallmarkShader=s;
		pos.set(p);
		T=Tri;
	}
	virtual void 			run								()
	{
		//добавить отметку на материале
		::Render->add_StaticWallmark(pWallmarkShader,pos, 
			0.09f, T,
			Level().ObjectSpace.GetStaticVerts());
	};
	virtual bool 			obsolete						()const{return false;}
};

static CPHSoundPlayer* object_snd_player(dxGeomUserData* data) noexcept { return data->ph_ref_object ? data->ph_ref_object->ph_sound_player() : nullptr; }

static void play_object(dxGeomUserData* data, SGameMtlPair* mtl_pair, const dContactGeom* c, bool check_vel = true, float* vol = nullptr) noexcept
{
	auto sp = object_snd_player(data);
	if (sp)
		sp->Play(mtl_pair, *(Fvector*)c->pos, check_vel, vol);
}

// # Hack: limit for effects
static u32 lastContactTime{};

template<class Pars>
void  TContactShotMark(CDB::TRI* T,dContactGeom* c)
{
	dBodyID b=dGeomGetBody(c->g1);
	dxGeomUserData* data;
	bool b_invert_normal{};
	if(!b) 
	{
		b=dGeomGetBody(c->g2);
		data=dGeomGetUserData(c->g2);
		b_invert_normal=true;
	}
	else
	{
		data=dGeomGetUserData(c->g1);
	}
	if(!b) return;
	dVector3 vel;
	dMass m;
	dBodyGetMass(b,&m);
	dBodyGetPointVel(b,c->pos[0],c->pos[1],c->pos[2],vel);
	dReal vel_cret=dFabs(dDOT(vel,c->normal))* _sqrt(m.mass);
	Fvector to_camera{}; to_camera.sub(cast_fv(c->pos), Device.vCameraPosition);
	float square_cam_dist=to_camera.square_magnitude();
	if(data)
	{
		SGameMtlPair* mtl_pair		= GMLib.GetMaterialPair(T->material,data->material);
		if(mtl_pair)
		{
			if(vel_cret>Pars::vel_cret_wallmark && !mtl_pair->m_pCollideMarks->empty())
			{
				wm_shader WallmarkShader = mtl_pair->m_pCollideMarks->GenerateWallmark();
				Level().ph_commander().add_call(xr_new<CPHOnesCondition>(),xr_new<CPHWallMarksCall>( *((Fvector*)c->pos),T, WallmarkShader));
			}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(square_cam_dist<SQUARE_SOUND_EFFECT_DIST)
			{
				SGameMtl* static_mtl =  GMLib.GetMaterialByIdx(T->material);
				if(!static_mtl->Flags.test(SGameMtl::flPassable))
				{
					if(vel_cret>Pars::vel_cret_sound)
					{
						if (!mtl_pair->CollideSounds.empty()) {
							float volume = collide_volume_min + vel_cret * (collide_volume_max - collide_volume_min) / (_sqrt(mass_limit) * default_l_limit - Pars::vel_cret_sound);
							auto sp = object_snd_player(data);
							if (sp)
								sp->PlayNext(mtl_pair, ((Fvector*)c->pos), &volume);
							else
								GET_RANDOM(mtl_pair->CollideSounds).play_no_feedback(nullptr, 0, 0, ((Fvector*)c->pos), &volume);
						}
					}
				}
				else
				{
					if(data->ph_ref_object&&!mtl_pair->CollideSounds.empty())
					{
						play_object(data, mtl_pair, c);
					}
				}
			}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// # Hack: limit for effects
			if (lastContactTime > Device.dwTimeGlobal)
				return;
			else
				lastContactTime = Device.dwTimeGlobal + 500;

			if(square_cam_dist<SQUARE_PARTICLE_EFFECT_DIST)
			{
				if(vel_cret>Pars::vel_cret_particles && !mtl_pair->CollideParticles.empty())
				{
					LPCSTR ps_name = *mtl_pair->CollideParticles[::Random.randI(0,mtl_pair->CollideParticles.size())];
					//отыграть партиклы столкновения материалов
					Level().ph_commander().add_call(xr_new<CPHOnesCondition>(),xr_new<CPHParticlesPlayCall>(*c,b_invert_normal,ps_name));
				}
			}
		}
	}
 }


ContactCallbackFun *ContactShotMark = &TContactShotMark<EffectPars>;
ContactCallbackFun *CharacterContactShotMark = &TContactShotMark<CharacterEffectPars>;