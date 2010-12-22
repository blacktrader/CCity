#include "../gamemodes/race.hpp"
#include <engine/e_server_interface.h>
#include <engine/e_config.h>
#include <game/server/gamecontext.hpp>
#include <game/mapitems.hpp>
#include <string>
#include <cstring>

#include "character.hpp"
#include "laser.hpp"
#include "projectile.hpp"

struct INPUT_COUNT
{
	int presses;
	int releases;
};

static INPUT_COUNT count_input(int prev, int cur)
{
	INPUT_COUNT c = {0,0};
	prev &= INPUT_STATE_MASK;
	cur &= INPUT_STATE_MASK;
	int i = prev;
	while(i != cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.presses++;
		else
			c.releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CHARACTER, MAX_CLIENTS)

// player
CHARACTER::CHARACTER()
: ENTITY(NETOBJTYPE_CHARACTER)
{
	proximity_radius = phys_size;
}

void CHARACTER::reset()
{
	destroy();
}

bool CHARACTER::spawn(PLAYER *player, vec2 pos, int team)
{
	player_state = PLAYERSTATE_UNKNOWN;
	emote_stop = -1;
	last_action = -1;
	active_weapon = WEAPON_GUN;
	last_weapon = WEAPON_HAMMER;
	queued_weapon = -1;

	//clear();
	this->player = player;
	this->pos = pos;
	this->team = team;
	
	core.reset();
	core.world = &game.world.core;
	core.pos = pos;
	game.world.core.characters[player->client_id] = &core;

	reckoning_tick = 0;
	mem_zero(&sendcore, sizeof(sendcore));
	mem_zero(&reckoningcore, sizeof(reckoningcore));
	
	game.world.insert_entity(this);
	alive = true;
	player->force_balanced = false;
	
	game.controller->on_character_spawn(this);
	
	if(player->authed)
	{
		weapons[WEAPON_HAMMER].got = true;
		weapons[WEAPON_HAMMER].ammo = -1;
		weapons[WEAPON_GUN].got = true;
		weapons[WEAPON_GUN].ammo = 10;
		weapons[WEAPON_SHOTGUN].got = true;
		weapons[WEAPON_SHOTGUN].ammo = 10;
		weapons[WEAPON_RIFLE].got = true;
		weapons[WEAPON_RIFLE].ammo = 10;
		weapons[WEAPON_GRENADE].got = true;
		weapons[WEAPON_GRENADE].ammo = 10;
		weapons[WEAPON_NINJA].got = true;
		weapons[WEAPON_NINJA].ammo = -1;
	}

	return true;
}

void CHARACTER::destroy()
{
	game.world.core.characters[player->client_id] = 0;
	alive = false;
}

void CHARACTER::set_weapon(int w)
{
	if(w == active_weapon)
		return;
		
	last_weapon = active_weapon;
	queued_weapon = -1;
	active_weapon = w;
	if(active_weapon < 0 || active_weapon >= NUM_WEAPONS)
		active_weapon = 0;
	
	game.create_sound(pos, SOUND_WEAPON_SWITCH);
}

bool CHARACTER::is_grounded()
{
	if(col_check_point((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	if(col_check_point((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2+5)))
		return true;
	return false;
}


int CHARACTER::handle_ninja()
{
	if(active_weapon != WEAPON_NINJA)
		return 0;
	
	vec2 direction = normalize(vec2(latest_input.target_x, latest_input.target_y));
	
	if(!col_get_ninjafly((int)pos.x, (int)pos.y) && player->ninjalol==1)
	{
		weapons[WEAPON_NINJA].got = false;
		active_weapon = last_weapon;
		if(active_weapon == WEAPON_NINJA)
			active_weapon = WEAPON_GUN;
		set_weapon(active_weapon);
		player->ninjalol = 0;	
	}

	if(player->authed || col_get_ninjafly((int)pos.x, (int)pos.y))
	{
		//do nothing
	}
	else
	{
	if ((server_tick() - ninja.activationtick) > (data->weapons.ninja.duration * server_tickspeed() / 1000))
	{
		// time's up, return
		weapons[WEAPON_NINJA].got = false;
		active_weapon = last_weapon;
		if(active_weapon == WEAPON_NINJA)
			active_weapon = WEAPON_GUN;
		set_weapon(active_weapon);
		return 0;
	}
	
	// force ninja weapon
	set_weapon(WEAPON_NINJA);
	}
	ninja.currentmovetime--;

	if (ninja.currentmovetime == 0)
	{
		// reset player velocity
		core.vel *= 0.2f;
		//return MODIFIER_RETURNFLAGS_OVERRIDEWEAPON;
	}

	if (ninja.currentmovetime > 0)
	{
		// Set player velocity
		core.vel = ninja.activationdir * data->weapons.ninja.velocity;
		vec2 oldpos = pos;
		move_box(&core.pos, &core.vel, vec2(phys_size, phys_size), 0.0f);
		// reset velocity so the client doesn't predict stuff
		core.vel = vec2(0.0f,0.0f);
		if ((ninja.currentmovetime % 2) == 0)
		{
			//create_smoke(pos);
		}

		// check if we hit anything along the way
		{
			CHARACTER *ents[64];
			vec2 dir = pos - oldpos;
			float radius = phys_size * 2.0f; //length(dir * 0.5f);
			vec2 center = oldpos + dir * 0.5f;
			int num = game.world.find_entities(center, radius, (ENTITY**)ents, 64, NETOBJTYPE_CHARACTER);

			for (int i = 0; i < num; i++)
			{
				// Check if entity is a player
				if (ents[i] == this)
					continue;
				// make sure we haven't hit this object before
				bool balreadyhit = false;
				for (int j = 0; j < numobjectshit; j++)
				{
					if (hitobjects[j] == ents[i])
						balreadyhit = true;
				}
				if (balreadyhit)
					continue;

				// check so we are sufficiently close
				if (distance(ents[i]->pos, pos) > (phys_size * 2.0f))
					continue;

				// hit a player, give him damage and stuffs...
				game.create_sound(ents[i]->pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(numobjectshit < 10)
					hitobjects[numobjectshit++] = ents[i];
					
				ents[i]->take_damage(vec2(0,10.0f), data->weapons.ninja.base->damage, player->client_id,WEAPON_NINJA);
			}
		}
		return 0;
	}

	return 0;
}


void CHARACTER::do_weaponswitch()
{
	if(reload_timer != 0) // make sure we have reloaded
		return;
		
	if(queued_weapon == -1) // check for a queued weapon
		return;

	if(player->authed)
	{
		//do nothing
	}
	else
	{
	if(weapons[WEAPON_NINJA].got) // if we have ninja, no weapon selection is possible
		return;
	}

	// switch weapon
	set_weapon(queued_weapon);
}

void CHARACTER::handle_weaponswitch()
{
	int wanted_weapon = active_weapon;
	if(queued_weapon != -1)
		wanted_weapon = queued_weapon;
	
	// select weapon
	int next = count_input(latest_previnput.next_weapon, latest_input.next_weapon).presses;
	int prev = count_input(latest_previnput.prev_weapon, latest_input.prev_weapon).presses;

	if(next < 128) // make sure we only try sane stuff
	{
		while(next) // next weapon selection
		{
			wanted_weapon = (wanted_weapon+1)%NUM_WEAPONS;
			if(weapons[wanted_weapon].got)
				next--;
		}
	}

	if(prev < 128) // make sure we only try sane stuff
	{
		while(prev) // prev weapon selection
		{
			wanted_weapon = (wanted_weapon-1)<0?NUM_WEAPONS-1:wanted_weapon-1;
			if(weapons[wanted_weapon].got)
				prev--;
		}
	}

	// direct weapon selection
	if(latest_input.wanted_weapon)
		wanted_weapon = input.wanted_weapon-1;

	// check for insane values
	if(wanted_weapon >= 0 && wanted_weapon < NUM_WEAPONS && wanted_weapon != active_weapon && weapons[wanted_weapon].got)
		queued_weapon = wanted_weapon;
	
	do_weaponswitch();
}

void CHARACTER::fire_weapon()
{
	
	if(reload_timer != 0 || freezetime > 0)
		return;
		
	do_weaponswitch();
	
	vec2 direction = normalize(vec2(latest_input.target_x, latest_input.target_y));
	
	bool fullauto = false;

	if(active_weapon == WEAPON_GRENADE || active_weapon == WEAPON_SHOTGUN || active_weapon == WEAPON_RIFLE)
		fullauto = true;
	
	if(active_weapon == WEAPON_GUN && player->ak==1)
		fullauto = true;

	if(active_weapon == WEAPON_NINJA && col_get_ninjafly((int)pos.x, (int)pos.y))
		fullauto = true;

	if(player->authed)    
		fullauto = true;

	// check if we gonna fire
	bool will_fire = false;
	if(count_input(latest_previnput.fire, latest_input.fire).presses) will_fire = true;
	if(fullauto && (latest_input.fire&1) && weapons[active_weapon].ammo) will_fire = true;
	if(!will_fire)
		return;
		
	// check for ammo
	if(!weapons[active_weapon].ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		
		if(player->authed)
			reload_timer = 1;
		
		reload_timer = 125 * server_tickspeed() / 1000;;
		game.create_sound(pos, SOUND_WEAPON_NOAMMO);
		return;
	}
	
	vec2 projectile_startpos = pos+direction*phys_size*0.75f;
	
	switch(active_weapon)
	{
		case WEAPON_HAMMER:
		{
			if(player->authed)
			{
				game.create_explosion(pos, 0, 0, true);
				game.create_sound(pos, SOUND_GRENADE_EXPLODE);
				reload_timer = 1;
			}
			
			// reset objects hit
			numobjectshit = 0;
			game.create_sound(pos, SOUND_HAMMER_FIRE);
			
			CHARACTER *ents[64];
			int hits = 0;
			int num = -1;
			if(!game.controller->is_race() || (game.controller->is_race() && (config.sv_teamdamage || config.sv_enemy_damage)))
				num = game.world.find_entities(pos+direction*phys_size*0.75f, phys_size*0.5f, (ENTITY**)ents, 64, NETOBJTYPE_CHARACTER);
 
			for (int i = 0; i < num; i++)
			{
				CHARACTER *target = ents[i];
				if (target == this)
					continue;
					
				// hit a player, give him damage and stuffs...
				vec2 fdir = normalize(ents[i]->pos - pos);

				// set his velocity to fast upward (for now)
				game.create_hammerhit(pos);
				
				if(config.sv_water_insta && ents[i]->team != team)
				{
					ents[i]->take_damage(vec2(0,-1.0f), 0, player->client_id, active_weapon);
					if(config.sv_water_strip && ents[i]->team != team && ents[i]->weapons[WEAPON_RIFLE].got)
					{
						ents[i]->weapons[WEAPON_RIFLE].got = false;
						//ents[i]->freezetime = config.sv_water_freezetime*2;
						if(ents[i]->active_weapon == WEAPON_RIFLE)
							ents[i]->active_weapon=WEAPON_HAMMER;
					}
					ents[i]->freezetime = config.sv_water_freezetime;
					game.send_emoticon(ents[i]->player->client_id, 12);
				}
				else
					ents[i]->take_damage(vec2(0,-1.0f), data->weapons.hammer.base->damage, player->client_id, active_weapon);
				
				vec2 dir;
				if (length(target->pos - pos) > 0.0f)
					dir = normalize(target->pos - pos);
				else
					dir = vec2(0,-1);
					
				target->core.vel += normalize(dir + vec2(0,-1.1f)) * 10.0f;
				hits++;
			}
			
			// if we hit anything, we have to wait for the reload
			if(hits)
			{
				if(player->authed)
					reload_timer = 1;

				if(config.sv_water_insta)
				{
					reload_timer = config.sv_water_freezetime*2;
				}
				else
				{
					reload_timer = server_tickspeed()/3;
					
					if(player->authed)
						reload_timer = 1;
				}
			}
		} break;

		case WEAPON_GUN:
		{
			if(player->gun==1)
			{
				float start = 0.0f;
                if (2%2==0) start = (-2/2 + 0.5)*135*0.001;
                else start = (-(2-1)/2)*135*0.001;
                for (float i = 0; i < 2; i+=1.0f) {

				float a = start+get_angle(direction)+i*135*0.001;
                float speed = 1.0f;
                float v = 1-fabs((i*135*0.001f+start)/start);
                if (0) speed = mix((float)750*0.001f, 1.0f, v);

                PROJECTILE *proj = new PROJECTILE(WEAPON_GUN,
					player->client_id,
                    projectile_startpos,
                    vec2(cos(a), sin(a))*speed+vec2(0, -0*0.001f),
                    (int)(server_tickspeed()*tuning.gun_lifetime),
                    5, 0, 0, SOUND_GRENADE_EXPLODE, WEAPON_GUN);

                    // pack the projectile and send it to the client directly
                    NETOBJ_PROJECTILE p;
                    proj->fill_info(&p);

                    msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
                    msg_pack_int(1);
                    for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
						msg_pack_int(((int *)&p)[i]);
                    msg_pack_end();
                    server_send_msg(player->client_id);
				}
			}
			if(player->authed)
			{            
				float start = 0.0f;
                if (3%2==0) start = (-3/2 + 0.5)*70*0.001;
                else start = (-(3-1)/2)*70*0.001;
                for (float i = 0; i < 3; i+=1.0f) {

				float a = start+get_angle(direction)+i*70*0.001;
                float speed = 1.0f;
                float v = 1-fabs((i*70*0.001f+start)/start);
                if (0) speed = mix((float)750*0.001f, 1.0f, v);

                PROJECTILE *proj = new PROJECTILE(WEAPON_GUN,
					player->client_id,
                    projectile_startpos,
                    vec2(cos(a), sin(a))*speed+vec2(0, -0*0.001f),
                    (int)(server_tickspeed()*tuning.gun_lifetime),
                    5, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_GUN);

                    // pack the projectile and send it to the client directly
                    NETOBJ_PROJECTILE p;
                    proj->fill_info(&p);

                    msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
                    msg_pack_int(1);
                    for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
						msg_pack_int(((int *)&p)[i]);
                    msg_pack_end();
                    server_send_msg(player->client_id);
					reload_timer = 5;
				}
			}

			PROJECTILE *proj = new PROJECTILE(WEAPON_GUN,
				player->client_id,
				projectile_startpos,
				direction,
				(int)(server_tickspeed()*tuning.gun_lifetime),
				1, 0, 0, -1, WEAPON_GUN);
			// pack the projectile and send it to the client directly
			NETOBJ_PROJECTILE p;
			proj->fill_info(&p);
			
			msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
			msg_pack_int(5);
			for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
				msg_pack_int(((int *)&p)[i]);
			msg_pack_end();
			server_send_msg(player->client_id);
							
			game.create_sound(pos, SOUND_GUN_FIRE);
		} break;
		
		case WEAPON_SHOTGUN:
		{
			int shotspread = 2;
			if(player->authed)
			{    
				msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
				msg_pack_int(shotspread*2+1);
			
				for(int i = -shotspread; i <= shotspread; i++)
				{
					float spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = get_angle(direction);
					a += spreading[i+2];
					float v = 1-(abs(i)/(float)shotspread);
					float speed = mix((float)tuning.shotgun_speeddiff, 1.0f, v);
					PROJECTILE *proj = new PROJECTILE(WEAPON_SHOTGUN,
						player->client_id,
						projectile_startpos,
						vec2(cosf(a), sinf(a))*speed,
						(int)(server_tickspeed()*tuning.shotgun_lifetime),
						1, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);
					
					// pack the projectile and send it to the client directly
					NETOBJ_PROJECTILE p;
					proj->fill_info(&p);
				
					for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
						msg_pack_int(((int *)&p)[i]);
			
					reload_timer = config.sv_reload_shotgun_admin;
				}
			}

			if(player->shotgun==1)
			{
				msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
				msg_pack_int(shotspread*2+1);
			
				for(int i = -shotspread; i <= shotspread; i++)
				{
					float spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
					float a = get_angle(direction);
					a += spreading[i+2];
					float v = 1-(abs(i)/(float)shotspread);
					float speed = mix((float)tuning.shotgun_speeddiff, 1.0f, v);
					PROJECTILE *proj = new PROJECTILE(WEAPON_SHOTGUN,
						player->client_id,
						projectile_startpos,
						vec2(cosf(a), sinf(a))*speed,
						(int)(server_tickspeed()*tuning.shotgun_lifetime),
						1, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);
					
					// pack the projectile and send it to the client directly
					NETOBJ_PROJECTILE p;
					proj->fill_info(&p);
				
					for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
						msg_pack_int(((int *)&p)[i]);
			
				}
			}

			msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
			msg_pack_int(shotspread*2+1);
			
			for(int i = -shotspread; i <= shotspread; i++)
			{
				float spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = get_angle(direction);
				a += spreading[i+2];
				float v = 1-(abs(i)/(float)shotspread);
				float speed = mix((float)tuning.shotgun_speeddiff, 1.0f, v);
				PROJECTILE *proj = new PROJECTILE(WEAPON_SHOTGUN,
					player->client_id,
					projectile_startpos,
					vec2(cosf(a), sinf(a))*speed,
					(int)(server_tickspeed()*tuning.shotgun_lifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);
					
				// pack the projectile and send it to the client directly
				NETOBJ_PROJECTILE p;
				proj->fill_info(&p);
				
				for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
					msg_pack_int(((int *)&p)[i]);
			}

			msg_pack_end();
			server_send_msg(player->client_id);					
			
			game.create_sound(pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			if(player->authed)
			{
				float start = 0.0f;
                if (5%2==0) start = (-5/2 + 0.5)*75*0.001;
                else start = (-(5-1)/2)*75*0.001;
                for (float i = 0; i < 5; i+=1.0f) {

				PROJECTILE *proj = new PROJECTILE(WEAPON_GRENADE,
					player->client_id,
					projectile_startpos,
                    direction+vec2(start + i*75*0.001, -100*0.001f),
					(int)(server_tickspeed()*tuning.grenade_lifetime),
                    5, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				// pack the projectile and send it to the client directly
				NETOBJ_PROJECTILE p;
				proj->fill_info(&p);
			
				msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
				msg_pack_int(1);
				for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
					msg_pack_int(((int *)&p)[i]);
				msg_pack_end();
				server_send_msg(player->client_id);
			
				reload_timer = config.sv_reload_grenade_admin;
			}

			game.create_sound(pos, SOUND_GRENADE_FIRE);
			}
			
			if(player->grenade==1)
			{
				float start = 0.0f;
                if (2%2==0) start = (-2/2 + 0.5)*130*0.001;
                else start = (-(2-1)/2)*130*0.001;
                for (float i = 0; i < 2; i+=1.0f) {

				PROJECTILE *proj = new PROJECTILE(WEAPON_GRENADE,
					player->client_id,
					projectile_startpos,
                    direction+vec2(start + i*130*0.001, -100*0.001f),
					(int)(server_tickspeed()*tuning.grenade_lifetime),
                    5, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

				// pack the projectile and send it to the client directly
				NETOBJ_PROJECTILE p;
				proj->fill_info(&p);
			
				msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
				msg_pack_int(1);
				for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
					msg_pack_int(((int *)&p)[i]);
				msg_pack_end();
				server_send_msg(player->client_id);
				}

				game.create_sound(pos, SOUND_GRENADE_FIRE);
			}

			PROJECTILE *proj = new PROJECTILE(WEAPON_GRENADE,
				player->client_id,
				projectile_startpos,
				direction,
				(int)(server_tickspeed()*tuning.grenade_lifetime),
				1, PROJECTILE::PROJECTILE_FLAGS_EXPLODE, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
			// pack the projectile and send it to the client directly
			NETOBJ_PROJECTILE p;
			proj->fill_info(&p);
			
			msg_pack_start(NETMSGTYPE_SV_EXTRAPROJECTILE, 0);
			msg_pack_int(1);
			for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
				msg_pack_int(((int *)&p)[i]);
			msg_pack_end();
			server_send_msg(player->client_id);

			game.create_sound(pos, SOUND_GRENADE_FIRE);

		} break;
		
		case WEAPON_RIFLE:
		{
			if(player->laser || player->authed || col_get_insta((int)pos.x, (int)pos.y))
			{
				new LASER(pos, direction, tuning.laser_reach, player->client_id, is_water);
				game.create_sound(pos, SOUND_RIFLE_FIRE);
				if(player->authed)
					reload_timer = config.sv_reload_laser_admin;
			}
			else
				game.send_chat_target(player->client_id, "Please buy Laser in the Shop!");

		} break;
		
		case WEAPON_NINJA:
		{
			attack_tick = server_tick();
			ninja.activationdir = direction;
			ninja.currentmovetime = data->weapons.ninja.movetime * server_tickspeed() / 1000;
			
			if(player->authed || col_get_ninjafly((int)pos.x, (int)pos.y))
			{
				ninja.currentmovetime = data->weapons.ninja.movetime * server_tickspeed() / 2875;
				reload_timer = 1;
				game.create_explosion(pos, 0, 0, true);
				game.create_sound(pos, SOUND_GRENADE_EXPLODE);
			}
			
			game.create_sound(pos, SOUND_NINJA_FIRE);
			// reset hit objects
			numobjectshit = 0;
		} break;
		
	}

	if(weapons[active_weapon].ammo > 0 && (!game.controller->is_race() || !config.sv_infinite_ammo)) // -1 == unlimited
		weapons[active_weapon].ammo--;
	attack_tick = server_tick();
	if(!reload_timer)
		reload_timer = data->weapons.id[active_weapon].firedelay * server_tickspeed() / 1000;

}

int CHARACTER::handle_weapons()
{
	vec2 direction = normalize(vec2(latest_input.target_x, latest_input.target_y));

	/*
	if(config.dbg_stress)
	{
		for(int i = 0; i < NUM_WEAPONS; i++)
		{
			weapons[i].got = true;
			weapons[i].ammo = 10;
		}

		if(reload_timer) // twice as fast reload
			reload_timer--;
	} */

	//if(active_weapon == WEAPON_NINJA)
	handle_ninja();


	// check reload timer
	if(reload_timer)
	{
		reload_timer--;
		return 0;
	}
	
	/*
	if (active_weapon == WEAPON_NINJA)
	{
		// don't update other weapons while ninja is active
		return handle_ninja();
	}*/

	// fire weapon, if wanted
	fire_weapon();

	// ammo regen
	int ammoregentime = data->weapons.id[active_weapon].ammoregentime;
	if(ammoregentime)
	{
		// If equipped and not active, regen ammo?
		if (reload_timer <= 0)
		{
			if (weapons[active_weapon].ammoregenstart < 0)
				weapons[active_weapon].ammoregenstart = server_tick();

			if ((server_tick() - weapons[active_weapon].ammoregenstart) >= ammoregentime * server_tickspeed() / 1000)
			{
				// Add some ammo
				weapons[active_weapon].ammo = min(weapons[active_weapon].ammo + 1, 10);
				weapons[active_weapon].ammoregenstart = -1;
			}
		}
		else
		{
			weapons[active_weapon].ammoregenstart = -1;
		}
	}
	
	return 0;
}

void CHARACTER::on_predicted_input(NETOBJ_PLAYER_INPUT *new_input)
{
	// check for changes
	if(mem_comp(&input, new_input, sizeof(NETOBJ_PLAYER_INPUT)) != 0)
		last_action = server_tick();
		
	// copy new input
	mem_copy(&input, new_input, sizeof(input));
	num_inputs++;
	
	// or are not allowed to aim in the center
	if(input.target_x == 0 && input.target_y == 0)
		input.target_y = -1;	
}

void CHARACTER::on_direct_input(NETOBJ_PLAYER_INPUT *new_input)
{
	mem_copy(&latest_previnput, &latest_input, sizeof(latest_input));
	mem_copy(&latest_input, new_input, sizeof(latest_input));
	
	if(num_inputs > 2 && team != -1)
	{
		handle_weaponswitch();
		fire_weapon();
	}
	
	mem_copy(&latest_previnput, &latest_input, sizeof(latest_input));
}

void CHARACTER::tick()
{
	if(player->force_balanced)
	{
		char buf[128];
		str_format(buf, sizeof(buf), "You were moved to %s due to team balancing", game.controller->get_team_name(team));
		game.send_broadcast(buf, player->client_id);
		
		player->force_balanced = false;
	}

	if(input.direction != 0 || input.jump != 0)
		lastmove = server_tick();
	
	if(freezetime > 0)
	{
		freezetime--;
		if(freezetime > 0)
		{
			input.direction = 0;
			input.jump = 0;
			core.hook_state = HOOK_RETRACT_START;
			game.send_emoticon(player->client_id,  11);
		}
	}
	
	core.input = input;
	core.tick(true);
	do_splash = false;
	
	core.vel.y -= tuning.gravity;

	if(col_is_water((int)(pos.x), (int)(pos.y)))
	{
		if(!is_water)
		{
			//play a cool sound
			game.create_sound(pos, SOUND_PLAYER_SPAWN);
			do_splash = true;
			game.create_explosion(pos, -1, -1, true);
		}
		core.vel.y += config.sv_water_gravity/100.0f;
		if(core.vel.x > config.sv_water_maxx/100.0f || core.vel.x < -config.sv_water_maxx/100.0f)
			core.vel.x *= config.sv_water_friction/100.0f;
		if(core.vel.y > config.sv_water_maxy/100.0f || core.vel.y < -config.sv_water_maxy/100.0f)
			core.vel.y *= config.sv_water_friction/100.0f;
		if(core.jumped >= 2)
			core.jumped = 1;
		
		if(col_get((int)(pos.x), (int)(pos.y))&COLFLAG_WATER_UP)
			core.vel.y -= config.sv_water_gain/100.0f;
		else if(col_get((int)(pos.x), (int)(pos.y))&COLFLAG_WATER_DOWN)
			core.vel.y += config.sv_water_gain/100.0f;
		else if(col_get((int)(pos.x), (int)(pos.y))&COLFLAG_WATER_LEFT)
			core.vel.x -= config.sv_water_gain/100.0f;
		else if(col_get((int)(pos.x), (int)(pos.y))&COLFLAG_WATER_RIGHT)
			core.vel.x += config.sv_water_gain/100.0f;
		
		is_water = true;
	}
	else
	{
		if(is_water)
		{
			//play another cool sound
			game.create_sound(pos, SOUND_PLAYER_SPAWN);
			do_splash = true;
			game.create_explosion(pos, -1, -1, true);
		}
		core.vel.y += tuning.gravity;
		
		is_water = false;
	}
	
	
	if(config.sv_water_oxygen)
	{
		if (is_water)
		{
			if((server_tick() % (int)(config.sv_water_oxy_drain / 1000.0f * 50)) == 0)
			{
				if(armor)
					armor--;
				else
				{
					take_damage(vec2(0,0), 1, player->client_id, WEAPON_WORLD);
					game.send_emoticon(player->client_id,  config.sv_water_oxy_emoteid);
				}
			}
		} 
		else if((server_tick() % (int)(config.sv_water_oxy_regen / 1000.0f * 50)) == 0 && armor < 10)
			armor++;
	}

	//door
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < game.controller->doors[i].apos_count; j++)
		{
			if(distance(pos, game.controller->doors[i].apos[j]) < 30.0f && server_tick()-game.controller->doors[i].change_tick > 50)
			{
				game.controller->doors[i].change_tick = server_tick();
				
				if(game.controller->doors[i].team == -1 || game.controller->doors[i].team == team)
				{
					game.controller->set_door(i, !game.controller->get_door(i));
					game.create_sound(pos, SOUND_WEAPON_NOAMMO);
				}
				else
				{
					game.controller->set_door(i, !game.controller->get_door(i));
					game.create_sound(pos, SOUND_WEAPON_NOAMMO);
				}
			}
		}
	}

	//race
	if(core.jumped >= 2 && config.sv_infinite_jumping || core.jumped >= 2 &&  player->jump==1)
		core.jumped = 1;
	
	if(player->emo0==true)
	{
		CHARACTER *chr = game.players[player->client_id]->get_character();
		if (chr)
		{

			chr->emote_type = EMOTE_PAIN;
			chr->emote_stop = server_tick() + server_tickspeed();

		}
	}

	if(player->emo1==true)
	{
		CHARACTER *chr = game.players[player->client_id]->get_character();
		if (chr)
		{

			chr->emote_type = EMOTE_HAPPY;
			chr->emote_stop = server_tick() + server_tickspeed();

		}
	}

	if(player->emo2==true)
	{
		CHARACTER *chr = game.players[player->client_id]->get_character();
		if (chr)
		{

			chr->emote_type = EMOTE_SURPRISE;
			chr->emote_stop = server_tick() + server_tickspeed();

		}
	}

	if(player->emo3==true)
	{
		CHARACTER *chr = game.players[player->client_id]->get_character();
		if (chr)
		{

			chr->emote_type = EMOTE_ANGRY;
			chr->emote_stop = server_tick() + server_tickspeed();

		}
	}

	if(player->emo4==true)
	{
		CHARACTER *chr = game.players[player->client_id]->get_character();
		if (chr)
		{

			chr->emote_type = EMOTE_BLINK;
			chr->emote_stop = server_tick() + server_tickspeed();

		}
	}
	
	if(player->lolsplash==true)
		game.create_death(pos, player->client_id);

	if(player->hook==1)
		core.hook_tick = 0;

	if(player->authed)
	{
		player->respawn_tick = 0.3;
		player->die_tick = 0.3;
		if(player->god==true)
		{
			health = 25;
			armor = 25;
		}
		core.hook_tick = 0;
		if(core.jumped >= 2 && player->fly==false)
		{
			game.create_explosion(pos-vec2(0.0f, 0.0f), -1, -1, true);
			game.create_sound(pos, SOUND_GRENADE_EXPLODE);
			core.jumped = 1;
			do_splash = false;
		}
		else
		{
			if(core.jumped >= 1 && player->fly==true)
			{
				if (core.triggered_events&COREEVENT_AIR_JUMP)
					core.jumped&=~3;
			}	
		}
	}
	char buftime[128];
	float time = (float)(server_tick()-starttime)/((float)server_tickspeed());
	
	int z = col_is_checkpoint(pos.x, pos.y);
	if(z && race_state == RACE_STARTED)
	{
		cp_active = z;
		cp_current[z] = time;
		cp_tick = server_tick() + server_tickspeed()*2;
	}
	if(race_state == RACE_STARTED && server_tick()-refreshtime >= server_tickspeed())
	{
		int int_time = (int)time;
		str_format(buftime, sizeof(buftime), "Current time: %d min %d sec", int_time/60, (int_time%60));
		
		if(cp_active && cp_tick > server_tick())
		{
			PLAYER_SCORE *pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_score(player->client_id, 0, 0);
			if(pscore && pscore->cp_time[cp_active] != 0)
			{
				char tmp[128];
				float diff = cp_current[cp_active] - pscore->cp_time[cp_active];
				str_format(tmp, sizeof(tmp), "\nCheckpoint | Diff : %s%5.3f", (diff >= 0)?"+":"", diff);
				strcat(buftime, tmp);
			}
		}
		
		game.send_broadcast(buftime, player->client_id);
		refreshtime = server_tick();
	}
	if(config.sv_regen > 0 && (server_tick()%config.sv_regen) == 0 && game.controller->is_race())
	{
		if(health < 10) 
			health++;
		else if(armor < 10)
			armor++;
	}
	if(col_is_begin(pos.x,pos.y) && game.controller->is_race() && (!weapons[WEAPON_GRENADE].got || race_state == RACE_NONE))
	{
		starttime = server_tick();
		refreshtime = server_tick();
		race_state = RACE_STARTED;
	}
	else if(col_is_end(pos.x, pos.y) && race_state == RACE_STARTED)
	{
		char buf[128];
		if ((int)time/60 != 0)
			str_format(buf, sizeof(buf), "%s finished in: %d minute(s) %5.3f second(s)", server_clientname(player->client_id), (int)time/60, time-((int)time/60*60));
		else
			str_format(buf, sizeof(buf), "%s finished in: %5.3f second(s)", server_clientname(player->client_id), time-((int)time/60*60));
		game.send_chat(-1,GAMECONTEXT::CHAT_ALL, buf);

		PLAYER_SCORE *pscore = ((GAMECONTROLLER_RACE*)game.controller)->score.search_score(player->client_id, 0, 0);
		if(pscore && time - pscore->score < 0)
		{
			str_format(buf, sizeof(buf), "New record: %5.3f second(s) better", time - pscore->score);
			game.send_chat(-1, GAMECONTEXT::CHAT_ALL, buf);
		}

		if(time < player->score || !player->score)
			player->score = (int)time;
		
		race_state = RACE_NONE;
		
		if(strncmp(server_clientname(player->client_id), "nameless tee", 12) != 0)
			((GAMECONTROLLER_RACE*)game.controller)->score.parsePlayer(player->client_id, (float)time, cp_current, player->save_x, player->save_y, player->diff);
	}
	else if(col_is_boost((int)core.pos.x, core.pos.y) && game.controller->is_race())
	{
		vec2 speedup;
		if(core.vel.x >= 0)
			speedup = vec2(core.vel.x*(((float)config.sv_speedup_mult)/10.0)+config.sv_speedup_add,core.vel.y);
		else 
			speedup = vec2(core.vel.x*(((float)config.sv_speedup_mult)/10.0)-config.sv_speedup_add,core.vel.y);
		core.vel.x = speedup.x;
		core.vel.y = speedup.y;
	}
	else if(col_is_boostR((int)core.pos.x, core.pos.y) && game.controller->is_race())
	{
		if(core.vel.x >= 0)
			core.vel.x = core.vel.x*(((float)config.sv_speedup_mult)/10.0)+config.sv_speedup_add;
		else 
			core.vel.x = config.sv_speedup_add;
	}
	else if(col_is_boostL((int)core.pos.x, core.pos.y) && game.controller->is_race())
	{
		if(core.vel.x <= 0)
			core.vel.x = core.vel.x*(((float)config.sv_speedup_mult)/10.0)-config.sv_speedup_add;
		else
			core.vel.x = 0-config.sv_speedup_add;
	}
	else if(col_is_boostV((int)core.pos.x, core.pos.y) && game.controller->is_race())
	{
		if(core.vel.y >= 0)
			core.vel.y = core.vel.y+config.sv_gravity_add;
		else 
			core.vel.y = core.vel.y-config.sv_jumper_add;
		
	}

	if(col_get_admin((int)pos.x, (int)pos.y)){
		if(player->authed)
		  {
			  //do nothing
		  }
		else
		{
			die(player->client_id, WEAPON_WORLD);
			game.send_chat_target(player->client_id, "Only for Admins.");
		}
	}

	if(col_get_wlist((int)pos.x, (int)pos.y)){
		if(player->wlist)
		  {
			  //do nothing
		  }
		else
		{
			die(player->client_id, WEAPON_WORLD);
			game.send_chat_target(player->client_id, "Only for Whitelist.");
		}
	}

	if(col_get_vip((int)pos.x, (int)pos.y)){
		if(player->vip || player->authed)
		  {
			  //do nothing
		  }
		else
		{
			die(player->client_id, WEAPON_WORLD);
			game.send_chat_target(player->client_id, "Only for VIP.");
		}
	}

	if(col_get_police((int)pos.x, (int)pos.y)){
		if(player->police || player->authed)
		  {
			  //do nothing
		  }
		else
		{
			die(player->client_id, WEAPON_WORLD);
			game.send_chat_target(player->client_id, "Only for Police.");
		}
	}

	if(player->money_save>=750)
	{
		player->account->update();
		player->money_save = 0;
	}

	if(col_get_money1((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{			
			if(player->wlist)
			{
				player->money_save += 1;			
				player->money += 1;
			}
			if(player->vip)
			{
				player->money_save += 1;			
				player->money += 1;
			}
			player->money += 1;
			player->money_save += 1;
			player->money_tick = 0;
		}

		char buf[128];
		str_format(buf, sizeof(buf), "[1]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_money2((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{
			if(player->wlist)
			{
				player->money_save += 2;			
				player->money += 2;
			}
			if(player->vip)
			{
				player->money_save += 2;			
				player->money += 2;
			}
			player->money += 2;
			player->money_save += 2;
			player->money_tick = 0;
		}

		char buf[128];
		str_format(buf, sizeof(buf), "[2]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_money5((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{
			if(player->wlist)
			{
				player->money_save += 5;			
				player->money += 5;
			}
			if(player->vip)
			{
				player->money_save += 5;			
				player->money += 5;
			}
			player->money += 5;
			player->money_save += 5;
			player->money_tick = 0;
		}
       
		char buf[128];
		str_format(buf, sizeof(buf), "[5]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_money10((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{
			if(player->wlist)
			{
				player->money_save += 10;			
				player->money += 10;
			}
			if(player->vip)
			{
				player->money_save += 10;			
				player->money += 10;
			}
			player->money += 10;
			player->money_save += 10;
			player->money_tick = 0;
		}
       
		char buf[128];
		str_format(buf, sizeof(buf), "[10]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_money20((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{
			if(player->wlist)
			{
				player->money_save += 20;			
				player->money += 20;
			}
			if(player->vip)
			{
				player->money_save += 20;			
				player->money += 20;
			}
			player->money += 20;
			player->money_save += 20;
			player->money_tick = 0;
		}
     
		char buf[128];
		str_format(buf, sizeof(buf), "[20]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_money50((int)pos.x, (int)pos.y)){	
		player->money_tick += 1;
		if(player->money_tick >= server_tickspeed())
		{
			if(player->wlist)
			{
				player->money_save += 50;			
				player->money += 50;
			}
			if(player->vip)
			{
				player->money_save += 50;			
				player->money += 50;
			}
			player->money += 50;
			player->money_save += 50;
			player->money_tick = 0;
		}

		char buf[128];
		str_format(buf, sizeof(buf), "[50]--Your Money: %d$", player->money);
        game.send_broadcast(buf, player->client_id);
	}

	if(col_get_ninjafly((int)pos.x, (int)pos.y) && !player->authed)
	{
		player->ninjalol = 1;
		weapons[WEAPON_NINJA].got = true;
		weapons[WEAPON_NINJA].ammo = -1;
		last_weapon = active_weapon;
		active_weapon = WEAPON_NINJA;
	}

	else if(col_is_gravity((int)core.pos.x, core.pos.y) && game.controller->is_race())
		core.vel.y = core.vel.y+config.sv_gravity_add;
 	else if(col_is_jumper((int)core.pos.x, core.pos.y) && game.controller->is_race())
		core.vel.y = core.vel.y-config.sv_jumper_add;

	z = col_is_teleport(pos.x, pos.y);
	if(config.sv_teleport && z && game.controller->is_race())
	{
		if(player->authed)
		{
			//do nothing
		}
		else
		{
		core.hooked_player = -1;
		core.hook_state = HOOK_RETRACTED;
		core.triggered_events |= COREEVENT_HOOK_RETRACT;
		core.hook_state = HOOK_RETRACTED;
		core.hook_pos = core.pos;
		}
		core.pos = teleport(z);
		if(config.sv_teleport_strip)
		{
			active_weapon = WEAPON_HAMMER;
			last_weapon = WEAPON_HAMMER;
			weapons[0].got = true;
			for(int i = 1; i < 5; i++)
				weapons[i].got = false;
		}
	}
	
	float phys_size = 28.0f;
	// handle death-tiles
	if(col_get((int)(pos.x+phys_size/2), (int)(pos.y-phys_size/2))&COLFLAG_DEATH ||
			col_get((int)(pos.x+phys_size/2), (int)(pos.y+phys_size/2))&COLFLAG_DEATH ||
			col_get((int)(pos.x-phys_size/2), (int)(pos.y-phys_size/2))&COLFLAG_DEATH ||
			col_get((int)(pos.x-phys_size/2), (int)(pos.y+phys_size/2))&COLFLAG_DEATH)
	{
		die(player->client_id, WEAPON_WORLD);
	}

	// handle weapons
	handle_weapons();

	player_state = input.player_state;

	// Previnput
	previnput = input;
	oldpos = core.pos;
	return;
}

float point_distance(vec2 point, vec2 line_start, vec2 line_end)
{
	float res = -1.0f;
	vec2 dir = normalize(line_end-line_start);
	for(int i = 0; i < length(line_end-line_start); i++)
	{
		vec2 step = dir;
		step.x *= i;
		step.y *= i;
		float dist = distance(step+line_start, point);
		if(res < 0 || dist < res)
			res = dist;
	}
	return res;
}

void CHARACTER::reset_pos()
{
	core.pos = oldpos;
	core.vel = vec2(0,0);
	
	if(core.jumped >= 2)
		core.jumped = 1;
}

void CHARACTER::tick_defered()
{
	// advance the dummy
	{
		WORLD_CORE tempworld;
		reckoningcore.world = &tempworld;
		reckoningcore.tick(false);
		reckoningcore.move();
		reckoningcore.quantize();
	}
	
	//lastsentcore;
	/*if(!dead)
	{*/
		vec2 start_pos = core.pos;
		vec2 start_vel = core.vel;
		bool stuck_before = test_box(core.pos, vec2(28.0f, 28.0f));
		
		core.move();

		if(is_hitting_door())
		{
			reset_pos();
			if(is_hitting_door() && !doorstuck)
			{
				game.send_emoticon(player->client_id,  11);
				doorstuck = true;
			}
		}
		else
			doorstuck = false;

		bool stuck_after_move = test_box(core.pos, vec2(28.0f, 28.0f));
		core.quantize();
		bool stuck_after_quant = test_box(core.pos, vec2(28.0f, 28.0f));
		pos = core.pos;
		
		if(!stuck_before && (stuck_after_move || stuck_after_quant))
		{
			dbg_msg("player", "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x", 
				stuck_before,
				stuck_after_move,
				stuck_after_quant,
				start_pos.x, start_pos.y,
				start_vel.x, start_vel.y,
				*((unsigned *)&start_pos.x), *((unsigned *)&start_pos.y),
				*((unsigned *)&start_vel.x), *((unsigned *)&start_vel.y));
		}

		int events = core.triggered_events;
		int mask = cmask_all_except_one(player->client_id);
		
		if(events&COREEVENT_GROUND_JUMP) game.create_sound(pos, SOUND_PLAYER_JUMP, mask);
		
		//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
		if(events&COREEVENT_HOOK_ATTACH_PLAYER) game.create_sound(pos, SOUND_HOOK_ATTACH_PLAYER, cmask_all());
		if(events&COREEVENT_HOOK_ATTACH_GROUND) game.create_sound(pos, SOUND_HOOK_ATTACH_GROUND, mask);
		if(events&COREEVENT_HOOK_HIT_NOHOOK) game.create_sound(pos, SOUND_HOOK_NOATTACH, mask);
		//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
	//}
	
	if(team == -1)
	{
		pos.x = input.target_x;
		pos.y = input.target_y;
	}
	
	// update the sendcore if needed
	{
		NETOBJ_CHARACTER predicted;
		NETOBJ_CHARACTER current;
		mem_zero(&predicted, sizeof(predicted));
		mem_zero(&current, sizeof(current));
		reckoningcore.write(&predicted);
		core.write(&current);

		// only allow dead reackoning for a top of 3 seconds
		if(reckoning_tick+server_tickspeed()*3 < server_tick() || mem_comp(&predicted, &current, sizeof(NETOBJ_CHARACTER)) != 0)
		{
			reckoning_tick = server_tick();
			sendcore = core;
			reckoningcore = core;
		}
	}
}

bool CHARACTER::increase_health(int amount)
{
	if(health >= 10)
		return false;
	health = clamp(health+amount, 0, 10);
	return true;
}

bool CHARACTER::increase_armor(int amount)
{
	if(armor >= 10)
		return false;
	armor = clamp(armor+amount, 0, 10);
	return true;
}

void CHARACTER::die(int killer, int weapon)
{
	/*if (dead || team == -1)
		return;*/
	int mode_special = game.controller->on_character_death(this, game.players[killer], weapon);

	dbg_msg("game", "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		killer, server_clientname(killer),
		player->client_id, server_clientname(player->client_id), weapon, mode_special);

	// send the kill message
	NETMSG_SV_KILLMSG msg;
	msg.killer = killer;
	msg.victim = player->client_id;
	msg.weapon = weapon;
	msg.mode_special = mode_special;
	msg.pack(MSGFLAG_VITAL);
	server_send_msg(-1);

	// a nice sound
	game.create_sound(pos, SOUND_PLAYER_DIE);

	// set dead state
	// TODO: do stuff here
	/*
	die_pos = pos;
	dead = true;
	*/
	
	// this is for auto respawn after 3 secs
	player->die_tick = server_tick();
	
	alive = false;
	game.world.remove_entity(this);
	game.world.core.characters[player->client_id] = 0;
	game.create_death(pos, player->client_id);
	
	// we got to wait 0.5 secs before respawning
	if(!player->authed)
		player->respawn_tick = server_tick()+server_tickspeed()/2;

	if(player->authed)
	{
		kamikaze(pos, 100, player->client_id);
		kamikaze(pos, 200, player->client_id);
		kamikaze(pos, 300, player->client_id);
		kamikaze(pos, 400, player->client_id);
		kamikaze(pos, 500, player->client_id);
		kamikaze(pos, 600, player->client_id);
		kamikaze(pos, 700, player->client_id);
	}
}

bool CHARACTER::take_damage(vec2 force, int dmg, int from, int weapon)
{

	if(!game.controller->is_race() || (game.controller->is_race() && (from == player->client_id || config.sv_enemy_damage)))
	if(!game.players[from]->get_character())
		return false;	
		core.vel += force;
	
	if(game.controller->is_friendly_fire(player->client_id, from) && !config.sv_teamdamage)
		return false;

	// player only inflicts half damage on self
	if(from == player->client_id)
		dmg = max(1, dmg/2);

	if(((from == player->client_id && !config.sv_rocket_jump_damage) || (weapon == WEAPON_HAMMER && !config.sv_hammer_damage) || !config.sv_enemy_damage) && game.controller->is_race())
		dmg = 0;

	if(config.sv_water_oxygen && weapon == WEAPON_WORLD)
		dmg = 1;
	
	if(col_get_afk((int)pos.x, (int)pos.y) || col_get_ninjafly((int)pos.x, (int)pos.y)){
		dmg = 0;
	}

	damage_taken++;

	// create healthmod indicator
	if(server_tick() < damage_taken_tick+25)
	{
		// make sure that the damage indicators doesn't group together
		game.create_damageind(pos, damage_taken*0.25f, dmg);
	}
	else
	{
		damage_taken = 0;
		game.create_damageind(pos, 0, dmg);
	}

	if(dmg)
	{
		if(!config.sv_water_oxygen && armor)
		{
			if(dmg > 1)
			{
				health--;
				dmg--;
			}
			
			if(dmg > armor)
			{
				dmg -= armor;
				armor = 0;
			}
			else
			{
				armor -= dmg;
				dmg = 0;
			}
		}
		
		health -= dmg;
	}

	damage_taken_tick = server_tick();

	// do damage hit sound
	if(from >= 0 && from != player->client_id && game.players[from])
		game.create_sound(game.players[from]->view_pos, SOUND_HIT, cmask_one(from));

	// check for death
	if(health <= 0)
	{
		die(from, weapon);
		
		// set attacker's face to happy (taunt!)
		if (from >= 0 && from != player->client_id && game.players[from])
		{
			CHARACTER *chr = game.players[from]->get_character();
			if (chr)
			{
				chr->emote_type = EMOTE_HAPPY;
				chr->emote_stop = server_tick() + server_tickspeed();
			}
		}
	
		return false;
	}

	if (dmg > 2)
		game.create_sound(pos, SOUND_PLAYER_PAIN_LONG);
	else
		game.create_sound(pos, SOUND_PLAYER_PAIN_SHORT);

	emote_type = EMOTE_PAIN;
	emote_stop = server_tick() + 500 * server_tickspeed() / 1000;

	// spawn blood?
	return true;
}

void CHARACTER::snap(int snapping_client)
{
	if(networkclipped(snapping_client) || (snapping_client != player->client_id && player->invisible==true))
		return;
	
	NETOBJ_CHARACTER *character = (NETOBJ_CHARACTER *)snap_new_item(NETOBJTYPE_CHARACTER, player->client_id, sizeof(NETOBJ_CHARACTER));
	
	// write down the core
	if(game.world.paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		character->tick = 0;
		core.write(character);
	}
	else
	{
		character->tick = reckoning_tick;
		sendcore.write(character);
	}

	if(do_splash)
		character->jumped = 3;

	// set emote
	if (emote_stop < server_tick())
	{
		emote_type = EMOTE_NORMAL;
		emote_stop = -1;
	}

	character->emote = emote_type;

	character->ammocount = 0;
	character->health = 0;
	character->armor = 0;
	
	character->weapon = active_weapon;
	character->attacktick = attack_tick;

	character->direction = input.direction;

	if(player->client_id == snapping_client)
	{
		character->health = health;
		character->armor = armor;
		if(weapons[active_weapon].ammo > 0)
			character->ammocount = weapons[active_weapon].ammo;
	}

	if (character->emote == EMOTE_NORMAL)
	{
		if(250 - ((server_tick() - last_action)%(250)) < 5)
			character->emote = EMOTE_BLINK;
	}

	character->player_state = player_state;
}

bool CHARACTER::is_hitting_door()
{
	bool done = false;
	for(int i = 0; i < 16 && !done; i++)
	{
		if(!game.controller->doors[i].valid)
			continue;
		if(!game.controller->get_door(i))
			continue;
		for(int j = 0; j < game.controller->doors[i].pos_count-1; j++)
		{
			if(point_distance(core.pos, game.controller->doors[i].pos[j], game.controller->doors[i].pos[j+1]) < 30.0f)
			{
				done = true;
				break;
			}
		}
	}
	return done;
}

void CHARACTER::draw_ring_explosions(vec2 center, vec2 pos, int owner)
{
  game.create_explosion(vec2(center.x+pos.x,center.y+pos.y), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x-pos.x,center.y+pos.y), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x+pos.x,center.y-pos.y), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x-pos.x,center.y-pos.y), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x+pos.y,center.y+pos.x), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x+pos.y,center.y-pos.x), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x-pos.y,center.y+pos.x), owner, WEAPON_NINJA, false);
  game.create_explosion(vec2(center.x-pos.y,center.y-pos.x), owner, WEAPON_NINJA, false);
}

void CHARACTER::kamikaze(vec2 center, int radius, int owner)
{
  int x,y,p;
  x=0;
  y=radius;
  p=3-2*radius;
  while(x<y)
  {
   draw_ring_explosions(center, vec2(x, y), owner);
   if(p<0)
      p=p+4*x+6;
   else
   {
    p=p+4*(x-y)+10;
    y=y-1;
   }
   x++;
  }
  if(x==y)
   draw_ring_explosions(center, vec2(x, y), owner);
} 
