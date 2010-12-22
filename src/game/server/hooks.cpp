/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <base/math.hpp>
#include <game/server/gamecontext.hpp>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
extern "C"
{
	#include <engine/e_memheap.h>
}
#include <game/version.hpp>
#include <game/collision.hpp>
#include <game/layers.hpp>
#include <game/mapitems.hpp>
#include <game/gamecore.hpp>
#include <game/server/entities/character.hpp>

#include "chatcmds.hpp"
#include "policesystem.hpp"

#include "gamecontext.hpp"
#include "gamemodes/race.hpp"
#include "gamemodes/tdm.hpp"
#include "gamemodes/ctf.hpp"

TUNING_PARAMS tuning;

static void check_pure_tuning()
{
	// might not be created yet during start up
	if(!game.controller)
		return;
	
	if(	strcmp(game.controller->gametype, "DM")==0 ||
		strcmp(game.controller->gametype, "TDM")==0 ||
		strcmp(game.controller->gametype, "CTF")==0)
	{
		TUNING_PARAMS p;
		if(memcmp(&p, &tuning, sizeof(TUNING_PARAMS)) != 0)
		{
			dbg_msg("server", "resetting tuning due to pure server");
			tuning = p;

		}
	}	
}

struct VOTEOPTION
{
	VOTEOPTION *next;
	VOTEOPTION *prev;
	char command[1];
};

static HEAP *voteoption_heap = 0;
static VOTEOPTION *voteoption_first = 0;
static VOTEOPTION *voteoption_last = 0;

void send_tuning_params(int cid)
{
	check_pure_tuning();
	
	msg_pack_start(NETMSGTYPE_SV_TUNEPARAMS, MSGFLAG_VITAL);
	int *params = (int *)&tuning;
	for(unsigned i = 0; i < sizeof(tuning)/sizeof(int); i++)
		msg_pack_int(params[i]);
	msg_pack_end();
	server_send_msg(cid);
}

void give_ninja(int client_id)
{
	PLAYER *p = game.players[client_id];
	CHARACTER* chr = game.players[client_id]->get_character();
	p->ninjalol = 0;
	chr->ninja.activationtick = server_tick();
	chr->weapons[WEAPON_NINJA].got = true;
	chr->weapons[WEAPON_NINJA].ammo = -1;
	chr->last_weapon = chr->active_weapon;
	chr->active_weapon = WEAPON_NINJA;
	game.create_sound(chr->pos, SOUND_PICKUP_NINJA);
}

// Server hooks

void mods_client_direct_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_direct_input((NETOBJ_PLAYER_INPUT *)input);
}

void mods_set_authed(int client_id, int status)
{
	if(game.players[client_id])
		game.players[client_id]->authed = status;
}

void mods_set_resistent(int client_id, int status)
{
	if(game.players[client_id])
		game.players[client_id]->resistent = status;
}

void mods_client_predicted_input(int client_id, void *input)
{
	if(!game.world.paused)
		game.players[client_id]->on_predicted_input((NETOBJ_PLAYER_INPUT *)input);
}

// Server hooks
void mods_tick()
{
	check_pure_tuning();
	
	game.tick();

#ifdef CONF_DEBUG
	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			NETOBJ_PLAYER_INPUT input = {0};
			input.direction = (i&1)?-1:1;
			game.players[MAX_CLIENTS-i-1]->on_predicted_input(&input);
		}
	}
#endif
}

void mods_snap(int client_id)
{
	game.snap(client_id);
}

void mods_client_enter(int client_id)
{
	//game.world.insert_entity(&game.players[client_id]);
	game.players[client_id]->respawn();
	dbg_msg("game", "join player='%d:%s'", client_id, server_clientname(client_id));

	game.players[client_id]->score = 0;
	
	if(game.controller->is_race())
		((GAMECONTROLLER_RACE*)game.controller)->score.initPlayer(client_id);
	
	char buf[512];
	str_format(buf, sizeof(buf), "%s entered and joined the %s", server_clientname(client_id), game.controller->get_team_name(game.players[client_id]->team));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf); 
	
	char buf1[512];
	str_format(buf1, sizeof(buf1), "Welcome to this Server by %s", config.sv_serverby);

	game.send_chat_target(client_id, buf1);
	game.send_chat_target(client_id, "Write /info to learn more.");

	dbg_msg("game", "team_join player='%d:%s' team=%d", client_id, server_clientname(client_id), game.players[client_id]->team);

	game.create_explosion(vec2(game.players[client_id]->view_pos.x, game.players[client_id]->view_pos.y), -1, -1, true);

}

void mods_connected(int client_id)
{
	int tmp[3] = { -1, -1, -1 };

	PLAYER_SCORE *pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_score(client_id, 1, 0);
	if(pscore)
	{
		tmp[0] = pscore->pos_x;
		tmp[1] = pscore->pos_y;
		tmp[2] = pscore->diff;
	}

	game.players[client_id] = new(client_id) PLAYER(client_id, tmp);

	//game.players[client_id].init(client_id);
	//game.players[client_id].client_id = client_id;
	
	// Check which team the player should be on
	if(config.sv_tournament_mode)
		game.players[client_id]->team = -1;
	else
		game.players[client_id]->team = game.controller->get_auto_team(client_id);
		
	(void) game.controller->check_team_balance();

	// send motd
	NETMSG_SV_MOTD msg;
	msg.message = config.sv_motd;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(client_id);
}

void mods_client_drop(int client_id)
{
	game.abort_vote_kick_on_disconnect(client_id);
	
	/* Account */
	// update accfile
	if(game.players[client_id]->logged_in)
	{
		game.players[client_id]->account->update();
		game.players[client_id]->account->reset();
	}
	
	game.players[client_id]->on_disconnect();
	delete game.players[client_id];
	game.players[client_id] = 0;
	
	(void) game.controller->check_team_balance();
}

/*static bool is_separator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

static const char *liststr_find(const char *str, const char *needle)
{
	int needle_len = strlen(needle);
	while(*str)
	{
		int wordlen = 0;
		while(str[wordlen] && !is_separator(str[wordlen]))
			wordlen++;
		
		if(wordlen == needle_len && strncmp(str, needle, needle_len) == 0)
			return str;
		
		str += wordlen+1;
	}
	
	return 0;
}*/

