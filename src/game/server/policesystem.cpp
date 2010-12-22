/* copyright (c) 2010 Game-Generation.org */

#include <stdio.h>
#include <string.h>

#include <engine/e_server_interface.h>
#include <game/version.hpp>
#include "gamecontext.hpp"
#include "policesystem.hpp"

void police_command(int client_id, NETMSG_CL_SAY *msg, PLAYER *p)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		char command[512];

		if(game.players[i])
		{
			/// AREST <ID>
			if(!game.players[i]->authed && !game.players[i]->police)
			{
				str_format(command, sizeof(command), "/arrest %d", i);				
				if((!str_comp_nocase(msg->message, command)) && game.controller->is_race())
				{
					char buf[512];
					if(game.players[i]->arest)
					{
						game.players[i]->arest = 0;
						game.players[i]->account->update();
						game.players[i]->kill_character(WEAPON_GAME);
						str_format(buf, sizeof(buf), "%s unarrested by the Police", server_clientname(i));	
						game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);	
					}
					else if(!game.players[i]->arest)
					{
						game.players[i]->arest = 1;
						game.players[i]->account->update();
						game.players[i]->kill_character(WEAPON_GAME);
						str_format(buf, sizeof(buf), "%s arrested by the Police", server_clientname(i));	
						game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);	
					}	

				}


				str_format(command, sizeof(command), "/kick %d", i);
				if((!str_comp_nocase(msg->message, command)) && game.controller->is_race())
				{		
					server_kick(i, "You was kicked by the Police!");			
					char buf[512];
					str_format(buf, sizeof(buf), "%s kicked by the Police", server_clientname(i));
					game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);					
				}


				str_format(command, sizeof(command), "/ban %d", i);
				if((!str_comp_nocase(msg->message, command)) && game.controller->is_race())
				{		
					char cmd[512]; 
					str_format(cmd, sizeof(cmd), "ban %d %d", i, 5);
					console_execute_line(cmd);server_kick(i, "You was kicked by the Police!");
			
					char buf[512];
					str_format(buf, sizeof(buf), "%s banned by the Police", server_clientname(i));
					game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);					
				}


				str_format(command, sizeof(command), "/kill %d", i);
				if((!str_comp_nocase(msg->message, command)) && game.controller->is_race())
				{
					game.players[i]->kill_character(WEAPON_GAME);
					char buf[512];
					str_format(buf, sizeof(buf), "%s Killed by Police", server_clientname(i));
					game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
				}
	
	
			}

		}	
	
	}	
	
	// ID LISTE
	if(!strncmp(msg->message, "/idlist", 7))
	{		
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i])
			{
				char buf[512];
				str_format(buf, sizeof(buf), "id='%d' name='%s' user='%s' money='%i' ", i, server_clientname(i), game.players[i]->acc_data.name, game.players[i]->money);
				game.send_chat_target(client_id, buf);
			}
		}
	}
	
}	



