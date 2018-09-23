#ifndef NETWORKENUMS_H
#define NETWORKENUMS_H

#include <iostream>

//Forward declaration of classes
//class Ship;

enum MESSAGECODES { 
	ALIVE,
	WORLDUPDATE,
	DEAD,
	REVIVE,
	PLAYER_STATS,
	NEW_PLAYER,
	SHOOT_BULLET,
	PLAYER_MOVEMENT,
	CREATE_ROOM,
	NO_PLAYERS
};

/***********************************************************************
*
*					Network messages and objects
*
***********************************************************************/

/// TODO: make all message classes!


class PlayerDetails
{
public:
	int m_playerGameID;
	std::string m_playerName;

	// Position of the actor.
	double posx;
	double posy;

	// Magnitude of velocity.
	double speed;
	// Unit velocity vector.
	double vx;
	double vy;

	//current movement states
	bool forward;
	bool left;
	bool right;

	// shoot state
	bool fire;

	int const getID() { return m_playerGameID; };
};


// Message objects. All must have a message code as the first integer and playerID as the second.
class Alive
{
public:
	int action;
	int player;

	Alive(int p) : action(ALIVE), player(p) {}
};

class CreateRoom
{
public:
	int left;
	int right;
	int top;
	int bottom;

	CreateRoom() {}
};

// send server that a new player is trying to join
class NewPlayer
{
public:
	int m_action;
	int m_playerID;
	std::string m_playerName;


	NewPlayer(int p, std::string name) : m_action(NEW_PLAYER), m_playerID(p), m_playerName(name) {};

	void set();

};

// Send a player update to the server: player died
class Dead
{
public:
	int action;
	int player;
	// add extra stats

	Dead(int p) : action(DEAD), player(p) {}
	//Kill(int p, int sx, int sy, int dx, int dy) : action(KILL), player(p), sourcex(sx), sourcey(sy), destx(dx), desty(dy) {}
};

// A player update to the server: revived player
class Revive
{
public:
	int action;
	int player;
	// add extra stats

	Revive(int p) : action(REVIVE), player(p) {}
};

class PlayerMovement
{
public:
	int action;
	int player;
	bool forward = false;
	bool left = false;
	bool right = false;
	bool fire = false;

	PlayerMovement(int p) : action(PLAYER_MOVEMENT), player(p) {}
	PlayerMovement() {}
};

// A player update to the server:  player state
class PlayerStats
{
public:
	int action;

	// player game ID
	int player;

	// Position of the actor.
	double posx;
	double posy;

	// Magnitude of velocity.
	double speed;
	// Unit velocity vector.
	double vx;
	double vy;

	// add extra stats

	PlayerStats() {};
	PlayerStats(double px, double py, double sp, double vx, double vy)
		: action(PLAYER_STATS), posx(px), posy(py), speed(sp), vx(vx), vy(vy) {}
};

// A player shot a bullet - from client to server
class ShootBullet
{
public:
	int action;
	int player;

	ShootBullet(int p) : action(SHOOT_BULLET), player(p) {}
};


#endif // !NETWORKENUMS_H
