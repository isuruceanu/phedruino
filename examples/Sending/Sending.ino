#include <manchester.h>

#define TX_PIN 8
#define RX_PIN 11
#define LED 13

Manchester *manchester;
uint8_t data[1] = {0x9D};

void setup() {
  manchester = new Manchester(TX_PIN, RX_PIN);
  Serial.begin(9600);
  Serial.println("Starting");
}

void loop() {
  delay(1000);
  Serial.println("sending...");
  manchester->Send(data, (uint8_t)1, true);

}

ISR(TIMER2_COMPA_vect)
{
  manchester->OnTimerMatchAInterrupt();
}

ISR(TIMER2_COMPB_vect)
{
  manchester->OnTimerMatchBInterrupt();
}
