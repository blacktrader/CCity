/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef GAME_MAPRES_COL_H
#define GAME_MAPRES_COL_H

#include <base/vmath.hpp>

enum
{
	COLFLAG_SOLID=1,
	COLFLAG_DEATH=2,
	COLFLAG_NOHOOK=4,
	COLFLAG_WATER=8,
	COLFLAG_WATER_UP=16,
	COLFLAG_WATER_DOWN=32,
	COLFLAG_WATER_LEFT=64,
	COLFLAG_WATER_RIGHT=128,
};

int col_init();
int col_is_solid(int x, int y);
int col_is_water(int x, int y);
void col_set(int x, int y, int flag);
int col_get(int x, int y);
int col_width();
int col_height();
int col_intersect_line(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);
int col_intersect_water(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);
int col_intersect_air(vec2 pos0, vec2 pos1, vec2 *out_collision, vec2 *out_before_collision);

//race
int col_is_teleport(int x, int y);
int col_is_checkpoint(int x, int y);
int col_is_begin(int x, int y);
int col_is_end(int x, int y);
int col_is_boost(int x, int y);
int col_is_boostV(int x, int y);
int col_is_boostR(int x, int y);
int col_is_boostL(int x, int y);
int col_is_jumper(int x, int y);
int col_is_gravity(int x, int y);
int col_get_admin(int x, int y);
int col_get_wlist(int x, int y);
int col_get_afk(int x, int y);
int col_get_vip(int x, int y);
int col_get_police(int x, int y);
int col_get_ninjafly(int x, int y);
int col_get_antisave(int x, int y);
int col_get_money1(int x, int y);
int col_get_money2(int x, int y);
int col_get_money5(int x, int y);
int col_get_money10(int x, int y);
int col_get_money20(int x, int y);
int col_get_money50(int x, int y);
int col_get_shop(int x, int y);
int col_get_food(int x, int y);
int col_get_insta(int x, int y);
vec2 teleport(int z);
#endif
