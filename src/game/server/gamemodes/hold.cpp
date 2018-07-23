/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "hold.h"
#include <iostream>

CGameControllerHOLD::CGameControllerHOLD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pFlag = 0;
	m_pGameType = "HOLD";
	m_GameFlags = GAMEFLAG_TEAMS|GAMEFLAG_FLAGS;
}

bool CGameControllerHOLD::OnEntity(int Index, vec2 Pos)
{
	std::cout << "add Entity " << Index << std::endl;	
	if(IGameController::OnEntity(Index, Pos))
		return true;

	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;
	if(Team == -1 || m_pFlag)
		return false;

	CFlag *F = new CFlag(&GameServer()->m_World, Team);
	F->m_StandPos = Pos;
	F->m_Pos = Pos;
	m_pFlag = F;
	GameServer()->m_World.InsertEntity(F);
	return true;
}

int CGameControllerHOLD::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponID)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, WeaponID);
	int HadFlag = 0;

	CFlag *F = m_pFlag;
	if(F && pKiller && pKiller->GetCharacter() && F->m_pCarryingCharacter == pKiller->GetCharacter())
		HadFlag |= 2;
	if(F && F->m_pCarryingCharacter == pVictim)
	{
		GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
		F->m_DropTick = Server()->Tick();
		F->m_pCarryingCharacter = 0;
		F->m_Vel = vec2(0,0);
			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
			pKiller->m_Score++;
			HadFlag |= 1;
	}

	return HadFlag;
}

void CGameControllerHOLD::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		// check score win condition
		if((g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit)) ||
			(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
		{
			if(m_SuddenDeath)
			{
				if(m_aTeamscore[TEAM_RED]/100 != m_aTeamscore[TEAM_BLUE]/100)
					EndRound();
			}
			else
			{
				if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
					EndRound();
				else
					m_SuddenDeath = 1;
			}
		}
	}
}

bool CGameControllerHOLD::CanBeMovedOnBalance(int ClientID)
{
	CCharacter* Character = GameServer()->m_apPlayers[ClientID]->GetCharacter();
	if(Character)
	{
		CFlag *F = m_pFlag;
		if(F && F->m_pCarryingCharacter == Character)
			return false;
	}
	return true;
}

void CGameControllerHOLD::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

	if(m_pFlag)
	{
		if(m_pFlag->m_AtStand) 
		{
			pGameDataObj->m_FlagCarrierRed = FLAG_ATSTAND;
			pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
		}
		else if(m_pFlag->m_pCarryingCharacter && m_pFlag->m_pCarryingCharacter->GetPlayer())
		{	
			if (m_pFlag->m_pCarryingCharacter->GetPlayer()->GetTeam() == TEAM_RED) 
			{
				pGameDataObj->m_FlagCarrierRed = m_pFlag->m_pCarryingCharacter->GetPlayer()->GetCID();
				pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
			}
			else 
			{
				pGameDataObj->m_FlagCarrierBlue = m_pFlag->m_pCarryingCharacter->GetPlayer()->GetCID();
				pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
			}
		}
		else	
		{
			pGameDataObj->m_FlagCarrierRed = FLAG_TAKEN;
			pGameDataObj->m_FlagCarrierBlue = FLAG_TAKEN;
		}
	}
	else	
	{
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
	}
}

void CGameControllerHOLD::Tick()
{
	IGameController::Tick();

	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	CFlag *F = m_pFlag;

	if(!F)
		return;
	
					
	// flag hits death-tile or left the game layer, reset it
	if(GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_DEATH || F->GameLayerClipped(F->m_Pos))
	{
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
		GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
		F->Reset();
		return;
	}

	//
	if(F->m_pCarryingCharacter)
	{
		// update flag position
		F->m_Pos = F->m_pCarryingCharacter->m_Pos;
		// increase score
		if ((Server()->Tick() % 100) == 0) 
		{
			
			F->m_pCarryingCharacter->GetPlayer()->m_Score += 1;
			m_aTeamscore[F->m_pCarryingCharacter->GetPlayer()->GetTeam()] += 1;
		}
		// drop on "oop" emote
		if (F->m_pCarryingCharacter->GetPlayer()->m_LastEmoticon == 0) 
		{
			if ((Server()->Tick()-F->m_pCarryingCharacter->GetPlayer()->m_EmoticonTick) < 3)
			{
				F->m_Pos.y -= 50;
				F->m_pCarryingCharacter = NULL;	
                		F->m_DropTick = Server()->Tick();
                		F->m_pCarryingCharacter = 0;
                		F->m_Vel = vec2(0,-10);
			}
		}
	}
	else
	{
		CCharacter *apCloseCCharacters[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
				continue;

				// take the flag
				if(F->m_AtStand)
				{
					F->m_GrabTick = Server()->Tick();
				}

				F->m_AtStand = 0;
				F->m_pCarryingCharacter = apCloseCCharacters[i];

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s'",
					F->m_pCarryingCharacter->GetPlayer()->GetCID(),
					Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[c];
					if(!pPlayer)
						continue;
					else if(pPlayer->GetTeam() == F->m_pCarryingCharacter->GetPlayer()->GetTeam())
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, c);
					else
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, c);
				}
				// demo record entry
				GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, -2);
				break;
			//}
		}

		if(!F->m_pCarryingCharacter && !F->m_AtStand)
		{
			if(Server()->Tick() > F->m_DropTick + Server()->TickSpeed()*30)
			{
				GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
				F->Reset();
			}
			else
			{
				F->m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
				GameServer()->Collision()->MoveBox(&F->m_Pos, &F->m_Vel, vec2(F->ms_PhysSize, F->ms_PhysSize), 0.5f);
			}
		}
	}
}
