#ifndef DEVICE_H
#define DEVICE_H

#include "./gpio/gpio.h"
#include "./runner/runner.h"

// KC868-A4: direct GPIO relays/inputs, ADC/DAC, DS18B20, buzzer, RS232, 433/IR.
#define GPIO_RELAY1 2
#define GPIO_RELAY2 15
#define GPIO_RELAY3 5
#define GPIO_RELAY4 4

#define GPIO_INPUT1 36
#define GPIO_INPUT2 39
#define GPIO_INPUT3 27
#define GPIO_INPUT4 14

#define GPIO_DAC1 25
#define GPIO_DAC2 26

// Silkscreen A1..A4. KinCony maps A1/A2 to the 0-5 V channels and A3/A4 to 4-20 mA.
#define GPIO_ADC1 34
#define GPIO_ADC2 35
#define GPIO_ADC3 32
#define GPIO_ADC4 33

#define GPIO_DS18B20 13
#define GPIO_BEEP 18
#define GPIO_BUTTON 0

#define GPIO_433_TX 21
#define GPIO_433_RX 19
#define GPIO_IR_RX 23
#define GPIO_IR_TX 22

#define RS232_RX_PIN 16
#define RS232_TX_PIN 17

#define PORTS_LEN 4

extern Device device;

void setupDevice();
void setupFirstDevice();
void setOutput();
void getInput();
void getOutput();
void getData();
void onSendDevice();
void deviceGPIO(Port* port, uint8_t type);
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif
