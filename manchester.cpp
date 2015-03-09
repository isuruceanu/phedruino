#include "manchester.h"
#include "util.h"

#define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )

static const uint8_t reverseTable[256] =
{
    R6(0), R6(2), R6(1), R6(3)
};

uint8_t reverseByte(uint8_t v)
{
    return reverseTable[v];
}



Manchester::Manchester(uint8_t tx, uint8_t rx)
{
	_pinTx = tx;
	_pinRx = rx;
	_status = Idle;
		
	_rxPort = portInputRegister(digitalPinToPort(_pinRx));

	_rxPCMSK = digitalPinToPCMSK(_pinRx); 
	_rxBitMask = digitalPinToBitMask(_pinRx);
}

void Manchester::Send(uint8_t *data, uint8_t len, boolean hasStartBit)
{
	_status = Sending;
	
	uint8_t shift = hasStartBit ? 1 : 0;
	uint8_t localLen = hasStartBit ? len + 2 : len;
	_bitIndex = 0;
	_bufferLen = localLen * 8;
	_rxBuffer = new uint8_t[localLen];

	uint8_t i = 0;

	if (hasStartBit)
	{
		_rxBuffer[0] = 0x80;
		_rxBuffer[localLen-1] = 0x01;
		i = 1;
		_bitIndex = 7;
		_bufferLen -= 7;
	}

	
	
	do{
		//inverting the byte	
		_rxBuffer[i] = reverseByte(data[i-shift]);
		i++;
	} while( i < len + shift);
			
	//stop interruptions
	cli();
	//set pin as OUTPUT
	pinMode(_pinTx, OUTPUT);
	digitalWrite(_pinTx, LOW);

	configureTimer2(TICKS_PER_MS-1); // = (16*10^6) / (1000 * 64) -1 (must be less 255));
	OCR2B = TICKS_PER_MS / 2 -1;
	TCNT2=0;

  	sbi(TIMSK2, OCIE2A);
	sbi(TIFR2, OCF2A);
	
  	//allow interrupt
  	sei();
}



Manchester::Status Manchester::GetStatus()
{
	return _status;
}

// Configure timer2 (8 bit) with CTC mode and 64 prescale
void Manchester::configureTimer2(uint8_t ocr)
{
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
	OCR2A = ocr;
	
	// turn on CTC mode
	sbi(TCCR2A, WGM21);
  	// Set CS22 for 64 prescaler
  	sbi(TCCR2B, CS22);
}

/*
	 Reading: 
 1. Set the Rx pin as output with pull up
 2. Disable interuption
 3. Enable pin change interruption to identify the transition in the middle of bit. Also need to know when data start to come using _isFirsttransition.
 4. Comfigure timer2 CTC mode with 3/4 of pariod
 
 */
void Manchester::StartRead(uint8_t len)
{
	if (_status == CommandSent) 
	{
		_status = Reading;
		pinMode(_pinRx, INPUT); //set pin as input
		digitalWrite(_pinRx, HIGH); // enable pullup resistor
				
		_bitIndex = 0;
		
		_rxBuffer = new uint8_t[len];
		do{
			_rxBuffer[_bitIndex++] = 0;
		} while(_bitIndex < len);

		_bufferLen = len * 8; //we need to receive _bufferLen bits;
		_bitIndex = 0;

		_isFirstTransition = 1; //
		
		cli();
		
		*_rxPCMSK |= _rxBitMask;//enable Pin Change interrupt

		configureTimer2(TICKS_PER_MS * 0.75);
		
		sei();
	}
}

uint8_t * Manchester::GetReadData()
{
	return _rxBuffer;
}

void Manchester::SetIdle()
{
	_status = Idle;
}

/* 
	Pin Change interrupt is used only for reading and detecting trasitions
	When first transition is detected, disable the pin change interrupt and start the timer2 to fire
	somewhere in the middle of the second mid-bit or 3/4 of the period.
	PCINT catchs only compulsory level change at the middle of bit
*/
void Manchester::OnPinChangeInterrupt()
{
    if (_isFirstTransition)
    {
    	_isFirstTransition = 0; //we received first transition which should be bit edge, set flag to 0 and skip it. 
    	return;
    }

    *_rxPCMSK &= ~_rxBitMask; //disable pin interruption
    //here we at the bit transition so read the rx pin store it and start timer2 with 3/4 of period 

    
    if (!(*_rxPort & _rxBitMask)) //not because idle->active = 0 and active->idle = 1
    {
    	_rxBuffer[_bitIndex / 8] |= (1 << (_bitIndex % 8));
    }
    

	if (_bitIndex < _bufferLen) {
		*_rxPCMSK |= _rxBitMask; //enable pin change interruption
		TCCR2A = 0;
		sbi(TIMSK2,OCIE2A);
		_bitIndex++;

	} else {
		//received all required bits
		_status = ReadingReady;
		cli();
		*_rxPCMSK &= ~_rxBitMask;
		cbi(TIMSK2, TOIE2);
		sei();
	}

}


/*
	Use COMPA interrupt to set line depending on seding bit and set TCCR2B 
	to fire COMPB interrupt in match
*/
void Manchester::OnTimerMatchAInterrupt()
{
	cli();
	if (_status == Sending)
	{
		digitalWrite(_pinTx, isset(_rxBuffer[_bitIndex / 8], _bitIndex % 8));
		_bitIndex++;
		
		sbi(TIMSK2, OCIE2B);
		sbi(TIFR2, OCF2B);
		
		
		if (_bitIndex > _bufferLen)
		{
			cbi(TIMSK2,OCIE2A);
			cbi(TIMSK2,OCIE2B);
			TCCR2A = 0;
			TCCR2B = 0;
			TCNT2 = 0;
			_status = CommandSent;
			digitalWrite(_pinTx, LOW);
		}
	}

	if (_status == Reading)
	{
		*_rxPCMSK |= _rxBitMask; //just enable pin change interruption
	}
	sei();
}

/* 
	Use COMPB to toggle line in the middle of transition 
	and disable COMP B match interrupt so we do not taggle accedantly on before COMP A fires
*/
void Manchester::OnTimerMatchBInterrupt()
{
	cli();
	
	digitalWrite(_pinTx, !digitalRead(_pinTx));
	cbi(TIMSK2, OCIE2B);
	cbi(TIFR2, OCF2B);
	sei();
}


