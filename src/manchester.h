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
    Manchester(uint8_t pin); //ctor
    void StartTransmition();
    void StopTransmition();
    void Transmite(uint8_t data);
    void Transmite(uint8_t *data, uint8_t len);
    
    void StartReading();
    void StopReading();
    int8_t PopQueue(struct Queue* queue);
    void PushQueue(struct Queue* queue, const uint8_t c);
 
 private:
  uint8_t _pin;
  uint16_t computeChecksum(const *uint8_t, uint16_t bytes);
    
  
}


#endif
