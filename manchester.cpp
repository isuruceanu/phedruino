#include "manchester.h"
#include "util.h"

Manchester::Manchester(uint8_t pin)
{
	_pin = pin;
	_status = Idle;
}

void Manchester::Send(uint8_t *data, uint8_t len, boolean sendPre)
{
	if (len > 20) len = 20;

	_status = Sending;
	_bitIndex = 0;
	_byteIndex = 0;
	_bufferLen = len;
	uint8_t i = 0;
	uint8_t shift =0;

	if (sendPre)
	{
		_buffer[0] = 0x02;
		_buffer[len+1] = 0x8000;
		shift = 1;
		_bufferLen += 2;
	}

	do
	{
		_buffer[i + shift] = encode(data[i]);
		i++;
		
	}while(i < len);

	startTransmition();
}

void Manchester::OnInterrupt()
{
	if (_status != Sending) return;

	if (_bitIndex < 16)
	{
		sendBit();
		_bitIndex++;
	}
	
	if (_bitIndex == 16)
	{
		_bitIndex = 0;
		_byteIndex++;
	}

	if (_byteIndex >= _bufferLen)
	{
		stopTransmition();
		_status = Idle;
	}
}

uint16_t Manchester::GetByte(uint8_t data)
{
	return encode(data);
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

	digitalWrite(_pin, isset(_buffer[_byteIndex], 15- _bitIndex));
	
}

void Manchester::ShowStatus()
{
   switch(_status)
   {
      case Idle:
       Serial.println("Idle");
       break;
      case Sending:
        Serial.println("Sending");
        break;
      case Reading:
      	Serial.println("Reading");
      	break;
      case ReadingTimeout:
      	Serial.println("Reading timeout");
      	break;
      case ReadingReady:
      	Serial.println("Reading ready");
      	break;
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

void Manchester::StartRead(uint8_t len)
{
	if (_status != Idle) return;
	
	pinMode(_pin, INPUT); //set pin as input
	digitalWrite(_pin, HIGH); // enable pullup resistor

	//attachInterrupt(_pin, onPinChangeInterrupt, CHANGE);
}

uint8_t * Manchester::GetReadData()
{

}

void Manchester::onPinChangeInterrupt()
{

}