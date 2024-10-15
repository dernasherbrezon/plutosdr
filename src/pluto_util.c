#include "pluto_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#define FIR_BUF_SIZE  8192

#define MAX_DATA_RATE 61440000UL
#define MIN_CLK 25000000UL
#define MIN_DATA_RATE (MIN_CLK / 12)
#define MIN_FIR_FILTER_DATA_RATE_2 (MIN_CLK / 24)
#define MIN_FIR_FILTER_DATA_RATE_4 (MIN_FIR_FILTER_DATA_RATE_2 / 2)
#define MIN_HDL_DATA_RATE (MIN_FIR_FILTER_DATA_RATE_4 / 8)

// taken from https://github.com/analogdevicesinc/libad9361-iio/blob/main/ad9361_baseband_auto_rate.c
static int16_t fir_128_4[] = {
    -15, -27, -23, -6, 17, 33, 31, 9, -23, -47, -45, -13, 34, 69, 67, 21, -49, -102, -99, -32, 69, 146, 143, 48, -96, -204, -200, -69, 129, 278, 275, 97, -170,
    -372, -371, -135, 222, 494, 497, 187, -288, -654, -665, -258, 376, 875, 902, 363, -500, -1201, -1265, -530, 699, 1748, 1906, 845, -1089, -2922, -3424,
    -1697, 2326, 7714, 12821, 15921, 15921, 12821, 7714, 2326, -1697, -3424, -2922, -1089, 845, 1906, 1748, 699, -530, -1265, -1201, -500, 363, 902, 875,
    376, -258, -665, -654, -288, 187, 497, 494, 222, -135, -371, -372, -170, 97, 275, 278, 129, -69, -200, -204, -96, 48, 143, 146, 69, -32, -99, -102, -49, 21,
    67, 69, 34, -13, -45, -47, -23, 9, 31, 33, 17, -6, -23, -27, -15};

static int16_t fir_128_2[] = {-170, 531, 134, -2952, -132, 4439, 194, -5060, 51, 4235, 277, -3352, 176, 2941, 260, -1977, 106, 1979, 61, -1356, -119, 1119, -174, -1215, -253, 493, -181, -1034, -93, 379, 95, -518, 233, 687, 350, 50, 328, 850, 220, 104, -6, 418, -238, -441, -440, -305, -498, -901,
                              -407,
                              -485, -148, -541, 183, 217, 506, 425, 684, 1027, 658, 874, 391, 822, -38, 84, -516, -418, -867, -1163, -969, -1290, -741, -1259, -232, -523, 429, 245, 1020, 1253, 1337, 1731, 1226, 1868, 675, 1172, -190, 157, -1105, -1217, -1764, -2193, -1896, -2673, -1383, -2151, -300,
                              -911,
                              1061,
                              940, 2274, 2649, 2887, 3786, 2582, 3676, 1284, 2315, -755, -223, -2990, -3139, -4687, -5637, -5111, -6622, -3724, -5450, -366, -1748, 4671, 4105, 10652, 11284, 16552, 18450, 21271, 24257, 23891, 27484, 23891, 27484, 21271, 24257, 16552, 18450, 10652, 11284, 4671, 4105,
                              -366,
                              -1748, -3724, -5450, -5111, -6622, -4687, -5637, -2990, -3139, -755, -223, 1284, 2315, 2582, 3676, 2887, 3786, 2274, 2649, 1061, 940, -300, -911, -1383, -2151, -1896, -2673, -1764, -2193, -1105, -1217, -190, 157, 675, 1172, 1226, 1868, 1337, 1731, 1020, 1253, 429, 245,
                              -232,
                              -523, -741, -1259,
                              -969, -1290, -867, -1163, -516, -418, -38, 84, 391, 822, 658, 874, 684, 1027, 506, 425, 183, 217, -148, -541, -407, -485, -498, -901, -440, -305, -238, -441, -6, 418, 220, 104, 328, 850, 350, 50, 233, 687, 95, -518, -93, 379, -181, -1034, -253, 493, -174, -1215, -119,
                              1119,
                              61, -1356, 106, 1979, 260, -1977, 176, 2941, 277, -3352, 51, 4235, 194, -5060, -132, 4439, 134, -2952, -170, 531};

//static int16_t fir_128_2[] = {
//    -0, 0, 1, -0, -2, 0, 3, -0, -5, 0, 8, -0, -11, 0, 17, -0, -24, 0, 33, -0, -45, 0, 61, -0, -80, 0, 104, -0, -134, 0, 169, -0,
//    -213, 0, 264, -0, -327, 0, 401, -0, -489, 0, 595, -0, -724, 0, 880, -0, -1075, 0, 1323, -0, -1652, 0, 2114, -0, -2819, 0, 4056, -0, -6883, 0, 20837, 32767,
//    20837, 0, -6883, -0, 4056, 0, -2819, -0, 2114, 0, -1652, -0, 1323, 0, -1075, -0, 880, 0, -724, -0, 595, 0, -489, -0, 401, 0, -327, -0, 264, 0, -213, -0,
//    169, 0, -134, -0, 104, 0, -80, -0, 61, 0, -45, -0, 33, 0, -24, -0, 17, 0, -11, -0, 8, 0, -5, -0, 3, 0, -2, -0, 1, 0, -0, 0};

