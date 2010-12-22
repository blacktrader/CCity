#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.hpp>

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/

class LIGHT;
struct DOOR
{
	DOOR()
	{
		act = true;
		change_tick = 0;
		for(int i = 0; i < 128; i++) {
			pos[i] = vec2(-42, -42);
			apos[i] = vec2(-42, -42);
		}
		pos_count = 0;
		apos_count = 0;
		team=-1;
	}
	
	int team;
	bool act;
	int pos_count;
	int apos_count;
	//int activators;
	int change_tick;
	vec2 pos[2];
	vec2 apos[128];
};

struct DOOR2
{
	DOOR2()
	{
		act = false;
		valid = false;
		change_tick = 0;
		for(int i = 0; i < 128; i++) {
			pos[i] = vec2(-42, -42);
			apos[i] = vec2(-42, -42);
			mark[i] = false;
		}
		tmpstart = vec2(-42,-42);
		tmpend = vec2(-42,-42);
		pos_count = 0;
		apos_count = 0;
		team=-1;
		circle = true;
		//activator = -1;
	}
	

	//int activator;
	int team;
	bool act;
	bool valid;
	bool circle;
	int pos_count;
	int apos_count;
	//int activators;
	int change_tick;
	
	vec2 tmppos[126];
	bool mark[128];
	vec2 tmpstart;
	vec2 tmpend;
	
	vec2 pos[128];
	vec2 apos[128];
	LIGHT *light[127];
	LIGHT *alight[128][5];
};

class GAMECONTROLLER
{
	vec2 spawn_points[3][64];
	int num_spawn_points[3];
protected:
	struct SPAWNEVAL
	{
		SPAWNEVAL()
		{
			got = false;
			friendly_team = -1;
			pos = vec2(100,100);
		}
			
		vec2 pos;
		bool got;
		int friendly_team;
		float score;
	};

	float evaluate_spawn_pos(SPAWNEVAL *eval, vec2 pos);
	void evaluate_spawn_type(SPAWNEVAL *eval, int type);
	bool evaluate_spawn(class PLAYER *p, vec2 *pos);

	void cyclemap();
	void resetgame();
	
	char map_wish[128];

	
	int round_start_tick;
	int game_over_tick;
	int sudden_death;
	
	int teamscore[2];
	
	int warmup;
	int round_count;
	
	int game_flags;
	int unbalanced_tick;
	bool force_balanced;
	
public:
	const char *gametype;

	bool is_teamplay() const;
	DOOR2 doors[16];
	
	GAMECONTROLLER();
	virtual ~GAMECONTROLLER();

	void do_team_score_wincheck();
	void do_player_score_wincheck();
	void do_race_time_check();
	
	void do_warmup(int seconds);
	
	void startround();
	void endround();
	void change_map(const char *to_map);
	
	bool is_friendly_fire(int cid1, int cid2);
	
	bool is_force_balanced();

	/*
	
	*/	
	virtual void tick();
	
	virtual void snap(int snapping_client);
	
	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.
			
		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.
			
		Returns:
			bool?
	*/
	virtual bool on_entity(int index, vec2 pos);
	virtual void init_doors();
	virtual void deinit_lights();
	virtual void init_lights();
	
	/*
		Function: on_character_spawn
			Called when a character spawns into the game world.
			
		Arguments:
			chr - The character that was spawned.
	*/
	virtual void on_character_spawn(class CHARACTER *chr);
	
	/*
		Function: on_character_death
			Called when a character in the world dies.
			
		Arguments:
			victim - The character that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon);


	virtual void on_player_info_change(class PLAYER *p);

	//
	virtual bool can_spawn(class PLAYER *p, vec2 *pos);

	/*
	
	*/	
	virtual const char *get_team_name(int team);
	virtual int get_auto_team(int notthisid);
	virtual bool can_join_team(int team, int notthisid);
	bool check_team_balance();
	bool can_change_team(PLAYER *pplayer, int jointeam);
	int clampteam(int team);

	virtual void post_reset();

	virtual bool get_door(int idx);
	virtual void set_door(int idx, bool active);
	virtual bool is_race() const;
};

#endif