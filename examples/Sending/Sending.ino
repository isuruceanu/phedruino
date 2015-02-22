#include <manchester.h>

#define M_PIN 12
#define LED 13

Manchester *manchester;
uint8_t data[2] = {0x9D, 0x26};

void setup() {
  Serial.println("Start sending");
  manchester = new Manchester(M_PIN);
  pinMode(11, OUTPUT);
  Serial.begin(9600);
  Serial.println("Starting");
}

void loop() {
  if (manchester->GetStatus() == Manchester::Idle)
  {
     manchester->Send(data, (uint8_t)2, true);  
  }
  
  Serial.println("end of loop");
}

ISR(TIMER0_COMPA_vect)
{
  manchester->OnInterrupt(); 
}
