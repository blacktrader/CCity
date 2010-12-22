/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <game/server/entities/character.hpp>
#include <game/server/player.hpp>
#include "tdm.hpp"

GAMECONTROLLER_TDM::GAMECONTROLLER_TDM()
{
	gametype = "[N]tdm";
	game_flags = GAMEFLAG_TEAMS;
}

int GAMECONTROLLER_TDM::on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon)
{
	GAMECONTROLLER::on_character_death(victim, killer, weapon);
			
	return 0;
}

void GAMECONTROLLER_TDM::tick()
{
	GAMECONTROLLER::tick();

	do_race_time_check();
}

bool GAMECONTROLLER_TDM::is_race() const
{
	return true;
}

