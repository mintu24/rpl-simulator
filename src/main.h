
#ifndef MAIN_H_
#define MAIN_H_

#include "base.h"
#include "node.h"

#define                 AUTOINC_ADDRESS_PART        16


extern                  GThread *rs_main_thread;


node_t *                rs_add_node();
void                    rs_load_params(char *filename);
void                    rs_quit();


#endif /* MAIN_H_ */
