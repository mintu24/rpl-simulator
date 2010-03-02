
#ifndef EVENT_H_
#define EVENT_H_

#include "base.h"
#include "node.h"


    /* a callback type representing an event handler */
typedef bool (* event_handler_t) (node_t *node, void *data1, void *data2);

    /* a callback type representing an event handler */
typedef void (* event_arg_str_t) (uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);

    /* structure representing an event (not to be confused with an event schedule) */
typedef struct event_t {

    char *                      name;
    char *                      layer;
    event_handler_t             handler;
    event_arg_str_t             str_func;
    bool                        loggable;

} event_t;


uint16                  event_register(char *name, char *layer, event_handler_t handler, event_arg_str_t str_func);
bool                    event_execute(uint16 event_id, node_t *node, void *data1, void *data2);
event_t                 event_find_by_id(uint16 event_id);
uint16                  event_get_count();
void                    event_set_logging(uint16 event_id, bool loggable);
bool                    event_get_logging(uint16 event_id);


#endif /* EVENT_H_ */
