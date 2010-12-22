/* copyright (c) 2008 rajh and gregwar. Score stuff */

#ifndef SCORE_H_RACE
#define SCORE_H_RACE
#include <engine/e_server_interface.h>

class PLAYER_SCORE
{
public:
	char name[MAX_NAME_LENGTH];
	float score, diff;
	int pos_x, pos_y;
	char ip[16];
	float cp_time[95];

	PLAYER_SCORE(const char *name, float score, const char *ip, float cp_time[95], int pos_x, int pos_y, float diff);

	bool operator==(const PLAYER_SCORE& other) { return (this->score == other.score); }
	bool operator<(const PLAYER_SCORE& other) { return (this->score < other.score); }
};

class SCORE
{
public:
	SCORE();
	
	void save();
	void load();
	PLAYER_SCORE *search_score(int id, bool score_ip, int *position);
	PLAYER_SCORE *search_name(const char *name, int *position, bool match_case);
	void parsePlayer(int id, float score, float cp_time[95], int x, int y, float diff);
	void initPlayer(int id);
	void top5_draw(int id, int debut);

	void wlist_parsePlayer(int id, int x, int y, int diff);
};

#endif
