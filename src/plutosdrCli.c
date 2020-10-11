#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "plutosdrCli.h"

#define BUF_SIZE 16384
#define FIR_BUF_SIZE	8192

// taken from iio-oscilloscope AD936x_LP_180kHz_521kSPS.ftr
static int16_t fir[] = { -170, 531, 134, -2952, -132, 4439, 194, -5060, 51, 4235, 277, -3352, 176, 2941, 260, -1977, 106, 1979, 61, -1356, -119, 1119, -174, -1215, -253, 493, -181, -1034, -93, 379, 95, -518, 233, 687, 350, 50, 328, 850, 220, 104, -6, 418, -238, -441, -440, -305, -498, -901, -407, -485, -148, -541, 183, 217, 506, 425, 684, 1027, 658, 874, 391, 822, -38, 84, -516, -418, -867, -1163, -969, -1290, -741, -1259, -232, -523, 429, 245, 1020, 1253, 1337, 1731, 1226, 1868, 675, 1172, -190, 157, -1105, -1217, -1764, -2193, -1896, -2673, -1383, -2151, -300, -911, 1061,
		940, 2274, 2649, 2887, 3786, 2582, 3676, 1284, 2315, -755, -223, -2990, -3139, -4687, -5637, -5111, -6622, -3724, -5450, -366, -1748, 4671, 4105, 10652, 11284, 16552, 18450, 21271, 24257, 23891, 27484, 23891, 27484, 21271, 24257, 16552, 18450, 10652, 11284, 4671, 4105, -366, -1748, -3724, -5450, -5111, -6622, -4687, -5637, -2990, -3139, -755, -223, 1284, 2315, 2582, 3676, 2887, 3786, 2274, 2649, 1061, 940, -300, -911, -1383, -2151, -1896, -2673, -1764, -2193, -1105, -1217, -190, 157, 675, 1172, 1226, 1868, 1337, 1731, 1020, 1253, 429, 245, -232, -523, -741, -1259,
		-969, -1290, -867, -1163, -516, -418, -38, 84, 391, 822, 658, 874, 684, 1027, 506, 425, 183, 217, -148, -541, -407, -485, -498, -901, -440, -305, -238, -441, -6, 418, 220, 104, 328, 850, 350, 50, 233, 687, 95, -518, -93, 379, -181, -1034, -253, 493, -174, -1215, -119, 1119, 61, -1356, 106, 1979, 260, -1977, 176, 2941, 277, -3352, 51, 4235, 194, -5060, -132, 4439, 134, -2952, -170, 531 };

static struct iio_context *ctx;
static struct iio_buffer *buffer;
static volatile sig_atomic_t app_running = true;

void plutosdr_cli_stop_async() {
	app_running = false;
}

