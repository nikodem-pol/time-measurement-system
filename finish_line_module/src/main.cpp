#include "RF24.h"
#include <stdlib.h>
#include "pico/stdlib.h"

#define BUILT_IN_LED 25
#define BEAM_BREAK_SENSOR 6
#define RED_LED 0

// nRF24L01 control pins
#define SPI_CE_PIN 14
#define SPI_CS_PIN 15

enum State : uint8_t
{
    inactive = 0,
    active = 1
};

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

    // Open writing pipe with address "1Node"
    radio.openWritingPipe(address[0]);

    // Open writing pipe with address "2Node"
    radio.openReadingPipe(1, address[1]);

    stdio_init_all();

    gpio_init(BUILT_IN_LED);
    gpio_set_dir(BUILT_IN_LED, GPIO_OUT);
    gpio_put(BUILT_IN_LED, true);

    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_put(RED_LED, false);

    gpio_init(BEAM_BREAK_SENSOR);
    gpio_set_dir(BEAM_BREAK_SENSOR, GPIO_IN);
    gpio_pull_up(BEAM_BREAK_SENSOR);

    State currentState = State::inactive;

    // Switch to RX mode
    radio.startListening();

    while (true)
    {
        while (currentState == State::inactive)
        {
            bool sensorState = gpio_get(BEAM_BREAK_SENSOR);
            gpio_put(RED_LED, sensorState);

            if (radio.available() && sensorState)
            {
                auto bytes = radio.getPayloadSize();
                radio.read(&payload, bytes);

                gpio_put(BUILT_IN_LED, false);

                // Switch to TX mode
                radio.stopListening();
                currentState = State::active;
            }
        }

        while (currentState == State::active)
        {
            if (!gpio_get(BEAM_BREAK_SENSOR))
            {
                for (int i = 0; i < 5; ++i)
                {
                    if (radio.write(&payload, sizeof(payload)))
                    {
                        // Switch to RX mode
                        radio.startListening();
                        currentState = State::inactive;
                        gpio_put(BUILT_IN_LED, true);
                        break;
                    }
                }
            }
        }
    }

    return 0;
}