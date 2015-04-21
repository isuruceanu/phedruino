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

	pinMode(_pinTx, OUTPUT);
	digitalWrite(_pinTx, LOW);	
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

/*
	 Reading: 
 1. Set the Rx pin as output with pull up
 2. Disable interuption
 3. Enable pin change interruption to identify the transition in the middle of bit. Also need to know when data start to come using _isFirsttransition.
 4. Comfigure timer2 CTC mode with 3/4 of pariod
 
 */
void Manchester::StartRead(uint8_t len, boolean hasStartBit)
{
	if (_status == Reading) return;
		cli();
		pinMode(_pinRx, INPUT_PULLUP);
		
		_status = Reading;
		
		_bitIndex = 0;
		
		uint8_t buffer = len  + hasStartBit ? 2 : 0;

		_rxBuffer = new uint8_t[buffer];
		
		if (hasStartBit)
			_bitIndex = 7;

		_bufferLen = (len * 8) + _bitIndex + (hasStartBit ? 1 : 0);
		
		_first = 1; //
		
		sbi(EICRA, ISC10);
		sbi(EIMSK, INT1);
		cbi(EIFR, INTF1);
		
		configureTimer2(TICKS_PER_MS * 0.75);
		TCCR2B = 0;//disable at this moment the timer
     	sbi(TIMSK2, OCIE2A);
		sbi(TIFR2, OCF2A);
		TCNT2 = TICKS_PER_MS * 0.5;		
		sei();
		
}

uint8_t * Manchester::GetReadData()
{
	return _rxBuffer;
}

volatile Manchester::Status Manchester::GetStatus()
{
	return _status;
}

// Configure timer2 (8 bit) with CTC mode and 64 prescale
void Manchester::configureTimer2(uint8_t ocr)
{
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
	OCR2B = 0;
	OCR2A = ocr;
	
	// turn on CTC mode
	sbi(TCCR2A, WGM21);
  	// Set CS22 for 64 prescaler
  	sbi(TCCR2B, CS22);
}

void Manchester::stopTimer2(void)
{
	cbi(TIMSK2,OCIE2A);
	cbi(TIMSK2,OCIE2B);
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
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
	cli();
    //disable pin interruption
    cbi(EIMSK, INT1);

    if (_first == 1) _first = 0; 
    else TCNT2 = 0;
    
    if (_bitIndex > _bufferLen){
    	_status = ReadingReady;
		stopTimer2();
    }
    else{
  		sbi(TCCR2B, CS22);
    }
    
    sei();
 
}


/*
	Use COMPA interrupt to set line depending on sending bit and set TCCR2B 
	to fire COMPB interrupt on match
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
			stopTimer2();
			_status = CommandSent;
			digitalWrite(_pinTx, LOW);
		}
	}

	if (_status == Reading)	{
		
		TCCR2B = 0;

		if (digitalRead(_pinRx) == 1) 
		{
			_rxBuffer[_bitIndex / 8] |= (1 << (7 - (_bitIndex % 8)));
		}
		
 		//enable pin interruption at any change
 		sbi(EIMSK, INT1);
   		_bitIndex++;
	}
	sei();
	
}

/* 
	Use COMPB to toggle line in the middle of transition 
	and disable COMPB match interrupt so we do not taggle accedantly before COMPA fires
*/
void Manchester::OnTimerMatchBInterrupt()
{
	cli();
	
	digitalWrite(_pinTx, !digitalRead(_pinTx));
	cbi(TIMSK2, OCIE2B);
	cbi(TIFR2, OCF2B);
	sei();
}