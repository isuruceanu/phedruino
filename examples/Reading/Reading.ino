#include <manchester.h>
#include "util.h"

#define TX_PIN 8
#define RX_PIN 3 //Ext-INT1
#define LED 13

Manchester *manchester;
uint8_t *data;

void setup()
{
  cli();
  EICRA =0;
  EIMSK = 0;
		
  Serial.begin(9600);
  Serial.println("Start reading TEST");
  delay(100);
  manchester = new Manchester(TX_PIN, RX_PIN);
  sei();
}

void loop()
{
  delay(40);
  if (manchester->GetStatus() == Manchester::Idle)
  {
    //Serial.println(manchester->GetStatus());
    manchester->StartRead(1, true);
  }
  
  if (manchester->GetStatus() == Manchester::ReadingReady)
  {
    Serial.println("Reading done!");
      delay(50);
     data = manchester->GetReadData();
     Serial.print("data[0]="); Serial.println(data[0]);
     Serial.print("data[1]="); Serial.println(data[1]);
     Serial.print("data[2]="); Serial.println(data[2]);
     manchester->SetIdle();
  }
}

ISR(INT1_vect)
{
  manchester->OnPinChangeInterrupt();
}

ISR(TIMER2_COMPA_vect)
{
   manchester->OnTimerMatchAInterrupt(); 
}
