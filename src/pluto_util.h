#ifndef pluto_util_h_
#define pluto_util_h_

#include <iio.h>
#include <stdint.h>
#include <stdio.h>

#define ERROR_CHECK_CODE(y, x)           \
  do {                           \
    int __err_rc = (x);          \
    if (__err_rc < 0) {             \
        fprintf(stderr, "%s\n", y);                                 \
        return __err_rc;                              \
    }                            \
  } while (0)

#define ERROR_CHECK_NOT_NULL(y, x)           \
  do {                           \
    if (x == NULL) {                      \
        fprintf(stderr, "%s\n", y);                                     \
        if( buffer != NULL ) {               \
          iio_buffer_destroy(buffer);                                     \
        } \
        if( lo != NULL ) {                   \
          iio_channel_attr_write_longlong(lo, "powerdown", 1);                                     \
        }                                     \
        iio_context_destroy(ctx); \
        return EXIT_FAILURE;                                  \
    }                            \
  } while (0)

typedef enum {
  RX, TX
} iio_direction;

typedef enum {
  IIO_GAIN_MODE_MANUAL = 0,
  IIO_GAIN_MODE_FAST_ATTACK = 1,
  IIO_GAIN_MODE_SLOW_ATTACK = 2,
  IIO_GAIN_MODE_HYBRID = 3
} gain_mode;


typedef enum {
  FORMAT_UNKNOWN = 0,
  FORMAT_CU8 = 1,
  FORMAT_CS16 = 2
} iq_format;

typedef struct {
  uint64_t sampling_freq; // Baseband sample rate in Hz
  uint64_t center_freq; // Local oscillator frequency in Hz
  gain_mode gain_control_mode;
  int64_t offset;
  double manual_gain;
} channel_config;

struct iio_context * plutosdr_find_first_ctx();

int plutosdr_disable_dds(struct iio_device *tx_device);

int plutosdr_set_sampling_frequency(struct iio_device *dev, struct iio_channel *phy_channel, struct iio_channel *lpc_channel, unsigned long int sampling_frequency);

#endif /* pluto_util_h_ */
