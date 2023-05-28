// LCD pins

#define RS      12
#define STRB    13

/* pin number to activate/deactivate the servo */
#define SERVO_PIN 7

/* pin number to activate/deactivate the siren */
#define SIREN_PIN 8

/* Device adress */
#define I2C_ADDR 0x50
#define ADC_ADDRESS 0x77
#define EEPROM_ADDR 0x50
#define COEFF_REG 0x10
/* GPIO18 - LED1 -P18 */
#define XCLR_PIN 11
/* Resolution (see data sheet) */
#define LM92_RES 0.0625

// Max number of LED display lines
#define MAX_LINES 4

// sonar pins
#define TRIGGER_PIN 14
#define ECHO_PIN  15

