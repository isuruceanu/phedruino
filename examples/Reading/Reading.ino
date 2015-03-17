#include <manchester.h>
#include "util.h"

#define TX_PIN 8
#define RX_PIN 3 //Ext-INT1
#define LED 13

Manchester *manchester;
uint8_t *data;

void setup()
{
  Serial.begin(9600);
  Serial.println("eStart reading TEST");
  delay(100);
  manchester = new Manchester(TX_PIN, RX_PIN);

}

void loop()
{
  delay(40);
  if (manchester->GetStatus() != Manchester::Reading)
  {
    Serial.println("---Reading again");
    manchester->StartRead(1, true);
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
