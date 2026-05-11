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