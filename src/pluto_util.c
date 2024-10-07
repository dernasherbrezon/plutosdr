#include "pluto_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#define FIR_BUF_SIZE  8192

#define ERROR_CHECK(x)           \
  do {                           \
    int __err_rc = (x);          \
    if (__err_rc < 0) {             \
        iio_context_destroy(ctx); \
        return __err_rc;                              \
    }                            \
  } while (0)

#define ERROR_CHECK_CODE(x)           \
  do {                           \
    int __err_rc = (x);          \
    if (__err_rc < 0) {             \
        return __err_rc;                              \
    }                            \
  } while (0)

// taken from iio-oscilloscope AD936x_LP_180kHz_521kSPS.ftr
static int16_t fir[] = {-170, 531, 134, -2952, -132, 4439, 194, -5060, 51, 4235, 277, -3352, 176, 2941, 260, -1977, 106, 1979, 61, -1356, -119, 1119, -174, -1215, -253, 493, -181, -1034, -93, 379, 95, -518, 233, 687, 350, 50, 328, 850, 220, 104, -6, 418, -238, -441, -440, -305, -498, -901, -407,
                        -485, -148, -541, 183, 217, 506, 425, 684, 1027, 658, 874, 391, 822, -38, 84, -516, -418, -867, -1163, -969, -1290, -741, -1259, -232, -523, 429, 245, 1020, 1253, 1337, 1731, 1226, 1868, 675, 1172, -190, 157, -1105, -1217, -1764, -2193, -1896, -2673, -1383, -2151, -300, -911,
                        1061,
                        940, 2274, 2649, 2887, 3786, 2582, 3676, 1284, 2315, -755, -223, -2990, -3139, -4687, -5637, -5111, -6622, -3724, -5450, -366, -1748, 4671, 4105, 10652, 11284, 16552, 18450, 21271, 24257, 23891, 27484, 23891, 27484, 21271, 24257, 16552, 18450, 10652, 11284, 4671, 4105, -366,
                        -1748, -3724, -5450, -5111, -6622, -4687, -5637, -2990, -3139, -755, -223, 1284, 2315, 2582, 3676, 2887, 3786, 2274, 2649, 1061, 940, -300, -911, -1383, -2151, -1896, -2673, -1764, -2193, -1105, -1217, -190, 157, 675, 1172, 1226, 1868, 1337, 1731, 1020, 1253, 429, 245, -232,
                        -523, -741, -1259,
                        -969, -1290, -867, -1163, -516, -418, -38, 84, 391, 822, 658, 874, 684, 1027, 506, 425, 183, 217, -148, -541, -407, -485, -498, -901, -440, -305, -238, -441, -6, 418, 220, 104, 328, 850, 350, 50, 233, 687, 95, -518, -93, 379, -181, -1034, -253, 493, -174, -1215, -119, 1119,
                        61, -1356, 106, 1979, 260, -1977, 176, 2941, 277, -3352, 51, 4235, 194, -5060, -132, 4439, 134, -2952, -170, 531};

static struct iio_buffer *buffer;
static volatile sig_atomic_t app_running = true;

void plutosdr_stop_async() {
  app_running = false;
}

int plutosdr_ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return 0;
  }
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix > lenstr) {
    return 0;
  }
  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void plutosdr_print_message(const char *message, ssize_t ret) {
  char err_str[1024];
  iio_strerror(-(int) ret, err_str, sizeof(err_str));
  fprintf(stderr, message, err_str);
}

struct iio_device *plutosdr_find_device_bysuffix(struct iio_context *ctx, const char *suffix) {
  unsigned int nb_devices = iio_context_get_devices_count(ctx);
  struct iio_device *dev = NULL;
  for (int i = 0; i < nb_devices; i++) {
    struct iio_device *curDev = iio_context_get_device(ctx, i);
    const char *name = iio_device_get_name(curDev);
    if (name == NULL) {
      continue;
    }
    if (plutosdr_ends_with(name, suffix)) {
      fprintf(stderr, "found device: %s\n", name);
      dev = curDev;
      break;
    }
  }
  return dev;
}

static struct iio_device *plutosdr_find_device(struct iio_context *ctx, iio_direction d) {
  switch (d) {
    case TX:
      return iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
    case RX:
      return iio_context_find_device(ctx, "cf-ad9361-lpc");
    default:
      return NULL;
  }
}

