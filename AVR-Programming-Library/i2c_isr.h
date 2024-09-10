#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#define I2C_SCL_FREQUENCY_PRESCALER 1 // possible values 1, 4, 16, 64

#define I2C_READY 0
#define I2C_BUSY 1
#define I2C_DONE 2
#define I2C_ERROR 3

volatile uint8_t i2c_status = I2C_READY;

/**
 * \brief Performs the next i2c action. This funstion must be called in the main loop as frequently as possible.
 * 
 * \param No parameters
 * 
 * \return void
 */
void i2c_run(void);

/**
 * \brief Performs the initialization routine for I2C device. 
 *		Should be called just once, before any work with I2C device.
 * 
 * \param unsigned long Frequency for I2C device in Hz. Typically - I2C_SCL_FREQUENCY_100
 *  (100 000 Hz) or I2C_SCL_FREQUENCY_400 (400 000 Hz)
 * 
 * \return void
 */
void i2c_master_init(unsigned long frequency);

/**
 * \brief Sends sequence of data bytes to Slave device with specified address. Sends START and STOP conditions.
 * 
 * \param address Typical I2C Slave Device address (not shifted)
 * \param data Array of bytes for sending
 * \param length Length of byte array
 * 
 * \return uint8_t On success - I2C_STATUS_SUCCESS. Otherwise - one of I2C_STATUS_ERROR codes.
 */
void i2c_master_send(uint8_t address, uint8_t* data, uint16_t length);