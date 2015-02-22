#include <manchester.h>

#define M_PIN 12
#define LED 13

Manchester *manchester;
uint8_t data[2];

void setup()
{
  Serial.begin(9600);
  Serial.println("Start reading prg.");
  manchester = new Manchester(M_PIN);
}

void loop()
{
  if (manchester->GetStatus() == Manchester::Idle)
  {
    manchester->StartRead((uint8_t)8);
  }  
  
  manchester->ShowStatus();
}


