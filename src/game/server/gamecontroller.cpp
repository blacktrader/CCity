/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <string.h>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/mapitems.hpp>

#include <game/generated/g_protocol.hpp>

#include "entities/pickup.hpp"
#include "entities/light.hpp"
#include "gamecontroller.hpp"
#include "gamecontext.hpp"



GAMECONTROLLER::GAMECONTROLLER()
{
	gametype = "unknown";
	
	//
	do_warmup(config.sv_warmup);
	game_over_tick = -1;
	sudden_death = 0;
	round_start_tick = server_tick();
	round_count = 0;
	game_flags = 0;
	teamscore[0] = 0;
	teamscore[1] = 0;
	map_wish[0] = 0;
	
	unbalanced_tick = -1;
	force_balanced = false;
	
	num_spawn_points[0] = 0;
	num_spawn_points[1] = 0;
	num_spawn_points[2] = 0;

}

GAMECONTROLLER::~GAMECONTROLLER()
{
}

float GAMECONTROLLER::evaluate_spawn_pos(SPAWNEVAL *eval, vec2 pos)
{
	float score = 0.0f;
	CHARACTER *c = (CHARACTER *)game.world.find_first(NETOBJTYPE_CHARACTER);
	for(; c; c = (CHARACTER *)c->typenext())
	{
		// team mates are not as dangerous as enemies
		float scoremod = 1.0f;
		if(eval->friendly_team != -1 && c->team == eval->friendly_team)
			scoremod = 0.5f;
			
		float d = distance(pos, c->pos);
		if(d == 0)
			score += 1000000000.0f;
		else
			score += 1.0f/d;
	}
	
	return score;
}

void GAMECONTROLLER::evaluate_spawn_type(SPAWNEVAL *eval, int t)
{
	// get spawn point
	for(int i  = 0; i < num_spawn_points[t]; i++)
	{
		vec2 p = spawn_points[t][i];
		float s = evaluate_spawn_pos(eval, p);
		if(!eval->got || eval->score > s)
		{
			eval->got = true;
			eval->score = s;
			eval->pos = p;
		}
	}
}

bool GAMECONTROLLER::can_spawn(PLAYER *player, vec2 *out_pos)
{
	SPAWNEVAL eval;
	
	// spectators can't spawn
	if(player->team == -1)
		return false;
	
	if(player->arest)
	{	
		evaluate_spawn_type(&eval, 1);
		evaluate_spawn_type(&eval, 2);
	}
	else
	{
		evaluate_spawn_type(&eval, 0);	
	}	

	*out_pos = eval.pos;
	return eval.got;
}


