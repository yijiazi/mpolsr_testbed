#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <string>
#include <list>
#include <iostream>

#include "olsreventmessage.h"
namespace sereadmo {
namespace olsrclient {

class Observer;

class Observable
{
public:

	void notify			(OlsrEventMessage message);
	void addObserver	(Observer* observer);
	void delObserver	(Observer* observer);
	
private:
	std::list<Observer*> list_observers_;
};
}
}
#endif
