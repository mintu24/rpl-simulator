#ifndef BASE_H_
#define BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#ifndef M_PI
#define M_PI           		3.14159265358979323846
#endif


    /**** configuration ****/

#define DEBUG


    /**** general types ****/

#ifndef bool
#define bool                int
#endif

#ifndef TRUE
#define FALSE               (bool) 0
#define TRUE                (bool) 1
#endif

#define int8                signed char
#define uint8               unsigned char
#define int16               signed short
#define uint16              unsigned short
#define int32               signed int
#define uint32              unsigned int
#define int64               signed long long
#define uint64              unsigned long long


    /**** specific types ****/

#define coord_t             uint16
#define percent_t           double


#define RES_DIR             "resources"


#define rs_info(args...)                rs_print(stdout, "* ", NULL, 0, NULL, args)

#ifdef DEBUG
#define rs_debug(args...)               rs_print(stderr, "@ ", __FILE__, __LINE__, __FUNCTION__, args)
#else
#define rs_debug(args...)
#endif  /* DEBUG */

#define rs_warn(args...)                rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_error(args...)               rs_print(stderr, "! ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_assert(cond)        { if (!(cond)) rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, "assertion '%s' failed", #cond); }


void rs_print(FILE *stream, char *sym, const char *file, int line, const char *function, const char *fmt, ...);
void rs_quit();


#endif /* BASE_H_ */
