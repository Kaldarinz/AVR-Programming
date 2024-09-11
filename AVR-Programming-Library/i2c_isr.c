#ifndef __AVR_ATmega328P__
#define __AVR_ATmega328P__
#endif

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include "i2c_isr.h"

#define ERR_NO_ERR 0
#define ERR_START 1
#define ERR_NOT_RESPOND 2

#define TWSR_READ_MASK 0b11111000

#define STATUS_START_SENT       0x08 // A START condition transmitted
#define STATUS_RESTART_SENT     0x10 // A repeated START condition transmitted
#define STATUS_MT_ADDR_ACK      0x18 // SLA+W transmitted; ACK recieved
#define STATUS_MT_ADDR_NACK     0x20 // SLA+W transmitted; NACK recieved
#define STATUS_MT_DATA_ACK      0x28 // Data transmitted; ACK recieved
#define STATUS_MT_DATA_NACK     0x30 // Data transmitted; NACK recieved
#define STATUS_MT_ARB_LOST      0x38 // Arbitration lost in SLA+W or data bytes
#define STATUS_MR_ADDR_ACK      0x40 // SLA+R transmitted; ACK recieved
#define STATUS_MR_ADDR_NACK     0x48 // SLA+R transmitted; NACK recieved
#define STATUS_MR_DATA_ACK      0x50 // Data recieved; ACK transmitted
#define STATUS_MR_DATA_NACK     0x58 // Data recieved; NACK transmitted 

#define MODE_NOT_SET 0 // Mode not defined
#define MODE_MT 1 // Master transmitter mode
#define MODE_MR 2 // Master reciever mode
#define MODE_ST 3 // Slave transmitter mode
#define MODE_SR 4 // Slave reciever mode

#define ADDR_NOT_SET 0x80

#define MAX_NACK 5 // Maximum amount of resends

volatile uint8_t addr = ADDR_NOT_SET; // Address of current slave
volatile uint8_t mode = MODE_NOT_SET; // Current i2c mode
uint8_t cur_data_index = 0;
uint8_t trans_data_len = 0;
uint8_t rec_data_len = 0;
uint8_t* trans_data;
uint8_t* rec_data;
uint8_t failed_ind = 0;



uint8_t i2c_run(void)
{
    uint8_t err = ERR_NO_ERR;
    uint8_t cur_stat = 0;
    // Do nothing if i2c operation is in progress.
    if ( !(TWCR & (1<<TWINT)) );
    // Otherwise take an action according to current i2c status
    else
    {       
        cur_stat = TWSR & TWSR_READ_MASK;
        switch (cur_stat)
        {
        // Start command sent
        case STATUS_START_SENT:
            // Reset failed attempts counter
            failed_ind = 0;   
            // Try to choose slave
            i2c_choose_slave();
            break;
        // Address for transmission acknowledged by slave
        case STATUS_MT_ADDR_ACK:
            //reset failed attempts counter
            failed_ind = 0;
            // Try to send next byte
            i2c_send_next_byte();
            break;
        // Address for transmission not acknowledged by slave
        case STATUS_MT_ADDR_NACK:
            // Increase failed attempts counter
            failed_ind++;
            // Try to choose slave
            i2c_choose_slave();
            break;
        // Data byte acknowledged by slave
        case STATUS_MT_DATA_ACK:
            // Reset failed attempts counter
            failed_ind = 0;
            // Try to send next byte
            i2c_send_next_byte();
            break;
        // Data not acknowledged by slave
        case STATUS_MT_DATA_NACK:
            // Increase failed attempts counter
            failed_ind++;
            // Resend last byte
            i2c_resend_byte();
            break;
        // Address for recieve acknowledged by slave
        case STATUS_MR_ADDR_ACK:
            // Reset failed attempts counter
            failed_ind = 0;
            // Request next byte
            i2c_request_next_byte();
            break;
        // Address for recieve not acknowledged by slave
        case STATUS_MR_ADDR_NACK:
            // Increase failed attempts counter
            failed_ind++;
            // Try to choose slave
            i2c_choose_slave();
            break;
        // Data recieved, acknowledge sent to slave
        case STATUS_MR_DATA_ACK:
            i2c_read_byte();
            //Request next byte
            i2c_request_next_byte();
            break;
        // Data recieved, not acknowledge sent to slave
        case STATUS_MR_DATA_NACK:
            i2c_read_byte();
            // Send stop command.
            TWCR &= ~(1<<TWSTA);
            TWCR |= (1<<TWSTO) | (1<<TWINT);
            i2c_status = I2C_DONE;
            i2c_clear();
            break;
        default:
            break;
        }
    }
    return cur_stat;
}

