#ifndef EBC_CIRCUITER_H
#define EBC_CIRCUITER_H

#include <ast/Module.h>

class Circuiter {
public:
	void shorten(Module& module);
	void shorten(Block& block);
};


#endif //EBC_CIRCUITER_H
