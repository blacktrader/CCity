/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_config.h>
#include <engine/e_server_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/server/gamecontext.hpp>
#include "light.hpp"

//////////////////////////////////////////////////
// LIGHT
//////////////////////////////////////////////////
LIGHT::LIGHT(vec2 from, vec2 to, int team, int width)
: ENTITY(NETOBJ_INVALID)
{
	this->from = from;
	this->to = to;
	this->visible = true;
	this->width = width;
	this->team = team;
	keep_points = false;
	this->id2 = snap_new_id();
	//do_bounce();
	
	game.world.insert_entity(this);
}


bool LIGHT::hit_character(vec2 from, vec2 to)
{
	return false;
	/*if(this->visual)
		return false;
	vec2 at;
	CHARACTER *owner_char = game.get_player_char(owner);
	CHARACTER *hit = game.world.intersect_character(pos, to, 0.0f, at, owner_char);
	if(!hit)
		return false;

	this->from = from;
	pos = at;
	energy = -1;
	if (config.sv_water_insta) {
		hit->take_damage(vec2(0,0), 100, owner, WEAPON_RIFLE);
	} else {
		hit->take_damage(vec2(0,0), tuning.LIGHT_damage, owner, WEAPON_RIFLE);
	}
	return true;*/
}

void LIGHT::do_bounce()
{
	/*
	eval_tick = server_tick();
	
	
	if(energy < 0)
	{
		//dbg_msg("LIGHT", "%d removed", server_tick());
		game.world.destroy_entity(this);
		return;
	}
	if(visual) {
		from = pos;
		energy = -1;
		return;
	}
	
	vec2 to = pos + dir*energy;
	vec2 org_to = to;
	
	
	if(col_intersect_line(pos, to, 0x0, &to))
	{
		if(!hit_character(pos, to))
		{
			// intersected
			from = pos;
			pos = to;

			vec2 temp_pos = pos;
			vec2 temp_dir = dir*4.0f;
			
			move_point(&temp_pos, &temp_dir, 1.0f, 0);
			pos = temp_pos;
			dir = normalize(temp_dir);
			
			energy -= distance(from, pos) + tuning.LIGHT_bounce_cost;
			bounces++;
			
			if(bounces > tuning.LIGHT_bounce_num)
				energy = -1;
				
			game.create_sound(pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!hit_character(pos, to))
		{
			if(!visual) {
				from = pos;
				pos = to;
				energy = -1;
			}
		}
	}
		
	//dbg_msg("LIGHT", "%d done %f %f %f %f", server_tick(), from.x, from.y, pos.x, pos.y);*/
}
	
void LIGHT::reset()
{
	//dbg_msg("light", "i died, oh my!");
	game.world.destroy_entity(this);
}

void LIGHT::tick()
{
	return;
	/*if(server_tick() > eval_tick+(server_tickspeed()*tuning.LIGHT_bounce_delay)/1000.0f)
	{
		do_bounce();
	}*/

}

void LIGHT::snap(int snapping_client)
{
	//if((networkclipped(snapping_client)))
	//	return;
		
	if(!this->visible && !keep_points)
		return;

	NETOBJ_LASER *obj;
	if(this->team == -1) {
		obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
		obj->x = (int)from.x;
		obj->y = (int)from.y;
		obj->from_x = (int)to.x;
		obj->from_y = (int)to.y;
		obj->start_tick = server_tick()-2;
	} else if (game.players[snapping_client]->team == this->team) {
		obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
		obj->x = (int)from.x;
		obj->y = (int)from.y;
		obj->from_x = (int)to.x;
		obj->from_y = (int)to.y;
		obj->start_tick = server_tick()-4;
	} else {
		obj = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id, sizeof(NETOBJ_LASER));
		obj->x = (int)from.x-5;
		obj->y = (int)from.y-5;
		obj->from_x = (int)to.x-5;
		obj->from_y = (int)to.y-5;
		obj->start_tick = server_tick();
	}
	NETOBJ_LASER *obj2 = (NETOBJ_LASER *)snap_new_item(NETOBJTYPE_LASER, id2, sizeof(NETOBJ_LASER));
	obj2->x = obj->from_x;
	obj2->y = obj->from_y;
	obj2->from_x = obj->x;
	obj2->from_y = obj->y;
	obj2->start_tick = obj->start_tick;
	
	if(!visible) {
		obj->x = obj->from_x;
		obj->y = obj->from_y;
		obj2->x = obj2->from_x;
		obj2->y = obj2->from_y;
	}
}
