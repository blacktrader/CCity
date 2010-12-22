/* copyright (c) 2010 Bobynator.*/

#include <stdio.h>
#include <string.h>

#include <engine/e_server_interface.h>
#include <game/version.hpp>
#include "gamecontext.hpp"
#include "chatcmds.hpp"

void chat_command(int client_id, NETMSG_CL_SAY *msg, PLAYER *p)
{
	// login
	if(!strncmp(msg->message, "/login", 6))
	{
		p->last_chat = time_get();
		
		// check if allready logged in
		if(p->logged_in){
			game.send_broadcast("You are allready logged in!", client_id);
			return;}
		
		char name[32];
		char pass[32];	
		if(sscanf(msg->message, "/login %s %s", name, pass) != 2)
		{
			// notify the user that he is stupid
			game.send_chat_target(client_id, "Please stick to the given structure:");
			game.send_chat_target(client_id, "/login <user> <pass>");
			return;
		}

		p->account->login(name, pass);	
	}
	// logout
	else if(!strcmp(msg->message, "/logout"))
	{
		p->last_chat = time_get();

		// check if allready logged in
		if(!p->logged_in){
			game.send_broadcast("You are allready logged out!", client_id);
			return;}

		p->logged_in = false;
		
		// update database
		p->account->update();
		
		// reset acc data
		p->account->reset();
		game.send_chat_target(client_id, "You are now logged out.");
		p->kill_character(WEAPON_GAME);
		p->team = -1;
		return;
	}
	// register
	else if(!strncmp(msg->message, "/register", 9))
	{
		p->last_chat = time_get();

		if(p->logged_in){
			game.send_broadcast("You are allready logged in!", client_id);
			return;}
		
		char name[32];
		char pass[32];
		if(sscanf(msg->message, "/register %s %s", name, pass) != 2)
		{
			// notify the user that he is stupid
			game.send_chat_target(client_id, "Please stick to the given structure:");
			game.send_chat_target(client_id, "/register <user> <pass>");
			return;
		}

		p->account->create(name, pass);
	}
	// change pw
	else if(!strncmp(msg->message, "/password", 9))
	{
		p->last_chat = time_get();
				
		// If player isnt logged in ...
		if(!p->logged_in){
			game.send_broadcast("You are not logged in!", client_id);
			return;}

		// Buffer for password entered
		char new_pass[128];

		// Buffer for msges to the user
		char buf[128];

		// Scan the values entered, if enered wrong ...
		if(sscanf(msg->message, "/password %s", new_pass) != 1)
		{
			// ... then notify the user
			game.send_chat_target(client_id, "Please stick to the given structure:");
			game.send_chat_target(client_id, "/password <new password>");
			return;
		}

		// Change the pass and save it
		p->account->new_password(new_pass);
	}
	// info about the player
	else if(!strcmp(msg->message, "/stats"))
	{
		p->last_chat = time_get();

		if(!p->logged_in){
			game.send_broadcast("You are not logged in!", client_id);
			return;}

		char buf[512];
		str_format(buf, sizeof(buf), "Username: %s", p->acc_data.name);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Password: %s", p->acc_data.pass);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Money: %d", p->money);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Endless Player Hook: %d", p->hook);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Gun Spread: %d", p->gun);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Gun Autoshot: %d ", p->ak);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Shot 3 Grenades: %d", p->grenade);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Shotgun Explode: %d", p->shotgun);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Infinite Jump: %d", p->jump);
		game.send_chat_target(client_id, buf);
		str_format(buf, sizeof(buf), "Laser: %d", p->laser);
		game.send_chat_target(client_id, buf);
	}
	// all chat commands
	else if(!strcmp(msg->message, "/account"))
	{
		p->last_chat = time_get();

		game.send_chat_target(client_id, "-----------ACCOUNT-----------");
		game.send_chat_target(client_id, "");
		game.send_chat_target(client_id, "/register <name> <pass>   //Register");
		game.send_chat_target(client_id, "/login <name> <pass>   //Login");
		game.send_chat_target(client_id, "/logout   //Logout");
		game.send_chat_target(client_id, "/stats   //About Me");
		game.send_chat_target(client_id, "/password <new password>   //change password");
		game.send_chat_target(client_id, "");
		game.send_chat_target(client_id, "-----------ACCOUNT-----------");
	}	
	// FIXED BY ALEXANDER BECKER
	else if(!strncmp(msg->message, "/transfer", 9))
	{
		p->last_chat = time_get();
		
		// check if logged in
		if(!p->logged_in){
			game.send_broadcast("You are not logged in!", client_id);
			return;}
		
		char name[32];
		int money;	
		if(sscanf(msg->message, "/transfer %s %d", name, &money) != 2)
		{
			game.send_chat_target(client_id, "Please stick to the given structure:");
			game.send_chat_target(client_id, "/transfer <user> <money>");
			return;
		}
		
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(game.players[i] && !strcmp(name, game.players[i]->acc_data.name))
			{			
				if(p->money >= money && money >= 1)
				{
					p->money -= money;
					game.players[i]->money += money;
					game.send_chat_target(client_id, "Money Transfered successfully");
				}
				else
					game.send_chat_target(client_id, "You have not enough Money!");
			}
		}
	}
	
}
