#ifndef OBSERVER_H
#define OBSERVER_H

#include "olsreventmessage.h"

namespace sereadmo {
namespace olsrclient {

class Observer
{
public:
	virtual void update(OlsrEventMessage message) = 0;
};

}
}
#endif
