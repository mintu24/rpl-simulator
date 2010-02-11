#ifndef MEASUREMENT_H_
#define MEASUREMENT_H_

#include <gtk/gtk.h>

#include "../base.h"
#include "mainwin.h"


GtkWidget *                     measurement_widget_create();

void                            measurement_system_to_gui();
void                            measurement_entries_to_gui();
void                            measurement_output_to_gui();


#endif /* MEASUREMENT_H_ */
