#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "pluto_util.h"

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;
	unsigned long int frequency;
	unsigned long int sample_rate;
	float gain = -1;
	unsigned int buffer_size;
	while ((opt = getopt(argc, argv, "f:s:g:hb:")) != EOF) {
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
		case 'h':
		default:
			fprintf(stderr, "%s -f frequency -s sample_rate -g gain [-b buffer size in samples] [-h]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (!frequency) {
		fprintf(stderr, "frequency not specified\n");
		return EXIT_FAILURE;
	}
	if (!sample_rate) {
		fprintf(stderr, "sample_rate not specified\n");
		return EXIT_FAILURE;
	}
	if (gain < 0) {
		fprintf(stderr, "gain not specified\n");
		return EXIT_FAILURE;
	}
	if (!buffer_size) {
    buffer_size = 256;
	}

	signal(SIGINT, plutosdr_stop_async);
	signal(SIGHUP, plutosdr_stop_async);
	signal(SIGSEGV, plutosdr_stop_async);
	signal(SIGTERM, plutosdr_stop_async);

	return plutosdr_rx(frequency, sample_rate, gain, buffer_size);
}

