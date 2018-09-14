
//Forward declaration of classes
class Ship;

enum MESSAGECODES { 
	ALIVE,
	WORLDUPDATE,
	KILL,
	REVIVE,
	PLAYER_STATS,
	POSITION_BULLET
};

/***********************************************************************
*
*					Network messages and objects
*
***********************************************************************/

/// TODO: make all message classes!

// Message objects. All must have a message code as the first integer and playerID as the second.
class Alive
{
public:
	int action;
	int player;

	Alive(int p) : action(ALIVE), player(p) {}
};

// Send a player update to the server: player died
class Kill
{
public:
	int action;
	int player;
	// add extra stats

	Kill(int p) : action(KILL), player(p) {}
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

// A player update to the server:  player state
class PlayerStats
{
public:
	int action;
	int player;
	Ship PlayerRef;
	// add extra stats

	PlayerStats(int p, Ship ref) : action(PLAYER_STATS), player(p), PlayerRef(ref) {}
};

// A player update to the server:  player state
class PossitionBullet
{
public:
	int action;
	int player;
	// add extra stats

	PossitionBullet(int p) : action(POSITION_BULLET), player(p) {}
};
