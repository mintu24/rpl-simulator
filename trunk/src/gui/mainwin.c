#include <gdk/gdk.h>
#include <cairo.h>

#include "mainwin.h"
#include "simfield.h"


    /**** global variables ****/

static coord_t current_x, current_y;
static double power = 0.5;
static gboolean moving = FALSE;
static GtkWidget *drawing_area;


    /**** callbacks ****/

static gboolean cb_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    rs_debug(NULL);

    draw_simfield(widget->window, widget->style->fg_gc[widget->state], current_x, current_y, power);

    return TRUE;
}

static gboolean cb_drawing_area_button_press(GtkDrawingArea *widget, GdkEventButton *event, gpointer data)
{
    rs_debug(NULL);

    moving = TRUE;

    current_x = event->x;
    current_y = event->y;

    snap_node_coords(&current_x, &current_y);

    gtk_widget_queue_draw(drawing_area);

    return TRUE;
}

static gboolean cb_drawing_area_button_release(GtkDrawingArea *widget, GdkEventButton *event, gpointer data)
{
    rs_debug(NULL);

    moving = FALSE;

    return TRUE;
}

static gboolean cb_drawing_area_motion_notify(GtkDrawingArea *widget, GdkEventMotion *event, gpointer data)
{
    rs_debug(NULL);

    if (!moving) {
        return FALSE;
    }

    current_x = event->x;
    current_y = event->y;

    snap_node_coords(&current_x, &current_y);

    gtk_widget_queue_draw(drawing_area);

    return TRUE;
}

static gboolean cb_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data)
{
    rs_debug(NULL);

    if (event->direction == GDK_SCROLL_UP) {
        if (power + 0.1 <= 1.0) {
            power += 0.1;
        }
        else {
            power = 1.0;
        }
    }
    else /* (event->direction == GDK_SCROLL_UP) */ {
        if (power - 0.1 >= 0.0) {
            power -= 0.1;
        }
        else {
            power = 0.0;
        }
    }

    snap_node_coords(&current_x, &current_y);

    gtk_widget_queue_draw(drawing_area);

    return TRUE;
}

static void cb_expander_toggle_button_clicked(GtkToggleButton *button, GtkWidget *widget)
{
    gtk_widget_set_visible(widget, gtk_toggle_button_get_active(button));
}


static void cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data)
{
    rs_debug(NULL);

    rs_quit();
}

static void cb_main_window_delete()
{
    rs_debug(NULL);

    rs_quit();
}


    /**** widget creation ****/

GtkWidget *create_params_widget()
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 200, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);

    /* System */
    GtkWidget *system_button = gtk_toggle_button_new_with_label("System");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(system_button), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), system_button, FALSE, TRUE, 0);

    GtkWidget *label1 = gtk_label_new("Label1");
    gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(system_button), "clicked", G_CALLBACK(cb_expander_toggle_button_clicked), label1);

    /* Nodes */
    GtkWidget *nodes_button = gtk_toggle_button_new_with_label("Nodes");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nodes_button), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), nodes_button, FALSE, TRUE, 0);

    GtkWidget *label2 = gtk_label_new("Label2");
    gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(nodes_button), "clicked", G_CALLBACK(cb_expander_toggle_button_clicked), label2);

    /* Display */
    GtkWidget *display_button = gtk_toggle_button_new_with_label("Display");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(display_button), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), display_button, FALSE, TRUE, 0);

    GtkWidget *label3 = gtk_label_new("Label3");
    gtk_box_pack_start(GTK_BOX(vbox), label3, FALSE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(display_button), "clicked", G_CALLBACK(cb_expander_toggle_button_clicked), label3);


    return scrolled_window;
}

GtkWidget *create_monitoring_widget()
{
    GtkWidget *label = gtk_label_new("Monitoring");

    return label;
}

