#include "stdafx.h"
#include "Room.h"
#include "Ship.h"
#include "Bullet.h"
#include "Server.h"
#include "Client.h"

#include <iostream>
#include <vector>

using namespace std;

Ship::Ship(Controller & cntrller, int initmode, std::string name, int ID) : Actor(), controller(cntrller), playmode(initmode), name(name), m_playerID(ID)
{
	posx = 0;
	posy = 0;

	radius = 14.0;
	speed = 50.0;

	direction = 0.0;

	type = SHIP;
	mode = playmode;

	bulletinterval = 0.3;
	bulletstimeout = 0.0;

	target = NULL;

	score = 0;
}


Ship::~Ship(void)
{

}

bool Ship::update(Model & m_model, double deltat)

{
	double newposx = posx;
	double newposy = posy;

	double bulletposx = posx;
	double bulletposy = posy;
	bool bullet = false;

	double accx = 0.0;
	double accy = 0.0;
	double thrust = 50.0;
	double rotrate = 2.5;

	double controlthrust = 0.0;
	double controlleft = 0.0;
	double controlright = 0.0;
	double controlfire = 0.0;

	// vheck if the svtive user is a client
	if (Client::getInstance().isClient()) // human controlled
	{
		//std::cout << "isClient" << std::endl;
		//char c = controller.lastKey ();
		//switch (c)
		//{
		//case 'W': controlthrust = 1.0; break;
		//case 'A': controlleft = 1.0; break;
		//case 'D': controlright = 1.0; break;
		//case VK_SPACE: controlfire = 1.0; break;
		//default:
		//	// unknown key.
		//	;
		//}
		PlayerMovement temp;
		if (controller.isActive('W'))
		{
			//controlthrust = 1.0;
			temp.forward = true;
			//Client::getInstance().m_networkMovement->forward = true;
		}
		else {
			//Client::getInstance().m_networkMovement->forward = false;
			temp.forward = false;
		}
		if (controller.isActive('A'))
		{
			//controlleft = 1.0;
			temp.left = true;
			//Client::getInstance().m_networkMovement->left = true;
		}
		else {
			//Client::getInstance().m_networkMovement->left = false;
			temp.left = false;
		}
		if (controller.isActive('D'))
		{
			//controlright = 1.0;
			temp.right = true;
			//Client::getInstance().m_networkMovement->right = true;
		}
		else {
			//Client::getInstance().m_networkMovement->right = false;
			temp.right = false;

		}
		if (controller.isActive(VK_SPACE))
		{
			//controlfire = 1.0;
			temp.fire = true;
			//Client::getInstance().m_networkMovement->fire = true;
		}
		else {
			temp.fire = false;
			//Client::getInstance().m_networkMovement->fire = false;
		}
		// check for change
		if (temp.forward != Client::getInstance().m_networkMovement->forward)
		{
			Client::getInstance().m_networkMovement->forward = temp.forward;
			Client::getInstance().m_networkMovement->left = temp.left;
			Client::getInstance().m_networkMovement->right = temp.right;
			Client::getInstance().m_networkMovement->fire = temp.fire;
			Client::getInstance().serialize(PLAYER_MOVEMENT);
		}
		else if (temp.fire != Client::getInstance().m_networkMovement->fire)
		{
			Client::getInstance().m_networkMovement->forward = temp.forward;
			Client::getInstance().m_networkMovement->left = temp.left;
			Client::getInstance().m_networkMovement->right = temp.right;
			Client::getInstance().m_networkMovement->fire = temp.fire;
			Client::getInstance().serialize(PLAYER_MOVEMENT);
		}
		else if(temp.right != Client::getInstance().m_networkMovement->right)
		{
			Client::getInstance().m_networkMovement->forward = temp.forward;
			Client::getInstance().m_networkMovement->left = temp.left;
			Client::getInstance().m_networkMovement->right = temp.right;
			Client::getInstance().m_networkMovement->fire = temp.fire;
			Client::getInstance().serialize(PLAYER_MOVEMENT);
		}
		else if (temp.left != Client::getInstance().m_networkMovement->left)
		{
			Client::getInstance().m_networkMovement->forward = temp.forward;
			Client::getInstance().m_networkMovement->left = temp.left;
			Client::getInstance().m_networkMovement->right = temp.right;
			Client::getInstance().m_networkMovement->fire = temp.fire;
			Client::getInstance().serialize(PLAYER_MOVEMENT);
		}
	}
	if (mode == AUTO) // AI! controlled.
	{
		doAI(m_model, controlthrust, controlleft, controlright, controlfire);
	}
	// network input players
	if (mode == NETWORKPLAYER)
	{
		//std::cout << "mode == NETWORKPLAYER" << std::endl;
		UpdateNetworkPlayer(controlthrust, controlleft, controlright, controlfire);
	}

	accx += controlthrust * thrust * -sin(direction);
	accy += controlthrust * thrust * cos(direction);
	direction -= controlleft * rotrate * deltat;
	direction += controlright * rotrate * deltat;

	if (controlthrust > 0.0)
	{
		thruston = true;
	}
	else
	{
		thruston = false;
	}

	double grav = 0.0;
	if (mode == RECOVERY)
	{
		grav = 0.0;
	}
	vx = vx + accx * deltat;
	vy = vy + (accy + grav) * deltat;
	newposx = posx + vx * deltat;
	newposy = posy + vy * deltat;

	if (m_model.canMove(posx, posy, newposx, newposy))
	{
		posx = newposx;
		posy = newposy;

		if (mode == RECOVERY)
		{
			recoverytimer -= deltat;
			if (recoverytimer < 0.0f)
			{
				mode = playmode;
			}
		}
	}
	else
	{
		// hit something.
		triggerKill();
	}

	if ((controlfire > 0.0) && (bulletstimeout < 0.0))
	{
		double fx = (1.4 * radius * -sin(direction));
		double fy = (1.4 * radius * cos(direction));
		double bulletposx = newposx + fx;
		double bulletposy = newposy + fy;

		double bulletspeed = 70.0;
		double flen = sqrt(fx * fx + fy * fy);
		double bvx = bulletspeed * fx / flen + vx;
		double bvy = bulletspeed * fy / flen + vy;

		m_model.addActor(new Bullet(bulletposx, bulletposy, bvx, bvy, this));
		bulletstimeout = bulletinterval;
	}

	bulletstimeout -= deltat;

	return true;
}