int plutosdr_get_rx_fir_enable(struct iio_device *dev, int *enable) {
  bool value;

  int ret = iio_device_attr_read_bool(dev, "in_out_voltage_filter_fir_en", &value);

  if (ret < 0) {
    ret = iio_channel_attr_read_bool(iio_device_find_channel(dev, "in", false), "voltage_filter_fir_en", &value);
  }

  if (!ret) {
    *enable = value;
  }

  return ret;
}

int plutosdr_set_txrx_fir_enable(struct iio_device *dev, int enable) {
  int ret = iio_device_attr_write_bool(dev, "in_out_voltage_filter_fir_en", enable != 0);
  if (ret < 0) {
    ret = iio_channel_attr_write_bool(iio_device_find_channel(dev, "out", false), "voltage_filter_fir_en", enable != 0);
  }
  return ret;
}

int plutosdr_find_first_ctx(struct iio_context **result) {
  struct iio_scan_context *scan_ctx = iio_create_scan_context(NULL, 0);
  if (scan_ctx == NULL) {
    perror("unable to create scan context");
    return -1;
  }
  struct iio_context_info **info;
  ssize_t ret;
  ret = iio_scan_context_get_info_list(scan_ctx, &info);
  if (ret < 0) {
    plutosdr_print_message("scanning for iio contexts failed: %s\n", ret);
    iio_scan_context_destroy(scan_ctx);
    return -1;
  }

  if (ret == 0) {
    fprintf(stderr, "no iio context found\n");
    iio_context_info_list_free(info);
    iio_scan_context_destroy(scan_ctx);
    return -1;
  }

  const char *uri = iio_context_info_get_uri(info[0]);
  fprintf(stderr, "using context uri: %s\n", uri);
  struct iio_context *ctx = iio_create_context_from_uri(uri);

  iio_context_info_list_free(info);
  iio_scan_context_destroy(scan_ctx);

  if (ctx == NULL) {
    perror("unable to create context");
    return -1;
  }
  ret = iio_context_set_timeout(ctx, 60000);
  if (ret != 0) {
    iio_context_destroy(ctx);
    return -1;
  }
  *result = ctx;
  return 0;
}

static struct iio_channel *plutosdr_find_lo_channel(struct iio_device *dev, iio_direction d) {
  switch (d) {
    // LO chan is always output, i.e. true
    case RX:
      return iio_device_find_channel(dev, "RX_LO", true);
    case TX:
      return iio_device_find_channel(dev, "TX_LO", true);
    default:
      return NULL;
  }
}

static struct iio_channel *plutosdr_find_phy_channel(struct iio_device *dev, iio_direction d) {
  switch (d) {
    case RX:
      return iio_device_find_channel(dev, "voltage0", false);
    case TX:
      return iio_device_find_channel(dev, "voltage0", true);
    default:
      return NULL;
  }
}

int plutosdr_configure_channel(struct iio_context *ctx, channel_config *cfg, iio_direction d) {
  struct iio_device *dev = plutosdr_find_device_bysuffix(ctx, "-phy");
  if (dev == NULL) {
    fprintf(stderr, "unable to find -phy device\n");
    return -1;
  }
  struct iio_channel *altvoltage = plutosdr_find_lo_channel(dev, d);
  if (altvoltage == NULL) {
    fprintf(stderr, "unable to find lo channel\n");
    return -1;
  }
  if (d == RX) {
    // completely disabling TX when doing RX only will significantly improve sensitivity
    // details: https://wiki.analog.com/university/tools/pluto/hacking/listening_to_yourself
    struct iio_channel *altvoltage1 = plutosdr_find_lo_channel(dev, TX);
    if (altvoltage1 != NULL) {
      ERROR_CHECK_CODE(iio_channel_attr_write_longlong(altvoltage1, "powerdown", 1));
    }
  }
  if (d == TX) {
    ERROR_CHECK_CODE(iio_channel_attr_write_longlong(altvoltage, "powerdown", 0));
  }
  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(altvoltage, "frequency", (long long) cfg->center_freq));
  fprintf(stderr, "frequency: %" PRIu64 "\n", cfg->center_freq);
  struct iio_channel *voltage = plutosdr_find_phy_channel(dev, d);
  if (voltage == NULL) {
    fprintf(stderr, "unable to find phy channel\n");
    return -1;
  }

  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(voltage, "sampling_frequency", cfg->sampling_freq));
  ERROR_CHECK_CODE(iio_channel_attr_write_longlong(voltage, "rf_bandwidth", cfg->sampling_freq));
  fprintf(stderr, "sampling frequency: %" PRIu64 "\n", cfg->sampling_freq);
  if (d == RX) {
    switch (cfg->gain_control_mode) {
      case IIO_GAIN_MODE_MANUAL:
        ERROR_CHECK_CODE(iio_channel_attr_write(voltage, "gain_control_mode", "manual"));
        ERROR_CHECK_CODE(iio_channel_attr_write_double(voltage, "hardwaregain", cfg->manual_gain));
        fprintf(stderr, "gain manual: %.2f\n", cfg->manual_gain);
        break;
      case IIO_GAIN_MODE_FAST_ATTACK:
        ERROR_CHECK_CODE(iio_channel_attr_write(voltage, "gain_control_mode", "fast_attack"));
        break;
      case IIO_GAIN_MODE_SLOW_ATTACK:
        ERROR_CHECK_CODE(iio_channel_attr_write(voltage, "gain_control_mode", "slow_attack"));
        break;
      case IIO_GAIN_MODE_HYBRID:
        ERROR_CHECK_CODE(iio_channel_attr_write(voltage, "gain_control_mode", "hybrid"));
        break;
      default:
        fprintf(stderr, "unknown gain mode: %d\n", cfg->gain_control_mode);
        return -1;
    }
  }
  return 0;
}

