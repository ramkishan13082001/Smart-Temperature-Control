System Working Conditions
The microcontroller continuously polls the LM35 sensor and makes decisions based on the following three temperature thresholds:

Low Temperature
Below 30°C
Fan is completely OFF
LCD reads: NORMAL
Buzzer is OFF
Medium Temperature
30°C – 40°C
Fan turns ON at Medium Speed
PWM Duty Cycle dynamically adjusts
LCD updates live temperature
High Temperature
Above 40°C
Fan spins at Full Speed (100% Duty Cycle)
Buzzer sounds ALARM
LCD reads: OVERHEAT!
🔌
Complete Connection Guide
To successfully build this project, refer strictly to the pinouts mapped below.

Microcontroller to LCD 16x2
PIC18F4580 Pin	LCD Pin	Description
RB0	Pin 4 (RS)	Register Select
GND	Pin 5 (RW)	Read/Write (Grounded for write-only)
RB1	Pin 6 (EN)	Enable Signal
RD0 - RD7	Pin 7 - 14	8-Bit Data Bus
+5V / GND	Pins 2 / 1	Display Power
Sensors and Actuators
Peripheral	PIC18F4580 Pin	Configuration Details
LM35 Sensor	AN0 (Pin 2)	Analog Input. Connect LM35 VCC to +5V and GND to GND. Output goes directly to AN0.
DC Fan (Proteus: MOTOR)	RC2 (CCP1)	PWM Output. In Proteus, search for MOTOR (Active DC Motor). Do NOT connect directly to PIC. RC2 goes to L293D Input 1.
Buzzer	RC0	Digital Output. Use an NPN transistor (e.g. BC547) if buzzer current exceeds 25mA.
Critical Note on the Fan Driver:
A PIC microcontroller pin can only output ~25mA of current. A DC cooling fan requires 200mA+. You MUST use a Motor Driver IC (like the L293D) or a power transistor (like TIP122) between the RC2 PWM pin and the Fan.
L293D Motor Driver Connections (for DC Fan)
L293D Pin	Connection Point	Purpose
Pin 1 (Enable 1)	+5V	Enables the first channel of the driver
Pin 2 (Input 1)	PIC18F4580 Pin RC2	Receives the PWM speed control signal
Pin 3 (Output 1)	DC Fan Terminal 1	Power Out to Fan (+)
Pin 4 & 5 (GND)	GND	Common Ground
Pin 6 (Output 2)	DC Fan Terminal 2	Power Out to Fan (-)
Pin 7 (Input 2)	GND	Grounded to force fan spin in one direction only
Pin 8 (Vs / VCC2)	+9V or +12V	High Current Motor Power Supply
Pin 16 (Vss / VCC1)	+5V	5V Logic Power Supply for the IC
💻
Complete Project Code
Copy this complete XC8 C code into MPLAB X IDE. It includes the logic for Analog-to-Digital conversion, Timer2 PWM, and LCD interfacing.

main.c (XC8 Compiler)
#include <xc.h>
#include <stdio.h> // For sprintf

#define _XTAL_FREQ 20000000

// --- Function Prototypes ---
void lcd_init();
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_string(const char *str);
void adc_init();
int  adc_read(int channel);
void pwm_init();
void pwm_set_duty(unsigned int duty);
void delay_ms(unsigned int ms);

