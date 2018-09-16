#include "stdafx.h"
#include "NetworkProtocols.h"
#include "Server.h"


//class Ship;

/* TODO: DESCRIPTION HERE


*/
void NewPlayer::set()
{
	Server::getInstance().addPlayer(*this);
}