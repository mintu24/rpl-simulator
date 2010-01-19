
#ifndef MAIN_H_
#define MAIN_H_

#include "base.h"
#include "node.h"


extern                  GThread *rs_main_thread;


void                    rs_add_node();
void                    rs_load_params(char *filename);
void                    rs_quit();


#endif /* MAIN_H_ */
