
#include "RF24.h"
#include <stdlib.h>
#include "pico/stdlib.h"

// nRF24L01 control pins
#define SPI_CE_PIN 14
#define SPI_CS_PIN 15

int main()
{
    RF24 radio(SPI_CE_PIN, SPI_CS_PIN);

    // Check if initialzation succeeded
    if (!radio.begin())
    {
        return 0;
    }

    // Set radio power
    radio.setPALevel(RF24_PA_LOW);
    int payload;

    // Radio module address
    uint8_t address[2][6] = {"1Node", "2Node"};
    radio.setChannel(124);
    radio.setPayloadSize(sizeof(payload));

    return 0;
}