void Ship::triggerKill()

{
	if (mode != RECOVERY)
	{
		score -= 1.0;
	}

	mode = RECOVERY;
	recoverytimer = 5.0;
	vx = (rand() % 50) - 25;
	vy = (rand() % 50) - 25;
	target = NULL;
}

void Ship::display(View & view, double offsetx, double offsety, double scale)

{
	// Find center of screen.
	int cx, cy;
	view.screenSize(cx, cy);
	cx = cx / 2;
	cy = cy / 2;

	int x = (int)((posx - (offsetx - cx)) * scale);
	int y = (int)((posy - (offsety - cy)) * scale);

	double base = radius / 2.0f;
	double height = radius * 2.0f;

	int bx = (int)(base * cos(direction) * scale);
	int by = (int)(base * sin(direction) * scale);

	int hx = (int)(height * -sin(direction) * scale);
	int hy = (int)(height * cos(direction) * scale);

	int r = 23;
	int g = 23;
	int b = 80;

	if (mode == RECOVERY)
	{
		r = 255;
		g = 0;
		b = 45;
		view.drawCircle(x, y, (int)(height * scale), r, g, b);
	}
	if (playmode == Ship::INPLAY)
		view.drawText(x, y, name, 48, 192, 192);
	else
		view.drawText(x, y, name);

	view.drawLine(x - hx / 2, y - hy / 2, x + bx - hx / 2, y + by - hy / 2, r, g, b);
	view.drawLine(x + hx - hx / 2, y + hy - hy / 2, x + bx - hx / 2, y + by - hy / 2, r, g, b);
	view.drawLine(x + hx - hx / 2, y + hy - hy / 2, x - bx - hx / 2, y - by - hy / 2, r, g, b);
	view.drawLine(x - hx / 2, y - hy / 2, x - bx - hx / 2, y - by - hy / 2, r, g, b);

	if (thruston)
	{
		view.drawLine(x - hx / 2, y - hy / 2, x - hx, y - hy, 255, 0, 0);
		view.drawLine(x - hx / 2, y - hy / 2, x - hx - bx, y - hy - by, 255, 0, 0);
		view.drawLine(x - hx / 2, y - hy / 2, x - hx + bx, y - hy + by, 255, 0, 0);
	}
}

