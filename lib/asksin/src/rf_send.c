/* 
 * Copyright by R.Koenig
 * Inspired by code from Dirk Tostmann
 * License: GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/parity.h>
#include <string.h>

#include "rf_send.h"

uint16_t credit_10ms = MAX_CREDIT;
//uint16_t credit_10ms = MAX_CREDIT / 2;
