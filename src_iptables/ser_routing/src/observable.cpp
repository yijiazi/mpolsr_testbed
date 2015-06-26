/******************************************************************
* Copyright (C) 2010                                                     
*     by      Pascal Lesage (Keosys)                                       
*             Xavier Lecourtier (Keosys) xavier.lecourtier@keosys.com                    
*             Sylvain David (IRCCyN, University of Nantes, France) sylvain.david@polytech.univ-nantes.fr          
*             Jiazi Yi (IRCCyN, University of Nantes, France) jiazi.yi@.univ-nantes.fr 
*      Benoit Parrein (IRCCyN, University of Nantes, France) benoit.parrein@polytech.univ-nantes.fr
*
*     Members of SEREADMO (French Research Grant ANR-05-RNRT-02803)                                                         
*                                                                                                                                     
*     This program is distributed in the hope that it will be useful,                                                          
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
****************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*******************************************************************/

#include "observable.h"
#include "observer.h"

void Observable::notify(OlsrEventMessage message)
{
	// Notifier tous les observers
	
	for (std::list<Observer*>::iterator it = list_observers_.begin();
		it != list_observers_.end();
		++it)
	{
		(*it)->update(message);
	}
}

void Observable::addObserver(Observer* observer)
{
	// Ajouter un observer a la liste
	this->list_observers_.push_back(observer);
}

void Observable::delObserver(Observer* observer)
{
	// Retirer un observer de la liste
	this->list_observers_.remove(observer);
}