int plutosdr_set_txrx_fir_enable(struct iio_device *dev, int enable) {
  int ret = iio_device_attr_write_bool(dev, "in_out_voltage_filter_fir_en", enable != 0);
  if (ret < 0) {
    ret = iio_channel_attr_write_bool(iio_device_find_channel(dev, "out", false), "voltage_filter_fir_en", enable != 0);
  }
  return ret;
}

int plutosdr_setup_fir_filter(struct iio_device *dev, int16_t *filter_128, int decimation) {
  char *buf;
  int len = 0;

  buf = malloc(FIR_BUF_SIZE);
  if (!buf) {
    return -ENOMEM;
  }
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "RX 3 GAIN -6 DEC %d\n", decimation);
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "TX 3 GAIN 0 INT %d\n", decimation);
  for (int i = 0, j = 0; i < 128; i++, j += 2) {
    len += snprintf(buf + len, FIR_BUF_SIZE - len, "%d,%d\n", filter_128[j], filter_128[j + 1]);
  }
  len += snprintf(buf + len, FIR_BUF_SIZE - len, "\n");
  ssize_t ret = iio_device_attr_write_raw(dev, "filter_fir_config", buf, len);
  free(buf);

  if (ret < 0) {
    fprintf(stderr, "unable to setup fir filter config");
    return -1;
  }

  ERROR_CHECK_CODE("unable to enable fir filter", plutosdr_set_txrx_fir_enable(dev, true));
  fprintf(stderr, "fir filter enabled %d\n", decimation);
  return 0;
}

int plutosdr_set_sampling_frequency(struct iio_device *dev, struct iio_channel *phy_channel, struct iio_channel *lpc_channel, unsigned long int sampling_frequency) {
  if (sampling_frequency < MIN_HDL_DATA_RATE) {
    return -1;
  } else if (sampling_frequency < MIN_FIR_FILTER_DATA_RATE_4) {
    if (8 * sampling_frequency < MIN_FIR_FILTER_DATA_RATE_2) {
      plutosdr_setup_fir_filter(dev, fir_128_4, 4);
    } else if (8 * sampling_frequency < MIN_DATA_RATE) {
      plutosdr_setup_fir_filter(dev, fir_128_2, 2);
    }
    ERROR_CHECK_CODE("unable to set sampling_frequency", iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", 8 * sampling_frequency));
    ERROR_CHECK_CODE("unable to set rf_bandwidth", iio_channel_attr_write_longlong(phy_channel, "rf_bandwidth", sampling_frequency));
    ERROR_CHECK_CODE("unable to set sampling frequency", iio_channel_attr_write_longlong(lpc_channel, "sampling_frequency", sampling_frequency));
  } else if (sampling_frequency < MIN_FIR_FILTER_DATA_RATE_2) {
    plutosdr_setup_fir_filter(dev, fir_128_4, 4);
    ERROR_CHECK_CODE("unable to set sampling_frequency", iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", sampling_frequency));
    ERROR_CHECK_CODE("unable to set rf_bandwidth", iio_channel_attr_write_longlong(phy_channel, "rf_bandwidth", sampling_frequency));
  } else if (sampling_frequency < MIN_DATA_RATE) {
    plutosdr_setup_fir_filter(dev, fir_128_2, 2);
    ERROR_CHECK_CODE("unable to set sampling_frequency", iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", sampling_frequency));
    ERROR_CHECK_CODE("unable to set rf_bandwidth", iio_channel_attr_write_longlong(phy_channel, "rf_bandwidth", sampling_frequency));
  } else if (sampling_frequency <= MAX_DATA_RATE) {
    ERROR_CHECK_CODE("unable to reset sampling_frequency", iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", 3000000));
    plutosdr_set_txrx_fir_enable(dev, false);
    ERROR_CHECK_CODE("unable to set sampling_frequency", iio_channel_attr_write_longlong(phy_channel, "sampling_frequency", sampling_frequency));
    ERROR_CHECK_CODE("unable to set rf_bandwidth", iio_channel_attr_write_longlong(phy_channel, "rf_bandwidth", sampling_frequency));
  } else {
    return -1;
  }
  return 0;
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

int plutosdr_disable_dds_channel(struct iio_device *tx_device, const char *channel_name) {
  struct iio_channel *channel = iio_device_find_channel(tx_device, channel_name, true);
  if (channel == NULL) {
    return -1;
  }
  ERROR_CHECK_CODE("cant to disable dds", iio_channel_attr_write_bool(channel, "raw", 0));
  return 0;
}

int plutosdr_disable_dds(struct iio_device *tx_device) {
  // normally disabling just TX1_I_F1 would disable all other DDS channels,
  // but I disable all just in case
  ERROR_CHECK_CODE("can't disable TX1_I_F1", plutosdr_disable_dds_channel(tx_device, "TX1_I_F1"));
  ERROR_CHECK_CODE("can't disable TX1_Q_F1", plutosdr_disable_dds_channel(tx_device, "TX1_Q_F1"));
  ERROR_CHECK_CODE("can't disable TX1_I_F2", plutosdr_disable_dds_channel(tx_device, "TX1_I_F2"));
  ERROR_CHECK_CODE("can't disable TX1_Q_F2", plutosdr_disable_dds_channel(tx_device, "TX1_Q_F2"));
  return 0;
}
