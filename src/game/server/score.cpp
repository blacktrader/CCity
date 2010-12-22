/* copyright (c) 2008 rajh and gregwar. Score stuff */

#include "score.hpp"
#include "gamecontext.hpp"
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <list>
#include <engine/e_config.h>
#include <engine/e_server_interface.h>

static LOCK score_lock = 0;

PLAYER_SCORE::PLAYER_SCORE(const char *name, float score, const char *ip, float cp_time[95], int pos_x, int pos_y, float diff)
{
	str_copy(this->name, name, sizeof(this->name));
	this->score = score;
	str_copy(this->ip, ip, sizeof(this->ip));

	for(int i = 0; i < 95; i++)
		this->cp_time[i] = cp_time[i];

	this->pos_x = pos_x;
	this->pos_y = pos_y;
	this->diff = diff;
}

std::list<PLAYER_SCORE> top;

SCORE::SCORE()
{
	if(score_lock == 0)
		score_lock = lock_create();
	load();
}

std::string save_file()
{
	std::ostringstream oss;
	oss << config.sv_map << "_record.nxt";
	return oss.str();
}

static void save_score_thread(void *)
{
	lock_wait(score_lock);
	std::fstream f;
	f.open(save_file().c_str(), std::ios::out);
	if(!f.fail())
	{
		int t = 0;
		for(std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
		{
			f << i->name << std::endl << i->score << std::endl  << i->ip << std::endl << i->pos_x << std::endl << i->pos_y << std::endl << i->diff << std::endl;
			if(config.sv_checkpoint_save)
			{
				for(int c = 0; c < 78; c++)
					f << i->cp_time[c+17] << " ";
				f << std::endl;
			}
			t++;
			if(t%50 == 0)
				thread_sleep(1);
		}
	}
	f.close();
	lock_release(score_lock);
}

void SCORE::save()
{
	void *save_thread = thread_create(save_score_thread, 0);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)save_thread);
#endif
}

void SCORE::load()
{
	lock_wait(score_lock);
	std::fstream f;
	f.open(save_file().c_str(), std::ios::in);
	top.clear();
	while (!f.eof() && !f.fail())
	{
		std::string tmpname, tmpscore, tmpip, tmpx, tmpy, tmpdiff, tmpcpline;
		std::getline(f, tmpname);
		if(!f.eof() && tmpname != "")
		{
			std::getline(f, tmpscore);
			std::getline(f, tmpip);
			std::getline(f, tmpx);
			std::getline(f, tmpy);
			std::getline(f, tmpdiff);

			float tmpcptime[95] = {0};
			if(config.sv_checkpoint_save)
			{
				std::getline(f, tmpcpline);
				if(tmpcpline.length() < 156)
					continue;

				char *time = strtok((char*)tmpcpline.c_str(), " ");
				int i = 0;
				while(time != NULL && i < 78)
				{
					tmpcptime[i+17] = atof(time);
					time = strtok(NULL, " ");
					i++;
				}
			}
			top.push_back(*new PLAYER_SCORE(tmpname.c_str(), atof(tmpscore.c_str()), tmpip.c_str(), tmpcptime, atoi(tmpx.c_str()), atoi(tmpy.c_str()), atof(tmpdiff.c_str())));
		}
	}
	f.close();
	lock_release(score_lock);
}

PLAYER_SCORE *SCORE::search_score(int id, bool score_ip, int *position)
{
	char ip[16];
	server_getip(id, ip);
	
	int pos = 1;
	for(std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
	{
		if(!strcmp(i->ip, ip) && config.sv_score_ip && score_ip)
		{
			if(position)
				*position = pos;
			return & (*i);
		}
		if(i->score != -1)
			pos++;
	}
	
	return search_name(server_clientname(id), position, 0);
}

PLAYER_SCORE *SCORE::search_name(const char *name, int *position, bool nocase)
{
	PLAYER_SCORE *player = 0;
	int pos = 1;
	int found = 0;
	for (std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end(); i++)
	{
		if(str_find_nocase(i->name, name))
		{
			if(position)
				*position = pos;
			if(nocase)
			{
				found++;
				player = & (*i);
			}
			if(!strcmp(i->name, name))
				return & (*i);
		}
		if(i->score != -1)
			pos++;
	}
	if(found > 1)
	{
		if(position)
			*position = -1;
		return 0;
	}
	return player;
}

void SCORE::parsePlayer(int id, float score, float cp_time[95], int x, int y, float diff)
{
	const char *name = server_clientname(id);
	char ip[16];
	server_getip(id, ip);
	
	lock_wait(score_lock);
	PLAYER_SCORE *player = search_score(id, 1, 0);
	if(player)
	{
		for(int c = 0; c < 95; c++)
		{
			if(player->cp_time[c] == 0 || player->score > score)
				player->cp_time[c] = cp_time[c];
		}
		if(player->score > score || player->score == -1)
		{
			player->score = score;
			str_copy(player->name, name, sizeof(player->name));
		}
	}
	else
		top.push_back(*new PLAYER_SCORE(name, score, ip, cp_time, x, y, diff));
	
	top.sort();
	lock_release(score_lock);
	save();
}

void SCORE::wlist_parsePlayer(int id, int x, int y, int diff)
{
	float empty_cp[95] = {0};
	const char *name = server_clientname(id);
	char ip[16];
	server_getip(id, ip);
	
	lock_wait(score_lock);
	PLAYER_SCORE *player = search_score(id, 1, 0);
	if(player)
	{
		player->pos_x = x;
		player->pos_y = y;
		player->diff = diff;
	}
	else
		top.push_back(*new PLAYER_SCORE(name, -1, ip, empty_cp, x, y, diff));

	top.sort();
	lock_release(score_lock);
	save();
}

void SCORE::initPlayer(int id)
{
	char ip[16];
	server_getip(id, ip);
	PLAYER_SCORE *player = search_score(id, 0, 0);
	if(player)
	{
		lock_wait(score_lock);
		str_copy(player->ip, ip, sizeof(player->ip));
		lock_release(score_lock);
		save();
	}
}

void SCORE::top5_draw(int id, int debut)
{
	int pos = 1;
	game.send_chat_target(id, "----------- Top 5 -----------");
	for (std::list<PLAYER_SCORE>::iterator i = top.begin(); i != top.end() && pos <= 5+debut; i++)
	{
		if(i->score < 0)
			continue;

		if(pos >= debut)
		{
			std::ostringstream oss;
			oss << pos << ". " << i->name << " Time: ";

			if ((int)(i->score)/60 != 0)
				oss << (int)(i->score)/60 << " minute(s) ";
			if (i->score-((int)i->score/60)*60 != 0)
				oss << i->score-((int)i->score/60)*60 <<" second(s)";

			game.send_chat_target(id, oss.str().c_str());
		}
		pos++;
	}
	game.send_chat_target(id, "-----------------------------");
}