void i2c_read_byte(void)
{
    rec_data[cur_data_index] = TWDR;
    printString("Read byte: ");
    printByte(cur_data_index);
    printString("\nValue = ");
    printByte(TWDR);
    printString("\n");
    // Increase data index
    cur_data_index++;
}
/**
 * \brief Requests next by from slave by setting TWCR register.
 * Decides to send ACK or NACK based on cur_data_index and rec_data_len.
 */
void i2c_request_next_byte(void)
{
    // Check if we requesting not the last byte
    if ( (cur_data_index+1)<rec_data_len )
    {
        // Request next byte and send ACK in return
        TWCR &= ~(1<<TWSTA)&~(1<<TWSTO);
        TWCR |= (1<<TWINT)|(1<<TWEA);
    }
    else
    {
        // Request next byte and send NACK in return
        TWCR &= ~(1<<TWSTA)&~(1<<TWSTO)&~(1<<TWEA);
        TWCR |= (1<<TWINT);
    }
}

void i2c_send_next_byte(void)
{
    if (cur_data_index<trans_data_len)
    {
        // Load next byte
        TWDR = trans_data[cur_data_index];
        // Increment data index
        cur_data_index++;
        // Send data
        TWCR &= ~(1<<TWSTA)&~(1<<TWSTO);
        TWCR |= (1<<TWINT);
    }
    // Send STOP if all data were sent
    else
        // Send stop command
        {
            TWCR &= ~(1<<TWSTA);
            TWCR |= (1<<TWSTO) | (1<<TWINT);
            i2c_status = I2C_DONE;
            i2c_clear();
        }
}

void i2c_resend_byte(void)
{
    if (failed_ind < MAX_NACK)
    {
        cur_data_index--;
        i2c_send_next_byte();
    }
    // Raise an error if maximum failed attempts achieved
    else
    {
        i2c_error(ERR_NOT_RESPOND);
        return;
    }

}

void i2c_choose_slave(void)
{
    if (failed_ind >= MAX_NACK)
        {
            i2c_error(ERR_NOT_RESPOND);
            return;
        }
    if (mode == MODE_MT)
        {
            // Load address
            TWDR = (addr<<1);
            // Set transmission mode
            TWDR &= ~(1);
            // Send data. 3 bits should be set according to DS.
            TWCR &= ~(1<<TWSTO)&~(1<<TWSTA);
            TWCR |= (1<<TWINT);
        }
    else if (mode == MODE_MR)
        {
            // Load address and set recieve mode
            TWDR = (addr<<1) | 1;
            // Send data. 3 bits should be set according to DS.
            TWCR &= ~(1<<TWSTO)&~(1<<TWSTA);
            TWCR |= (1<<TWINT) | (1<<TWEN);
        }
    else
        i2c_error(ERR_START);
}

void i2c_error(uint8_t err)
{
    i2c_status = I2C_ERROR;
    i2c_clear();
}

void i2c_clear(void)
{
    addr = ADDR_NOT_SET;
    mode = MODE_NOT_SET;
    cur_data_index = 0;
}

void i2c_init(unsigned long frequency)
{
    _set_prescaler();
    volatile uint8_t prescaler = clock_prescale_get();
    TWBR = (uint8_t)((((F_CPU * (1<<prescaler) / frequency) / I2C_SCL_FREQUENCY_PRESCALER) - 16 ) / 2); // Set bitrate
    i2c_status = I2C_READY;
    return;
}

void i2c_master_send(uint8_t address, uint8_t* data, uint8_t length)
{
    // Load data in globals
    addr = address;
    mode = MODE_MT;
    trans_data = data;
    trans_data_len = length;
    cur_data_index = 0;
    // Set busy status
    i2c_status = I2C_BUSY;
    // reset control register
	TWCR = 0;
	// transmit START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
}

void i2c_master_read(uint8_t address, uint8_t* data, uint8_t length)
{
    // Load data in globals
    addr = address;
    rec_data = data;
    mode = MODE_MR;
    rec_data_len = length;
    cur_data_index = 0;
    // Set busy status
    i2c_status = I2C_BUSY;
    // reset control register
	TWCR = 0;
	// transmit START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
}

/**
 * \brief Internal method. Sets prescaler value (I2C_SCL_FREQUENCY_PRESCALER) in TWSR register.
 * 
 * \param No parameters
 * 
 * \return void
 */
void _set_prescaler(void) 
{
    switch (I2C_SCL_FREQUENCY_PRESCALER)
    {
    case 1:
        TWSR &= ~(1 << TWPS0);
        TWSR &= ~(1 << TWPS1);
        break;
    case 4:
        TWSR |= (1 << TWPS0);
        TWSR &= ~(1 << TWPS1);
        break;
    case 16:
        TWSR &= ~(1 << TWPS0);
        TWSR |= (1 << TWPS1);
        break;
    case 64:
        TWSR |= (1 << TWPS0) | (1 << TWPS1);
        break;
    default:
        TWSR &= ~(1 << TWPS0);
        TWSR &= ~(1 << TWPS1);
        break;
    }
    return;
}