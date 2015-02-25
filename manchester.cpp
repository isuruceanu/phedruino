#include "manchester.h"
#include "util.h"

Manchester::Manchester(uint8_t tx, uint8_t rx)
{
	_pinTx = tx;
	_pinRx = rx;
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

void Manchester::OnTxTimerMatchInterrup()
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

	digitalWrite(_pinTx, isset(_buffer[_byteIndex], 15- _bitIndex));
	
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
	pinMode(_pinTx, OUTPUT);

	configureTimer2(124); // = (16*10^6) / (2000 * 64) -1 (must be less 255));
  	
  	// enable timer compare interrupt
  	sbi(TIMSK2,OCIE2A);

  	//allow interrupt
  	sei();
}

void Manchester::stopTransmition()
{
	cbi(TIMSK2,OCIE2A);
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
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
  	// Set CS01 and CS00 bits for 64 prescaler
  	sbi(TCCR2B, CS20);
  	sbi(TCCR2B, CS21);
}

/*
	 Reading: 
 1. Set the Rx pin as output with pull up
 2. Disable interuption
 3. Enable pin change interruption to identify the transition
 4. Configure timer2 with Overflow interrupt, need to identify timeout
 5. Read line status so we know where it started
 6. enable interruptions
 */
void Manchester::StartRead(uint8_t len)
{
	if (_status != Idle) return;
	
	pinMode(_pinRx, INPUT); //set pin as input
	digitalWrite(_pinTx, HIGH); // enable pullup resistor
	_byteIndex = 0;
	_timeoutCount = 50;
	_bufferLen = len;
	
	uint8_t i =0;
	do
	{
		_rxBuffer[i] = 0;		
		i++;
	}while(i < _byteIndex);
	
	cli();
	
	_status = Reading;

	configureTimer2(0xFF);
	sbi(TIMSK2, TOIE2);

	_lineStatus = digitalRead(_pinRx);

	sei();
}

void Manchester::stopReading()
{
	cli();
	cbi(TIMSK2, TOIE2);
	detachInterrupt(0);
	sei();
}

uint8_t * Manchester::GetReadData()
{
	if (_status = ReadingReady)
	{
		Serial.print("ByteIndex=");
		Serial.println(_byteIndex);
		Serial.print("Line=");
		Serial.println(_lineStatus);
		Serial.println("_rxBuffer");
		
		uint8_t i =0;
		do
		{
			Serial.println(_rxBuffer[i]);
			i++;
		}while(i < _byteIndex);
	}
	return _rxBuffer;
}

/* 
	When a transition is detected store the timer1 OCR0A value to buffer
	and clear the OCR0A
*/
void Manchester::OnRxPinChangeInterrupt()
{
    _rxBuffer[_byteIndex] = TCNT2;
    TCNT2 = 0;
    
    if (_byteIndex >= _bufferLen)
    {
    	stopReading();
    	_status = ReadingReady;
    }
    _byteIndex++;
}

void Manchester::OnRxTimerOverflowInterrupt()
{
	if (_timeoutCount-- < 1)
	{
		stopReading();
		_status = ReadingTimeout;
	}
}