bool GAMECONTROLLER::on_entity(int index, vec2 pos)
{
	int type = -1;
	int subtype = 0;

	int DOOR_BASE = 17;
	
	if(index == ENTITY_SPAWN)
		spawn_points[0][num_spawn_points[0]++] = pos;
	else if(index == ENTITY_SPAWN_RED)
		spawn_points[1][num_spawn_points[1]++] = pos;
	else if(index == ENTITY_SPAWN_BLUE)
		spawn_points[2][num_spawn_points[2]++] = pos;
	else if(index >= DOOR_BASE && index < DOOR_BASE+16) {
		if(doors[index-DOOR_BASE].tmpstart == vec2(-42,-42)) {
			doors[index-DOOR_BASE].tmpstart = pos;
			doors[index-DOOR_BASE].tmpend = pos;
			doors[index-DOOR_BASE].valid = true;
			dbg_msg("door", "doorstart-> %f:%f in %d", pos.x, pos.y, index-DOOR_BASE);
		} else {
			doors[index-DOOR_BASE].tmpend = pos;
			doors[index-DOOR_BASE].circle = false;
			dbg_msg("door", "doorend-> %f:%f in %d,%d(%d)", pos.x, pos.y, index, DOOR_BASE, index-DOOR_BASE);
		}
	}
	else if(index >= DOOR_BASE+16 && index < DOOR_BASE+32) {
		int doorno = index-DOOR_BASE-16;
		if(doors[doorno].pos_count < 126) {
			doors[doorno].tmppos[doors[doorno].pos_count++] = pos;
			dbg_msg("door", "doortile-> %f:%f in %d", pos.x, pos.y, doorno);
			//Door completed =D
		}
	}
	else if(index >= DOOR_BASE+32 && index < DOOR_BASE+48) {
		int doorno = index-DOOR_BASE-32;
		int idx = doors[doorno].apos_count++;
		
		int team;
		if(doorno <10) {
			team = -1;
		} else if(doorno >=10 && doorno < 13) {
			team = 0; //red door
		} else {
			team = 1; //blue door
		}
		doors[doorno].apos[idx] = pos;
	}
	if (!config.sv_water_insta)
	{
		if(index == ENTITY_ARMOR_1)
			type = POWERUP_ARMOR;
		else if(index == ENTITY_HEALTH_1)
			type = POWERUP_HEALTH;
		else if(index == ENTITY_WEAPON_SHOTGUN)
		{
			type = POWERUP_WEAPON;
			subtype = WEAPON_SHOTGUN;
		}
		else if(index == ENTITY_WEAPON_GRENADE)
		{
			type = POWERUP_WEAPON;
			subtype = WEAPON_GRENADE;
		}
		else if(index == ENTITY_WEAPON_RIFLE)
		{
			type = POWERUP_WEAPON;
			subtype = WEAPON_RIFLE;
		}
		else if(index == ENTITY_POWERUP_NINJA && config.sv_powerups)
		{
			type = POWERUP_NINJA;
			subtype = WEAPON_NINJA;
		}
	}

	if((config.sv_water_insta && config.sv_water_strip) || (!config.sv_water_insta))
	{
		if(index == ENTITY_WEAPON_RIFLE)
		{
			type = POWERUP_WEAPON;
			subtype = WEAPON_RIFLE;
		}
 	}
	
	if(type != -1)
	{
		PICKUP *pickup = new PICKUP(type, subtype);
		pickup->pos = pos;
		return true;
	}

	return false;
}

void GAMECONTROLLER::deinit_lights()
{
	dbg_msg("door", "unlights =D");
	for(int doorno = 0; doorno < 16; doorno++) {
		for(int j = 0; j < doors[doorno].pos_count-1; j++) {
			delete doors[doorno].light[j];
		}
		for(int idx = 0; idx < doors[doorno].apos_count; idx++) {
			delete doors[doorno].alight[idx][0];
			delete doors[doorno].alight[idx][1];
			delete doors[doorno].alight[idx][2];
			delete doors[doorno].alight[idx][3];
			delete doors[doorno].alight[idx][4];
		}
	}
}

void GAMECONTROLLER::init_lights()
{
	dbg_msg("door", "lights =D");
	for(int doorno = 0; doorno < 16; doorno++) {
		for(int j = 0; j < doors[doorno].pos_count-1; j++) {
			vec2 from=doors[doorno].pos[j];
			vec2 to  =doors[doorno].pos[j+1];
			if(j==0 && !doors[doorno].circle)
				from = from - normalize(to-from)*17;
				
			if(j==doors[doorno].pos_count-2 && !doors[doorno].circle)
				to = to + normalize(to-from)*17;
			doors[doorno].light[j] = new LIGHT(from, to, doors[doorno].team);
			//if(j==0 || j == doors[doorno].pos_count-2)
			doors[doorno].light[j]->keep_points = true;
		}
		int team = doors[doorno].team;
		for(int idx = 0; idx < doors[doorno].apos_count; idx++) {
			doors[doorno].alight[idx][0] = new LIGHT(doors[doorno].apos[idx], doors[doorno].apos[idx], team);
			doors[doorno].alight[idx][1] = new LIGHT( vec2(doors[doorno].apos[idx].x-15,doors[doorno].apos[idx].y),  vec2(doors[doorno].apos[idx].x,doors[doorno].apos[idx].y-15), team, 4) ;
			doors[doorno].alight[idx][2] = new LIGHT( vec2(doors[doorno].apos[idx].x,doors[doorno].apos[idx].y-15),  vec2(doors[doorno].apos[idx].x+15,doors[doorno].apos[idx].y), team, 4) ;
			doors[doorno].alight[idx][3] = new LIGHT( vec2(doors[doorno].apos[idx].x+15,doors[doorno].apos[idx].y),  vec2(doors[doorno].apos[idx].x,doors[doorno].apos[idx].y+15), team, 4) ;
			doors[doorno].alight[idx][4] = new LIGHT( vec2(doors[doorno].apos[idx].x,doors[doorno].apos[idx].y+15),  vec2(doors[doorno].apos[idx].x-15,doors[doorno].apos[idx].y), team, 4) ;
		}
	}
}

