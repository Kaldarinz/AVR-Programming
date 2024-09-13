#include <avr/io.h>

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

uint8_t addr; // Address of current slave
uint8_t mode; // Current i2c mode
uint8_t cur_data_index;
uint8_t trans_data_len;
uint8_t rec_data_len;
uint8_t* trans_data;
uint8_t* rec_data;
uint8_t failed_ind;

/**
 * \brief Reads data from TWDR register into rec_data and advances cur_data_index
 */
void _i2c_read_byte(void);

/**
 * \brief Requests next byte from slave by setting TWCR register.
 * Decides to send ACK or NACK based on cur_data_index and rec_data_len.
 */
void _i2c_request_next_byte(void);

/**
 * \brief Checks if there are some data to send and send it by loading trans_data into
 * TWDR register and setting TWCR register, otherwise stops communication.
 */
void i2c_send_next_byte(void);