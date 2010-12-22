/* copyright (c) 2010 Bobynator.*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fstream>
#include "account.hpp"
#include <engine/e_config.h>

#include "account.hpp"
#include "gamecontext.hpp"
#include <base/math.hpp>
#include <game/server/gamecontext.hpp>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/version.hpp>
#include <game/collision.hpp>
#include <game/layers.hpp>
#include <game/mapitems.hpp>
#include <game/gamecore.hpp>
#include <game/server/entities/character.hpp>
#include "chatcmds.hpp"
PLAYER_ACCOUNT::PLAYER_ACCOUNT(PLAYER *player)
{
   this->player = player;
}

void PLAYER_ACCOUNT::login(char *name, char *pass)
{
	// check for to long name and pw
	if(strlen(name) > 15){
		game.send_chat_target(player->client_id, "ERROR: name is to long!");
		return;}
	else if(strlen(pass) > 15){
		game.send_chat_target(player->client_id, "ERROR: password is to long!");
		return;}
	
	// check for existing accounts
	if(exists(name) == false)
	{
		if (config.sv_log_account)
			dbg_msg("account", "Account '%s' does not exists", name);
		game.send_chat_target(player->client_id, "This account does not exists.");
		game.send_chat_target(player->client_id, "Please register first. (/register <user> <pass>)");
		return;
	}
	
	char bufname[128];
	str_format(bufname, sizeof(bufname), "accounts/%s.acc", name);
	
	// buf for data
	char username[32];
	char userpass[32];
	char usermoney[32];
	char up1[32];
	char up2[32];
	char up3[32];
	char up4[32];
	char up5[32];
	char up6[32];
	char vip[32];
	char police[32];
	char admin[32];
	char wlist[32];
	char laser[32];
	char arest[32];

	// load data
	FILE* accfile;
	accfile = fopen(bufname, "r");
	fscanf (accfile, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", username, userpass, usermoney, up1, up2, up3, up4, up5, up6, vip, police, admin, wlist, laser, arest);
	fclose (accfile);

	// check for correct pass and name
	if(strcmp(username, name))
	{
		if (config.sv_log_account)
			dbg_msg("account", "Account '%s' is not logged in due to wrong username", name);
		game.send_chat_target(player->client_id, "The username you entered is wrong.");
		return;
	}
	if(strcmp(userpass, pass))
	{
		if (config.sv_log_account)
			dbg_msg("account", "Account '%s' is not logged in due to wrong password", name);
		game.send_chat_target(player->client_id, "The password you entered is wrong.");
		return;
	}
	
	// check if someone is already logged in
	if(is_logged_in(name) == true)
	{
		if (config.sv_log_account)
			dbg_msg("account", "Account '%s' already logged in", name);
		game.send_chat_target(player->client_id, "This account is already logged in!");
		return;
	}

	// login
	str_copy(player->acc_data.name, username, 32);
	str_copy(player->acc_data.pass, userpass, 32);
	player->money += ctoi(usermoney);
	player->ak = ctoi(up1);
	player->gun = ctoi(up2);
	player->grenade = ctoi(up3);
	player->shotgun = ctoi(up4);
	player->hook = ctoi(up5);
	player->jump = ctoi(up6);
	player->vip = ctoi(vip);
	player->police = ctoi(police);
	player->authed = ctoi(admin);
	player->wlist = ctoi(wlist);
	player->laser = ctoi(laser);
	player->arest = ctoi(arest);

	player->logged_in = true;

	player->set_team(0);

	if (config.sv_log_account)
			dbg_msg("account", "Account '%s' logged in sucessfully", name);
	game.send_chat_target(player->client_id, "You are now logged in.");
}

int PLAYER_ACCOUNT::ctoi(char *number)
{
	if(!strcmp(number, ""))
		return 0;
	else
		return atoi(number);
}

void PLAYER_ACCOUNT::create(char *name, char *pass)
{
	// check for to long name and pw
	if(strlen(name) > 15){
		game.send_chat_target(player->client_id, "ERROR: name is to long!");
		return;}
	else if(strlen(pass) > 15){
		game.send_chat_target(player->client_id, "ERROR: password is to long!");
		return;}
	
	// check for existing accounts
	if(exists(name) == true)
	{
		if (config.sv_log_account)
			dbg_msg("account", "Account '%s' allready exists", name);	
		game.send_chat_target(player->client_id, "This account allready exists!");
		return;
	}
	



	
	player->money = config.sv_startmoney/2;
	// create account
	char bufname[128];
	str_format(bufname, sizeof(bufname), "accounts/%s.acc", name);
	FILE * accfile;
	accfile = fopen (bufname,"a+");
	str_format(bufname, sizeof(bufname), "%s\n%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d", name, pass, player->money, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	fputs(bufname, accfile);
	fclose(accfile);
	
	if (config.sv_log_account)
			dbg_msg("account", "Account '%s' was successfully created", name);				
	game.send_chat_target(player->client_id, "Acoount was created successfully.");
	game.send_chat_target(player->client_id, "You may login now. (/login <user> <pass>)");

}
bool PLAYER_ACCOUNT::exists(const char * name)
{
	char bufname[128];
	str_format(bufname, sizeof(bufname), "accounts/%s.acc", name);
    if (FILE * accfile = fopen(bufname, "r")) 
    {
        fclose(accfile);
        return true;
    }
    return false;
}

bool PLAYER_ACCOUNT::is_logged_in(const char * name)
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(game.players[i] && !strcmp(name, game.players[i]->acc_data.name))
			return true;
	}
	return false;
}

void PLAYER_ACCOUNT::update()
{
	char bufname[128];
	str_format(bufname, sizeof(bufname), "accounts/%s.acc", player->acc_data.name);
	std::remove(bufname);
	FILE * accfile;
	accfile = fopen (bufname,"a+");
	str_format(bufname, sizeof(bufname), "%s\n%s\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d", player->acc_data.name, player->acc_data.pass, player->money, player->ak, player->gun, player->grenade, player->shotgun, player->hook, player->jump, player->vip, player->police, player->authed, player->wlist, player->laser, player->arest);
	fputs(bufname, accfile);
	fclose(accfile);
	str_format(bufname, sizeof(bufname), "Account '%s' updated!",player->acc_data.name);
	if (config.sv_log_account)
		dbg_msg("account", bufname);
}

void PLAYER_ACCOUNT::reset()
{
	str_copy(player->acc_data.name, "", 32);
	str_copy(player->acc_data.pass, "", 32);
	player->money = 0;
	player->ak = 0;
	player->gun = 0;
	player->grenade = 0;
	player->shotgun = 0;
	player->hook = 0;
	player->jump = 0;
	player->vip = 0;
	player->police = 0;
	player->authed = 0;
	player->wlist = 0;
	player->laser = 0;
	player->arest = 0;
}

void PLAYER_ACCOUNT::erase()
{
	char bufname[128];
	str_format(bufname, sizeof(bufname), "accounts/%s.acc",player->acc_data.name);
	std::remove(bufname);
	str_format(bufname, sizeof(bufname), "Account '%s' erased!",player->acc_data.name);
	if (config.sv_log_account)
		dbg_msg("account", bufname);	
}

void PLAYER_ACCOUNT::new_password(char *new_pass)
{
	// check if new password to long
	if(strlen(new_pass) > 15){
		game.send_chat_target(player->client_id, "ERROR: password is to long!");
		return;}

	// update pass
	str_copy(player->acc_data.pass, new_pass, 32);
	player->account->update();

	// dbg msg
	char bufname[128];
	str_format(bufname, sizeof(bufname), "'%s''s password changed!",player->acc_data.name);
	if (config.sv_log_account)
		dbg_msg("account", bufname);
	game.send_chat_target(player->client_id, "Password changed sucessfully");
}

