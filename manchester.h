/* Manchester code for openTherm protocol
  Bit encoding method    : Manchester / Bi-phase-L
  Bit value '1'          : active-to-idle transition
  Bit value '0'          : idle-to-active transition
  Bit rate               : 1000 bits/sec nominal
  Period between 
  mid-bit transmitions   : 900us .. 1500us(nominal 1ms)
  
  
  
  Active ........|----->........-----|................
                 |                   |
                 ^                   |
                 |                   |
  idle   ..______|...................|____>>___.......
                bit 0               bit 1
            
       clk:  1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 
      data:   1   0   0   1   1   1   0   1
manchester:  1 0 0 1 0 1 1 0 1 0 1 0 0 1 1 0  
  
  A  B  XOR  !XOR
  1  1   0     1
  0  1   1     0
  1  0   1     0
  0  0   0     1
  */

#ifndef _MANCHESTER_H_
#define _MANCHESTER_H_

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

class Manchester
{
  public:

    enum Status 
    {
      Idle = 0,
      Sending = 1,
      CommandSent = 2,
      Reading = 3,
      ReadingTimeout =4,
      ReadingReady = 5,
    };

    Manchester(uint8_t tx, uint8_t rx); //ctor
    Status GetStatus();
    void SetIdle();
    uint16_t GetByte(uint8_t data);
  
    void Send(uint8_t *data, uint8_t len, boolean sendPre);
    void StartRead(uint8_t len, void (*func)());

    volatile uint8_t* GetReadData();

    void OnTxTimerMatchInterrup();

    void OnRxPinChangeInterrupt();
    void OnRxTimerOverflowInterrupt();
    void ShowStatus();


 private:
    void startTransmition();
    void stopTransmition(); 
    void stopReading(); 
    
    

    void send(uint8_t data);
    void sendBit();

    uint16_t encode(uint8_t);

    void configureTimer2(uint8_t);
  
    uint8_t _pinTx;
    uint8_t _pinRx;
    volatile uint16_t _buffer[20];
    
    volatile uint8_t _rxBuffer[40];

    volatile uint8_t _bufferLen = 0;
    volatile uint8_t _bitIndex = 0;
    volatile uint8_t _byteIndex = 0;
    
    uint16_t _timeoutCount = 1000;
    Status _status;
};


#endif
