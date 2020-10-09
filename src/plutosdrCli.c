#include <stdio.h>
#include <stdlib.h>
#include <iio.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 16384

int endsWith(const char *str, const char *suffix) {
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

void printUserFriendlyMessage(const char *message, int ret) {
	char err_str[1024];
	iio_strerror(-(int) ret, err_str, sizeof(err_str));
	fprintf(stderr, message, err_str);
}

int plutosdrConfigureAndRun(unsigned long int frequency, unsigned long int sampleRate, float gain) {
	struct iio_scan_context *scan_ctx;
	struct iio_context_info **info;
	struct iio_context *ctx = NULL;
	ssize_t ret;

	scan_ctx = iio_create_scan_context(NULL, 0);
	if (!scan_ctx) {
		fprintf(stderr, "Unable to create scan context\n");
		return EXIT_FAILURE;
	}
	ret = iio_scan_context_get_info_list(scan_ctx, &info);
	if (ret < 0) {
		printUserFriendlyMessage("Scanning for IIO contexts failed: %s\n", ret);
		iio_scan_context_destroy(scan_ctx);
		return EXIT_FAILURE;
	}

	if (ret == 0) {
		fprintf(stderr, "No IIO context found\n");
		iio_context_info_list_free(info);
		iio_scan_context_destroy(scan_ctx);
		return EXIT_FAILURE;
	}

	const char *uri = iio_context_info_get_uri(info[0]);
	fprintf(stderr, "Using the context uri: %s\n", uri);
	ctx = iio_create_context_from_uri(uri);

	iio_context_info_list_free(info);
	iio_scan_context_destroy(scan_ctx);

	if (ctx == NULL) {
		printUserFriendlyMessage("Unable to create context: %s\n", -errno);
		return EXIT_FAILURE;
	}

	ret = iio_context_set_timeout(ctx, 60000);
	if (ret < 0) {
		printUserFriendlyMessage("IIO contexts set timeout failed : %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	unsigned int nb_devices = iio_context_get_devices_count(ctx);
	struct iio_device *dev = NULL;
	for (int i = 0; i < nb_devices; i++) {
		struct iio_device *curDev = iio_context_get_device(ctx, i);
		const char *name = iio_device_get_name(curDev);
		if (name == NULL) {
			continue;
		}
		if (endsWith(name, "-phy")) {
			fprintf(stderr, "Using device: %s\n", name);
			dev = curDev;
			break;
		}
	}

	if (dev == NULL) {
		fprintf(stderr, "Unable to find -phy device\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	struct iio_channel *altVoltage0 = iio_device_find_channel(dev, "altvoltage0", true);
	if (altVoltage0 == NULL) {
		fprintf(stderr, "Unable to find channel: altvoltage0\n");
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	const int n = snprintf(NULL, 0, "%lu", frequency);
	char buf[n + 1];
	snprintf(buf, n + 1, "%lu", frequency);
	ret = iio_channel_attr_write(altVoltage0, "frequency", buf);
	if (ret < 0) {
		printUserFriendlyMessage("unable to set frequency : %s\n", ret);
		iio_context_destroy(ctx);
		return EXIT_FAILURE;
	}

	//FIXME do the rest

	return EXIT_SUCCESS;
}
