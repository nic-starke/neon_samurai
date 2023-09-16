
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

static int test = 0;

ISR(testisr)
{

}

int main (void)
{
  wdt_enable(WDTO_2S);
  sei();

  while (1)
  {
    wdt_reset();
  }
}