int plutosdr_cli_ends_with(const char *str, const char *suffix) {
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

void plutosdr_cli_print_message(const char *message, int ret) {
	char err_str[1024];
	iio_strerror(-(int) ret, err_str, sizeof(err_str));
	fprintf(stderr, message, err_str);
}

struct iio_device* plutosdr_cli_find_device_with_input_channels(struct iio_context *ctx, const char *suffix) {
	unsigned int nb_devices = iio_context_get_devices_count(ctx);
	unsigned int nb_channels = 0;
	for (int i = 0; i < nb_devices; i++) {
		struct iio_device *curDev = iio_context_get_device(ctx, i);
		const char *name = iio_device_get_name(curDev);
		if (name == NULL) {
			continue;
		}
		if (!plutosdr_cli_ends_with(name, suffix)) {
			continue;
		}
		nb_channels = iio_device_get_channels_count(curDev);
		for (int j = 0; j < nb_channels; j++) {
			struct iio_channel *ch = iio_device_get_channel(curDev, j);
			if (!iio_channel_is_output(ch)) {
				fprintf(stderr, "input device: %s\n", name);
				return curDev;
			}
		}
	}
	return NULL;
}

struct iio_device* plutosdr_cli_find_device_bysuffix(struct iio_context *ctx, const char *suffix) {
	unsigned int nb_devices = iio_context_get_devices_count(ctx);
	struct iio_device *dev = NULL;
	for (int i = 0; i < nb_devices; i++) {
		struct iio_device *curDev = iio_context_get_device(ctx, i);
		const char *name = iio_device_get_name(curDev);
		if (name == NULL) {
			continue;
		}
		if (plutosdr_cli_ends_with(name, suffix)) {
			fprintf(stderr, "device: %s\n", name);
			dev = curDev;
			break;
		}
	}
	return dev;
}

int plutosdr_cli_get_rx_fir_enable(struct iio_device *dev, int *enable) {
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

int plutosdr_cli_set_txrx_fir_enable(struct iio_device *dev, int enable) {
	int ret = iio_device_attr_write_bool(dev, "in_out_voltage_filter_fir_en", !!enable);
	if (ret < 0) {
		ret = iio_channel_attr_write_bool(iio_device_find_channel(dev, "out", false), "voltage_filter_fir_en", !!enable);
	}
	return ret;
}

int plutosdr_cli_write_buffer_stdout() {
	void *start = iio_buffer_start(buffer);
	size_t len = (intptr_t) iio_buffer_end(buffer) - (intptr_t) start;

	while (len > 0) {
		size_t nb = fwrite(start, 1, len, stdout);
		if (!nb) {
			return nb;
		}

		len -= nb;
		start = (void*) ((intptr_t) start + nb);
	}
	return 0;
}

int plutosdr_cli_configure_and_run(unsigned long int frequency, unsigned long int sampleRate, float gain, unsigned int bufferSize) {
	struct iio_scan_context *scan_ctx;
	struct iio_context_info **info;
	ssize_t ret;

	scan_ctx = iio_create_scan_context(NULL, 0);
	if (!scan_ctx) {
		fprintf(stderr, "unable to create scan context\n");
		return EXIT_FAILURE;
	}
	ret = iio_scan_context_get_info_list(scan_ctx, &info);
	if (ret < 0) {
		plutosdr_cli_print_message("scanning for iio contexts failed: %s\n", ret);
		iio_scan_context_destroy(scan_ctx);
		return EXIT_FAILURE;
	}

	if (ret == 0) {
		fprintf(stderr, "no iio context found\n");
		iio_context_info_list_free(info);
		iio_scan_context_destroy(scan_ctx);
		return EXIT_FAILURE;
	}

	const char *uri = iio_context_info_get_uri(info[0]);
	fprintf(stderr, "context uri: %s\n", uri);
	ctx = iio_create_context_from_uri(uri);

	iio_context_info_list_free(info);
	iio_scan_context_destroy(scan_ctx);

	if (ctx == NULL) {
		plutosdr_cli_print_message("unable to create context: %s\n", -errno);
		return EXIT_FAILURE;
	}

	ret = iio_context_set_timeout(ctx, 60000);
	if (ret < 0) {
		plutosdr_cli_print_message("unable to setup context timeout: %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	struct iio_device *dev = plutosdr_cli_find_device_bysuffix(ctx, "-phy");
	if (dev == NULL) {
		fprintf(stderr, "unable to find -phy device\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	struct iio_channel *altVoltage0 = iio_device_find_channel(dev, "altvoltage0", true);
	if (altVoltage0 == NULL) {
		fprintf(stderr, "unable to find channel: altvoltage0\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	ret = iio_channel_attr_write_longlong(altVoltage0, "frequency", frequency);
	if (ret < 0) {
		plutosdr_cli_print_message("unable to set frequency: %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	struct iio_channel *voltage0 = iio_device_find_channel(dev, "voltage0", false);
	if (voltage0 == NULL) {
		fprintf(stderr, "unable to find channel: voltage0\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	bool filterFirEnabled;
	ret = iio_channel_attr_read_bool(voltage0, "filter_fir_en", &filterFirEnabled);
	if (ret < 0) {
		plutosdr_cli_print_message("unable to read filter_fir_en attribute: %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	if (!filterFirEnabled) {
		char *buf;
		int len = 0;

		buf = malloc(FIR_BUF_SIZE);
		if (!buf) {
			iio_context_destroy(ctx);
			return -ENOMEM;
		}
		len += snprintf(buf + len, FIR_BUF_SIZE - len, "RX 3 GAIN -6 DEC 4\n");
		len += snprintf(buf + len, FIR_BUF_SIZE - len, "TX 3 GAIN 0 INT 4\n");
		for (int i = 0, j = 0; i < 128; i++, j += 2) {
			len += snprintf(buf + len, FIR_BUF_SIZE - len, "%d,%d\n", fir[j], fir[j + 1]);
		}
		len += snprintf(buf + len, FIR_BUF_SIZE - len, "\n");
		ret = iio_device_attr_write_raw(dev, "filter_fir_config", buf, len);
		free(buf);

		if (ret < 0) {
			plutosdr_cli_print_message("unable to setup fir filter config: %s\n", ret);
			iio_context_destroy(ctx);
			return EXIT_FAILURE;
		}

		ret = plutosdr_cli_set_txrx_fir_enable(dev, true);
		if (ret < 0) {
			plutosdr_cli_print_message("unable to enable fir filter: %s\n", ret);
			iio_context_destroy(ctx);
			return EXIT_FAILURE;
		}

		fprintf(stderr, "fir filter enabled\n");
	}

	ret = iio_channel_attr_write_longlong(voltage0, "sampling_frequency", sampleRate);
	if (ret < 0) {
		plutosdr_cli_print_message("unable to setup sampling frequency: %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	ret = iio_channel_attr_write_longlong(voltage0, "rf_bandwidth", 400000);
	if (ret < 0) {
		plutosdr_cli_print_message("unable to setup rf_bandwidth: %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	if (gain > 0.0) {
		ret = iio_channel_attr_write(voltage0, "gain_control_mode", "manual");
		if (ret < 0) {
			plutosdr_cli_print_message("unable to switch to manual gain: %s\n", ret);
			iio_context_destroy(ctx);
			return EXIT_FAILURE;
		}

		ret = iio_channel_attr_write_double(voltage0, "hardwaregain", gain);
		if (ret < 0) {
			plutosdr_cli_print_message("unable to setup gain: %s\n", ret);
			iio_context_destroy(ctx);
			return EXIT_FAILURE;
		}

		fprintf(stderr, "gain manual: %.2f\n", gain);
	} else {
		ret = iio_channel_attr_write(voltage0, "gain_control_mode", "slow_attack");
		if (ret < 0) {
			plutosdr_cli_print_message("unable to switch to slow_attack gain: %s\n", ret);
			iio_context_destroy(ctx);
			return EXIT_FAILURE;
		}

		fprintf(stderr, "gain auto: slow_attack\n");
	}

	struct iio_device *inputDevice = plutosdr_cli_find_device_with_input_channels(ctx, "-lpc");
	if (inputDevice == NULL) {
		fprintf(stderr, "unable to find input device\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}
	unsigned int nb_channels = iio_device_get_channels_count(inputDevice);
	for (int i = 0; i < nb_channels; i++) {
		struct iio_channel *ch = iio_device_get_channel(inputDevice, i);
		if (ch == NULL) {
			continue;
		}
		if (!iio_channel_is_output(ch)) {
			iio_channel_enable(ch);
		}
	}

	ssize_t sample_size = iio_device_get_sample_size(inputDevice);
	if (sample_size == 0) {
		fprintf(stderr, "unable to get sample size, returned 0\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	} else if (sample_size < 0) {
		plutosdr_cli_print_message("unable to get sample size: %s\n", errno);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "buffer size: %d samples\n", bufferSize);
	buffer = iio_device_create_buffer(inputDevice, bufferSize, false);
	if (!buffer) {
		plutosdr_cli_print_message("unable to allocate buffer: %s\n", errno);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	while (app_running) {
		ret = iio_buffer_refill(buffer);
		if (ret < 0) {
			if (app_running) {
				plutosdr_cli_print_message("unable to refill buffer: %s\n", ret);
			}
			break;
		}

		ret = plutosdr_cli_write_buffer_stdout();
		if (ret < 0) {
			fprintf(stderr, "unable to write to stdout\n");
			break;
		}
	}

	iio_buffer_cancel(buffer);
	iio_context_destroy(ctx);
	fprintf(stderr, "stopped\n");
	return EXIT_SUCCESS;
}
