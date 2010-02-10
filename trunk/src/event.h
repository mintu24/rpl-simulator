
#ifndef EVENT_H_
#define EVENT_H_

#include "base.h"
#include "node.h"


    /* a callback type representing an event handler */
typedef bool (* event_handler_t) (node_t *node, void *data1, void *data2);

    /* structure representing an event (not to be confused with an event schedule) */
typedef struct event_t {

    char *                      name;
    event_handler_t             handler;

} event_t;


uint16                  event_register(char *name, event_handler_t handler);
bool                    event_execute(uint16 event_id, node_t *node, void *data1, void *data2);
event_t                 event_find_by_id(uint16 event_id);


#endif /* EVENT_H_ */
