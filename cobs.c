#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cobs.h"



/** COBS encode data to buffer
	@param data Pointer to input data to encode
	@param length Number of bytes to encode
	@param buffer Pointer to encoded output buffer
	@return Encoded buffer length in bytes
	@note Does not output delimiter byte
*/
size_t cobs_encode(const void *data, size_t length, uint8_t *buffer)
{
	assert(data && buffer);

	uint8_t *encode = buffer; // Encoded byte pointer
	uint8_t *codep = encode++; // Output code pointer
	uint8_t code = 1; // Code value

	for (const uint8_t *byte = (const uint8_t *)data; length--; ++byte)
	{
		if (*byte) // Byte not zero, write it
			*encode++ = *byte, ++code;

		if (!*byte || code == 0xff) // Input is zero or block completed, restart
		{
			*codep = code, code = 1, codep = encode;
			if (!*byte || length)
				++encode;
		}
	}
	*codep = code; // Write final code value
    // add delimiter byte
    *encode = 0;
    return encode - buffer + 1; // Return encoded length
}

/** COBS decode data from buffer
	@param buffer Pointer to encoded input bytes
	@param length Number of bytes to decode
	@param data Pointer to decoded output data
	@return Number of bytes successfully decoded
	@note Stops decoding if delimiter byte is found
*/
size_t cobsDecode_raw(const uint8_t *buffer, size_t length, void *data)
{
    // log the input arguments with printf
    //printf("cobsDecode_raw: buffer=%p, length=%d, data=%p\n", buffer, length, data);
	assert(buffer && data);

	const uint8_t *byte = buffer; // Encoded input byte pointer
	uint8_t *decode = (uint8_t *)data; // Decoded output byte pointer

	for (uint8_t code = 0xff, block = 0; byte < buffer + length; --block)
	{
		if (block) // Decode block byte
			*decode++ = *byte++;
		else
		{
			if (code != 0xff) // Encoded zero, write it
				*decode++ = 0;
			block = code = *byte++; // Next block length
			if (!code) // Delimiter code found
				break;
		}
	}

	return (size_t)(decode - (uint8_t *)data);
}




// Appends data to the internal buffer and attempts to deserialize the accumulated data into a frame
// @param acc Pointer to the accumulator structure
// @param data Pointer to the data to append
// @param data_size Number of bytes to append
// @param[out] reminder Pointer to the reminder of the data that was not consumed
// @param[out] reminder_size Number of bytes in the reminder
// @param[out] frame Pointer to the frame that was deserialized, if any
// @param[out] frame_size Number of bytes in the frame
// @return COBS_ACCUMULATOR_STATUS_SUCCESS if the data was consumed, COBS_ACCUMULATOR_STATUS_ERROR_OVERFULL if the buffer was full, COBS_ACCUMULATOR_STATUS_ERROR_DESER if the deserializer failed, and COBS_ACCUMULATOR_STATUS_SUCCESS if a frame was consumed in the data given.
// @note The frame is not zero-terminated
// @note Once the data is consumed, the accumulator is reset, and the remainder is returned for the next call

enum cobs_accumulator_status_t cobs_accumulator_append(struct cobs_accumulator_t *acc, const uint8_t *data, size_t data_size, uint8_t *reminder, size_t *reminder_size, uint8_t *frame, size_t *frame_size)
{
    // log the input arguments with printf
    //printf("cobs_accumulator_append: acc=%p, data=%p, data_size=%d, reminder=%p, reminder_size=%p, frame=%p, frame_size=%p\n", acc, data, data_size, reminder, reminder_size, frame, frame_size);
    assert(acc && data && reminder && reminder_size && frame && frame_size);

    // if the data received is empty, return CONSUMED
    if (data_size == 0)
    {
        *reminder_size = 0;
        *frame_size = 0;
        return COBS_ACCUMULATOR_STATUS_CONSUMED;
    }

