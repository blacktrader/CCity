/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

// layer types
enum
{
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME, // not used
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,
	
	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,
	

	CURVETYPE_STEP=0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,
	
	// game layer tiles
	ENTITY_NULL=0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_RED,
	ENTITY_SPAWN_BLUE,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_RIFLE,
	NUM_ENTITIES,
	
	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_WATER,
	TILE_WATER_UP,
	TILE_WATER_DOWN,
	TILE_WATER_LEFT,
	TILE_WATER_RIGHT,
	TILE_ADMIN,
	TILE_WLIST,
	TILE_AFK,
	TILE_VIP,
	TILE_POLICE,
	TILE_NINJAFLY,
	TILE_ANTISAVE,

	TILE_SHOP=16,
	TILE_FOOD,
	TILE_MONEY1,
	TILE_MONEY2,
	TILE_MONEY5,
	TILE_MONEY10,
	TILE_MONEY20,
	TILE_MONEY50,
	TILE_MC,
	TILE_INSTA,
	
	TILE_GRAVITY=27,
	TILE_BOOSTV,
	TILE_BOOSTL,
	TILE_BOOSTR,
	TILE_BOOST,
	TILE_JUMPER,
	
	TILE_BEGIN,
	TILE_END,
	
	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	
	LAYERFLAG_DETAIL=1,
	
	ENTITY_OFFSET=255-16*4,
};

typedef struct
{
	int x, y; // 22.10 fixed point
} POINT;

typedef struct
{
	int r, g, b, a;
} COLOR;

typedef struct
{
	POINT points[5];
	COLOR colors[4];
	POINT texcoords[4];
	
	int pos_env;
	int pos_env_offset;
	
	int color_env;
	int color_env_offset;
} QUAD;

typedef struct
{
	unsigned char index;
	unsigned char flags;
	unsigned char skip;
	unsigned char reserved;
} TILE;

typedef struct 
{
	int version;
	int width;
	int height;
	int external;
	int image_name;
	int image_data;
} MAPITEM_IMAGE;

struct MAPITEM_GROUP_v1
{
	int version;
	int offset_x;
	int offset_y;
	int parallax_x;
	int parallax_y;

	int start_layer;
	int num_layers;
} ;


struct MAPITEM_GROUP : public MAPITEM_GROUP_v1
{
	enum { CURRENT_VERSION=2 };
	
	int use_clipping;
	int clip_x;
	int clip_y;
	int clip_w;
	int clip_h;
} ;

typedef struct
{
	int version;
	int type;
	int flags;
} MAPITEM_LAYER;

typedef struct
{
	MAPITEM_LAYER layer;
	int version;
	
	int width;
	int height;
	int flags;
	
	COLOR color;
	int color_env;
	int color_env_offset;
	
	int image;
	int data;
} MAPITEM_LAYER_TILEMAP;

typedef struct
{
	MAPITEM_LAYER layer;
	int version;
	
	int num_quads;
	int data;
	int image;
} MAPITEM_LAYER_QUADS;

typedef struct
{
	int version;
} MAPITEM_VERSION;

typedef struct
{
	int time; // in ms
	int curvetype;
	int values[4]; // 1-4 depending on envelope (22.10 fixed point)
} ENVPOINT;

typedef struct
{
	int version;
	int channels;
	int start_point;
	int num_points;
	int name;
} MAPITEM_ENVELOPE;

#endif
