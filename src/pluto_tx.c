#include <iio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pluto_util.h"

static volatile sig_atomic_t app_running = true;

void plutosdr_stop_async() {
  app_running = false;
}

iq_format detect_format(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename) return FORMAT_UNKNOWN;
  if (strcmp(dot + 1, "cu8") == 0) {
    return FORMAT_CU8;
  }
  return FORMAT_UNKNOWN;
}

int main(int argc, char *argv[]) {
  int opt;
  opterr = 0;
  unsigned long int frequency = 0;
  unsigned long int sampleRate = 0;
  unsigned int buffer_size = 0;
  char *filename = NULL;
  while ((opt = getopt(argc, argv, "f:s:i:hb:")) != EOF) {
    switch (opt) {
      case 'f':
        frequency = strtoul(optarg, NULL, 10);
        break;
      case 's':
        sampleRate = strtoul(optarg, NULL, 10);
        break;
      case 'b':
        buffer_size = strtoul(optarg, NULL, 10);
        break;
      case 'i':
        filename = optarg;
        break;
      case 'h':
      default:
        fprintf(stderr, "%s -f frequency -s sampleRate [-i inputFile] [-b buffer size in samples] [-h]\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (frequency == 0) {
    fprintf(stderr, "frequency not specified\n");
    return EXIT_FAILURE;
  }
  if (sampleRate == 0) {
    fprintf(stderr, "sampleRate not specified\n");
    return EXIT_FAILURE;
  }
  if (buffer_size == 0) {
    buffer_size = 256;
  }


  FILE *input;
  iq_format format = FORMAT_UNKNOWN;
  if (filename == NULL) {
    input = stdin;
  } else {
    format = detect_format(filename);
    input = fopen(filename, "rb");
    if (input == NULL) {
      fprintf(stderr, "unable to open file: %s\n", filename);
      return EXIT_FAILURE;
    }
  }
  if (format == FORMAT_UNKNOWN) {
    fprintf(stderr, "unable to detect format from the filename");
    return EXIT_FAILURE;
  }

  signal(SIGINT, plutosdr_stop_async);
  signal(SIGHUP, plutosdr_stop_async);
  signal(SIGSEGV, plutosdr_stop_async);
  signal(SIGTERM, plutosdr_stop_async);

  struct iio_buffer *buffer = NULL;
  struct iio_context *ctx = plutosdr_find_first_ctx();
  ERROR_CHECK_NOT_NULL("unable to find ctx", ctx);

  struct iio_device *dev = iio_context_find_device(ctx, "ad9361-phy");
  ERROR_CHECK_NOT_NULL("unable to find device", dev);
  struct iio_channel *lo = iio_device_find_channel(dev, "TX_LO", true);
  ERROR_CHECK_NOT_NULL("unable to find lo channel", lo);
  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(lo, "powerdown", 0));
  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(lo, "frequency", (long long) frequency));

  // Find the transmit device
  struct iio_device *tx = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
  ERROR_CHECK_NOT_NULL("unable to find tx device", tx);
  ERROR_CHECK_CODE(plutosdr_disable_dds(tx));

  // Find TX channels (I and Q)
  struct iio_channel *tx0_i = iio_device_find_channel(tx, "voltage0", true);
  ERROR_CHECK_NOT_NULL("unable to find tx I channel", tx0_i);
  struct iio_channel *tx0_q = iio_device_find_channel(tx, "voltage1", true);
  ERROR_CHECK_NOT_NULL("unable to find tx Q channel", tx0_q);

  // Enable channels
  iio_channel_enable(tx0_i);
  iio_channel_enable(tx0_q);

  struct iio_channel *phy_channel = iio_device_find_channel(dev, "voltage0", true);
  ERROR_CHECK_NOT_NULL("unable to find phy channel", phy_channel);
  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", sampleRate));

  // Create a TX buffer
  buffer = iio_device_create_buffer(tx, buffer_size, false);
  ERROR_CHECK_NOT_NULL("unable to create buffer", buffer);

  size_t input_buffer_size;
  if (format == FORMAT_CU8) {
    input_buffer_size = buffer_size * sizeof(uint8_t) * 2;
  }
  uint8_t *input_buffer = malloc(input_buffer_size);
  ERROR_CHECK_NOT_NULL("unable to init temporary buffefr", input_buffer);

  while (app_running) {
    size_t actually_read = fread(input_buffer, sizeof(uint8_t), input_buffer_size, input);
    if (actually_read == 0) {
      break;
    }

    void *p_start = iio_buffer_first(buffer, tx0_i);

    size_t samples_count;
    if (format == FORMAT_CU8) {
      samples_count = actually_read / (sizeof(uint8_t) * 2);
      for (size_t i = 0; i < actually_read; i++) {
        ((int16_t *) p_start)[i] = (int16_t) (((input_buffer[i] - 128.0) / 128.0) * 32768);
      }
    }

    // always use push_partial because normal push won't reset buffer to full length
    // https://github.com/analogdevicesinc/libiio/blob/e65a97863c3481f30c6ea8642bda86125a7ee39d/buffer.c#L154
    ssize_t nbytes_tx = iio_buffer_push_partial(buffer, samples_count);
    if (nbytes_tx < 0) {
      fprintf(stderr, "short write\n");
      break;
    }
  }
  if (filename != NULL) {
    fclose(input);
  }
  iio_channel_attr_write_longlong(lo, "powerdown", 1);
  iio_buffer_cancel(buffer);
  iio_context_destroy(ctx);
  fprintf(stderr, "stopped\n");
  return EXIT_SUCCESS;
}
