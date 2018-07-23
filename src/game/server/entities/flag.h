/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_FLAG_H
#define GAME_SERVER_ENTITIES_FLAG_H

#include <game/server/entity.h>

class CFlag : public CEntity
{
public:
	enum
	{
		NUM_PARTICLES = 12,
		NUM_IDS = NUM_PARTICLES,
	};

	static const int ms_PhysSize = 14;
	CCharacter *m_pCarryingCharacter;
	vec2 m_Vel;
	vec2 m_StandPos;
	
	int m_Team;
	int m_AtStand;
	int m_DropTick;
	int m_GrabTick;
	int m_IDs[NUM_IDS];

	CFlag(CGameWorld *pGameWorld, int Team);

	virtual void Reset();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void Tick();
};

#endif
