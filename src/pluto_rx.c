#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "pluto_util.h"

static volatile sig_atomic_t app_running = true;

void plutosdr_stop_async() {
  app_running = false;
}

int main(int argc, char *argv[]) {
  int opt;
  opterr = 0;
  unsigned long int frequency = 0;
  unsigned long int sample_rate = 0;
  float gain = 0;
  unsigned int buffer_size = 0;
  char *filename = NULL;
  unsigned long int number_of_samples_to_read = 0;
  while ((opt = getopt(argc, argv, "f:s:g:o:n:hb:")) != EOF) {
    switch (opt) {
      case 'f':
        frequency = strtoul(optarg, NULL, 10);
        break;
      case 's':
        sample_rate = strtoul(optarg, NULL, 10);
        break;
      case 'b':
        buffer_size = strtoul(optarg, NULL, 10);
        break;
      case 'g':
        gain = strtof(optarg, NULL);
        break;
      case 'o':
        filename = optarg;
        break;
      case 'n':
        number_of_samples_to_read = strtoul(optarg, NULL, 10);
        break;
      case 'h':
      default:
        fprintf(stderr, "%s -f frequency -s sample_rate [-g gain] [-o output_file] [-b buffer size in samples] [-h]\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (frequency == 0) {
    fprintf(stderr, "frequency not specified\n");
    return EXIT_FAILURE;
  }
  if (sample_rate == 0) {
    fprintf(stderr, "sample_rate not specified\n");
    return EXIT_FAILURE;
  }
  if (gain < 0) {
    fprintf(stderr, "invalid gain specified\n");
    return EXIT_FAILURE;
  }
  if (buffer_size == 0) {
    buffer_size = 256;
  }

  FILE *output;
  if (filename == NULL) {
    output = stdout;
  } else {
    output = fopen(filename, "wb");
    if (output == NULL) {
      fprintf(stderr, "unable to open file: %s\n", filename);
      return EXIT_FAILURE;
    }
  }

  signal(SIGINT, plutosdr_stop_async);
  signal(SIGHUP, plutosdr_stop_async);
  signal(SIGSEGV, plutosdr_stop_async);
  signal(SIGTERM, plutosdr_stop_async);

  struct iio_buffer *buffer = NULL;
  struct iio_channel *lo = NULL;
  struct iio_context *ctx = plutosdr_find_first_ctx();
  ERROR_CHECK_NOT_NULL("unable to find ctx", ctx);

  struct iio_device *dev = iio_context_find_device(ctx, "ad9361-phy");
  ERROR_CHECK_NOT_NULL("unable to find device", dev);

  // completely disabling TX when doing RX only will significantly improve sensitivity
  // details: https://wiki.analog.com/university/tools/pluto/hacking/listening_to_yourself
  struct iio_channel *tx_lo = iio_device_find_channel(dev, "TX_LO", true);
  ERROR_CHECK_NOT_NULL("unable to find tx lo channel", tx_lo);
  ERROR_CHECK_CODE("unable to power down tx", iio_channel_attr_write_longlong(tx_lo, "powerdown", 1));

  lo = iio_device_find_channel(dev, "RX_LO", true);
  ERROR_CHECK_NOT_NULL("unable to find lo channel", lo);
  ERROR_CHECK_CODE("unable to power up rx", iio_channel_attr_write_longlong(lo, "powerdown", 0));
  ERROR_CHECK_CODE("unable to set frequency", iio_channel_attr_write_longlong(lo, "frequency", (long long) frequency));

  // Find the rx device
  struct iio_device *rx = iio_context_find_device(ctx, "cf-ad9361-lpc");
  ERROR_CHECK_NOT_NULL("unable to find rx device", rx);

  struct iio_channel *phy_channel = iio_device_find_channel(dev, "voltage0", false);
  ERROR_CHECK_NOT_NULL("unable to find phy channel", phy_channel);

  // Find RX channels (I and Q)
  struct iio_channel *rx0_i = iio_device_find_channel(rx, "voltage0", false);
  ERROR_CHECK_NOT_NULL("unable to find rx I channel", rx0_i);
  struct iio_channel *rx0_q = iio_device_find_channel(rx, "voltage1", false);
  ERROR_CHECK_NOT_NULL("unable to find rx Q channel", rx0_q);

  // Enable channels
  iio_channel_enable(rx0_i);
  iio_channel_enable(rx0_q);

  ERROR_CHECK_CODE("unable to set sampling_frequency", plutosdr_set_sampling_frequency(dev, phy_channel, rx0_i, sample_rate));
  if (gain != 0.0) {
    ERROR_CHECK_CODE("unable to set gain_control_mode", iio_channel_attr_write(phy_channel, "gain_control_mode", "manual"));
    ERROR_CHECK_CODE("unable to set gain", iio_channel_attr_write_double(phy_channel, "hardwaregain", gain));
  } else {
    ERROR_CHECK_CODE("unable to set gain_control_mode", iio_channel_attr_write(phy_channel, "gain_control_mode", "slow_attack"));
  }

  // Create a RX buffer
  buffer = iio_device_create_buffer(rx, buffer_size, false);
  ERROR_CHECK_NOT_NULL("unable to create buffer", buffer);

  size_t remaining_bytes = number_of_samples_to_read * sizeof(int16_t);

  while (app_running) {
    ssize_t actual_bytes_read = iio_buffer_refill(buffer);
    if (actual_bytes_read < 0) {
      if (app_running) {
        fprintf(stderr, "unable to read bytes\n");
      }
      break;
    }
    uint8_t *p_start = (uint8_t *) iio_buffer_first(buffer, rx0_i);
    size_t written = 0;
    size_t remaining = actual_bytes_read;
    while (remaining > 0) {
      size_t actually_written = fwrite(p_start + written, sizeof(uint8_t), remaining, output);
      if (actually_written == 0) {
        fprintf(stderr, "cannot write more\n");
        break;
      }
      remaining -= actually_written;
      written += actually_written;
    }

    if (number_of_samples_to_read != 0) {
      if (written > remaining_bytes) {
        break;
      }
      remaining_bytes -= written;
    }
  }

  if (filename != NULL) {
    fclose(output);
  }
  iio_buffer_cancel(buffer);
  iio_context_destroy(ctx);
  fprintf(stderr, "stopped\n");
  return EXIT_SUCCESS;
}

