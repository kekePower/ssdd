#include <gtk/gtk.h>
#include <glib.h>
#include "resources.h"

// Function declarations (prototypes)
static void execute_command(const gchar *command);
static void show_about_dialog(GtkWidget *widget);
static void button_clicked(GtkWidget *widget, gpointer data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void show_confirmation_dialog(GtkWidget *widget, const gchar *label, const gchar *command);
static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text, const gchar *icon_name, const gchar *command, int pos);

static void execute_command(const gchar *command) {
    GError *error = NULL;
    gboolean ret = g_spawn_command_line_async(command, &error);
    if (!ret) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "Error executing command: %s\n%s",
                                        command,
                                        error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
    }
}

static void show_about_dialog(GtkWidget *widget) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *image;
    GtkWidget *box;
    const gchar *about_text =
        "\n<b>About Simple ShutDown Dialog</b>\n\n"
        "<b>Version:</b> 1.4\n"
        "<b>Author:</b> kekePower\n"
        "<b>URL:</b> <a href=\"https://git.kekepower.com/kekePower/ssdd\">https://git.kekepower.com/kekePower/ssdd</a>\n"
        "<b>Description:</b> This is a simple Shutdown Dialog for Openbox.\n";

    dialog = gtk_dialog_new_with_buttons("About Simple ShutDown Dialog",
                                        NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Close",
                                        GTK_RESPONSE_CLOSE,
                                        NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(content_area), box);

    image = gtk_image_new_from_resource("/org/gtk/ssdd/ssdd-icon.png");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 250); // Assuming original size is 500x500
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), about_text);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void button_clicked(GtkWidget *widget, gpointer data) {
    const gchar *command = (const gchar *)data;
    const gchar *label = g_object_get_data(G_OBJECT(widget), "label");

    if (g_strcmp0(command, "exit") == 0) {
        g_application_quit(G_APPLICATION(g_object_get_data(G_OBJECT(widget), "app")));
        return;
    }

    if (g_strcmp0(command, "about") == 0) {
        show_about_dialog(widget);
    } else {
        GtkWidget *window = gtk_widget_get_toplevel(widget);
        show_confirmation_dialog(window, label, command);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape) {
        g_application_quit(G_APPLICATION(data));
        return TRUE; // Event handled
    }
    return FALSE; // Event not handled
}

static void show_confirmation_dialog(GtkWidget *parent_window, const gchar *label, const gchar *command) {
    GtkWidget *dialog;
    gint response;

    gchar *message = g_strdup_printf("Are you sure you want to %s?", label);

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation");

    // Set the transient parent correctly
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent_window));

    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(content_area), box);

    GtkWidget *image = gtk_image_new_from_icon_name("dialog-warning", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

    GtkWidget *label_widget = gtk_label_new(message);
    gtk_box_pack_start(GTK_BOX(box), label_widget, TRUE, TRUE, 0);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           "Yes", GTK_RESPONSE_YES,
                           "No", GTK_RESPONSE_NO,
                           NULL); // Add NULL to terminate the list of buttons

    GtkWidget *button_box = gtk_dialog_get_content_area(GTK_DIALOG(dialog)); // Get content area instead of action area
    gtk_box_set_spacing(GTK_BOX(button_box), 10); // Adjust spacing as needed
    gtk_widget_set_margin_top(button_box, 10);

    gtk_widget_show_all(dialog); // Show the dialog and its children

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(message);

    if (response == GTK_RESPONSE_YES) {
        execute_command(command);
    }
}

static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text, const gchar *icon_name, const gchar *command, int pos) {
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *image;
    GtkWidget *label;

    button = gtk_button_new();
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // Set internal box spacing to 10
    gtk_container_add(GTK_CONTAINER(button), box);

    image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);

    label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    // Set margins for the button contents
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    gtk_widget_set_margin_start(box, 5);
    gtk_widget_set_margin_end(box, 5);

    g_object_set_data(G_OBJECT(button), "app", app);
    g_object_set_data(G_OBJECT(button), "label", (gpointer) label_text);
    g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), (gpointer) command);
    gtk_grid_attach(GTK_GRID(grid), button, pos % 4, pos / 4, 1, 1);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    const gchar *buttons[] = {
        "openbox --exit",
        "systemctl reboot",
        "systemctl poweroff",
        "dm-tool switch-to-greeter",
        "systemctl suspend",
        "systemctl hibernate",
        "about",
        "exit"
    };
    const gchar *icons[] = {
        "system-log-out",
        "view-refresh",
        "system-shutdown",
        "system-users",
        "media-playback-pause",
        "media-playback-stop",
        "help-about",
        "application-exit"
    };
    const gchar *labels[] = {     // Define the labels array here
        "Logout",
        "Reboot",
        "Shutdown",
        "Switch User",
        "Suspend",
        "Hibernate",
        "About",
        "Exit"
    };

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple ShutDown Dialog");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Load the icon with error handling
    GError *error = NULL;
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_resource("/org/gtk/ssdd/ssdd-icon.png", &error);
    if (icon_pixbuf) {
        gtk_window_set_icon(GTK_WINDOW(window), icon_pixbuf);
        g_object_unref(icon_pixbuf); // Free the pixbuf
    } else {
        g_warning("Failed to load icon: %s", error->message);
        g_error_free(error);
    }

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), app);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 0); // Set the row spacing to 0
    gtk_grid_set_column_spacing(GTK_GRID(grid), 0); // Set the column spacing to 0
    gtk_container_add(GTK_CONTAINER(window), grid);

    for (int i = 0; i < 8; i++) {
        create_button(grid, app, labels[i], icons[i], buttons[i], i);
    }

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Register the resource
    g_resources_register(resources_get_resource());

    app = gtk_application_new("org.gtk.ssdd", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
