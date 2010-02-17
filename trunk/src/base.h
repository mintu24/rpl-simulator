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

#define RS_VERSION          "0.1"


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

#define coord_t             float
#define percent_t           float
#define sim_time_t          uint32


#define RES_DIR             "../resources"

#define DEBUG_MAIN          (1 << 0)
#define DEBUG_EVENT         (1 << 1)
#define DEBUG_SYSTEM        (1 << 2)
#define DEBUG_PHY           (1 << 3)
#define DEBUG_MAC           (1 << 4)
#define DEBUG_IP            (1 << 5)
#define DEBUG_ICMP          (1 << 6)
#define DEBUG_RPL           (1 << 7)
#define DEBUG_GUI           (1 << 8)
#define DEBUG_EVENTS_MUTEX  (1 << 9)
#define DEBUG_SCHEDULES_MUTEX   (1 << 10)
#define DEBUG_NODES_MUTEX   (1 << 11)
#define DEBUG_MEASURES_MUTEX    (1 << 12)

#define DEBUG_NONE          0
#define DEBUG_MINIMAL       (DEBUG_MAIN | DEBUG_SYSTEM | DEBUG_EVENT)
#define DEBUG_PROTO         (DEBUG_PHY | DEBUG_MAC | DEBUG_IP | DEBUG_ICMP | DEBUG_RPL)
#define DEBUG_MUTEX         (DEBUG_EVENTS_MUTEX | DEBUG_SCHEDULES_MUTEX | DEBUG_NODES_MUTEX | DEBUG_MEASURES_MUTEX)
#define DEBUG_ALL           (DEBUG_MINIMAL | DEBUG_PROTO | DEBUG_GUI | DEBUG_MUTEX)

#define DEBUG               (DEBUG_MAIN | DEBUG_RPL)

#define rs_info(args...)                rs_print(stdout, "* ", NULL, 0, NULL, args)

#ifdef DEBUG
#define rs_debug(cat, args...)          { if (cat & (DEBUG)) rs_print(stderr, "@ ", __FILE__, __LINE__, __FUNCTION__, args); }
#else
#define rs_debug(node, args...)
#endif  /* DEBUG */

#define rs_warn(args...)                rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_error(args...)               rs_print(stderr, "! ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_assert(cond)                 { if (!(cond)) rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, "assertion '%s' failed", #cond); }

extern char *               rs_app_dir;

void                        rs_print(FILE *stream, char *sym, const char *file, int line, const char *function, const char *fmt, ...);


#endif /* BASE_H_ */
