#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.hpp"
//account include
#include "account.hpp"

// player object
class PLAYER
{
	MACRO_ALLOC_POOL_ID()
private:
	CHARACTER *character;
public:
	PLAYER(int client_id, int args[3]);
	~PLAYER();

	PLAYER_ACCOUNT *account;


	// TODO: clean this up
	int authed;
	int resistent;
	char skin_name[64];
	int use_custom_color;
	int color_body;
	int color_feet;
	int muted;
	int police;

	
	int respawn_tick;
	int die_tick;
	//
	bool spawning;
	int client_id;
	int team;
	int score;
	bool force_balanced;
	
	//
	int vote;
	int64 last_votecall;
	bool invisible;
	bool fly;
	bool god;
	bool rb;
	int rbc;
	bool emo0;
	bool emo1;
	bool emo2;
	bool emo3;
	bool emo4;
	int ninjalol;
	bool lolsplash;




	//---MONEY(STUFF)
		int money;
		int money_tick;
		int money_save;
	//---ITEMS
		int ak;
		int gun;
		int grenade;
		int shotgun;
		int hook;
		int jump;
		int vip;
		int wlist;
		int laser;
	// AREST
		int arest;
	//~~~MONEY&BUY~~~

	//
	int64 last_chat;
	int64 last_setteam;
	int64 last_changeinfo;
	int64 last_emote;
	int64 last_kill;

	// network latency calculations	
	struct
	{
		int accum;
		int accum_min;
		int accum_max;
		int avg;
		int min;
		int max;	
	} latency;
	
	// this is used for snapping so we know how we can clip the view for the player
	vec2 view_pos;

	void init(int client_id);
	
	CHARACTER *get_character();
	
	void kill_character(int weapon);

	void try_respawn();
	void respawn();
	void set_team(int team);
	
	void tick();
	void snap(int snapping_client);

	void on_direct_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_predicted_input(NETOBJ_PLAYER_INPUT *new_input);
	void on_disconnect();

	//race var
	int starttime;
	int refreshtime;
	int race_state;
	int besttick;
	int lasttick;
	float currentTime[79];
	float bestTime[79];
	float bestLap;

	int save_x;
	int save_y;
	int diff;

	/* Account data */
	bool logged_in;
	
	// data for database
	struct ACC_DATA
	{
		char name[32];
		char pass[32];
	};
	ACC_DATA acc_data;
};

#endif
