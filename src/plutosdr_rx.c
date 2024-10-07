#include <getopt.h>
#include <stdlib.h>
#include <signal.h>

#include "plutosdr.h"

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;
	unsigned long int frequency;
	unsigned long int sampleRate;
	float gain = -1;
	unsigned int bufferSize;
	while ((opt = getopt(argc, argv, "f:s:g:hb:")) != EOF) {
		switch (opt) {
		case 'f':
			frequency = strtoul(optarg, NULL, 10);
			break;
		case 's':
			sampleRate = strtoul(optarg, NULL, 10);
			break;
		case 'b':
			bufferSize = strtoul(optarg, NULL, 10);
			break;
		case 'g':
			gain = strtof(optarg, NULL);
			break;
		case 'h':
		default:
			fprintf(stderr, "%s -f frequency -s sampleRate -g gain [-b buffer size in samples] [-h]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (!frequency) {
		fprintf(stderr, "frequency not specified\n");
		return EXIT_FAILURE;
	}
	if (!sampleRate) {
		fprintf(stderr, "sampleRate not specified\n");
		return EXIT_FAILURE;
	}
	if (gain < 0) {
		fprintf(stderr, "gain not specified\n");
		return EXIT_FAILURE;
	}
	if (!bufferSize) {
		bufferSize = 256;
	}

	signal(SIGINT, plutosdr_stop_async);
	signal(SIGHUP, plutosdr_stop_async);
	signal(SIGSEGV, plutosdr_stop_async);
	signal(SIGTERM, plutosdr_stop_async);

	return plutosdr_configure_and_run(frequency, sampleRate, gain, bufferSize);
}

