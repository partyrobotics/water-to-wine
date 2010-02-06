#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Bit manipulation macros
#define sbi(a, b) ((a) |= 1 << (b))       //sets bit B in variable A
#define cbi(a, b) ((a) &= ~(1 << (b)))    //clears bit B in variable A
#define tbi(a, b) ((a) ^= 1 << (b))       //toggles bit B in variable A

#define STATE_IDLE                  1
#define STATE_DISPENSE              2
#define STATE_OUT_OF_WATER          3
#define STATE_OUT_OF_WINE           4
#define STATE_OUT_OF_WATER_AND_WINE 5

#define TRANSITION_DISPENSE_START   1
#define TRANSITION_DISPENSE_STOP    2
#define TRANSITION_WATER_HI         3
#define TRANSITION_WATER_LOW        4
#define TRANSITION_WINE_HI          5
#define TRANSITION_WINE_LOW         6

typedef struct 
{
    unsigned char old_state;
    unsigned char transition;
    unsigned char new_state;
} Transition;

#define NUM_TRANSITIONS 12
Transition transition_table[NUM_TRANSITIONS] = 
{
    { STATE_IDLE,                  TRANSITION_DISPENSE_START, STATE_DISPENSE              },
    { STATE_IDLE,                  TRANSITION_WINE_LOW,       STATE_OUT_OF_WINE           },
    { STATE_IDLE,                  TRANSITION_WATER_LOW,      STATE_OUT_OF_WATER          },

    { STATE_DISPENSE,              TRANSITION_DISPENSE_STOP,  STATE_IDLE                  },
    { STATE_DISPENSE,              TRANSITION_WINE_LOW,       STATE_OUT_OF_WINE           },
    { STATE_DISPENSE,              TRANSITION_WATER_LOW,      STATE_OUT_OF_WATER          },

    { STATE_OUT_OF_WINE,           TRANSITION_WINE_HI,        STATE_IDLE                  },
    { STATE_OUT_OF_WINE,           TRANSITION_WATER_LOW,      STATE_OUT_OF_WATER_AND_WINE },

    { STATE_OUT_OF_WATER,          TRANSITION_WATER_HI,       STATE_IDLE                  },
    { STATE_OUT_OF_WATER,          TRANSITION_WINE_LOW,       STATE_OUT_OF_WATER_AND_WINE },

    { STATE_OUT_OF_WATER_AND_WINE, TRANSITION_WATER_HI,       STATE_OUT_OF_WINE           },
    { STATE_OUT_OF_WATER_AND_WINE, TRANSITION_WINE_HI,        STATE_OUT_OF_WATER          },
};


unsigned char is_dispense(void)
{
    return !(PINA & (1<<PINA0));
}

unsigned char is_water_low(void)
{
    return !(PINA & (1<<PINA1));
}

unsigned char is_wine_low(void)
{
    return !(PINA & (1<<PINA2));
}

void water_on(void)
{
    sbi(PORTB, 2);
}

void water_off(void)
{
    cbi(PORTB, 2);
}

void wine_on(void)
{
    sbi(PORTB, 3);
}

void wine_off(void)
{
    cbi(PORTB, 3);
}

void power_led_on(void)
{
    sbi(PORTA, 7);
}

void power_led_off(void)
{
    cbi(PORTA, 7);
}

void wine_led_on(void)
{
    sbi(PORTA, 6);
}

void wine_led_off(void)
{
    cbi(PORTA, 6);
}

void water_led_on(void)
{
    sbi(PORTA, 5);
}

void water_led_off(void)
{
    cbi(PORTA, 5);
}

void state_idle(void)
{
    water_off();
    wine_off();

    power_led_on();
    wine_led_off();
    water_led_off();
}

void state_dispense(void)
{
    water_on();
    wine_on();
}

void state_out_of_wine(void)
{
    water_off();
    wine_off();
    wine_led_on();
    water_led_off();
}

void state_out_of_water_and_wine(void)
{
    wine_led_on();
    water_led_on();
}

void state_out_of_water(void)
{
    water_off();
    wine_off();
    water_led_on();
    wine_led_off();
}

unsigned char get_transition(void)
{
    static unsigned char water_float = 0;
    static unsigned char wine_float = 0;
    static unsigned char dispense_switch = 0;

    for(;;)
    {
        if (is_water_low() != water_float)
        {
            water_float = !water_float;
            return water_float ? TRANSITION_WATER_LOW : TRANSITION_WATER_HI;
        }
        if (is_wine_low() != wine_float)
        {
            wine_float = !wine_float;
            return wine_float ? TRANSITION_WINE_LOW : TRANSITION_WINE_HI;
        }
        if (is_dispense() != dispense_switch)
        {
            dispense_switch = !dispense_switch;
            return dispense_switch ? TRANSITION_DISPENSE_START : TRANSITION_DISPENSE_STOP;
        }
        _delay_ms(10);
    }
}

void pin_setup(void)
{
    // Pull down inputs
    PINA &= ~((1<<PA0)|(1<<PA1)|(1<<PA2));

    // Pins used for output
    // Wine pump, water valve
    DDRB |= (1<<PB3)|(1<<PB2);
    // Water LED, Wine LED, Power LED
    DDRA |= (1<<PA5)|(1<<PA6)|(1<<PA7);
    // On board status LED
    DDRD |= (1<<PD7);
}

void flash_led(unsigned char num)
{
    unsigned char i, j;

    for(j = 0; j < num; j++)
    {
        wine_led_on();
        water_led_on();
        power_led_on();
        cbi(PORTD, 7);
        for(i = 0; i < 10; i++)
            _delay_ms(10);

        wine_led_off();
        water_led_off();
        power_led_off();
        sbi(PORTD, 7);
        for(i = 0; i < 10; i++)
            _delay_ms(10);
    }
}

int main(void)
{
    pin_setup();
    flash_led(3);

    unsigned char state = STATE_IDLE;
    unsigned char new_state = 0, trans, i;
    state_idle();

    for(;;)
    {
        trans = get_transition();
        for(i = 0; i < NUM_TRANSITIONS; i++)
        {
            if (transition_table[i].old_state == state && transition_table[i].transition == trans)
            {
                new_state = transition_table[i].new_state;
                break;
            }
        }

        switch(new_state)
        {
            case STATE_IDLE:
                state_idle();
                break;

            case STATE_DISPENSE:
                state_dispense();
                break;

            case STATE_OUT_OF_WATER:
                state_out_of_water();
                break;

            case STATE_OUT_OF_WINE:
                state_out_of_wine();
                break;

            case STATE_OUT_OF_WATER_AND_WINE:
                state_out_of_water_and_wine();
                break;
        }
        state = new_state;
    }
}
