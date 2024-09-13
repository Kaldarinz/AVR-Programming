#ifndef __AVR_ATmega328P__
#define __AVR_ATmega328P__
#endif

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include "i2c_isr.h"
#include "i2c_isr_p.h"

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
            _i2c_request_next_byte();
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
            _i2c_read_byte();
            //Request next byte
            _i2c_request_next_byte();
            break;
        // Data recieved, not acknowledge sent to slave
        case STATUS_MR_DATA_NACK:
            _i2c_read_byte();
            _i2c_stop();
            break;
        default:
            break;
        }
    }
    return cur_stat;
}

void _i2c_read_byte(void)
{
    rec_data[cur_data_index] = TWDR;
    // Increase data index
    cur_data_index++;
}

void _i2c_request_next_byte(void)
{
    // Check if we are requesting NOT the last byte
    if ( (cur_data_index+1)<rec_data_len )
    {
        // Request next byte and send ACK in return
        TWCR &= ~(1<<TWSTA)&~(1<<TWSTO);
        TWCR |= (1<<TWINT)|(1<<TWEA);
    }
    // If we are requesting the last byte
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
        _i2c_stop();
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

/**
 * \brief Starts i2c communication by setting TWCR register and updates i2c_status.
 */
void _i2c_start(void)
{
    // Set busy status
    i2c_status = I2C_BUSY;
    // reset control register
	TWCR = 0;
	// transmit START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
}
/**
 * \brief Stops i2c communication by setting TWCR register, updates i2c_status and clears global vars.
 */
void _i2c_stop(void)
{
    // Send stop command
    TWCR &= ~(1<<TWSTA);
    TWCR |= (1<<TWSTO) | (1<<TWINT);
    i2c_status = I2C_DONE;
    i2c_clear();
}

void i2c_master_send(uint8_t address, uint8_t* data, uint8_t length)
{
    // Load data in globals
    addr = address;
    mode = MODE_MT;
    trans_data = data;
    trans_data_len = length;
    cur_data_index = 0;
    _i2c_start();
}

void i2c_master_read(uint8_t address, uint8_t* data, uint8_t length)
{
    // Load data in globals
    addr = address;
    rec_data = data;
    mode = MODE_MR;
    rec_data_len = length;
    cur_data_index = 0;
    _i2c_start();
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