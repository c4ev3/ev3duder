/**
 * @file cat.c
 * @brief unimplemented
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

/**
 * @brief Concatenate files to the LCD screen.
 * @param [in] path remote FILE*s
 * @param [in] count number of FILE*s to cat
 *
 * @retval error according to enum #ERR
 * @see http://topikachu.github.io/python-ev3/UIdesign.html
 */
#define MAX_READ 1024

extern int cat(const char *rem, size_t count)
{


}

