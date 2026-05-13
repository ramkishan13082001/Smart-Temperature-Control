# 🌡️ Smart Temperature-Controlled Fan System

An automated climate control system built using the **PIC18F4580 microcontroller**. This project continuously monitors real-time environmental temperature using an **LM35 sensor**, dynamically adjusts a cooling fan's speed via **PWM**, sounds an audible alert during critical conditions, and displays real-time statuses on a **16x2 LCD display**.

---

## ⚙️ System Working Conditions

The microcontroller continuously polls the LM35 sensor and executes logic routines based on three temperature thresholds:

### 🔵 Low Temperature
* **Threshold:** Below 30°C
* **Fan State:** Completely OFF (0% Duty Cycle)
* **LCD Readout:** `STATUS: NORMAL`
* **Buzzer State:** OFF

### 🟡 Medium Temperature
* **Threshold:** 30°C – 40°C
* **Fan State:** ON at Medium Speed (50% Duty Cycle)
* **LCD Readout:** Live Temperature Updates + `STATUS: FAN ON`
* **Buzzer State:** OFF

### 🔴 High Temperature
* **Threshold:** Above 40°C
* **Fan State:** Full Speed (100% Duty Cycle)
* **LCD Readout:** `STATUS: OVERHEAT`
* **Buzzer State:** ON (Continuous Alarm)

---

## 🔌 Hardware Connection Guide

> [!CAUTION]
> **Critical Note on the Fan Driver:** A PIC microcontroller pin can only output ~25mA of current, whereas a DC cooling fan requires 200mA+. You **MUST** use a Motor Driver IC (like the L293D) or a power transistor (like TIP122) between the RC2 PWM pin and the Fan to prevent damaging the microcontroller.

Refer strictly to the mapping tables below to wire your physical hardware or configure your Proteus simulation workspace.

### 📺 Microcontroller to LCD 16x2

| PIC18F4580 Pin | LCD Pin | Description |
| :--- | :--- | :--- |
| **RB0** | Pin 4 (RS) | Register Select |
| **GND** | Pin 5 (RW) | Read/Write (Grounded for write-only) |
| **RB1** | Pin 6 (EN) | Enable Signal |
| **RD0 - RD7** | Pin 7 - 14 | 8-Bit Data Bus (D0 - D7) |
| **+5V / GND** | Pins 2 / 1 | Display Power Supply |

### 🔌 Sensors and Actuators

| Peripheral | PIC18F4580 Pin | Configuration Details |
| :--- | :--- | :--- |
| **LM35 Sensor** | AN0 (Pin 2) | Analog Input. Connect LM35 VCC to +5V and GND to GND. Output goes directly to AN0. |
| **DC Fan** *(Proteus: MOTOR)* | RC2 (CCP1) | PWM Output. In Proteus, search for `MOTOR` (Active DC Motor). Route through the L293D driver. |
| **Buzzer** | RC0 | Digital Output. Use an NPN transistor (e.g., BC547) if buzzer current exceeds 25mA. |

### ⚡ L293D Motor Driver Connections (for DC Fan)

| L293D Pin | Connection Point | Purpose |
| :--- | :--- | :--- |
| **Pin 1 (Enable 1)** | +5V | Enables the first channel of the driver |
| **Pin 2 (Input 1)** | PIC18F4580 Pin RC2 | Receives the PWM speed control signal |
| **Pin 3 (Output 1)** | DC Fan Terminal 1 | Power Out to Fan (+) |
| **Pin 4 & 5 (GND)** | GND | Common Ground |
| **Pin 6 (Output 2)** | DC Fan Terminal 2 | Power Out to Fan (-) |
| **Pin 7 (Input 2)** | GND | Grounded to force fan spin in one direction only |
| **Pin 8 (Vs / VCC2)** | +9V or +12V | High Current Motor Power Supply |
| **Pin 16 (Vss / VCC1)** | +5V | 5V Logic Power Supply for the IC |

---

## 💻 Firmware Implementation

Copy this complete XC8 C code into your **MPLAB X IDE**. It features fully integrated drivers for Analog-to-Digital conversion, Timer2 PWM generation, and 8-bit LCD interfacing.

### `main.c` (XC8 Compiler)

```c
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
```

---

## 🧩 Code Architecture & Logic

* **`adc_read()`**: Automates hardware data conversion via the microcontroller's analog module. By asserting the `GO` bit, it polls the internal registers synchronously until completion, packing the split data from registers `ADRESH` and `ADRESL` into a standard 10-bit integer.
* **The Math Magic**: The LM35 precision sensor generates an absolute linear output of $10\text{mV}/\degree\text{C}$. The software converts raw 10-bit step signals ($0-1023$) against a $5.0\text{V}$ reference into a real-world voltage value. Scaling this result by $100$ produces an accurate temperature integer value directly in Celsius.
* **`pwm_init()`**: Sets up Timer2 alongside the hardware CCP1 (Capture/Compare/PWM) peripheral block. This allows the system to sustain high-frequency background PWM signals autonomously, saving vital CPU processing cycles.
* **`pwm_set_duty()`**: Accepts values ranging between $0$ (stalled) up to $1023$ (maximum throughput limits) to deliver smooth duty cycle modulations to the external motor controller driver circuitry.
