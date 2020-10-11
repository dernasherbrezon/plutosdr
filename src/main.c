#include <getopt.h>
#include <stdlib.h>
#include "plutosdrCli.h"
#include "sigHandler.h"

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;
	unsigned long int frequency;
	unsigned long int sampleRate;
	float gain;
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
	if (!gain) {
		fprintf(stderr, "gain not specified\n");
		return EXIT_FAILURE;
	}
	if (!bufferSize) {
		bufferSize = 256;
	}

	setup_sig_handler();

	return plutosdrConfigureAndRun(frequency, sampleRate, gain, bufferSize);
}