void Ship::doAI(Model & m_model, double & controlthrust, double & controlleft, double & controlright, double & controlfire)

{
	if (target == NULL)
	{
		// find someone to shoot at.
		vector <Actor *> actors = m_model.getActors();
		for (std::vector <Actor *>::iterator i = actors.begin(); i != actors.end(); i++)
		{
			if (((*i)->getType() == SHIP) && (*i != this) /*&& ((Ship *)*i)->isFairGame()*/ && (rand() % 5 == 1))
			{
				target = (Ship *)(*i);
				break;
			}
		}
	}

	if (target != NULL)
	{
		double fx = -sin(direction);
		double fy = cos(direction);
		double vx;
		double vy;
		target->getPosition(vx, vy);
		vx = vx - posx;
		vy = vy - posy;

		// use the sign of the cross product of the forward vector and the vector to the target to tell which way to turn.
		double cp = vx * fy - fx * vy;
		if (cp > 0)
		{
			controlleft = 1.0;
		}
		else
		{
			controlright = 1.0;
		}

		double dist = sqrt(vx * vx + vy * vy);
		if (dist > 200.0)
		{
			controlthrust = 1.0;
		}
		if ((dist > 50.0) && (dist < 200.0))
		{
			controlfire = 1.0;
		}

		//if (!target->isFairGame())	// is the target active? If not, choose another
		//{
		//	target = NULL;
		//}
	}
}

void Ship::UpdateNetworkPlayer(double & controlthrust, double & controlleft, double & controlright, double & controlfire)
{
	size_t size = 0;

	std::shared_ptr<std::vector<std::shared_ptr<PlayerDetails>>> list;

	if (Server::getInstance().isServer()) {
		list = Server::getInstance().getPlayerDetails();
		size = Server::getInstance().getPlayerDetails()->size();
	}
	if (Client::getInstance().isClient()) {
		list = Client::getInstance().getPlayerDetails();
		size = Client::getInstance().getPlayerDetails()->size();
		//std::cout << "getPlayerDetails.size() = " << std::to_string(list->size()) << std::endl;
	}

	for (size_t i = 0; i < size; i++)
	{

		//std::cout << "m_playerID == " << std::to_string(m_playerID) << "  ---  list->at(i)->getID() == " << std::to_string(list->at(i)->getID()) << std::endl;
		if (list->at(i)->getID() == m_playerID)
		{
			//std::cout << "list->at(i)->getID() == m_playerID" << std::endl;

			if (list->at(i)->forward) {
				controlthrust = 1.0;
				//std::cout << "forward!!!!" << std::endl;
			}
			if (list->at(i)->left) {
				controlleft = 1.0;
				//std::cout << "left!!!!" << std::endl;
			}
			if (list->at(i)->right) {
				controlright = 1.0;
				///std::cout << "right!!!!" << std::endl;
			}
			if (list->at(i)->fire) {
				controlfire = 1.0;
				//std::cout << "fire!!!!" << std::endl;
			}
		}
	}
}

double Ship::getScore()

{
	return score;
}

void Ship::addHit()

{
	score += 1.5;
}

bool Ship::isFairGame()

{
	return (mode != RECOVERY);
}


PlayerStats & Ship::getShipNetworkStats()
{
	PlayerStats sp(posx,posy,speed,vx,vy);
	return sp;
}
