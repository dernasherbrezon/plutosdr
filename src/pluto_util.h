#ifndef pluto_util_h_
#define pluto_util_h_

#include <iio.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
  RX, TX
} iio_direction;

typedef enum {
  IIO_GAIN_MODE_MANUAL = 0,
  IIO_GAIN_MODE_FAST_ATTACK = 1,
  IIO_GAIN_MODE_SLOW_ATTACK = 2,
  IIO_GAIN_MODE_HYBRID = 3
} gain_mode;

typedef struct {
  uint64_t sampling_freq; // Baseband sample rate in Hz
  uint64_t center_freq; // Local oscillator frequency in Hz
  gain_mode gain_control_mode;
  int64_t offset;
  double manual_gain;
} channel_config;

int plutosdr_rx(unsigned long int frequency, unsigned long int sample_rate, float gain, unsigned int buffer_size);

int plutosdr_tx(unsigned long int frequency, unsigned long int sample_rate, unsigned int buffer_size, FILE *fp);

int plutosdr_find_first_ctx(struct iio_context **result);

int plutosdr_configure_channel(struct iio_context *ctx, channel_config *cfg, iio_direction type);

void plutosdr_stop_async();

#endif /* pluto_util_h_ */
