#include <manchester.h>

#define M_PIN 12

Manchester *manchester;
uint8_t data[2];

void setup() {
  Serial.println("Start sending");
  manchester = new Manchester(M_PIN);
  data[0] = 0x9D;
  data[1] = 0x26;
}

void loop() {
  if (manchester->GetStatus())
  {
     Serial.println("Sending data");
     manchester->Send(data, (uint8_t)2);
  }
  
}

ISR(TIMER0_COMPA_vect)
{
   manchester->OnInterrupt(); 
}
