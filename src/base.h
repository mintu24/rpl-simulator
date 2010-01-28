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


#define RES_DIR             "resources"

#define DEBUG_MAIN          (1 << 0)
#define DEBUG_SYSTEM        (1 << 1)
#define DEBUG_NODE          (1 << 2)
#define DEBUG_PHY           (1 << 3)
#define DEBUG_MAC           (1 << 4)
#define DEBUG_IP            (1 << 5)
#define DEBUG_ICMP          (1 << 6)
#define DEBUG_RPL           (1 << 7)
#define DEBUG_GUI           (1 << 8)
#define DEBUG_MUTEX         (1 << 9)

#define DEBUG_MINIMAL       (DEBUG_MAIN | DEBUG_SYSTEM | DEBUG_NODE)
#define DEBUG_PROTO         (DEBUG_PHY | DEBUG_MAC | DEBUG_IP | DEBUG_ICMP | DEBUG_RPL)
#define DEBUG_ALL           (DEBUG_MINIMAL | DEBUG_PROTO | DEBUG_GUI | DEBUG_MUTEX)

//#define DEBUG               DEBUG_RPL

#define proto_node_lock(proto, mutex)                           \
    { rs_debug(DEBUG_MUTEX, "locking " proto " mutex (%d)", (mutex)->depth);         \
    g_static_rec_mutex_lock(mutex);                             \
    rs_debug(DEBUG_MUTEX, proto " mutex locked (%d)", (mutex)->depth); }

#define proto_node_unlock(proto, mutex)                         \
    { if ((mutex)->depth == 1) (mutex)->depth = 0; g_static_rec_mutex_unlock(mutex);                         \
    rs_debug(DEBUG_MUTEX, proto " mutex unlocked (%d)", (mutex)->depth); }

#define rs_info(args...)                rs_print(stdout, "* ", NULL, 0, NULL, args)

#ifdef DEBUG
#define rs_debug(cat, args...)          { if (cat & (DEBUG)) rs_print(stderr, "@ ", __FILE__, __LINE__, __FUNCTION__, args); }
#else
#define rs_debug(args...)
#endif  /* DEBUG */

#define rs_warn(args...)                rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_error(args...)               rs_print(stderr, "! ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_assert(cond)                 { if (!(cond)) rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, "assertion '%s' failed", #cond); }


void                        rs_print(FILE *stream, char *sym, const char *file, int line, const char *function, const char *fmt, ...);


#endif /* BASE_H_ */