void GAMECONTROLLER::init_doors()
{
	dbg_msg("door", "i inits nao!");

	for(int doorno = 0; doorno < 16; doorno++) {
		if(doorno <10) {
			doors[doorno].team = -1;
		} else if(doorno >=10 && doorno < 13) {
			doors[doorno].team = 0; //red door
		} else {
			doors[doorno].team = 1; //blue door
		}
		if(!doors[doorno].valid) 
			continue;
		doors[doorno].pos[0] = doors[doorno].tmpstart;
		for(int i = 0; i < doors[doorno].pos_count; i++) {
			//dbg_msg("door", "i has a door %f:%f", doors[doorno].pos[i].x, doors[doorno].pos[i].y);
			int dist = -1;
			for(int j = 0; j < doors[doorno].pos_count; j++) {
				if(doors[doorno].mark[j])
					continue;
				if(dist < 0)
					dist = j;
				else {
					float olddist = distance(doors[doorno].tmppos[dist], doors[doorno].pos[i]);
					float newdist = distance(doors[doorno].tmppos[j], doors[doorno].pos[i]);
					if(newdist < olddist) dist = j;
				}
			}
			doors[doorno].pos[i+1] = doors[doorno].tmppos[dist];
			doors[doorno].mark[dist] = true;
		}
			
		doors[doorno].pos_count += 2;
		doors[doorno].pos[doors[doorno].pos_count-1] = doors[doorno].tmpend;
	}
	dbg_msg("door","i s dun!");
	init_lights();
	for(int i = 0; i < 16; i++)
		set_door(i, true);
}

void GAMECONTROLLER::endround()
{
	if(warmup) // game can't end when we are running warmup
		return;
		
	game.world.paused = true;

	// save all accounts here to if there is no map change
	if(!strcmp(config.sv_map, map_wish))
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(game.players[i] && game.players[i]->logged_in)
				game.players[i]->account->update();

	game_over_tick = server_tick();
	sudden_death = 0;
}

void GAMECONTROLLER::resetgame()
{
	game.world.reset_requested = true;
}

const char *GAMECONTROLLER::get_team_name(int team)
{
	if(is_teamplay())
	{
		if(team == 0)
			return "red team";
		else if(team == 1)
			return "blue team";
	}
	else
	{
		if(team == 0)
			return "game";
	}
	
	return "spectators";
}

static bool is_separator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void GAMECONTROLLER::startround()
{
	resetgame();
	
	round_start_tick = server_tick();
	sudden_death = 0;
	game_over_tick = -1;
	game.world.paused = false;
	teamscore[0] = 0;
	teamscore[1] = 0;
	unbalanced_tick = -1;
	force_balanced = false;
	for(int i = 0; i < 16; i++) {
		set_door(i, true);
	}
	dbg_msg("game","start round type='%s' teamplay='%d'", gametype, game_flags&GAMEFLAG_TEAMS);
}

void GAMECONTROLLER::change_map(const char *to_map)
{
	str_copy(map_wish, to_map, sizeof(map_wish));
	endround();
}

