#include "manchester.h"
#include "util.h"

Manchester::Manchester(uint8_t pin)
{
	_pin = pin;
	_status = Idle;
}

void Manchester::Send(uint8_t *data, uint8_t len)
{
	if (len > 20) len = 20;

	_status = Sending;
	_bitIndex = 0;
	_byteIndex = 0;
	_bufferLen = len;
	uint8_t i = 0;
	do
	{
		_buffer[i] = encode(data[i]);
		i++;
	}while(i < len);

	startTransmition();
}

void Manchester::OnInterrupt()
{
	if (_status != Sending) return;

	if (_bitIndex < 8)
	{
		sendBit();
		_bitIndex++;
	}
	else
	{
		_bitIndex = 0;
		_byteIndex++;
		if (_byteIndex >= _bufferLen)
		{
			stopTransmition();
			_status = Idle;
		}
	}
}

Manchester::Status Manchester::GetStatus()
{
	return _status;
}

uint16_t Manchester::encode(uint8_t data)
{
	uint8_t i = 0;
	uint16_t result = 0;
	do
	{
		if (isset(data, i))
		{
			result |= (1 << (2 *i)) | (1 << (2*i +1));
		}
		
		i++;
	}while(i < 8);

	return ~(0xAAAA ^ result);
}

void Manchester::sendBit()
{
	if (_byteIndex > _bufferLen) return;

	if (isset(_buffer[_byteIndex], _bitIndex))
	{
		digitalWrite(_pin, HIGH);
	}
	else
	{
		digitalWrite(_pin, LOW);
	}
	
}

/// Will use Timer0 at 2KHz CTC mode. 
/// for future this should be configured
void Manchester::startTransmition()
{
	//stop interruptions
	cli();
	//set pin as OUTPUT
	pinMode(_pin, OUTPUT);

	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;
	OCR0A = 124; // = (16*10^6) / (2000 * 64) -1 (must be less 255)
	// turn on CTC mode
	sbi(TCCR0A, WGM01);
  	// Set CS01 and CS00 bits for 64 prescaler
  	sbi(TCCR0B, CS00);
  	sbi(TCCR0B, CS01);
  	
  	// enable timer compare interrupt
  	sbi(TIMSK0,OCIE0A);

  	//allow interrupt
  	sei();
}

void Manchester::stopTransmition()
{
	cbi(TIMSK0,OCIE0A);
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;
}