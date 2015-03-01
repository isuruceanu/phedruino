#include <manchester.h>

#define TX_PIN 12
#define RX_PIN 2
#define LED 13

Manchester *manchester;
uint8_t data[2];

static void onPinChange()
{
  manchester->OnRxPinChangeInterrupt();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Start reading prg.");
  manchester = new Manchester(TX_PIN, RX_PIN);
  pinMode(LED, OUTPUT);

}

void loop()
{
  if (manchester->GetStatus() == Manchester::Idle)
  {
    Serial.println("Start READING");
    manchester->StartRead(30);
    attachInterrupt(0, onPinChange, CHANGE);
  }  
  
  if (manchester->GetStatus() == Manchester::ReadingReady)
  {
    manchester->GetReadData();
  }
  
  manchester->ShowStatus();
  
}

ISR(TIMER2_OVF_vect)
{
   manchester->OnRxTimerOverflowInterrupt(); 
}