void main() {
    int adc_value;
    float voltage;
    int temperature;
    char buffer[16];

    // --- Port Configuration ---
    TRISD = 0x00;   // PORTD as output (LCD Data)
    TRISB0 = 0;     // RB0 as output (LCD RS)
    TRISB1 = 0;     // RB1 as output (LCD EN)
    TRISC0 = 0;     // RC0 as output (Buzzer)
    TRISC2 = 0;     // RC2 as output (PWM Fan)
    
    RC0 = 0; // Ensure Buzzer is OFF initially

    // --- Subsystem Initialization ---
    lcd_init();
    adc_init();
    pwm_init();

    lcd_string("SMART FAN SYSTEM");
    delay_ms(2000);
    lcd_command(0x01); // Clear Display

    // --- Main Automation Loop ---
    while(1) {
        // 1. Read LM35 Sensor
        adc_value = adc_read(0); // Read AN0
        
        // 2. Calculate Temperature
        // 10-bit ADC (1024 steps). Vref = 5V. 
        // LM35 outputs 10mV per degree Celsius.
        voltage = (adc_value * 5.0) / 1024.0;
        temperature = (int)(voltage * 100.0); 

        // 3. Update LCD Line 1
        lcd_command(0x80); 
        sprintf(buffer, "TEMP = %d C   ", temperature);
        lcd_string(buffer);

        // 4. Execute Logic Rules
        lcd_command(0xC0); // Move cursor to Line 2
        
        if (temperature < 30) {
            pwm_set_duty(0);    // 0% Duty -> Fan OFF
            RC0 = 0;            // Buzzer OFF
            lcd_string("STATUS: NORMAL  ");
        } 
        else if (temperature >= 30 && temperature <= 40) {
            pwm_set_duty(512);  // 50% Duty -> Fan Medium
            RC0 = 0;            // Buzzer OFF
            lcd_string("STATUS: FAN ON  ");
        } 
        else if (temperature > 40) {
            pwm_set_duty(1023); // 100% Duty -> Fan Full Speed
            RC0 = 1;            // Buzzer ON (ALARM!)
            lcd_string("STATUS: OVERHEAT");
        }
        
        delay_ms(500); // Sample every 0.5 seconds
    }
}

// ---------------- ADC Functions ----------------
void adc_init() {
    ADCON0 = 0x01; // Enable ADC, Channel 0
    ADCON1 = 0x0E; // Vref = VDD/VSS, set AN0 to Analog, others digital
    ADCON2 = 0xA9; // Right justified result, Fosc/8, 12 TAD
}

int adc_read(int channel) {
    ADCON0 = (unsigned char)((channel << 2) | 0x01); // Load channel
    ADCON0bits.GO = 1; // Start conversion
    while(ADCON0bits.GO); // Wait for conversion to finish
    return ((ADRESH << 8) + ADRESL); // Return 10-bit result
}

// ---------------- PWM Functions ----------------
void pwm_init() {
    PR2 = 0xFF;    // Set PWM Period
    CCP1CON = 0x0C;// Configure CCP1 module for PWM mode
    T2CON = 0x06;  // Turn on Timer2 with Prescaler 16
}

void pwm_set_duty(unsigned int duty) {
    // duty limits: 0 (0%) to 1023 (100%)
    CCPR1L = duty >> 2; // Store top 8 bits
    CCP1CON = (unsigned char)((CCP1CON & 0xCF) | ((duty & 0x03) << 4));
}

// ---------------- LCD & Delay Utilities ----------------
void lcd_init() {
    delay_ms(15);
    lcd_command(0x38); // 8-bit mode, 2 lines
    lcd_command(0x0C); // Display ON, Cursor OFF
    lcd_command(0x06); // Auto-increment cursor
    lcd_command(0x01); // Clear display
    delay_ms(2);
}

void lcd_command(unsigned char cmd) {
    PORTD = cmd;
    PORTBbits.RB0 = 0; // RS = 0 for command
    PORTBbits.RB1 = 1; // EN = 1
    delay_ms(1);
    PORTBbits.RB1 = 0; // EN = 0
}

void lcd_data(unsigned char data) {
    PORTD = data;
    PORTBbits.RB0 = 1; // RS = 1 for data
    PORTBbits.RB1 = 1; // EN = 1
    delay_ms(1);
    PORTBbits.RB1 = 0; // EN = 0
}

void lcd_string(const char *str) {
    while(*str) {
        lcd_data(*str++);
    }
}

void delay_ms(unsigned int ms) {
    while(ms--) {
        __delay_ms(1);
    }
}
🧩 Code Explanation
adc_read(): Starts the hardware analog-to-digital conversion by setting the GO bit. It then waits in a while loop until the conversion is finished before reading the 10-bit result from ADRESH and ADRESL.
Math Magic: The LM35 outputs exactly 10 millivolts (0.01V) per degree. By multiplying the raw ADC value by 5.0 (system voltage) and dividing by 1024, we get the real voltage. Multiplying that voltage by 100 instantly converts it into degrees Celsius!
pwm_init(): Configures the CCP1 hardware module and turns on Timer2. This generates a continuous hardware PWM wave in the background without tying up the main CPU loop.
pwm_set_duty(): The duty cycle accepts a value from 0 to 1023 (10-bit resolution). We send 0 for fan OFF, 512 for 50% power, and 1023 for full 100% blast.
