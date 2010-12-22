/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */


/* client */
MACRO_CONFIG_INT(cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements")
MACRO_CONFIG_INT(cl_nameplates, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show nameplates")
MACRO_CONFIG_INT(cl_nameplates_always, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show nameplats disregarding of distance")
MACRO_CONFIG_INT(cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup")

MACRO_CONFIG_INT(cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter")

MACRO_CONFIG_INT(cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance")

MACRO_CONFIG_INT(cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day")

MACRO_CONFIG_STR(cl_version_server, 100, "version.teeworlds.com", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for new versions")

MACRO_CONFIG_INT(player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors")
MACRO_CONFIG_INT(player_color_body, 65408, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color")
MACRO_CONFIG_INT(player_color_feet, 65408, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color")
MACRO_CONFIG_STR(player_skin, 64, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin")

MACRO_CONFIG_INT(ui_page, 5, 0, 9, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page")
MACRO_CONFIG_STR(ui_server_address, 128, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address")
MACRO_CONFIG_INT(ui_scale, 100, 1, 100000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale")

MACRO_CONFIG_INT(ui_color_hue, 160, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue")
MACRO_CONFIG_INT(ui_color_sat, 70, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation")
MACRO_CONFIG_INT(ui_color_lht, 175, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness")
MACRO_CONFIG_INT(ui_color_alpha, 228, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha")

MACRO_CONFIG_INT(gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping")

/* server */
MACRO_CONFIG_INT(sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warpup before round starts")
MACRO_CONFIG_STR(sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients")
MACRO_CONFIG_INT(sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage")
MACRO_CONFIG_STR(sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between")
MACRO_CONFIG_INT(sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating")
MACRO_CONFIG_INT(sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja")
MACRO_CONFIG_INT(sv_scorelimit, 20, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(sv_timelimit, 0, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_STR(sv_gametype, 32, "dm", CFGFLAG_SERVER, "Game type (dm, tdm, ctf)")
MACRO_CONFIG_INT(sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator")
MACRO_CONFIG_INT(sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection")

MACRO_CONFIG_INT(sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators")
MACRO_CONFIG_INT(sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams")

MACRO_CONFIG_INT(sv_vote_kick, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players")
MACRO_CONFIG_INT(sv_vote_kick_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time to ban a player if kicked by vote. 0 makes it just use kick")
MACRO_CONFIG_INT(sv_vote_scorelimit, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to change score limit")
MACRO_CONFIG_INT(sv_vote_timelimit, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to change time limit")

/* debug */
#ifdef CONF_DEBUG /* this one can crash the server if not used correctly */
	MACRO_CONFIG_INT(dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "")
#endif

MACRO_CONFIG_INT(dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(sv_reserved_slots, 0, 0, 12, CFGFLAG_SERVER, "")
MACRO_CONFIG_STR(sv_reserved_slots_pass, 32, "", CFGFLAG_SERVER, "")

/* race */
MACRO_CONFIG_INT(sv_regen, 0, 0, 0, CFGFLAG_SERVER, "Set regeneration")
MACRO_CONFIG_INT(sv_infinite_ammo, 1, 0, 1, CFGFLAG_SERVER, "Enable or disable infinite ammo")
MACRO_CONFIG_INT(sv_infinite_jumping, 0, 0, 1, CFGFLAG_SERVER, "Enable or disable infinite jumping")
MACRO_CONFIG_INT(sv_teleport, 1, 0, 1, CFGFLAG_SERVER, "Enable or disable teleportation")
MACRO_CONFIG_INT(sv_teleport_grenade, 0, 0, 1, CFGFLAG_SERVER, "Enable or disable teleport of grenade")
MACRO_CONFIG_INT(sv_teleport_kill, 0, 0, 1, CFGFLAG_SERVER, "Teleporting one someone kills him")
MACRO_CONFIG_INT(sv_teleport_strip, 0, 0, 1, CFGFLAG_SERVER, "Enable or disable keeping weapon after teleporting")
MACRO_CONFIG_INT(sv_rocket_jump_damage, 0, 0, 1, CFGFLAG_SERVER, "Enable or disable rocket jump damage")
MACRO_CONFIG_INT(sv_pickup_respawn, 1, 0, 10, CFGFLAG_SERVER, "Time before a pickup respawn")
MACRO_CONFIG_INT(sv_speedup_mult, 10, 1, 100, CFGFLAG_SERVER, "Boost power by multiplication")
MACRO_CONFIG_INT(sv_speedup_add, 5, -100, 100, CFGFLAG_SERVER, "Boost power")
MACRO_CONFIG_INT(sv_jumper_add, 7, -100, 100, CFGFLAG_SERVER, "Jumper power")
MACRO_CONFIG_INT(sv_gravity_add, 3, -100, 100, CFGFLAG_SERVER, "Gravity power")
MACRO_CONFIG_INT(sv_score_ip, 1, 0, 1, CFGFLAG_SERVER, "Adds the IPs to the recordfile")
MACRO_CONFIG_INT(sv_checkpoint_save, 1, 0, 1, CFGFLAG_SERVER, "Saves checkpoints to the recordfile")
MACRO_CONFIG_INT(sv_enemy_damage, 0, 0, 1, CFGFLAG_SERVER, "Enable damage from enemys")
MACRO_CONFIG_INT(sv_hammer_damage, 1, 0, 1, CFGFLAG_SERVER, "Enable damage from hammer")
MACRO_CONFIG_INT(sv_count_suicide, 0, 0, 1, CFGFLAG_SERVER, "Enable counting of suicide kills")
MACRO_CONFIG_INT(sv_count_teamkill, 0, 0, 1, CFGFLAG_SERVER, "Enable counting of team kills")
MACRO_CONFIG_INT(sv_count_kill, 0, 0, 1, CFGFLAG_SERVER, "Enable counting of normal kills")
MACRO_CONFIG_STR(sv_whitelist, 1000, "whitelist.cfg", CFGFLAG_SERVER, "Selects whitelist file")
MACRO_CONFIG_STR(sv_blacklist, 1000, "blacklist.cfg", CFGFLAG_SERVER, "Selects blacklist file")
MACRO_CONFIG_INT(sv_autoreset, 0, 0, 1, CFGFLAG_SERVER, "Auto 'tune_reset' and 'exec autoexec.cfg' on Mapchange")
MACRO_CONFIG_INT(sv_load_end, 0, 0, 1, CFGFLAG_SERVER, "End of race on +load")
MACRO_CONFIG_INT(sv_rainbow, 0, 0, 1, CFGFLAG_SERVER, "Enable or disable rainbow colors")
MACRO_CONFIG_INT(sv_rainbow_admin, 1, 0, 1, CFGFLAG_SERVER, "Rainbow for Admins")
MACRO_CONFIG_INT(sv_wlist, 1, 0, 1, CFGFLAG_SERVER, "Buy Wlist")
MACRO_CONFIG_INT(sv_reload_shotgun_admin, 7, 0, 100500, CFGFLAG_SERVER, "Admin Reload Time Shotgun")
MACRO_CONFIG_INT(sv_reload_grenade_admin, 13, 0, 100500, CFGFLAG_SERVER, "Admin Reload Time Grenade")
MACRO_CONFIG_INT(sv_reload_laser_admin, 10, 0, 100500, CFGFLAG_SERVER, "Admin Reload Time Laser")
MACRO_CONFIG_STR(sv_serverby, 1000, "Teevision.de", CFGFLAG_SERVER, "Start MSG")
MACRO_CONFIG_INT(sv_explosion_jump, 7, -100, 100, CFGFLAG_SERVER, "Jumper power")
MACRO_CONFIG_INT(sv_startmoney, 0, -100000000, 100000000, CFGFLAG_SERVER, "Start Money")

/* watermod */
MACRO_CONFIG_INT(sv_water_gravity, 30, -10000, 10000, CFGFLAG_SERVER, "gravty")
MACRO_CONFIG_INT(sv_water_maxx, 600, -10000, 10000, CFGFLAG_SERVER, "maxx")
MACRO_CONFIG_INT(sv_water_maxy, 450, -10000, 10000, CFGFLAG_SERVER, "maxy")
MACRO_CONFIG_INT(sv_water_friction, 90, -10000, 10000, CFGFLAG_SERVER, "friction")
MACRO_CONFIG_INT(sv_water_insta, 0, 0, 1, CFGFLAG_SERVER, "insta gib")
MACRO_CONFIG_INT(sv_water_strip, 1, 0, 1, CFGFLAG_SERVER, "if using insta gib, strip weapon first")
MACRO_CONFIG_INT(sv_water_freezetime, 60, 0, 100000, CFGFLAG_SERVER, "if using insta gib, freeze time the hammer freezes you (50=1 sec, 100 = 2 sec, ...)")
MACRO_CONFIG_INT(sv_water_oxygen, 0, 0, 1, CFGFLAG_SERVER, "use oxygen")
MACRO_CONFIG_INT(sv_water_oxy_drain, 1300, -100000, 100000, CFGFLAG_SERVER, "oxygen drainage")
MACRO_CONFIG_INT(sv_water_oxy_regen, 250, -100000, 100000, CFGFLAG_SERVER, "oxygen regeneration")
MACRO_CONFIG_INT(sv_water_oxy_emoteid, 3, 0, 100000, CFGFLAG_SERVER, "emote id")
MACRO_CONFIG_INT(sv_water_laserjump, 0, 0, 1, CFGFLAG_SERVER, "laser jumps =D")
MACRO_CONFIG_INT(sv_water_kicktime, 10000, 0, 10000, CFGFLAG_SERVER, "auto kick time")
MACRO_CONFIG_INT(sv_water_rambo, 0, 0, 1, CFGFLAG_SERVER, "easter egg")
MACRO_CONFIG_INT(sv_water_gain, 100, 0, 100000, CFGFLAG_SERVER, "speed change when accelerated by water")
MACRO_CONFIG_INT(sv_water_reflect, 1, 0, 1, CFGFLAG_SERVER, "reflect lasers by water")


/* By Game-Generation.org */
MACRO_CONFIG_INT(sv_log_chat, 1, 0, 1, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_log_account, 0, 0, 1, CFGFLAG_SERVER, "")

MACRO_CONFIG_INT(sv_kick_admin, 0, 0, 1, CFGFLAG_SERVER, "Enable kick Admin")
MACRO_CONFIG_INT(sv_kick_resistent, 0, 0, 1, CFGFLAG_SERVER, "Enable kick resistent Players")
MACRO_CONFIG_INT(sv_kick_whitelisted, 0, 0, 1, CFGFLAG_SERVER, "Enable kick whitelisted Players")
MACRO_CONFIG_INT(sv_kick_police, 0, 0, 1, CFGFLAG_SERVER, "Enable kick Police")

MACRO_CONFIG_INT(sv_buy_shop, 1, 0, 1, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_buy_restaurant, 1, 0, 1, CFGFLAG_SERVER, "")

MACRO_CONFIG_INT(sv_price_hook, 5000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_gun, 7000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_gunauto, 8000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_grenade, 10000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_shotgun, 10000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_jump, 10000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_vip, 15000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_wlist, 100000, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_laser, 100000, 0, 100000000, CFGFLAG_SERVER, "")

MACRO_CONFIG_INT(sv_price_drink, 10, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_energy, 20, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_burger, 50, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_hotdog, 50, 0, 100000000, CFGFLAG_SERVER, "")
MACRO_CONFIG_INT(sv_price_kaviar, 500, 0, 100000000, CFGFLAG_SERVER, "")


