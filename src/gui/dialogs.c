
#include "dialogs.h"
#include "mainwin.h"

    /**** global variables ****/

static GtkWidget *              add_more_dialog = NULL;
static GtkWidget *              node_number_spin = NULL;
static GtkWidget *              rectangular_radio = NULL;
static GtkWidget *              triangular_radio = NULL;
static GtkWidget *              random_radio = NULL;
static GtkWidget *              rectangular_horiz_dist_spin = NULL;
static GtkWidget *              rectangular_vert_dist_spin = NULL;
static GtkWidget *              triangular_horiz_dist_spin = NULL;
static GtkWidget *              triangular_vert_dist_spin = NULL;
static GtkWidget *              rectangular_row_length_spin = NULL;
static GtkWidget *              rectangular_image = NULL;
static GtkWidget *              triangular_image = NULL;
static GtkWidget *              random_image = NULL;


    /**** local function prototypes ****/

void                cb_add_more_dialog_pattern_changed(GtkWidget *widget, gpointer data);


    /**** exported functions ****/

bool dialogs_init()
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/dialogs.glade", rs_app_dir, RES_DIR);

    GError *error = NULL;
    gtk_builder_add_from_file(gtk_builder, path, &error);
    if (error != NULL) {
        rs_error("failed to load dialogs ui: %s", error->message);
        return FALSE;
    }

    /* add_more_dialog widgets */
    add_more_dialog = (GtkWidget *) gtk_builder_get_object(gtk_builder, "add_more_dialog");
    node_number_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "node_number_spin");
    rectangular_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "rectangular_radio");
    triangular_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "triangular_radio");
    random_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "random_radio");
    rectangular_horiz_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "rectangular_horiz_dist_spin");
    rectangular_vert_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "rectangular_vert_dist_spin");
    triangular_horiz_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "triangular_horiz_dist_spin");
    triangular_vert_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "triangular_vert_dist_spin");
    rectangular_row_length_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "rectangular_row_length_spin");
    rectangular_image = (GtkWidget *) gtk_builder_get_object(gtk_builder, "rectangular_image");
    triangular_image = (GtkWidget *) gtk_builder_get_object(gtk_builder, "triangular_image");
    random_image = (GtkWidget *) gtk_builder_get_object(gtk_builder, "random_image");

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(node_number_spin), 20);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rectangular_radio), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(rectangular_horiz_dist_spin), 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(rectangular_vert_dist_spin), 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(triangular_horiz_dist_spin), 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(triangular_vert_dist_spin), 10);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(rectangular_row_length_spin), 6);

    snprintf(path, sizeof(path), "%s/%s/rectangular-pattern.png", rs_app_dir, RES_DIR);
    gtk_image_set_from_file(GTK_IMAGE(rectangular_image), path);
    snprintf(path, sizeof(path), "%s/%s/triangular-pattern.png", rs_app_dir, RES_DIR);
    gtk_image_set_from_file(GTK_IMAGE(triangular_image), path);
    snprintf(path, sizeof(path), "%s/%s/random-pattern.png", rs_app_dir, RES_DIR);
    gtk_image_set_from_file(GTK_IMAGE(random_image), path);

    cb_add_more_dialog_pattern_changed(NULL, NULL);

    return TRUE;
}

bool dialogs_done()
{
    return TRUE;
}

add_more_dialog_info_t *add_more_dialog_run()
{
    uint32 result = gtk_dialog_run(GTK_DIALOG(add_more_dialog));
    gtk_widget_set_visible(GTK_WIDGET(add_more_dialog), FALSE);

    if (!result) {
        return NULL;
    }

    add_more_dialog_info_t *dialog_info = malloc(sizeof(add_more_dialog_info_t));

    dialog_info->node_number = gtk_spin_button_get_value(GTK_SPIN_BUTTON(node_number_spin));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectangular_radio))) {
        dialog_info->pattern = ADD_MORE_DIALOG_PATTERN_RECTANGULAR;
        dialog_info->horiz_dist = gtk_spin_button_get_value(GTK_SPIN_BUTTON(rectangular_horiz_dist_spin));
        dialog_info->vert_dist = gtk_spin_button_get_value(GTK_SPIN_BUTTON(rectangular_vert_dist_spin));
        dialog_info->row_length = gtk_spin_button_get_value(GTK_SPIN_BUTTON(rectangular_row_length_spin));
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(triangular_radio))) {
        dialog_info->pattern = ADD_MORE_DIALOG_PATTERN_TRIANGULAR;
        dialog_info->horiz_dist = gtk_spin_button_get_value(GTK_SPIN_BUTTON(triangular_horiz_dist_spin));
        dialog_info->vert_dist = gtk_spin_button_get_value(GTK_SPIN_BUTTON(triangular_vert_dist_spin));
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(random_radio))) {
        dialog_info->pattern = ADD_MORE_DIALOG_PATTERN_RANDOM;
    }

    return dialog_info;
}


    /**** local functions ****/

void cb_add_more_dialog_pattern_changed(GtkWidget *widget, gpointer data)
{
    bool rectangular_pattern = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectangular_radio));
    bool triangular_pattern = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(triangular_radio));
    //bool random_pattern = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(random_radio));

    gtk_widget_set_sensitive(rectangular_horiz_dist_spin, rectangular_pattern);
    gtk_widget_set_sensitive(rectangular_vert_dist_spin, rectangular_pattern);
    gtk_widget_set_sensitive(rectangular_row_length_spin, rectangular_pattern);

    gtk_widget_set_sensitive(triangular_horiz_dist_spin, triangular_pattern);
    gtk_widget_set_sensitive(triangular_vert_dist_spin, triangular_pattern);
}