int plutosdr_setup_fir_filter(struct iio_context *ctx) {
  struct iio_device *dev = plutosdr_find_device_bysuffix(ctx, "-phy");
  if (dev == NULL) {
    fprintf(stderr, "unable to find -phy device\n");
    return -1;
  }

  char *buf;
  int len = 0;

  buf = malloc(FIR_BUF_SIZE);
  if (!buf) {
    return -ENOMEM;
  }
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "RX 3 GAIN -6 DEC 4\n");
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "TX 3 GAIN 0 INT 4\n");
  for (int i = 0, j = 0; i < 128; i++, j += 2) {
    len += snprintf(buf + len, FIR_BUF_SIZE - len, "%d,%d\n", fir[j], fir[j + 1]);
  }
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "\n");
  ssize_t ret = iio_device_attr_write_raw(dev, "filter_fir_config", buf, len);
  free(buf);

  if (ret < 0) {
    fprintf(stderr, "unable to setup fir filter config");
    return -1;
  }

  ERROR_CHECK_CODE(plutosdr_set_txrx_fir_enable(dev, true));
  fprintf(stderr, "fir filter enabled\n");
  return 0;
}

int plutosdr_rx(unsigned long int frequency, unsigned long int sample_rate, float gain, unsigned int buffer_size, unsigned long int number_of_samples_to_read, FILE *output) {
  ssize_t ret;
  struct iio_context *ctx;
  ERROR_CHECK_CODE(plutosdr_find_first_ctx(&ctx));

  channel_config cfg = {
      .manual_gain = gain,
      .sampling_freq = sample_rate,
      .center_freq = frequency,
  };
  if (gain == 0.0) {
    cfg.gain_control_mode = IIO_GAIN_MODE_MANUAL;
  } else {
    cfg.gain_control_mode = IIO_GAIN_MODE_SLOW_ATTACK;
  }

  ERROR_CHECK(plutosdr_configure_channel(ctx, &cfg, RX));
  ERROR_CHECK(plutosdr_setup_fir_filter(ctx));

  struct iio_device *rx_device = plutosdr_find_device(ctx, RX);
  if (rx_device == NULL) {
    fprintf(stderr, "unable to find rx device\n");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }
  unsigned int nb_channels = iio_device_get_channels_count(rx_device);
  for (int i = 0; i < nb_channels; i++) {
    struct iio_channel *ch = iio_device_get_channel(rx_device, i);
    if (ch == NULL) {
      continue;
    }
    if (!iio_channel_is_output(ch)) {
      iio_channel_enable(ch);
      ERROR_CHECK(iio_channel_attr_write_longlong(ch, "sampling_frequency", (long long) sample_rate));
    }
  }

  fprintf(stderr, "buffer size: %d samples\n", buffer_size);
  buffer = iio_device_create_buffer(rx_device, buffer_size, false);
  if (!buffer) {
    plutosdr_print_message("unable to allocate buffer: %s\n", errno);
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }

  struct iio_channel *rx_i = iio_device_find_channel(rx_device, "voltage0", false);
  if (rx_i == NULL) {
    fprintf(stderr, "unable to find channel\n");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }

  size_t remaining_bytes = number_of_samples_to_read * sizeof(int16_t);

  while (app_running) {
    ret = iio_buffer_refill(buffer);
    if (ret < 0) {
      if (app_running) {
        plutosdr_print_message("unable to refill buffer: %s\n", ret);
      }
      break;
    }

    uint8_t *p_end = (uint8_t *) iio_buffer_end(buffer);
    uint8_t *p_start = (uint8_t *) iio_buffer_first(buffer, rx_i);

    size_t written = 0;
    size_t remaining = p_end - p_start;
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

  iio_buffer_cancel(buffer);
  iio_context_destroy(ctx);
  fprintf(stderr, "stopped\n");
  return EXIT_SUCCESS;
}

int plutosdr_disable_dds_channel(struct iio_device *tx_device, const char *channel_name) {
  struct iio_channel *channel = iio_device_find_channel(tx_device, channel_name, true);
  if (channel == NULL) {
    return -1;
  }
  ERROR_CHECK_CODE(iio_channel_attr_write_bool(channel, "raw", 0));
  return 0;
}

int plutosdr_disable_dds(struct iio_device *tx_device) {
  // normally disabling just TX1_I_F1 would disable all other DDS channels,
  // but I disable all just in case
  ERROR_CHECK_CODE(plutosdr_disable_dds_channel(tx_device, "TX1_I_F1"));
  ERROR_CHECK_CODE(plutosdr_disable_dds_channel(tx_device, "TX1_Q_F1"));
  ERROR_CHECK_CODE(plutosdr_disable_dds_channel(tx_device, "TX1_I_F2"));
  ERROR_CHECK_CODE(plutosdr_disable_dds_channel(tx_device, "TX1_Q_F2"));
  return 0;
}

int plutosdr_tx(unsigned long int frequency, unsigned long int sample_rate, unsigned int buffer_size, FILE *fp) {
  struct iio_context *ctx;
  ERROR_CHECK_CODE(plutosdr_find_first_ctx(&ctx));

  channel_config cfg = {
      .sampling_freq = sample_rate,
      .center_freq = frequency,
  };

  ERROR_CHECK(plutosdr_configure_channel(ctx, &cfg, TX));

  struct iio_device *tx_device = plutosdr_find_device(ctx, TX);
  if (tx_device == NULL) {
    fprintf(stderr, "unable to find tx device\n");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }
  ERROR_CHECK(plutosdr_disable_dds(tx_device));

  struct iio_channel *tx_i = iio_device_find_channel(tx_device, "voltage0", true);
  if (tx_i == NULL) {
    fprintf(stderr, "unable to find tx I channel\n");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }
  struct iio_channel *tx_q = iio_device_find_channel(tx_device, "voltage1", true);
  if (tx_q == NULL) {
    fprintf(stderr, "unable to find tx Q channel\n");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }
  iio_channel_enable(tx_i);
  iio_channel_enable(tx_q);

  fprintf(stderr, "buffer size: %d samples\n", buffer_size);
  buffer = iio_device_create_buffer(tx_device, buffer_size, false);
  if (!buffer) {
    plutosdr_print_message("unable to allocate buffer: %s\n", errno);
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }

  size_t sample_size = 2 * sizeof(int16_t); // i + q
  size_t input_buffer_size = buffer_size * sample_size;
  uint8_t *input_buffer = malloc(input_buffer_size);
  if (input_buffer == NULL) {
    fprintf(stderr, "unable to allocate buffer");
    iio_context_destroy(ctx);
    return EXIT_FAILURE;
  }
  while (app_running) {
    size_t actually_read = fread(input_buffer, sizeof(uint8_t), input_buffer_size, fp);
    if (actually_read == 0) {
      break;
    }

    void *p_start = iio_buffer_first(buffer, tx_i);

    // put [-1;1] into 16bit interval [-32768;32768]
    // pluto has 16bit DAC for TX
    //FIXME detect file input format
//    volk_32f_s32f_convert_16i((int16_t *) p_start, (const float *) (input + processed), 32768, batch * 2);

    // always use push_partial because normal push won't reset buffer to full length
    // https://github.com/analogdevicesinc/libiio/blob/e65a97863c3481f30c6ea8642bda86125a7ee39d/buffer.c#L154
    ssize_t nbytes_tx = iio_buffer_push_partial(buffer, actually_read / sample_size);
    if (nbytes_tx < 0) {
      fprintf(stderr, "short write\n");
      break;
    }
  }

  iio_buffer_cancel(buffer);
  iio_context_destroy(ctx);
  fprintf(stderr, "stopped\n");
  return EXIT_SUCCESS;
}
