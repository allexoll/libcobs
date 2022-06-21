// header guard 
#ifndef COBS_H
#define COBS_H

#include <stdint.h>
#include <stdlib.h>

/** COBS accumulator structure 
 * @param buffer Pointer to buffer to accumulate data into
 * @param buffer_size Size of the buffer
 * @param idx Index of the next byte to write to the buffer
 * @note The buffer is not zero-terminated
 * @note The buffer is not zero-filled
 * @param acc_state Current state of the accumulator
*/
struct cobs_accumulator_t {
    uint8_t *buffer;
    size_t buffer_size;
    size_t idx;
};

enum cobs_accumulator_status_t {
    COBS_ACCUMULATOR_STATUS_CONSUMED,   // data was consumed but there was no frame to return
    COBS_ACCUMULATOR_STATUS_SUCCESS,    // data was consumed and a frame was returned
    COBS_ACCUMULATOR_STATUS_ERROR_OVERFULL, // data was consumed but the buffer was full, we are returning the reminder of the data
    COBS_ACCUMULATOR_STATUS_ERROR_DESER,    // data was consumed but the deserializer failed. returns the reminder of the data if any
};


enum cobs_decoder_state_t {
    COBS_DECODER_STATE_IDLE,
    COBS_DECODER_STATE_GRAB,
    COBS_DECODER_STATE_GRABCHAIN,
};


struct cobs_decoder_t{
    uint8_t *buffer;
    size_t dest_idx;
    enum cobs_decoder_state_t state;
};

enum cobs_decode_status_t{
    COBS_DECODE_STATUS_SUCCESS,
    COBS_DECODE_STATUS_ERROR_INVALID_DATA,
    COBS_DECODE_STATUS_ERROR_INVALID_LENGTH,
};


enum cobs_decode_status_t cobs_decode(const uint8_t *source, size_t source_size, uint8_t *dest, size_t *dest_size);


void cobs_accumulator_reset(struct cobs_accumulator_t *acc);

void cobs_accumulator_init(struct cobs_accumulator_t *acc, uint8_t *buffer, size_t buffer_size);


enum cobs_accumulator_status_t cobs_accumulator_append(struct cobs_accumulator_t *acc, const uint8_t *data, size_t data_size, uint8_t *reminder, size_t *reminder_size, uint8_t *frame, size_t *frame_size);

// COBS encode function
// input args: buffer to encode, size of the data in the buffer to encode, output args: encoded buffer, size of the encoded buffer
// return: encoded size
size_t cobs_encode(const void *data, size_t length, uint8_t *buffer);


void cobs_accumulator_append_test();

#endif // COBS_H