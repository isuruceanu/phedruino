#include "manchester.h"


Manchester::Manchester(uint8_t pin)
{
	_pin = pin;
}

void Manchester::StartTransmition()
{
	pinMode(_pin, OUTPUT);
}
