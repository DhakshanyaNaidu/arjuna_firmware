# ARJUNA v3.0 — Wiring Guide
# All pins verified from:
#   - Heltec WiFi_Kit_series/variants/heltec_wifi_lora_32_V3/pins_arduino.h
#   - Meshtastic firmware/variants/esp32s3/heltec_v3/variant.h
#   - Datasheet HTIT-WB32LA_V3_Rev1_1.pdf

## Fixed Internal Hardware (do NOT rewire)

| Signal      | GPIO | Notes                                  |
|-------------|------|----------------------------------------|
| OLED SDA    | 17   | SDA_OLED — internal PCB trace          |
| OLED SCL    | 18   | SCL_OLED — internal PCB trace          |
| OLED RST    | 21   | RST_OLED — internal PCB trace          |
| LoRa CS     | 8    | Internal SPI                           |
| LoRa DIO1   | 14   | IRQ — RadioLib ISR pin                 |
| LoRa RST    | 12   | Internal                               |
| LoRa BUSY   | 13   | Internal                               |
| LoRa MOSI   | 10   | Internal SPI                           |
| LoRa MISO   | 11   | Internal SPI                           |
| LoRa SCK    | 9    | Internal SPI                           |
| Vext        | 36   | Drive LOW in setup() — powers OLED+LoRa|
| ADC_CTRL    | 37   | Drive LOW in setup() — battery ADC     |
| VBAT sense  | 1    | ADC1_CH0 — battery voltage             |
| LED         | 35   | LED_BUILTIN                            |
| UART TX     | 43   | Serial (USB-CDC)                       |
| UART RX     | 44   | Serial (USB-CDC)                       |

## External Wiring Required

### 4x3 Keypad Matrix
Wire keypad rows and columns to these GPIO pins.
All are bidirectional with internal pull-up support.

| Keypad Pin | GPIO | Direction    | Datasheet ref |
|------------|------|--------------|---------------|
| Row 1      | 3    | OUTPUT       | J3 pin 14     |
| Row 2      | 4    | OUTPUT       | J3 pin 15     |
| Row 3      | 5    | OUTPUT       | J3 pin 16     |
| Row 4      | 6    | OUTPUT       | J3 pin 17     |
| Col 1      | 33   | INPUT_PULLUP | J2 pin 12     |
| Col 2      | 34   | INPUT_PULLUP | J2 pin 11     |
| Col 3      | 38   | INPUT_PULLUP | J3 pin 11     |

```
Keypad physical layout:
  [1] [2] [3]     Row1(GPIO3)
  [4] [5] [6]     Row2(GPIO4)
  [7] [8] [9]     Row3(GPIO5)
  [*] [0] [#]     Row4(GPIO6)
   |   |   |
  C1  C2  C3
 (33)(34)(38)
```

### Buzzer
| Signal    | GPIO | Notes               |
|-----------|------|---------------------|
| Buzzer +  | 2    | LEDC PWM channel 0  |
| Buzzer -  | GND  |                     |

Use a passive buzzer (not active) for best tone control.
Maximum GPIO current is 40mA — add a transistor for louder buzzers.

## AVOID these GPIOs for external use
| GPIO     | Reason                                      |
|----------|---------------------------------------------|
| 8-14     | LoRa SPI (CS, CLK, MOSI, MISO, RST, BUSY, DIO1) |
| 17,18    | OLED I2C (SDA_OLED, SCL_OLED)              |
| 19,20    | USB D-/D+ (internal)                       |
| 21       | OLED RST                                   |
| 26-32    | Internal flash SPI (do not use)            |
| 35       | LED_BUILTIN                                |
| 36       | Vext power control                         |
| 37       | ADC_CTRL battery divider                   |
| 43,44    | UART0 TX/RX (Serial monitor)               |
| 45       | Strapping pin (boot mode)                  |
| 46       | Input only, strapping                      |