void GAMECONTROLLER::cyclemap()
{
	if(map_wish[0] != 0)
	{
		dbg_msg("game", "rotating map to %s", map_wish);
		str_copy(config.sv_map, map_wish, sizeof(config.sv_map));
		map_wish[0] = 0;
		round_count = 0;
		return;
	}
	if(!strlen(config.sv_maprotation))
		return;

	if(round_count < config.sv_rounds_per_map-1)
		return;
		
	// handle maprotation
	const char *map_rotation = config.sv_maprotation;
	const char *current_map = config.sv_map;
	
	int current_map_len = strlen(current_map);
	const char *next_map = map_rotation;
	while(*next_map)
	{
		int wordlen = 0;
		while(next_map[wordlen] && !is_separator(next_map[wordlen]))
			wordlen++;
		
		if(wordlen == current_map_len && strncmp(next_map, current_map, current_map_len) == 0)
		{
			// map found
			next_map += current_map_len;
			while(*next_map && is_separator(*next_map))
				next_map++;
				
			break;
		}
		
		next_map++;
	}
	
	// restart rotation
	if(next_map[0] == 0)
		next_map = map_rotation;

	// cut out the next map	
	char buf[512];
	for(int i = 0; i < 512; i++)
	{
		buf[i] = next_map[i];
		if(is_separator(next_map[i]) || next_map[i] == 0)
		{
			buf[i] = 0;
			break;
		}
	}
	
	// skip spaces
	int i = 0;
	while(is_separator(buf[i]))
		i++;
	
	round_count = 0;
	
	dbg_msg("game", "rotating map to %s", &buf[i]);
	str_copy(config.sv_map, &buf[i], sizeof(config.sv_map));
}

void GAMECONTROLLER::post_reset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i])
		{
			game.players[i]->respawn();
			game.players[i]->score = 0;
		}
	}
}
	
void GAMECONTROLLER::on_player_info_change(class PLAYER *p)
{
	const int team_colors[2] = {65387, 10223467};
	if(is_teamplay())
	{
		if(p->team >= 0 || p->team <= 1)
		{
			p->use_custom_color = 1;
			p->color_body = team_colors[p->team];
			p->color_feet = team_colors[p->team];
		}
	}
}


int GAMECONTROLLER::on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon)
{
	// do scoreing
	if(!killer)
		return 0;
	if(killer == victim->player && (is_race() && config.sv_count_suicide))
		victim->player->score--; // suicide
	else
	{
		if(is_teamplay() && victim->team == killer->team && (is_race() && config.sv_count_teamkill))
			killer->score--; // teamkill
		else if (!is_race() || (is_race() && (config.sv_count_kill || config.sv_count_teamkill)))
			killer->score++; // normal kill
	}
	return 0;
}

void GAMECONTROLLER::on_character_spawn(class CHARACTER *chr)
{
	// default health
	chr->health = 10;
	
	// give default weapons
	if(config.sv_water_insta) {
		chr->weapons[WEAPON_HAMMER].got = 1;
		chr->weapons[WEAPON_HAMMER].ammo = -1;
		chr->weapons[WEAPON_GUN].got = 0;
		//chr->weapons[WEAPON_GUN].ammo = 10;
		chr->weapons[WEAPON_RIFLE].got = 1;
		chr->weapons[WEAPON_RIFLE].ammo = -1;
		chr->active_weapon=WEAPON_RIFLE;
	} else {
		chr->weapons[WEAPON_HAMMER].got = 1;
		chr->weapons[WEAPON_HAMMER].ammo = -1;
		chr->weapons[WEAPON_GUN].got = 1;
		chr->weapons[WEAPON_GUN].ammo = 10;
	}
}

void GAMECONTROLLER::do_warmup(int seconds)
{
	warmup = seconds*server_tickspeed();
}

bool GAMECONTROLLER::is_friendly_fire(int cid1, int cid2)
{
	if(cid1 == cid2)
		return false;
	
	if(is_teamplay())
	{
		if(!game.players[cid1] || !game.players[cid2])
			return false;
			
		if(game.players[cid1]->team == game.players[cid2]->team)
			return true;
	}
	
	return false;
}

bool GAMECONTROLLER::is_force_balanced()
{
	if(force_balanced)
	{
		force_balanced = false;
		return true;
	}
	else
		return false;
}

void GAMECONTROLLER::tick()
{
	// do warmup
	if(warmup)
	{
		warmup--;
		if(!warmup)
			startround();
	}
	
	if(game_over_tick != -1)
	{
		// game over.. wait for restart
		if(server_tick() > game_over_tick+server_tickspeed()*10)
		{
			cyclemap();
			startround();
			round_count++;
		}
	}
	
	// do team-balancing
	if (is_teamplay() && unbalanced_tick != -1 && server_tick() > unbalanced_tick+config.sv_teambalance_time*server_tickspeed()*60)
	{
		dbg_msg("game", "Balancing teams");
		
		int t[2] = {0,0};
		int tscore[2] = {0,0};
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i] && game.players[i]->team != -1)
			{
				t[game.players[i]->team]++;
				tscore[game.players[i]->team]+=game.players[i]->score;
			}
		}
		
		// are teams unbalanced?
		if(abs(t[0]-t[1]) >= 2)
		{
			int m = (t[0] > t[1]) ? 0 : 1;
			int num_balance = abs(t[0]-t[1]) / 2;
			
			do
			{
				PLAYER *p = 0;
				int pd = tscore[m];
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(!game.players[i])
						continue;
					
					// remember the player who would cause lowest score-difference
					if(game.players[i]->team == m && (!p || abs((tscore[m^1]+game.players[i]->score) - (tscore[m]-game.players[i]->score)) < pd))
					{
						p = game.players[i];
						pd = abs((tscore[m^1]+p->score) - (tscore[m]-p->score));
					}
				}
				
				// move the player to other team without losing his score
				// TODO: change in player::set_team needed: player won't lose score on team-change
				int score_before = p->score;
				p->set_team(m^1);
				p->score = score_before;
				
				p->respawn();
				p->force_balanced = true;
			} while (--num_balance);
			
			force_balanced = true;
		}
		unbalanced_tick = -1;
	}
	
	// update browse info
	int prog = -1;
	if(config.sv_timelimit > 0)
		prog = max(prog, (server_tick()-round_start_tick) * 100 / (config.sv_timelimit*server_tickspeed()*60));

	if(config.sv_scorelimit)
	{
		if(is_teamplay())
		{
			prog = max(prog, (teamscore[0]*100)/config.sv_scorelimit);
			prog = max(prog, (teamscore[1]*100)/config.sv_scorelimit);
		}
		else
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(game.players[i])
					prog = max(prog, (game.players[i]->score*100)/config.sv_scorelimit);
			}
		}
	}

	if(warmup)
		prog = -1;
		
	server_setbrowseinfo(gametype, prog);
}


bool GAMECONTROLLER::is_teamplay() const
{
	return game_flags&GAMEFLAG_TEAMS;
}

void GAMECONTROLLER::snap(int snapping_client)
{
	NETOBJ_GAME *gameobj = (NETOBJ_GAME *)snap_new_item(NETOBJTYPE_GAME, 0, sizeof(NETOBJ_GAME));
	gameobj->paused = game.world.paused;
	gameobj->game_over = game_over_tick==-1?0:1;
	gameobj->sudden_death = sudden_death;
	
	gameobj->score_limit = config.sv_scorelimit;
	gameobj->time_limit = config.sv_timelimit;
	gameobj->round_start_tick = round_start_tick;
	gameobj->flags = game_flags;
	
	gameobj->warmup = warmup;
	
	gameobj->round_num = (strlen(config.sv_maprotation) && config.sv_rounds_per_map) ? config.sv_rounds_per_map : 0;
	gameobj->round_current = round_count+1;
	
	
	if(snapping_client == -1)
	{
		// we are recording a demo, just set the scores
		gameobj->teamscore_red = teamscore[0];
		gameobj->teamscore_blue = teamscore[1];
	}
	else
	{
		// TODO: this little hack should be removed
		gameobj->teamscore_red = is_teamplay() ? teamscore[0] : game.players[snapping_client]->score;
		gameobj->teamscore_blue = teamscore[1];
	}
}

int GAMECONTROLLER::get_auto_team(int notthisid)
{
	// this will force the auto balancer to work overtime aswell
	if(config.dbg_stress)
		return 0;
	
	int numplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i] && i != notthisid)
		{
			if(game.players[i]->team == 0 || game.players[i]->team == 1)
				numplayers[game.players[i]->team]++;
		}
	}

	int team = 0;
	if(is_teamplay())
		team = numplayers[0] > numplayers[1] ? 1 : 0;
		
	if(can_join_team(team, notthisid))
		return team;
	return -1;
}

bool GAMECONTROLLER::get_door(int idx)
{
	return doors[idx].act;
}

void GAMECONTROLLER::set_door(int idx, bool active)
{
	if(!doors[idx].valid)
		return;
	doors[idx].act = active;

	for(int i = 0; i < doors[idx].apos_count; i++) {
		for(int j = 1; j < 5; j++) {
			doors[idx].alight[i][j]->visible = active;
		}
	}
	for(int i = 0; i < doors[idx].pos_count-1; i++) {
		doors[idx].light[i]->visible = active;
	}
}

