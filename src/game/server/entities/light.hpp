/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#ifndef GAME_SERVER_ENTITY_LIGHT_H
#define GAME_SERVER_ENTITY_LIGHT_H

#include <game/server/entity.hpp>

class CHARACTER;

class LIGHT : public ENTITY
{
	vec2 from;
	vec2 to;
	int bounces;
	int eval_tick;
	int id2;
	
	bool hit_character(vec2 from, vec2 to);
	void do_bounce();
	
public:
	bool visible;
	bool keep_points;
	int width;
	int team;
	LIGHT(vec2 from, vec2 to, int team, int width = 0);
	
	virtual void reset();
	virtual void tick();
	virtual void snap(int snapping_client);
};

#endif
