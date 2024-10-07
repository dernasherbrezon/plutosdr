#include <iio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pluto_util.h"

#define MAX_SIZE 2000000

int main(int argc, char *argv[]) {
  int opt;
  opterr = 0;
  unsigned long int frequency;
  unsigned long int sampleRate;
  unsigned int bufferSize;
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
        bufferSize = strtoul(optarg, NULL, 10);
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

  if (!frequency) {
    fprintf(stderr, "frequency not specified\n");
    return EXIT_FAILURE;
  }
  if (!sampleRate) {
    fprintf(stderr, "sampleRate not specified\n");
    return EXIT_FAILURE;
  }
  if (!bufferSize) {
    bufferSize = 256;
  }

  FILE *input;
  if (filename == NULL) {
    input = stdin;
  } else {
    input = fopen(filename, "rb");
    if (input == NULL) {
      fprintf(stderr, "unable to open file: %s\n", filename);
      return EXIT_FAILURE;
    }
  }

  signal(SIGINT, plutosdr_stop_async);
  signal(SIGHUP, plutosdr_stop_async);
  signal(SIGSEGV, plutosdr_stop_async);
  signal(SIGTERM, plutosdr_stop_async);

  return plutosdr_tx(frequency, sampleRate, bufferSize, input);
}