void mods_message(int msgtype, int client_id)
{
	void *rawmsg = netmsg_secure_unpack(msgtype);
	PLAYER *p = game.players[client_id];

	if(!rawmsg)
	{
		dbg_msg("server", "dropped weird message '%s' (%d), failed on '%s'", netmsg_get_name(msgtype), msgtype, netmsg_failed_on());
		return;
	}
	
	if(msgtype == NETMSGTYPE_CL_SAY)
	{
		NETMSG_CL_SAY *msg = (NETMSG_CL_SAY *)rawmsg;
		if(config.sv_log_chat)
		{
			dbg_msg("Log:Chat", "SAY '%s' from '%s' CID(%i)", msg->message, game.players[client_id]->acc_data.name,client_id);
		}
		int team = msg->team;
		if(team)
			team = p->team;
		else
			team = GAMECONTEXT::CHAT_ALL;

		if(config.sv_spamprotection && p->last_chat+time_freq() > time_get())
			game.players[client_id]->last_chat = time_get();
	
		else if(p->muted>0)
		{
			int time;
			time = p->muted;
		// Coded by BotoX
			if(time >= 60*server_tickspeed())
			{
				int sec;
				sec = time;
			
				while(sec >= 60*server_tickspeed())
					sec -= 60*server_tickspeed();

				time = time/server_tickspeed();
				time = time/60;

				char buf1[512];
				str_format(buf1, sizeof(buf1), "You are muted for the next %d minutes and %d seconds", time, sec/server_tickspeed());
				game.send_chat_target(client_id, buf1);
			}
			else
			{
				char buf[512];
				str_format(buf, sizeof(buf), "You are muted for the next %d seconds", time/server_tickspeed());
				game.send_chat_target(client_id, buf);
			}
		// Coded by BotoX
		}
		else
 		{
			if(msg->message[0] == '.' || msg->message[0] == '!' || msg->message[0] == '/' || msg->message[0] == '+')
			{
				if(!str_comp_nocase(msg->message, ".info") || !str_comp_nocase(msg->message, "!info") || !str_comp_nocase(msg->message, "/info") || !str_comp_nocase(msg->message, "+info"))
				{					
					game.send_chat_target(client_id, " # CCity mod by Wolf");
					game.send_chat_target(client_id, " # Baseing on [N]City");
					game.send_chat_target(client_id, " # Visit Game-Generation.org");
					game.send_chat_target(client_id, " # Visit mikey-tw.tk");
					game.send_chat_target(client_id, " # Visit Teevision.de");
					game.send_chat_target(client_id, " # /cmdlist for all Commands!");
				}

				if(!str_comp_nocase(msg->message, ".cmdlist") || !str_comp_nocase(msg->message, "!cmdlist") || !str_comp_nocase(msg->message, "/cmdlist") || !str_comp_nocase(msg->message, "+cmdlist"))
				{
					game.send_chat_target(client_id, " /info -- Information about MOD");
					game.send_chat_target(client_id, " /top5 -- Toplist");
					game.send_chat_target(client_id, " /rank -- Your Rank");
					game.send_chat_target(client_id, " /account -- Account Commandlist");
					if(p->wlist)
					{
						game.send_chat_target(client_id, " /save -- Save Position");
						game.send_chat_target(client_id, " /load -- Load Position");
						game.send_chat_target(client_id, " /left, /right, /up, /down -- Move through Walls");
						game.send_chat_target(client_id, " /weapons -- Get Grenade & Shotgun");
						game.send_chat_target(client_id, " /ninja -- Get Ninja");
					}
					if(p->authed)
					{
						game.send_chat_target(client_id, " /fly -- Fly with Jumps");
						game.send_chat_target(client_id, " /invisible -- Invisible");
						game.send_chat_target(client_id, " /god -- God Mode");
						game.send_chat_target(client_id, " /emote -- Emotes");
					}
					if(p->police)
					{
						game.send_chat_target(client_id, " /idlist -- View ID List");
						game.send_chat_target(client_id, " /arrest <ID> -- Arest player with ID <ID>");
						game.send_chat_target(client_id, " /kill <ID> -- Kill player with ID <ID>");
						game.send_chat_target(client_id, " /kick <ID> -- Kick player with ID <ID>");
					}
				}
				if(!strncmp(msg->message, "/", 1))
				{
					chat_command(client_id, msg, game.players[client_id]);

					if(game.players[client_id]->authed || game.players[client_id]->police){
						police_command(client_id, msg, game.players[client_id]);
					}
				}
				if(game.players[client_id]->team == -1)
				{
					game.send_chat_target(client_id, "+bf001!");
					return;
				}
				// ACCOUNT + POLICE SYSTEM

				else if((!str_comp_nocase(msg->message, ".left") || !str_comp_nocase(msg->message, "!left") || !str_comp_nocase(msg->message, "/left") || !str_comp_nocase(msg->message, "+left")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest)
					{
						if(!p->authed && col_get_antisave((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money20((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money50((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y))
						{
							game.send_chat_target(client_id, "You can't do this here!");
							return;
						}
						CHARACTER* chr = game.players[client_id]->get_character();
						if(chr)
						{
							chr->core.pos.x-=32;
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}
				else if((!str_comp_nocase(msg->message, ".right") || !str_comp_nocase(msg->message, "!right") || !str_comp_nocase(msg->message, "/right") || !str_comp_nocase(msg->message, "+right")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest)
					{
						if(!p->authed && col_get_antisave((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money20((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money50((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y))
						{
							game.send_chat_target(client_id, "You can't do this here!");
							return;
						}
						CHARACTER* chr = game.players[client_id]->get_character();
						if(chr)
						{
							chr->core.pos.x+=32;
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}
				else if((!str_comp_nocase(msg->message, ".ninja") || !str_comp_nocase(msg->message, "!ninja") || !str_comp_nocase(msg->message, "/ninja") || !str_comp_nocase(msg->message, "+ninja")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest)
					{
						if(p->authed)
						{
							game.send_chat_target(client_id, "Please use /weapons!");
						}
						else
						{
							give_ninja(client_id);
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}
				else if((!str_comp_nocase(msg->message, ".up") || !str_comp_nocase(msg->message, "!up") || !str_comp_nocase(msg->message, "/up") || !str_comp_nocase(msg->message, "+up")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest)
					{
						if(!p->authed && col_get_antisave((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money20((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money50((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y))
						{
							game.send_chat_target(client_id, "You can't do this here!");
							return;
						}
						CHARACTER* chr = game.players[client_id]->get_character();
						if(chr)
						{
							chr->core.pos.y-=32;
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}
				else if((!str_comp_nocase(msg->message, ".down") || !str_comp_nocase(msg->message, "!down") || !str_comp_nocase(msg->message, "/down") || !str_comp_nocase(msg->message, "+down")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest)
					{
						if(!p->authed && col_get_antisave((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money20((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || !p->authed && col_get_money50((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y))
						{
							game.send_chat_target(client_id, "You can't do this here!");
							return;
						}
						CHARACTER* chr = game.players[client_id]->get_character();
						if(chr)
						{
							chr->core.pos.y+=32;
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}

				else if((!str_comp_nocase(msg->message, ".invisible") || !str_comp_nocase(msg->message, "!invisible") || !str_comp_nocase(msg->message, "/invisible") || !str_comp_nocase(msg->message, "+invisible")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->invisible == true)
							{
								game.players[client_id]->invisible = false;
								game.send_chat_target(client_id, "Invisiblity deactivated");
							}

							else	
							{
								game.players[client_id]->invisible = true;
								game.send_chat_target(client_id, "Invisiblity activated");
							}
						return;
					}
				}
				
				else if((!str_comp_nocase(msg->message, ".fly") || !str_comp_nocase(msg->message, "!fly") || !str_comp_nocase(msg->message, "/fly") || !str_comp_nocase(msg->message, "+fly")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->fly == true)
							{
								game.players[client_id]->fly = false;
								game.send_chat_target(client_id, "Flying deactivated");
							}

							else	
							{
								game.players[client_id]->fly = true;
								game.send_chat_target(client_id, "Flying activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".god") || !str_comp_nocase(msg->message, "!god") || !str_comp_nocase(msg->message, "/god") || !str_comp_nocase(msg->message, "+god")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->god == true)
							{
								game.players[client_id]->god = false;
								game.send_chat_target(client_id, "God Mode deactivated");
							}

							else	
							{
								game.players[client_id]->god = true;
								game.send_chat_target(client_id, "God Mode activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".rainbow") || !str_comp_nocase(msg->message, "!rainbow") || !str_comp_nocase(msg->message, "/rainbow") || !str_comp_nocase(msg->message, "+rainbow")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if(config.sv_rainbow_admin==1)
						{
							if (game.players[client_id]->rb == true)
								{
									game.players[client_id]->rb = false;
									game.send_chat_target(client_id, "Rainbow deactivated");
								}

								else	
								{
									game.players[client_id]->rb = true;
									game.send_chat_target(client_id, "Rainbow activated");
								}
							return;
						}
						else
							game.send_chat_target(client_id, "Rainbow function deactivated!");
					}
				}					

				else if((!str_comp_nocase(msg->message, ".splash") || !str_comp_nocase(msg->message, "!splash") || !str_comp_nocase(msg->message, "/splash") || !str_comp_nocase(msg->message, "+splash")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->lolsplash == true)
							{
								game.players[client_id]->lolsplash = false;
								game.send_chat_target(client_id, "Funny Splash deactivated");
							}

							else	
							{
								game.players[client_id]->lolsplash = true;
								game.send_chat_target(client_id, "Funny Splash activated");
							}
							return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".emote") || !str_comp_nocase(msg->message, "!emote") || !str_comp_nocase(msg->message, "/emote") || !str_comp_nocase(msg->message, "+god")) && game.controller->is_race())
				{
					if(p->authed)
					{
						game.send_chat_target(client_id, "/pain -- Emote Pain");
						game.send_chat_target(client_id, "/happy -- Emote Happy");
						game.send_chat_target(client_id, "/surprise -- Emote Surprise");
						game.send_chat_target(client_id, "/angry -- Emote Angry");
						game.send_chat_target(client_id, "/blink -- Emote Blink");
					}
				}

				else if((!str_comp_nocase(msg->message, ".pain") || !str_comp_nocase(msg->message, "!pain") || !str_comp_nocase(msg->message, "/pain") || !str_comp_nocase(msg->message, "+pain")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->emo0 == true)
							{
								game.players[client_id]->emo0 = false;
								game.send_chat_target(client_id, "Pain Emote deactivated");
							}

							else	
							{
								game.players[client_id]->emo1 = false;
								game.players[client_id]->emo2 = false;
								game.players[client_id]->emo3 = false;
								game.players[client_id]->emo4 = false;
								game.players[client_id]->emo0 = true;
								game.send_chat_target(client_id, "Pain Emote activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".happy") || !str_comp_nocase(msg->message, "!happy") || !str_comp_nocase(msg->message, "/happy") || !str_comp_nocase(msg->message, "+happy")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->emo1 == true)
							{
								game.players[client_id]->emo1 = false;
								game.send_chat_target(client_id, "Happy Emote deactivated");
							}

							else	
							{
								game.players[client_id]->emo0 = false;
								game.players[client_id]->emo2 = false;
								game.players[client_id]->emo3 = false;
								game.players[client_id]->emo4 = false;
								game.players[client_id]->emo1 = true;
								game.send_chat_target(client_id, "Happy Emote activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".surprise") || !str_comp_nocase(msg->message, "!surprise") || !str_comp_nocase(msg->message, "/surprise") || !str_comp_nocase(msg->message, "+surprise")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->emo2 == true)
							{
								game.players[client_id]->emo2 = false;
								game.send_chat_target(client_id, "Surprise Emote deactivated");
							}

							else	
							{
								game.players[client_id]->emo1 = false;
								game.players[client_id]->emo0 = false;
								game.players[client_id]->emo3 = false;
								game.players[client_id]->emo4 = false;
								game.players[client_id]->emo2 = true;
								game.send_chat_target(client_id, "Surprise Emote activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".angry") || !str_comp_nocase(msg->message, "!angry") || !str_comp_nocase(msg->message, "/angry") || !str_comp_nocase(msg->message, "+angry")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->emo3 == true)
							{
								game.players[client_id]->emo3 = false;
								game.send_chat_target(client_id, "Angry Emote deactivated");
							}

							else	
							{
								game.players[client_id]->emo1 = false;
								game.players[client_id]->emo2 = false;
								game.players[client_id]->emo0 = false;
								game.players[client_id]->emo4 = false;
								game.players[client_id]->emo3 = true;
								game.send_chat_target(client_id, "Angry Emote activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".blink") || !str_comp_nocase(msg->message, "!blink") || !str_comp_nocase(msg->message, "/blink") || !str_comp_nocase(msg->message, "+blink")) && game.controller->is_race())
				{
					if(p->authed)
					{
						if (game.players[client_id]->emo4 == true)
							{
								game.players[client_id]->emo4 = false;
								game.send_chat_target(client_id, "Blink Emote deactivated");
							}

							else	
							{
								game.players[client_id]->emo1 = false;
								game.players[client_id]->emo2 = false;
								game.players[client_id]->emo3 = false;
								game.players[client_id]->emo0 = false;
								game.players[client_id]->emo4 = true;
								game.send_chat_target(client_id, "Blink Emote activated");
							}
						return;
					}
				}

				else if((!str_comp_nocase(msg->message, ".weapons") || !str_comp_nocase(msg->message, "!weapons") || !str_comp_nocase(msg->message, "/weapons") || !str_comp_nocase(msg->message, "+weapons")) && game.controller->is_race())
				{
					if((p->wlist || game.players[client_id]->authed) && !p->arest )
					{
						game.players[client_id]->get_character()->weapons[WEAPON_HAMMER].got = true;
						game.players[client_id]->get_character()->weapons[WEAPON_HAMMER].ammo = -1;
						game.players[client_id]->get_character()->weapons[WEAPON_GUN].got = true;
						game.players[client_id]->get_character()->weapons[WEAPON_GUN].ammo = 10;
						game.players[client_id]->get_character()->weapons[WEAPON_SHOTGUN].got = true;
						game.players[client_id]->get_character()->weapons[WEAPON_SHOTGUN].ammo = 10;
						game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].got = false;
						game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].ammo = 10;
						game.players[client_id]->get_character()->weapons[WEAPON_GRENADE].got = true;
						game.players[client_id]->get_character()->weapons[WEAPON_GRENADE].ammo = 10;
						game.players[client_id]->get_character()->weapons[WEAPON_NINJA].ammo = -1;

						if(p->authed || p->laser)
						{
							game.players[client_id]->get_character()->weapons[WEAPON_NINJA].got = true;
							game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].got = true;
						}
						
						game.send_chat_target(client_id, "Now you have all Weapons :D");
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}
				
				// Written by Alexander Becker
				else if((!str_comp_nocase(msg->message, ".buy") || !str_comp_nocase(msg->message, "!buy") || !str_comp_nocase(msg->message, "/buy") || !str_comp_nocase(msg->message, "+buy")) && game.controller->is_race())
				{
					int temp;
					temp = 0;					
					
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0){
						char buf[512];
						str_format(buf, sizeof(buf), "/drink -- (+1 Health)  [%d$]", config.sv_price_drink);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/energy -- (+2 Health)  [%d$]", config.sv_price_energy);
						game.send_chat_target(client_id, buf); 
						str_format(buf, sizeof(buf), "/burger -- (+5 Health)  [%d$]", config.sv_price_burger);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/hotdog -- (+5 Health)  [%d$]", config.sv_price_hotdog);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/kaviar -- (+10 Health, +10 Armor)  [%d$]", config.sv_price_kaviar);
						game.send_chat_target(client_id, buf);
						/* OLD SYSTEM :
						game.send_chat_target(client_id, "/drink -- Buy Drink(+1 Health)[10$]");
						game.send_chat_target(client_id, "/energy -- Buy Energy Drink(+2 Health)[20$]");
						game.send_chat_target(client_id, "/burger -- Buy Burger(+5 Health)[50$]");
						game.send_chat_target(client_id, "/hotdog -- Buy Hotdog(+5 Health)[50$]");
						game.send_chat_target(client_id, "/kaviar -- Buy Kaviar(+10 Health, +10 Armor)[500$]");
						*/
						temp += 1;
					}
										
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0){	
						char buf[512];
						str_format(buf, sizeof(buf), "/hook -- Endless Player Hook [%d$]", config.sv_price_hook);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/gun -- Gun Spread [%d$]", config.sv_price_gun);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/gunauto -- Gun Autoshot [%d$]", config.sv_price_gunauto);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/grenade -- Shot 3 Grenades [%d$]", config.sv_price_grenade);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/shotgun -- Shotgun Explode [%d$]", config.sv_price_shotgun);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/jump -- Infinite Jump [%d$]", config.sv_price_jump);
						game.send_chat_target(client_id, buf);
						str_format(buf, sizeof(buf), "/vip -- Get VIP [%d$]", config.sv_price_vip);
						game.send_chat_target(client_id, buf);
						/* OLD SYSTEM :
						game.send_chat_target(client_id, "/hook -- Endless Player Hook [5.000$]");
						game.send_chat_target(client_id, "/gun -- Gun Spread [7.000$]");
						game.send_chat_target(client_id, "/gunauto -- Gun Autoshot [8.000$]");
						game.send_chat_target(client_id, "/grenade -- Shot 3 Grenades [10.000$]");
						game.send_chat_target(client_id, "/shotgun -- Shotgun Explode [10.000$]");
						game.send_chat_target(client_id, "/jump -- Infinite Jump [10.000$]");
						game.send_chat_target(client_id, "/vip -- Get VIP [15.000$]");
						*/
						if(config.sv_wlist==1)
						//	game.send_chat_target(client_id, "/wlist -- Get Whitelist [100.000$]");
							str_format(buf, sizeof(buf), "/wlist -- Get Whitelist [%d$]", config.sv_price_wlist);
							game.send_chat_target(client_id, buf);

					//	game.send_chat_target(client_id, "/laser -- Activate Laser [100.000$]");
						str_format(buf, sizeof(buf), "/laser -- Activate Laser [%d$]", config.sv_price_laser);
						game.send_chat_target(client_id, buf);
						temp += 1;
					}

					if (temp == 0) {
						game.send_chat_target(client_id, "Enter Restaurant or Shop first!");
					}
					
				}

				else if((!str_comp_nocase(msg->message, ".drink") || !str_comp_nocase(msg->message, "!drink") || !str_comp_nocase(msg->message, "/drink") || !str_comp_nocase(msg->message, "+drink")) && game.controller->is_race())
				{
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0)
					{
						int price;
						price = config.sv_price_drink;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Drink");
							
							CHARACTER* chr = game.players[client_id]->get_character();
							chr->health += 1;
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Restaurant first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".energy") || !str_comp_nocase(msg->message, "!energy") || !str_comp_nocase(msg->message, "/energy") || !str_comp_nocase(msg->message, "+energy")) && game.controller->is_race())
				{
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0)
					{
						int price;
						price = config.sv_price_energy;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Energy Drink");
							
							CHARACTER* chr = game.players[client_id]->get_character();
							chr->health += 2;
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Restaurant first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".burger") || !str_comp_nocase(msg->message, "!burger") || !str_comp_nocase(msg->message, "/burger") || !str_comp_nocase(msg->message, "+burger")) && game.controller->is_race())
				{
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0)
					{
						int price;
						price = config.sv_price_burger;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Burger");
							
							CHARACTER* chr = game.players[client_id]->get_character();
							chr->health += 5;
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Restaurant first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".hotdog") || !str_comp_nocase(msg->message, "!hotdog") || !str_comp_nocase(msg->message, "/hotdog") || !str_comp_nocase(msg->message, "+hotdog")) && game.controller->is_race())
				{
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0)
					{
						int price;
						price = config.sv_price_hotdog;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Hotdog");
							
							CHARACTER* chr = game.players[client_id]->get_character();
							chr->health += 5;
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Restaurant first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".kaviar") || !str_comp_nocase(msg->message, "!kaviar") || !str_comp_nocase(msg->message, "/kaviar") || !str_comp_nocase(msg->message, "+kaviar")) && game.controller->is_race())
				{
					if(col_get_food((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_restaurant == 0)
					{
						int price;
						price = config.sv_price_kaviar;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Kaviar");
							
							CHARACTER* chr = game.players[client_id]->get_character();
							chr->health += 10;
							chr->armor += 10;
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Restaurant first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".gunauto") || !str_comp_nocase(msg->message, "!gunauto") || !str_comp_nocase(msg->message, "/gunauto") || !str_comp_nocase(msg->message, "+gunauto")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_gunauto;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Gun Autoshot");
							p->ak = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".gun") || !str_comp_nocase(msg->message, "!gun") || !str_comp_nocase(msg->message, "/gun") || !str_comp_nocase(msg->message, "+gun")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{

						int price;
						price = config.sv_price_gun;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Gun Spread");
							p->gun = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".grenade") || !str_comp_nocase(msg->message, "!grenade") || !str_comp_nocase(msg->message, "/grenade") || !str_comp_nocase(msg->message, "+grenade")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_grenade;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Shot 3 Grenades");
							p->grenade = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".shotgun") || !str_comp_nocase(msg->message, "!shotgun") || !str_comp_nocase(msg->message, "/shotgun") || !str_comp_nocase(msg->message, "+shotgun")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_shotgun;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Shotgun Explode");
							p->shotgun = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".hook") || !str_comp_nocase(msg->message, "!hook") || !str_comp_nocase(msg->message, "/hook") || !str_comp_nocase(msg->message, "+hook")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_hook;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Endless Player Hook");
							p->hook = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".jump") || !str_comp_nocase(msg->message, "!jump") || !str_comp_nocase(msg->message, "/jump") || !str_comp_nocase(msg->message, "+jump")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_jump;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Infinite Jump");
							p->jump = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".vip") || !str_comp_nocase(msg->message, "!vip") || !str_comp_nocase(msg->message, "/vip") || !str_comp_nocase(msg->message, "+vip")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_vip;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: VIP");
							p->vip = 1;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!str_comp_nocase(msg->message, ".wlist") || !str_comp_nocase(msg->message, "!wlist") || !str_comp_nocase(msg->message, "/wlist") || !str_comp_nocase(msg->message, "+wlist")) && game.controller->is_race())
				{
					if(config.sv_wlist==1)
					{
						if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
						{
							int price;
							price = config.sv_price_wlist;
							if(p->money >= price)
							{
								p->money -= price;
								game.send_chat_target(client_id, "Bought: Whitelist");
								p->wlist = 1;
								p->account->update();
							}
							else
								game.send_chat_target(client_id, "Not enough Money");
						}
						else
						{					
							game.send_chat_target(client_id, "Enter Shop first!");
						}
					}
				}

				else if((!str_comp_nocase(msg->message, ".laser") || !str_comp_nocase(msg->message, "!laser") || !str_comp_nocase(msg->message, "/laser") || !str_comp_nocase(msg->message, "+laser")) && game.controller->is_race())
				{
					if(col_get_shop((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || config.sv_buy_shop == 0)
					{
						int price;
						price = config.sv_price_laser;
						if(p->money >= price)
						{
							p->money -= price;
							game.send_chat_target(client_id, "Bought: Laser");
							p->laser = 1;
							game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].got = true;
							game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].ammo = 10;
							p->account->update();
						}
						else
							game.send_chat_target(client_id, "Not enough Money");
					}
					else
					{					
						game.send_chat_target(client_id, "Enter Shop first!");
					}
				}

				else if((!strncmp(msg->message, ".top5", 5) || !strncmp(msg->message, "!top5", 5) || !strncmp(msg->message, "/top5", 5)) && game.controller->is_race())
				{
					const char *pt = msg->message;
					int number = 0;
					pt += 6;
					while(*pt && *pt >= '0' && *pt <= '9')
					{
						number = number*10+(*pt-'0');
						pt++;
					}
					if(number)
						((GAMECONTROLLER_RACE*)game.controller)->score.top5_draw(client_id, number);
					else
						((GAMECONTROLLER_RACE*)game.controller)->score.top5_draw(client_id, 0);
				}

				else if((!str_comp_nocase(msg->message, ".rank") || !str_comp_nocase(msg->message, "!rank") || !str_comp_nocase(msg->message, "/rank")) && game.controller->is_race())
				{
					char buf[512];
					const char *name = msg->message;
					name += 6;
					int pos;
					PLAYER_SCORE *pscore;
					
					if(!strcmp(msg->message, "/rank"))
						pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_score(client_id, 1, &pos);
					else
						pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_name(name, &pos, 1);

					if(pscore && pos > -1 && pscore->score != -1)
					{
						float time = pscore->score;

						str_format(buf, sizeof(buf), "Rank: %d (%s)", pos, pscore->name);
						game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);

						if ((int)time/60 >= 1)
							str_format(buf, sizeof(buf), "Time: %d minute%s %f seconds", (int)time/60, (int)time/60 != 1 ? "s" : "" , time-((int)time/60)*60);
						else
							str_format(buf, sizeof(buf), "Time: %f seconds", time-((int)time/60)*60);
						game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);

						if ((int)(pos - 3) >= 5)
							str_format(buf, sizeof(buf), "Top5: '/top5 %d'!", (pos - 3));
						else
							str_format(buf, sizeof(buf), "Top5: '/top5'!");

						game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
						game.players[client_id]->last_chat = time_get()+time_freq()*3;
						return;
					}
					else if(pos == -1)
						str_format(buf, sizeof(buf), "Several players were found.");
					else
						str_format(buf, sizeof(buf), "%s is not ranked", strcmp(msg->message, "/rank")?name:server_clientname(client_id));

					game.send_chat_target(client_id, buf);
				}
				else if((!str_comp_nocase(msg->message, ".save") || !str_comp_nocase(msg->message, "!save") || !str_comp_nocase(msg->message, "/save") || !str_comp_nocase(msg->message, "+save")) && game.controller->is_race())
				{
					if (p->wlist)
					{
						if(col_get_money50((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || col_get_money20((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y) || col_get_antisave((int)game.players[client_id]->view_pos.x, (int)game.players[client_id]->view_pos.y)){				
							game.send_chat_target(client_id, "You can't save here!");
						}
						else
						{
							int checker;
							checker = game.players[client_id]->team;
							if(checker==-1)
							{
								game.send_chat_target(client_id, "Join Game to save Position!");
							}
							else
							{
								CHARACTER* chr = game.players[client_id]->get_character();
								int _x = game.players[client_id]->view_pos.x;
								int _y = game.players[client_id]->view_pos.y;
								int _diff = -1;
								if (game.players[client_id]->team != -1 && chr && chr->race_state != RACE_NONE)
									_diff = server_tick() - chr->starttime;

  								game.players[client_id]->save_x = _x;
  								game.players[client_id]->save_y = _y;
								game.players[client_id]->diff = _diff;

								((GAMECONTROLLER_RACE*)game.controller)->score.wlist_parsePlayer(client_id, _x, _y, _diff);
								game.send_chat_target(client_id, "Position saved!");
							}
						}
					}
					else
						game.send_chat_target(client_id, "Not whitelisted!");
				}
				
				else if((!str_comp_nocase(msg->message, ".load") || !str_comp_nocase(msg->message, "!load") || !str_comp_nocase(msg->message, "/load") || !str_comp_nocase(msg->message, "+load")) && game.controller->is_race())
				{
					if (p->wlist && !p->arest)
					{
  						CHARACTER* chr = game.players[client_id]->get_character();
  						int save_x = game.players[client_id]->save_x;
  						int save_y = game.players[client_id]->save_y;
		  
  						if(chr && save_x != -1 && save_y != -1)
  						{
  							if(config.sv_load_end || game.players[client_id]->diff <= 0)
  								chr->race_state = RACE_NONE;
							else
								chr->starttime = server_tick()-game.players[client_id]->diff;

  							chr->core.pos.x = save_x;
  							chr->core.pos.y = save_y;
  							game.send_chat_target(client_id, "Position loadet.");
							
							if(p->authed)
							{
								// do nothing ;)
							}
							else
							{
								game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].got = false;
								
								if(game.players[client_id]->get_character()->active_weapon==WEAPON_RIFLE)
								{
									game.players[client_id]->get_character()->active_weapon = WEAPON_GUN;
									game.players[client_id]->get_character()->weapons[WEAPON_RIFLE].got = false;
									game.players[client_id]->get_character()->active_weapon = WEAPON_GUN;
								}
							}

  						} else
  							game.send_chat_target(client_id, "No position setted.");
					}
					else
						game.send_chat_target(client_id, "Not whitelisted! (or in arest)");
				}

			}
			else
			{			
					game.players[client_id]->last_chat = time_get();
					game.send_chat(client_id, team, msg->message);
			}
		}	
	}

	else if(msgtype == NETMSGTYPE_CL_CALLVOTE)
		{
		int64 now = time_get();
		if(game.vote_closetime)
		{
			game.send_chat_target(client_id, "Wait for current vote to end before calling a new one.");
			return;
		}
		
		int64 timeleft = p->last_votecall + time_freq()*60 - now;
		if(timeleft > 0)
		{
			char chatmsg[512] = {0};
			str_format(chatmsg, sizeof(chatmsg), "You must wait %d seconds before making another vote", (timeleft/time_freq())+1);
			game.send_chat_target(client_id, chatmsg);
			return;
		}
		
		char chatmsg[512] = {0};
		char desc[512] = {0};
		char cmd[512] = {0};
		NETMSG_CL_CALLVOTE *msg = (NETMSG_CL_CALLVOTE *)rawmsg;
		if(str_comp_nocase(msg->type, "option") == 0)
		{
			VOTEOPTION *option = voteoption_first;
			while(option)
			{
				if(str_comp_nocase(msg->value, option->command) == 0)
				{
					str_format(chatmsg, sizeof(chatmsg), "%s called vote to change server option '%s'", server_clientname(client_id), option->command);
					str_format(desc, sizeof(desc), "%s", option->command);
					str_format(cmd, sizeof(cmd), "%s", option->command);
					break;
				}

				option = option->next;
			}
			
			if(!option)
			{
				str_format(chatmsg, sizeof(chatmsg), "'%s' isn't an option on this server", msg->value);
				game.send_chat_target(client_id, chatmsg);
				return;
			}
		}
		else if(str_comp_nocase(msg->type, "kick") == 0)
		{
			if(!config.sv_vote_kick)
			{
				game.send_chat_target(client_id, "Server does not allow voting to kick players");
				return;
			}
			int kick_id = atoi(msg->value);
			if(kick_id < 0 || kick_id >= MAX_CLIENTS || !game.players[kick_id])
			{
				game.send_chat_target(client_id, "Invalid client id to kick");
				return;
			}
			if(game.players[kick_id]->authed && !config.sv_kick_admin)
			{
				game.send_chat_target(client_id, "Server does not allow voting to kick admins");
				return;
			}
			if(game.players[kick_id]->resistent && !config.sv_kick_resistent)
			{
				game.send_chat_target(client_id, "Server does not allow voting to kick resistent players");
				return;
			}
			if(game.players[kick_id]->wlist && !config.sv_kick_whitelisted)
			{
				game.send_chat_target(client_id, "User is whitelisted!");
				return;
			}
			if(game.players[kick_id]->police && !config.sv_kick_police)
			{
				game.send_chat_target(client_id, "User is Police!");
				return;
			}
			str_format(chatmsg, sizeof(chatmsg), "%s called for vote to kick '%s'", server_clientname(client_id), server_clientname(kick_id));
			str_format(desc, sizeof(desc), "Kick '%s'", server_clientname(kick_id));
			str_format(cmd, sizeof(cmd), "kick %d", kick_id);
			if (!config.sv_vote_kick_bantime)
				str_format(cmd, sizeof(cmd), "kick %d", kick_id);
			else
				str_format(cmd, sizeof(cmd), "ban %d %d", kick_id, config.sv_vote_kick_bantime);
		}
		
		if(cmd[0])
		{
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chatmsg);
			game.start_vote(desc, cmd);
			p->vote = 1;
			game.vote_creator = client_id;
			p->last_votecall = now;
			game.send_vote_status(-1);
		}
	}
	else if(msgtype == NETMSGTYPE_CL_VOTE)
	{
		if(!game.vote_closetime)
			return;
	
		if(p->vote == 0)
		{
			NETMSG_CL_VOTE *msg = (NETMSG_CL_VOTE *)rawmsg;
			p->vote = msg->vote;
			game.send_vote_status(-1);
			
			char buf[512];
			if(msg->vote==-1)
				str_format(buf, sizeof(buf), "%s votes no", server_clientname(client_id));
			if(msg->vote==1)
				str_format(buf, sizeof(buf), "%s votes yes", server_clientname(client_id));
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);

		}
	}
	else if (msgtype == NETMSGTYPE_CL_SETTEAM && !game.world.paused)
	{
		NETMSG_CL_SETTEAM *msg = (NETMSG_CL_SETTEAM *)rawmsg;
		
		if(config.sv_spamprotection && p->last_setteam+time_freq()*3 > time_get())
			return;

		// Switch team on given client and kill/respawn him
		if(game.controller->can_join_team(msg->team, client_id))
		{
			if(game.controller->can_change_team(p, msg->team))
			{
				p->last_setteam = time_get();
				p->set_team(msg->team);
				(void) game.controller->check_team_balance();
			}
			else
				game.send_broadcast("Teams must be balanced, please join other team", client_id);
		}
		else
		{
			char buf[128];
			str_format(buf, sizeof(buf), "Only %d active players are allowed", config.sv_max_clients-config.sv_spectator_slots);
			game.send_broadcast(buf, client_id);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_CHANGEINFO || msgtype == NETMSGTYPE_CL_STARTINFO)
	{
		NETMSG_CL_CHANGEINFO *msg = (NETMSG_CL_CHANGEINFO *)rawmsg;
		
		if(config.sv_spamprotection && p->last_changeinfo+time_freq()*5 > time_get())
			return;
			
		p->last_changeinfo = time_get();
		
		p->use_custom_color = msg->use_custom_color;
		p->color_body = msg->color_body;
		p->color_feet = msg->color_feet;

		// check for invalid chars
		unsigned char *name = (unsigned char *)msg->name;
		while (*name)
		{
			if(*name < 32)
				*name = ' ';
			name++;
		}

		// copy old name
		char oldname[MAX_NAME_LENGTH];
		str_copy(oldname, server_clientname(client_id), MAX_NAME_LENGTH);
		
		server_setclientname(client_id, msg->name);
		if(msgtype == NETMSGTYPE_CL_CHANGEINFO && strcmp(oldname, server_clientname(client_id)) != 0)
		{
			char chattext[256];
			str_format(chattext, sizeof(chattext), "%s changed name to %s", oldname, server_clientname(client_id));
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, chattext);
		}
		
		// set skin
		str_copy(p->skin_name, msg->skin, sizeof(p->skin_name));
		
		game.controller->on_player_info_change(p);
		
		if(msgtype == NETMSGTYPE_CL_STARTINFO)
		{
			// send vote options
			NETMSG_SV_VOTE_CLEAROPTIONS clearmsg;
			clearmsg.pack(MSGFLAG_VITAL);
			server_send_msg(client_id);
			VOTEOPTION *current = voteoption_first;
			while(current)
			{
				NETMSG_SV_VOTE_OPTION optionmsg;
				optionmsg.command = current->command;
				optionmsg.pack(MSGFLAG_VITAL);
				server_send_msg(client_id);
				current = current->next;
			}
			
			// send tuning parameters to client
			send_tuning_params(client_id);

			//
			NETMSG_SV_READYTOENTER m;
			m.pack(MSGFLAG_VITAL|MSGFLAG_FLUSH);
			server_send_msg(client_id);
		}
	}
	else if (msgtype == NETMSGTYPE_CL_EMOTICON && !game.world.paused)
	{
		NETMSG_CL_EMOTICON *msg = (NETMSG_CL_EMOTICON *)rawmsg;
		
		if(config.sv_spamprotection && p->last_emote+time_freq()*3 > time_get())
			return;
			
		p->last_emote = time_get();
		
		game.send_emoticon(client_id, msg->emoticon);
	}
	else if (msgtype == NETMSGTYPE_CL_KILL && !game.world.paused)
	{
		if(p->last_kill+time_freq()*3 > time_get())
			return;
		
		p->last_kill = time_get();
		p->kill_character(WEAPON_SELF);
		p->respawn_tick = server_tick()+server_tickspeed()*3;
		if(game.controller->is_race())
			p->respawn_tick = server_tick();

	}
}

static void con_tune_param(void *result, void *user_data)
{
	const char *param_name = console_arg_string(result, 0);
	float new_value = console_arg_float(result, 1);

	if(tuning.set(param_name, new_value))
	{
		dbg_msg("tuning", "%s changed to %.2f", param_name, new_value);
		send_tuning_params(-1);
	}
	else
		console_print("No such tuning parameter");
}

static void con_tune_reset(void *result, void *user_data)
{
	TUNING_PARAMS p;
	tuning = p;
	send_tuning_params(-1);
	console_print("tuning reset");
}

static void con_tune_dump(void *result, void *user_data)
{
	for(int i = 0; i < tuning.num(); i++)
	{
		float v;
		tuning.get(i, &v);
		dbg_msg("tuning", "%s %.2f", tuning.names[i], v);
	}
}


static void con_change_map(void *result, void *user_data)
{
	game.controller->change_map(console_arg_string(result, 0));
}

static void con_restart(void *result, void *user_data)
{
	if(console_arg_num(result))
		game.controller->do_warmup(console_arg_int(result, 0));
	else
		game.controller->startround();
}

static void con_broadcast(void *result, void *user_data)
{
	game.send_broadcast(console_arg_string(result, 0), -1);
}

static void con_say(void *result, void *user_data)
{
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, console_arg_string(result, 0));
}

static void con_say_by(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid])
		return;
	game.send_chat(cid, GAMECONTEXT::CHAT_ALL, console_arg_string(result, 1));
}

static void con_say_team_by(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid])
		return;
	game.send_chat(cid, game.players[cid]->team, console_arg_string(result, 1));
}

static void con_say_to(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
    if(!game.players[cid])
		return;
    game.send_chat_target(cid, console_arg_string(result, 1));
}

static void con_set_team(void *result, void *user_data)
{
	int client_id = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int team = clamp(console_arg_int(result, 1), -1, 1);
	
	dbg_msg("", "%d %d", client_id, team);
	
	if(!game.players[client_id])
		return;
	
	game.players[client_id]->set_team(team);
	(void) game.controller->check_team_balance();
}

static void con_kill_pl(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(!game.players[cid])
		return;
		
	game.players[cid]->kill_character(WEAPON_GAME);
	char buf[512];
	str_format(buf, sizeof(buf), "%s Killed by admin", server_clientname(cid));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}

static void con_kill_all(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	for (int i = 0; i < MAX_CLIENTS; i++)
		if(game.players[i])
			game.players[i]->kill_character(WEAPON_GAME);

	char buf[128];
	str_format(buf, sizeof(buf), "All players killed by admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}

static void con_ban_all(void *result, void *user_data)
{
	int min = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);

	if(!game.controller->is_race() && min > 0)
		return;
	
	char cmd[512]; 

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i] && !game.players[i]->wlist)
		{
			str_format(cmd, sizeof(cmd), "ban %d %d", i, min);
			console_execute_line(cmd);
		}
	}

	char buf[128];
	str_format(buf, sizeof(buf), "All players banned by admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}

static void con_kick_all(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	for (int i = 0; i < MAX_CLIENTS; i++)
		if(game.players[i] && !game.players[i]->wlist)
			server_kick(i, "You was kicked by 'kick_all'");

	char buf[128];
	str_format(buf, sizeof(buf), "All players are kicked.");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}

static void con_teleport_all(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);

	if(!game.controller->is_race() || !game.players[cid])
		return;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i])
		{
			CHARACTER* chr = game.players[i]->get_character();
			if(chr)
			{
				chr->core.pos = game.players[cid]->view_pos;
				chr->starttime = game.players[cid]->starttime;
			}
		}
	}

	char buf[128];
	str_format(buf, sizeof(buf), "All players teleported to player:");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
	str_format(buf, sizeof(buf), "'%s'", server_clientname(cid));
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
}

static void con_teleport(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
 	int cid1 = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
 	int cid2 = clamp(console_arg_int(result, 1), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid1] && game.players[cid2])
	{
		CHARACTER* chr = game.players[cid1]->get_character();
		if(chr)
		{
			chr->core.pos = game.players[cid2]->view_pos;
			chr->race_state = RACE_NONE;
		}
	}
}

static void con_teleport_to(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		CHARACTER* chr = game.players[cid]->get_character();
		if(chr)
		{
			chr->core.pos.x = console_arg_int(result, 1);
			chr->core.pos.y = console_arg_int(result, 2);
			chr->race_state = RACE_NONE;
		}
	}
}

static void con_teleport_num(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		CHARACTER* chr = game.players[cid]->get_character();
		int index = console_arg_int(result, 1)+16;

		if(chr && index > 0 && index < 96)
		{
			vec2 tmp = teleport(index);
			if(tmp == vec2(0, 0))
				return;

			chr->core.pos = tmp;
			chr->race_state = RACE_NONE;
		}
	}
}

static void con_switch_door(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	int index = console_arg_int(result, 0)-1;

	if (index > -1 && index < 16)
	{
		if(!game.controller->doors[index].valid)
		{
			dbg_msg("Door", "Door %d => invalid!", index+1);
			return;
		}

		game.controller->set_door(index, !game.controller->get_door(index));
		dbg_msg("Door", "Door %d => %s!", index+1, (game.controller->get_door(index) ? "locked" : "unlocked"));
	}
	else
		dbg_msg("Door", "Door %d => invalid!", index+1);
}

static void con_get_pos(void *result, void *user_data)
{
	if(!game.controller->is_race())
		return;
	
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
		dbg_msg("Tele","%s pos: %d @ %d", server_clientname(cid), (int)game.players[cid]->view_pos.x, (int)game.players[cid]->view_pos.y);
}

static void con_addvote(void *result, void *user_data)
{
	int len = strlen(console_arg_string(result, 0));
	
	if(!voteoption_heap)
		voteoption_heap = memheap_create();
	
	VOTEOPTION *option = (VOTEOPTION *)memheap_allocate(voteoption_heap, sizeof(VOTEOPTION) + len);
	option->next = 0;
	option->prev = voteoption_last;
	if(option->prev)
		option->prev->next = option;
	voteoption_last = option;
	if(!voteoption_first)
		voteoption_first = option;
	
	mem_copy(option->command, console_arg_string(result, 0), len+1);
	dbg_msg("server", "added option '%s'", option->command);
}

static void con_vote(void *result, void *user_data)
{
	if(str_comp_nocase(console_arg_string(result, 0), "yes") == 0)
		game.vote_enforce = GAMECONTEXT::VOTE_ENFORCE_YES;
	else if(str_comp_nocase(console_arg_string(result, 0), "no") == 0)
		game.vote_enforce = GAMECONTEXT::VOTE_ENFORCE_NO;
	dbg_msg("server", "forcing vote %s", console_arg_string(result, 0));
}

static void con_set_name(void *result, void *user_data)
{
	server_setclientname(console_arg_int(result, 0), console_arg_string(result, 1));
}

static void con_set_money(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		game.players[cid]->money = console_arg_int(result, 1);
	}
}

static void con_add_money(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		game.players[cid]->money += console_arg_int(result, 1);
	}
}
// ADDED BY GAME_GENERATION.ORG , ALEXANDER BECKER
static void con_get_money(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		game.players[cid]->money -= console_arg_int(result, 1);
	}
}

static void con_pause(void *result, void *user_data)
{
	game.world.paused = true;
	dbg_msg("server", "Game paused by Admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, "Game paused by Admin");
}
	
static void con_start(void *result, void *user_data)
{
	game.world.paused = false;
	dbg_msg("server", "Game started by Admin");
	game.send_chat(-1, GAMECONTEXT::CHAT_ALL, "Game started by Admin");
}

static void con_money(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		dbg_msg("server", "User %d:%s Money: %d", cid, server_clientname(cid), game.players[cid]->money);
	}
}

static void con_mute(void *result, void *user_data)
{
	
 	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int time = console_arg_int(result, 1)*server_tickspeed();
	if(game.players[cid])
	{
		game.players[cid]->muted=time;
		// Coded by BotoX
		if(time >= 60*server_tickspeed())
		{
			int sec;
			sec = time;
			
			while(sec >= 60*server_tickspeed())
				sec -= 60*server_tickspeed();

			time = time/server_tickspeed();
			time = time/60;

			char buf1[512];
			str_format(buf1, sizeof(buf1), "%s muted by admin for %d minutes and %d seconds", server_clientname(cid), time, sec/server_tickspeed());
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf1);
		}
		else
		{
			char buf[512];
			str_format(buf, sizeof(buf), "%s muted by admin for %d seconds", server_clientname(cid),time/server_tickspeed());
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
		}
		// Coded by BotoX
	}
}

static void con_set_score(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	if(game.players[cid])
	{
		game.players[cid]->score = console_arg_int(result, 1);
	}
}

static void con_update(void *result, void *user_data)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(game.players[i] && game.players[i]->logged_in)
			game.players[i]->account->update();
	
	dbg_msg("account", "All Accounts Updated!");
}
 
static void con_auth(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int authed = console_arg_int(result, 1);
	if(game.players[cid])
	{
		game.players[cid]->authed = authed;
		dbg_msg("server", "User %d:%s Authed: %d", cid, server_clientname(cid), game.players[cid]->authed);
	}
}

static void con_police(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int police = console_arg_int(result, 1);
	if(game.players[cid])
	{
		game.players[cid]->police = police;
		dbg_msg("server", "User %d:%s Police: %d", cid, server_clientname(cid), game.players[cid]->police);
	}
}

static void con_vip(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int vip = console_arg_int(result, 1);
	if(game.players[cid])
	{
		game.players[cid]->vip = vip;
		dbg_msg("server", "User %d:%s Vip: %d", cid, server_clientname(cid), game.players[cid]->vip);
	}
}

static void con_wlist(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int wlist = console_arg_int(result, 1);
	if(game.players[cid])
	{
		game.players[cid]->wlist = wlist;
			dbg_msg("server", "User %d:%s Whitelist: %d", cid, server_clientname(cid), game.players[cid]->wlist);
	}
}

static void con_wlistc(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	dbg_msg("server", "User %d:%s Whitelist: %d", cid, server_clientname(cid), game.players[cid]->wlist);
}

static void con_vipc(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	dbg_msg("server", "User %d:%s Vip: %d", cid, server_clientname(cid), game.players[cid]->vip);
}

static void con_policec(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	dbg_msg("server", "User %d:%s Police: %d", cid, server_clientname(cid), game.players[cid]->police);
}

static void con_authc(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	dbg_msg("server", "User %d:%s Authed: %d", cid, server_clientname(cid), game.players[cid]->authed);
}

static void con_ver(void *result, void *user_data)
{
	console_print("Current Version: 1.5 | Modded by Game-Generation.org for Teevision.de");
}
static void con_arest(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	int arest = console_arg_int(result, 1);
	if(game.players[cid])
	{
		game.players[cid]->arest = arest;
		dbg_msg("server", "User %d:%s Arest: %d", cid, server_clientname(cid), game.players[cid]->arest);
	}
}
static void con_arestc(void *result, void *user_data)
{
	int cid = clamp(console_arg_int(result, 0), 0, (int)MAX_CLIENTS-1);
	dbg_msg("server", "User %d:%s Arest: %d", cid, server_clientname(cid), game.players[cid]->arest);
}

void mods_console_init()
{
	MACRO_REGISTER_COMMAND("tune", "si", CFGFLAG_SERVER, con_tune_param, 0, "");
	MACRO_REGISTER_COMMAND("tune_reset", "", CFGFLAG_SERVER, con_tune_reset, 0, "");
	MACRO_REGISTER_COMMAND("tune_dump", "", CFGFLAG_SERVER, con_tune_dump, 0, "");
	MACRO_REGISTER_COMMAND("change_map", "r", CFGFLAG_SERVER, con_change_map, 0, "");
	MACRO_REGISTER_COMMAND("restart", "?i", CFGFLAG_SERVER, con_restart, 0, "");
	MACRO_REGISTER_COMMAND("broadcast", "r", CFGFLAG_SERVER, con_broadcast, 0, "");
	MACRO_REGISTER_COMMAND("say", "r", CFGFLAG_SERVER, con_say, 0, "");
	MACRO_REGISTER_COMMAND("say_by", "ir", CFGFLAG_SERVER, con_say_by, 0, "");
	MACRO_REGISTER_COMMAND("say_team_by", "ir", CFGFLAG_SERVER, con_say_team_by, 0, "");
	MACRO_REGISTER_COMMAND("say_to", "ir", CFGFLAG_SERVER, con_say_to, 0, "");
	MACRO_REGISTER_COMMAND("mute", "ii", CFGFLAG_SERVER, con_mute, 0, "");
	MACRO_REGISTER_COMMAND("set_team", "ii", CFGFLAG_SERVER, con_set_team, 0, "");
	MACRO_REGISTER_COMMAND("addvote", "r", CFGFLAG_SERVER, con_addvote, 0, "");
	MACRO_REGISTER_COMMAND("vote", "r", CFGFLAG_SERVER, con_vote, 0, "");
	MACRO_REGISTER_COMMAND("tele_all", "i", CFGFLAG_SERVER, con_teleport_all, 0, "");
	MACRO_REGISTER_COMMAND("teleport_all", "i", CFGFLAG_SERVER, con_teleport_all, 0, "");
	MACRO_REGISTER_COMMAND("tele", "ii", CFGFLAG_SERVER, con_teleport, 0, "");
	MACRO_REGISTER_COMMAND("teleport", "ii", CFGFLAG_SERVER, con_teleport, 0, "");
	MACRO_REGISTER_COMMAND("tele_to", "iii", CFGFLAG_SERVER, con_teleport_to, 0, "");
	MACRO_REGISTER_COMMAND("teleport_to", "iii", CFGFLAG_SERVER, con_teleport_to, 0, "");
	MACRO_REGISTER_COMMAND("tele_num", "ii", CFGFLAG_SERVER, con_teleport_num, 0, "");
	MACRO_REGISTER_COMMAND("teleport_num", "ii", CFGFLAG_SERVER, con_teleport_num, 0, "");
	MACRO_REGISTER_COMMAND("get_pos", "i", CFGFLAG_SERVER, con_get_pos, 0, "");
	MACRO_REGISTER_COMMAND("pos", "i", CFGFLAG_SERVER, con_get_pos, 0, "");
	MACRO_REGISTER_COMMAND("kill_pl", "i", CFGFLAG_SERVER, con_kill_pl, 0, "");
	MACRO_REGISTER_COMMAND("kill_all", "", CFGFLAG_SERVER, con_kill_all, 0, "");
	MACRO_REGISTER_COMMAND("kick_all", "", CFGFLAG_SERVER, con_kick_all, 0, "");
	MACRO_REGISTER_COMMAND("ban_all", "i", CFGFLAG_SERVER, con_ban_all, 0, "");
	MACRO_REGISTER_COMMAND("switch_door", "i", CFGFLAG_SERVER, con_switch_door, 0, "");
	MACRO_REGISTER_COMMAND("pause", "", CFGFLAG_SERVER, con_pause, 0, "");
	MACRO_REGISTER_COMMAND("start", "", CFGFLAG_SERVER, con_start, 0, "");
	MACRO_REGISTER_COMMAND("ver", "", CFGFLAG_SERVER, con_ver, 0, "");
	MACRO_REGISTER_COMMAND("ud", "", CFGFLAG_SERVER, con_update, 0, "");
	MACRO_REGISTER_COMMAND("update", "", CFGFLAG_SERVER, con_update, 0, "");
	MACRO_REGISTER_COMMAND("version", "", CFGFLAG_SERVER, con_ver, 0, "");
	MACRO_REGISTER_COMMAND("auth", "ii", CFGFLAG_SERVER, con_auth, 0, "");
	MACRO_REGISTER_COMMAND("auth_check", "i", CFGFLAG_SERVER, con_authc, 0, "");
	MACRO_REGISTER_COMMAND("police", "ii", CFGFLAG_SERVER, con_police, 0, "");
	MACRO_REGISTER_COMMAND("police_check", "i", CFGFLAG_SERVER, con_policec, 0, "");
	MACRO_REGISTER_COMMAND("vip", "ii", CFGFLAG_SERVER, con_vip, 0, "");
	MACRO_REGISTER_COMMAND("vip_check", "i", CFGFLAG_SERVER, con_vipc, 0, "");
	MACRO_REGISTER_COMMAND("wlist", "ii", CFGFLAG_SERVER, con_wlist, 0, "");
	MACRO_REGISTER_COMMAND("wlist_check", "i", CFGFLAG_SERVER, con_wlistc, 0, "");
	MACRO_REGISTER_COMMAND("set_name", "ii", CFGFLAG_SERVER, con_set_name, 0, "");
	MACRO_REGISTER_COMMAND("money", "i", CFGFLAG_SERVER, con_money, 0, "");
	MACRO_REGISTER_COMMAND("set_money", "ii", CFGFLAG_SERVER, con_set_money, 0, "");
	MACRO_REGISTER_COMMAND("add_money", "ii", CFGFLAG_SERVER, con_add_money, 0, "");
	MACRO_REGISTER_COMMAND("get_money", "ii", CFGFLAG_SERVER, con_get_money, 0, "");
	MACRO_REGISTER_COMMAND("set_score", "ii", CFGFLAG_SERVER, con_set_score, 0, "");
	MACRO_REGISTER_COMMAND("arest", "ii", CFGFLAG_SERVER, con_arest, 0, "");
	MACRO_REGISTER_COMMAND("arest_check", "i", CFGFLAG_SERVER, con_arestc, 0, "");
}
//con_get_money
void mods_init()
{
	//if(!data) /* only load once */
		//data = load_data_from_memory(internal_data);
		
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		snap_set_staticsize(i, netobj_get_size(i));

	layers_init();
	col_init();

	// reset everything here
	//world = new GAMEWORLD;
	//players = new PLAYER[MAX_CLIENTS];

	if(config.sv_autoreset)
	{
		con_tune_reset(NULL, NULL);
		console_execute_file("autoexec.cfg");
	}

	char buf[512];

	str_format(buf, sizeof(buf), "data/maps/%s.cfg", config.sv_map);
	console_execute_file(buf);

	str_format(buf, sizeof(buf), "data/maps/%s.map.cfg", config.sv_map);
	console_execute_file(buf);

	// select gametype
	//if(!str_comp_nocase(config.sv_gametype, "nctf") || !str_comp_nocase(config.sv_gametype, "ctf"))
	//	game.controller = new GAMECONTROLLER_CTF;
	//else if(!str_comp_nocase(config.sv_gametype, "ntdm") || !str_comp_nocase(config.sv_gametype, "tdm"))
	//game.controller = new GAMECONTROLLER_TDM;
	//else
	game.controller = new GAMECONTROLLER_RACE;

	// setup core world
	//for(int i = 0; i < MAX_CLIENTS; i++)
	//	game.players[i].core.world = &game.world.core;

	// create all entities from the game layer
	MAPITEM_LAYER_TILEMAP *tmap = layers_game_layer();
	TILE *tiles = (TILE *)map_get_data(tmap->data);
	
	
	for(int y = 0; y < tmap->height; y++)
	{
		for(int x = 0; x < tmap->width; x++)
		{
			int index = tiles[y*tmap->width+x].index;
			
			if(index >= ENTITY_OFFSET)
			{
				vec2 pos(x*32.0f+16.0f, y*32.0f+16.0f);
				game.controller->on_entity(index-ENTITY_OFFSET, pos);
			}
		}
	}
	game.controller->init_doors();
	//game.world.insert_entity(game.controller);

#ifdef CONF_DEBUG
	if(config.dbg_dummies)
	{
		for(int i = 0; i < config.dbg_dummies ; i++)
		{
			mods_connected(MAX_CLIENTS-i-1);
			mods_client_enter(MAX_CLIENTS-i-1);
			if(game.controller->is_teamplay())
				game.players[MAX_CLIENTS-i-1]->team = i&1;
		}
	}
#endif
}

void mods_shutdown()
{
	/* Account */
	// update all players
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(game.players[i] && game.players[i]->logged_in)
		{
			game.players[i]->account->update();
			game.players[i]->account->reset();
		}
	
	delete game.controller;
	game.controller = 0;
	game.clear();
}

void mods_presnap() {}
void mods_postsnap()
{
	game.events.clear();
}

extern "C" const char *mods_net_version() { return GAME_NETVERSION; }
extern "C" const char *mods_version() { return GAME_VERSION; }
