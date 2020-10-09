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

int plutosdrConfigureAndRun(unsigned long int frequency, unsigned long int sampleRate, float gain);

#endif /* plutosdrCli_H_ */