    // iterate over the data input to find out if there is a zero byte, null otherwise
    const uint8_t *data_end = data + data_size;
    const uint8_t *data_zero = NULL;
    for (const uint8_t *data_iter = data; data_iter < data_end; ++data_iter)
    {
        if (*data_iter == 0)
        {
            data_zero = data_iter;
            break;
        }
    }

    // if there is a zero byte
    if (data_zero)
    {
        // can we fit the data before the zero byte in the buffer of the accumulator?
        if (acc->idx + (data_zero - data) < acc->buffer_size)
        {
            // copy the data before the zero byte to the buffer
            memcpy(acc->buffer + acc->idx, data, data_zero - data);
            acc->idx += data_zero - data;
            // return the reminder of the data after the zero byte
            // copy the reminder to the reminder buffer
            memcpy(reminder, data_zero + 1, data_end - data_zero - 1);
            // set the reiminder size
            *reminder_size = data_end - data_zero - 1;
            // now decode the accumulator buffer using cobsDecode_raw and put it in the frame output
            *frame_size = cobsDecode_raw(acc->buffer, acc->idx, frame);
            // reset the accumulator
            acc->idx = 0;
            // if the deserialization failed, return ERROR_DESER
            if (*frame_size == 0)
                return COBS_ACCUMULATOR_STATUS_ERROR_DESER;
            // return SUCCESS
            return COBS_ACCUMULATOR_STATUS_SUCCESS;
        }
        else {  // it cannot fit, so we reset the accumulator and return ERROR_OVERFULL, and return the post-zero byte data
            acc->idx = 0;
            memcpy(reminder, data_zero + 1, data_end - data_zero - 1);
            *reminder_size = data_end - data_zero - 1;
            return COBS_ACCUMULATOR_STATUS_ERROR_OVERFULL;
        }
    }
    else {  // there is no zero byte, so we can just append the data to the buffer
        // does it fit in the accumulator buffer?
        if (acc->idx + data_size < acc->buffer_size)
        {
            // append the data to the buffer
            memcpy(acc->buffer + acc->idx, data, data_size);
            acc->idx += data_size;
            // set correct reminder & frame
            *reminder_size = 0;
            *frame_size = 0;
            // return CONSUMED
            return COBS_ACCUMULATOR_STATUS_CONSUMED;
        }
        else {  // it cannot fit, so we reset the accumulator and return ERROR_OVERFULL
            acc->idx = 0;
            // copy the reminder to the reminder buffer
            memcpy(reminder, data, data_size);
            *reminder_size = data_size;
            // set correct frame
            *frame_size = 0;
            return COBS_ACCUMULATOR_STATUS_ERROR_OVERFULL;
        }
    }
}

void cobs_accumulator_init(struct cobs_accumulator_t *acc, uint8_t *buffer, size_t buffer_size)
{
    assert(acc && buffer);

    acc->buffer = buffer;
    acc->buffer_size = buffer_size;
    acc->idx = 0;
}
void cobs_accumulator_reset(struct cobs_accumulator_t *acc)
{
    assert(acc);
    acc->idx = 0;
}


enum cobs_decode_status_t cobs_decode(const uint8_t *source, size_t source_size, uint8_t *dest, size_t *dest_size)
{
    // create accumulator and use it to decode the source
    struct cobs_accumulator_t acc;
    uint8_t buffer[512];
    uint8_t reminder[256];
    size_t reminder_size = 0;
    uint8_t frame[256];
    size_t frame_size = 0;

    cobs_accumulator_init(&acc, buffer, 512);
    enum cobs_accumulator_status_t res = cobs_accumulator_append(&acc, source, source_size, reminder, &reminder_size, frame, &frame_size);
    if (res != COBS_ACCUMULATOR_STATUS_SUCCESS)
    {
        return COBS_DECODE_STATUS_ERROR_INVALID_DATA;
    }
    // copy data from the frame to the destination
    memcpy(dest, frame, frame_size);
    *dest_size = frame_size;
    return COBS_DECODE_STATUS_SUCCESS;

}