GtkWidget *create_drawing_area()
{
    drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(drawing_area, GDK_ALL_EVENTS_MASK);
    gtk_signal_connect(GTK_OBJECT(drawing_area), "expose-event", GTK_SIGNAL_FUNC(cb_drawing_area_expose), NULL);
    gtk_signal_connect(GTK_OBJECT(drawing_area), "button-press-event", GTK_SIGNAL_FUNC(cb_drawing_area_button_press), NULL);
    gtk_signal_connect(GTK_OBJECT(drawing_area), "button-release-event", GTK_SIGNAL_FUNC(cb_drawing_area_button_release), NULL);
    gtk_signal_connect(GTK_OBJECT(drawing_area), "motion-notify-event", GTK_SIGNAL_FUNC(cb_drawing_area_motion_notify), NULL);
    gtk_signal_connect(GTK_OBJECT(drawing_area), "scroll-event", GTK_SIGNAL_FUNC(cb_drawing_area_scroll), NULL);

    return drawing_area;
}

GtkWidget *create_menu_bar()
{
    GtkWidget *menu_bar = gtk_menu_bar_new();

    /* file menu */
    GtkWidget *file_menu = gtk_menu_new();

    gtk_menu_append(file_menu, gtk_separator_menu_item_new());

    GtkWidget *quit_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    gtk_signal_connect(GTK_OBJECT(quit_menu_item), "activate", GTK_SIGNAL_FUNC(cb_quit_menu_item_activate), NULL);
    gtk_menu_append(file_menu, quit_menu_item);

    GtkWidget *file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_bar_append(menu_bar, file_menu_item);

    /* simulation menu */
    GtkWidget *simulation_menu = gtk_menu_new();

    GtkWidget *start_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
    gtk_menu_append(simulation_menu, start_menu_item);

    GtkWidget *stop_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
    gtk_menu_append(simulation_menu, stop_menu_item);

    GtkWidget *simulation_menu_item = gtk_menu_item_new_with_mnemonic("_Simulation");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(simulation_menu_item), simulation_menu);
    gtk_menu_bar_append(menu_bar, simulation_menu_item);

    /* help menu */
    GtkWidget *help_menu = gtk_menu_new();

    GtkWidget *about_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    gtk_menu_append(help_menu, about_menu_item);

    GtkWidget *help_menu_item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);
    gtk_menu_bar_append(menu_bar, help_menu_item);

    return menu_bar;
}

GtkWidget *create_tool_bar()
{
    GtkWidget *toolbar = gtk_toolbar_new();

    GtkToolItem *start_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_tool_item_set_tooltip_text(start_toolbar_item, "Start Simulation");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), start_toolbar_item, 0);

    GtkToolItem *stop_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    gtk_tool_item_set_tooltip_text(stop_toolbar_item, "Stop Simulation");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), stop_toolbar_item, 1);

    return toolbar;
}

GtkWidget *create_status_bar()
{
    GtkWidget *status_bar = gtk_statusbar_new();

    return status_bar;
}

GtkWidget *create_content_widget()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 5);

    GtkWidget *params_widget = create_params_widget();
    gtk_box_pack_start(GTK_BOX(hbox), params_widget, FALSE, TRUE, 0);

    GtkWidget *drawing_area = create_drawing_area();
    gtk_box_pack_start(GTK_BOX(hbox), drawing_area, TRUE, TRUE, 0);

    GtkWidget *monitoring_widget = create_monitoring_widget();
    gtk_box_pack_start(GTK_BOX(hbox), monitoring_widget, FALSE, TRUE, 0);

    return hbox;
}

GtkWidget *main_window_create()
{
    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), MAIN_WIN_WIDTH, MAIN_WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(main_window), "RPL Simulator");
    gtk_signal_connect(GTK_OBJECT(main_window), "delete-event", GTK_SIGNAL_FUNC(cb_main_window_delete), NULL);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);

    GtkWidget *tool_bar = create_tool_bar();
    gtk_box_pack_start(GTK_BOX(vbox), tool_bar, FALSE, TRUE, 0);

    GtkWidget *content_widget = create_content_widget();
    gtk_box_pack_start(GTK_BOX(vbox), content_widget, TRUE, TRUE, 0);

    GtkWidget *status_bar = create_status_bar();
    gtk_box_pack_start(GTK_BOX(vbox), status_bar, FALSE, TRUE, 0);

    return main_window;
}
