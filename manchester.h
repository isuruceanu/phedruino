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

#define TICKS_PER_MS F_CPU/64/1000   

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
    volatile Status GetStatus();
    void SetIdle();
      
    void Send(uint8_t *data, uint8_t len, boolean sendPre);
    void StartRead(uint8_t len, boolean hasStartBit);

    uint8_t* GetReadData();

    void OnTimerMatchAInterrupt();
    void OnTimerMatchBInterrupt();
    void OnPinChangeInterrupt();
    
    
 private:
    
    void configureTimer2(uint8_t);
    void stopTimer2(void);
  
    uint8_t _pinTx;
    
    uint8_t _pinRx;
    
    uint8_t* _rxBuffer;
    volatile uint8_t _bufferLen = 0;
    volatile uint8_t _bitIndex = 0;
    volatile uint8_t _first;
    
    
    volatile Status _status;
};



#endif
