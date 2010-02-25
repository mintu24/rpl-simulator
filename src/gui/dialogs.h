#ifndef DIALOGS_H_
#define DIALOGS_H_

#include <gtk/gtk.h>

#include "../base.h"

#define ADD_MORE_DIALOG_PATTERN_RECTANGULAR         0
#define ADD_MORE_DIALOG_PATTERN_TRIANGULAR          1
#define ADD_MORE_DIALOG_PATTERN_RANDOM              2


typedef struct add_more_dialog_info_t {

    uint16          node_number;
    uint8           pattern;
    coord_t         horiz_dist;
    coord_t         vert_dist;
    uint16          row_length;

} add_more_dialog_info_t;


bool                            dialogs_init();
bool                            dialogs_done();

add_more_dialog_info_t *        add_more_dialog_run();

#endif /* DIALOGS_H_ */
