/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/
//
// teamplay_gamerules.h
//

//++ BulliT
#if !defined(_TEAMPLAY_H_)
#include "multiplay_gamerules.h"
#define _TEAMPLAY_H_
//-- Martin Webrant
#define MAX_TEAMNAME_LENGTH 16
#define MAX_TEAMS 32

#define TEAMPLAY_TEAMLISTLENGTH MAX_TEAMS* MAX_TEAMNAME_LENGTH

class CHalfLifeTeamplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeTeamplay();

	virtual bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer);
	virtual bool IsTeamplay(void);
	virtual bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker);
	virtual int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget);
	virtual const char* GetTeamID(CBaseEntity* pEntity);
	virtual bool ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target);
	virtual int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled);
	virtual void InitHUD(CBasePlayer* pl);
	virtual void DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor);
	//++ BulliT
	// virtual const char *GetGameDescription( void ) { return "HL Teamplay"; }  // this is the game name that gets seen in the server browser
	//-- Martin Webrant
	virtual void UpdateGameMode(CBasePlayer* pPlayer); // the client needs to be informed of the current game mode
	virtual void PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor);
	virtual void Think(void);
	virtual int GetTeamIndex(const char* pTeamName);
	virtual const char* GetIndexedTeamName(int teamIndex);
	virtual bool IsValidTeam(const char* pTeamName);
	const char* SetDefaultPlayerTeam(CBasePlayer* pPlayer);
	virtual void ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib);
	//++ BulliT
	void RecountTeams(void);
	const char* TeamWithFewestPlayers(void);
	//-- Martin Webrant

private:
	//++ BulliT
	// void RecountTeams( void );
	// const char *TeamWithFewestPlayers( void );
	//-- Martin Webrant

	bool m_DisableDeathMessages;
	bool m_DisableDeathPenalty;
	bool m_teamLimit; // This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];
};
#endif //_TEAMPLAY_H_
