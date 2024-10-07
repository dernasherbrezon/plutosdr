#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "pluto_util.h"

int main(int argc, char *argv[]) {
  int opt;
  opterr = 0;
  unsigned long int frequency = 0;
  unsigned long int sample_rate = 0;
  float gain = -1;
  unsigned int buffer_size = 0;
  char *filename = NULL;
  unsigned long int number_of_samples_to_read = 0;
  while ((opt = getopt(argc, argv, "f:s:g:o:n:hb:")) != EOF) {
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
      case 'o':
        filename = optarg;
        break;
      case 'n':
        number_of_samples_to_read = strtoul(optarg, NULL, 10);
        break;
      case 'h':
      default:
        fprintf(stderr, "%s -f frequency -s sample_rate -g gain [-o output_file] [-b buffer size in samples] [-h]\n", argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (frequency == 0) {
    fprintf(stderr, "frequency not specified\n");
    return EXIT_FAILURE;
  }
  if (sample_rate == 0) {
    fprintf(stderr, "sample_rate not specified\n");
    return EXIT_FAILURE;
  }
  if (gain < 0) {
    fprintf(stderr, "gain not specified\n");
    return EXIT_FAILURE;
  }
  if (buffer_size == 0) {
    buffer_size = 256;
  }

  FILE *output;
  if (filename == NULL) {
    output = stdout;
  } else {
    output = fopen(filename, "wb");
    if (output == NULL) {
      fprintf(stderr, "unable to open file: %s\n", filename);
      return EXIT_FAILURE;
    }
  }

  signal(SIGINT, plutosdr_stop_async);
  signal(SIGHUP, plutosdr_stop_async);
  signal(SIGSEGV, plutosdr_stop_async);
  signal(SIGTERM, plutosdr_stop_async);

  int code = plutosdr_rx(frequency, sample_rate, gain, buffer_size, number_of_samples_to_read, output);
  if (filename != NULL) {
    fclose(output);
  }
  return code;
}

