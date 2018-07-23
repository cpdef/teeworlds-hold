/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG)
{
	m_Team = Team;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = NULL;
	m_GrabTick = 0;
	
	for (int i=0; i<CFlag::NUM_IDS;i++)
		m_IDs[i] = Server()->SnapNewID();	

	Reset();
}

void CFlag::Reset()
{
	m_pCarryingCharacter = NULL;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0,0);
	m_GrabTick = 0;
}

void CFlag::TickPaused()
{
	++m_DropTick;
	if(m_GrabTick)
		++m_GrabTick;
}

void CFlag::Tick()
{
	if (GameServer()->Collision()->CheckPoint(m_Pos.x, m_Pos.y))
		m_Pos.y += 30;
}

void CFlag::Snap(int SnappingClient)
{

	int team = -1; 
	if (m_pCarryingCharacter && m_pCarryingCharacter->GetPlayer())
		team = m_pCarryingCharacter->GetPlayer()->GetTeam();
	
	// change color every X ticks:
	if (team == -1)
	{
		if ((Server()->Tick() % 200) < 100)
			team = TEAM_RED;
		else
			team = TEAM_BLUE;
	}
	else
	{
		if (team == TEAM_RED)
			team = TEAM_BLUE;
		else
			team = TEAM_RED;
	}

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = team;
	
	// just fancy stuff :D
	if(!m_pCarryingCharacter)
	{
		for(int i=0; i<CFlag::NUM_PARTICLES; i++)
		{
			float Radius = 30+(Server()->Tick()%25)*3;
			float Angle = (float) i+(float)Server()->Tick();  //2.0f * pi * (1/CFlag::NUM_PARTICLES) * (float)i;
			vec2 RelPos = vec2(Radius * cos(Angle), Radius * sin(Angle));
			vec2 ParticlePos = m_Pos + RelPos;
			
			CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[i], sizeof(CNetObj_Projectile)));
			if(pObj)
			{
				pObj->m_X = (int)ParticlePos.x;
				pObj->m_Y = (int)ParticlePos.y;
				pObj->m_VelX = (int)RelPos.x;
				pObj->m_VelY = (int)RelPos.y;
				pObj->m_StartTick = Server()->Tick();
				pObj->m_Type = WEAPON_HAMMER;
			}
		}
	}
}
