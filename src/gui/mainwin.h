#ifndef MAINWIN_H_
#define MAINWIN_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "../base.h"

#define MAIN_WIN_WIDTH		800
#define MAIN_WIN_HEIGHT		600

#ifndef gtk_widget_set_visible
#define gtk_widget_set_visible(widget, visible) {if (visible) gtk_widget_show(widget); else gtk_widget_hide(widget);}
#endif


void                main_win_init();


#endif /* MAINWIN_H_ */
