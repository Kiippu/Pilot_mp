#ifndef __PILOT_SHIP
#define __PILOT_SHIP

#include "Actor.h"

class Ship : public Actor
{
private:
	Controller & controller;

	// the angle the ship is facing.
	double direction;

	// the control mode - currently keyboard, or simple AI
	int playmode;
	// state for an internal state machine.
	int mode;

	std::string name;

	// support variable, used to disable the ship for a certain amount of
	// time, when hit.
	double recoverytimer;

	// firing rate.
	double bulletinterval;
	// timeout until the next bullet can be fired.
	double bulletstimeout;

	// the current target.
	Ship * target;

	// reference to world
	 

	double score;

	bool thruston;

public:
	enum ActorTypes { INPLAY, AUTO, RECOVERY, NETWORKPLAYER };

	Ship(Controller & cntrller, int initmode, std::string name);
	virtual ~Ship(void);

	// Read input and update state accordingly.
	virtual bool update(Model & m_model, double deltat);

	// Show the Ship.
	virtual void display(View & view, double offsetx, double offsety, double scale);

	// Switch to killed state.
	void triggerKill();

	// simple control routine.
	void doAI(Model & m_model, double & controlthrust, double & controlleft, double & controlright, double & controlfire);

	// update network players
	void UpdateNetworkPlayer();

	// return current score
	double getScore();

	// register a hit of one of our bullets on another ship.
	void addHit();

	// returns true if the ship is allowed to be shot.
	bool isFairGame();

	// player stats to be serialized
	PlayerStats & getShipNetworkStats();
};

#endif // __PILOT_SHIP