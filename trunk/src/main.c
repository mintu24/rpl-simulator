
#include <gtk/gtk.h>

#include "gui/mainwin.h"


void rs_print(FILE *stream, char *sym, const char *fname, const char *fmt, ...)
{
    char string[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);

    if (strlen(string) > 0) {
        fprintf(stream, "%s%s(): %s\n", sym, fname, string);
    }
    else {
        fprintf(stream, "%s%s()\n", sym, fname);
    }
}

void rs_quit()
{
    gtk_main_quit();
}


int main(int argc, char *argv[]) {
	rs_info("hello");

	gtk_init(&argc, &argv);
	GtkWidget *main_window = main_window_create();
	gtk_widget_show_all(main_window);
	gtk_main();

	rs_info("bye!");

	return 0;
}
