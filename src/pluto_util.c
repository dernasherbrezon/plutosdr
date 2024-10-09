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

struct iio_context *plutosdr_find_first_ctx() {
  struct iio_scan_context *scan_ctx = iio_create_scan_context(NULL, 0);
  if (scan_ctx == NULL) {
    perror("unable to create scan context");
    return NULL;
  }
  struct iio_context_info **info;
  ssize_t ret;
  ret = iio_scan_context_get_info_list(scan_ctx, &info);
  if (ret < 0) {
//    plutosdr_print_message("scanning for iio contexts failed: %s\n", ret);
    iio_scan_context_destroy(scan_ctx);
    return NULL;
  }

  if (ret == 0) {
    fprintf(stderr, "no iio context found\n");
    iio_context_info_list_free(info);
    iio_scan_context_destroy(scan_ctx);
    return NULL;
  }

  const char *uri = iio_context_info_get_uri(info[0]);
  fprintf(stderr, "using context uri: %s\n", uri);
  struct iio_context *ctx = iio_create_context_from_uri(uri);

  iio_context_info_list_free(info);
  iio_scan_context_destroy(scan_ctx);

  if (ctx == NULL) {
    perror("unable to create context");
    return NULL;
  }
  ret = iio_context_set_timeout(ctx, 60000);
  if (ret != 0) {
    iio_context_destroy(ctx);
    return NULL;
  }
  return ctx;
}

//FIXME
int plutosdr_setup_fir_filter(struct iio_context *ctx) {
  struct iio_device *dev = NULL;//plutosdr_find_device_bysuffix(ctx, "-phy");
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
