#include <manchester.h>

#define TX_PIN 12
#define RX_PIN 11
#define LED 13

Manchester *manchester;
uint8_t data[2] = {0x9D, 0x26};

void setup() {
  Serial.println("Start sending");
  manchester = new Manchester(TX_PIN, RX_PIN);
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

ISR(TIMER2_COMPA_vect)
{
  manchester->OnTxTimerMatchInterrup(); 
}