bool GAMECONTROLLER::can_join_team(int team, int notthisid)
{
	(void)team;
	int numplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i] && i != notthisid)
		{
			if(game.players[i]->team >= 0 || game.players[i]->team == 1)
				numplayers[game.players[i]->team]++;
		}
	}
	
	return (numplayers[0] + numplayers[1]) < config.sv_max_clients-config.sv_spectator_slots;
}

bool GAMECONTROLLER::check_team_balance()
{
	if(!is_teamplay() || !config.sv_teambalance_time)
		return true;
	
	int t[2] = {0, 0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		PLAYER *p = game.players[i];
		if(p && p->team != -1)
			t[p->team]++;
	}
	
	if(abs(t[0]-t[1]) >= 2)
	{
		dbg_msg("game", "Team is NOT balanced (red=%d blue=%d)", t[0], t[1]);
		if (game.controller->unbalanced_tick == -1)
			game.controller->unbalanced_tick = server_tick();
		return false;
	}
	else
	{
		dbg_msg("game", "Team is balanced (red=%d blue=%d)", t[0], t[1]);
		game.controller->unbalanced_tick = -1;
		return true;
	}
}

bool GAMECONTROLLER::can_change_team(PLAYER *pplayer, int jointeam)
{
	int t[2] = {0, 0};
	
	if (!is_teamplay() || jointeam == -1 || !config.sv_teambalance_time)
		return true;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		PLAYER *p = game.players[i];
		if(p && p->team != -1)
			t[p->team]++;
	}
	
	// simulate what would happen if changed team
	t[jointeam]++;
	if (pplayer->team != -1)
		t[jointeam^1]--;
	
	// there is a player-difference of at least 2
	if(abs(t[0]-t[1]) >= 2)
	{
		// player wants to join team with less players
		if ((t[0] < t[1] && jointeam == 0) || (t[0] > t[1] && jointeam == 1))
			return true;
		else
			return false;
	}
	else
		return true;
}

void GAMECONTROLLER::do_player_score_wincheck()
{
	if(game_over_tick == -1  && !warmup)
	{
		// gather some stats
		int topscore = 0;
		int topscore_count = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i])
			{
				if(game.players[i]->score > topscore)
				{
					topscore = game.players[i]->score;
					topscore_count = 1;
				}
				else if(game.players[i]->score == topscore)
					topscore_count++;
			}
		}
		
		// check score win condition
		if((config.sv_scorelimit > 0 && topscore >= config.sv_scorelimit) ||
			(config.sv_timelimit > 0 && (server_tick()-round_start_tick) >= config.sv_timelimit*server_tickspeed()*60))
		{
			if(topscore_count == 1)
				endround();
			else
				sudden_death = 1;
		}
	}
}

void GAMECONTROLLER::do_team_score_wincheck()
{
	if(game_over_tick == -1 && !warmup)
	{
		// check score win condition
		if((config.sv_scorelimit > 0 && (teamscore[0] >= config.sv_scorelimit || teamscore[1] >= config.sv_scorelimit)) ||
			(config.sv_timelimit > 0 && (server_tick()-round_start_tick) >= config.sv_timelimit*server_tickspeed()*60))
		{
			if(teamscore[0] != teamscore[1])
				endround();
			else
				sudden_death = 1;
		}
	}
}

void GAMECONTROLLER::do_race_time_check()
{
	if(game_over_tick == -1 && !warmup)
	{
		if((config.sv_timelimit > 0 && (server_tick()-round_start_tick) >= config.sv_timelimit*server_tickspeed()*60))
			endround();
	}
}

int GAMECONTROLLER::clampteam(int team)
{
	if(team < 0) // spectator
		return -1;
	if(is_teamplay())
		return team&1;
	return  0;
}

GAMECONTROLLER *gamecontroller = 0;

bool GAMECONTROLLER::is_race() const
{	
	return true;
	//return (!str_comp_nocase(this->gametype,"CTF") || !str_comp_nocase(this->gametype,"DM") || !str_comp_nocase(this->gametype,"TDM"));
}
