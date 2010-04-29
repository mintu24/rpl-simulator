/*
   RPL Simulator.

   Copyright (c) Calin Crisan 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef BASE_H_
#define BASE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#ifndef M_PI
#define M_PI                        3.14159265358979323846
#endif


    /**** configuration ****/

#define RS_VERSION                  "r70"


    /**** general types ****/

#ifndef bool
#define bool                        int
#endif

#ifndef TRUE
#define FALSE                       (bool) 0
#define TRUE                        (bool) 1
#endif

#define int8                        signed char
#define uint8                       unsigned char
#define int16                       signed short
#define uint16                      unsigned short
#define int32                       signed int
#define uint32                      unsigned int
#define int64                       signed long long
#define uint64                      unsigned long long


    /**** specific types ****/

#define coord_t                     float
#define percent_t                   float
#define sim_time_t                  int32


    /**** others ****/

#define RES_DIR                     "../resources"
#define SCENARIO_DIR                "../scenarios"
// #define USE_DOCUMENT_COLORS_AND_SIZES /* define this to use a white background and other colors accordingly, in the sim field */


    /**** debug stuff ****/

#define DEBUG_MAIN                  (0 << 0)
#define DEBUG_EVENT                 (0 << 1)
#define DEBUG_SYSTEM                (0 << 2)
#define DEBUG_MEASURE               (0 << 3)
#define DEBUG_PHY                   (0 << 4)
#define DEBUG_MAC                   (0 << 5)
#define DEBUG_IP                    (0 << 6)
#define DEBUG_ICMP                  (0 << 7)
#define DEBUG_RPL                   (1 << 8)
#define DEBUG_GUI                   (0 << 9)
#define DEBUG_EVENTS_MUTEX          (0 << 10)
#define DEBUG_SCHEDULES_MUTEX       (0 << 11)
#define DEBUG_NODES_MUTEX           (0 << 12)
#define DEBUG_MEASURES_MUTEX        (0 << 13)
#define DEBUG_SCENARIO              (0 << 14)

#define DEBUG_NONE                  0
#define DEBUG_MINIMAL               (DEBUG_MAIN | DEBUG_SYSTEM | DEBUG_EVENT)
#define DEBUG_PROTO                 (DEBUG_PHY | DEBUG_MAC | DEBUG_IP | DEBUG_ICMP | DEBUG_RPL)
#define DEBUG_MUTEX                 (DEBUG_EVENTS_MUTEX | DEBUG_SCHEDULES_MUTEX | DEBUG_NODES_MUTEX | DEBUG_MEASURES_MUTEX)
#define DEBUG_ALL                   (DEBUG_MINIMAL | DEBUG_PROTO | DEBUG_GUI | DEBUG_MUTEX)

#define DEBUG                       1

#define rs_info(args...)            rs_print(stdout, "* ", NULL, 0, NULL, args)

#ifdef DEBUG
#define rs_debug(cat, args...)      { if (cat && (DEBUG)) rs_print(stderr, "@ ", __FILE__, __LINE__, __FUNCTION__, args); }
#define rs_assert(cond)             { if (!(cond)) rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, "assertion '%s' failed", #cond); }
#else
#define rs_debug(node, args...)
#define rs_assert(cond)
#endif  /* DEBUG */

#define rs_warn(args...)            rs_print(stderr, "# ", __FILE__, __LINE__, __FUNCTION__, args)
#define rs_error(args...)           rs_print(stderr, "! ", __FILE__, __LINE__, __FUNCTION__, args)

extern char *                       rs_app_dir;
extern char *                       rs_scenario_file_name;

void                                rs_print(FILE *stream, char *sym, const char *file, int line, const char *function, const char *fmt, ...);


#endif /* BASE_H_ */
