/*
 * plutosdrCli.h
 *
 *  Created on: Oct 9, 2020
 *      Author: dernasherbrezon
 */

#ifndef plutosdrCli_H_
#define plutosdrCli_H_

#include <stdint.h>
#include <stdio.h>
#include <iio.h>
#include <signal.h>

int plutosdr_configure_and_run(unsigned long int frequency, unsigned long int sampleRate, float gain, unsigned int bufferSize);
void plutosdr_stop_async();

#endif /* plutosdrCli_H_ */
