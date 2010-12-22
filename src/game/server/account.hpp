/* copyright (c) 2010 Bobynator.*/

#ifndef ACCOUNT_H
#define ACCOUNT_H
#include <engine/e_server_interface.h>

class PLAYER_ACCOUNT
{
public:
	class PLAYER *player;
	PLAYER_ACCOUNT(PLAYER *player);

	void login(char *name, char *pass);
	void create(char *name, char *pass);
	void update();
	void reset();
	void erase();
	void new_password(char *new_pass);

	bool exists(const char * name);
	bool is_logged_in(const char * name);
	int ctoi(char *number);
};

#endif
