#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#define TRACE_NUM       (8)
#define TRACE_LOST      (100)   /* 丢线标志，比任何正常误差都大 */

void    trace_init(void);
uint8_t trace_bits(void);
int16_t trace_error(void);

#endif