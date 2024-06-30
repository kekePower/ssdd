#include <gtk/gtk.h>
#include <stdlib.h>
#include "resources.h"  // Include the generated resource header

static void execute_command(const gchar *command) {
    int ret = system(command);
    if (ret != 0) {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "Error executing command: %s",
                                        command);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

static void show_confirmation_dialog(GtkWidget *widget, gpointer data) {
    const gchar *command = (const gchar *) data;
    GtkWidget *dialog;
    gint response;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "Are you sure you want to execute: %s?",
                                    command);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Yes", GTK_RESPONSE_YES);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "No", GTK_RESPONSE_NO);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
        execute_command(command);
    }
}

static void show_about_dialog(GtkWidget *widget) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *image;
    GtkWidget *box;
    const gchar *about_text = 
        "\nAbout Stig's ShutDown Dialog\n\n"
        "<b>Version:</b> 1.0\n"
        "<b>Author:</b> kekePower\n"
        "<b>URL:</b> <a href=\"https://git.kekepower.com/kekePower/ssdd\">https://git.kekepower.com/kekePower/ssdd</a>\n"
        "<b>Description:</b> This is a simple Shutdown Dialog for Openbox.\n";

    dialog = gtk_dialog_new_with_buttons("About Stig's ShutDown Dialog",
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Close",
                                         GTK_RESPONSE_CLOSE,
                                         NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content_area), box);

    image = gtk_image_new_from_resource("/org/gtk/example/ssdd-icon.png");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 250);  // Assuming original size is 500x500
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
    const gchar *command = (const gchar *) data;
    if (g_strcmp0(command, "exit") == 0) {
        g_application_quit(G_APPLICATION(g_object_get_data(G_OBJECT(widget), "app")));
        return;
    }

    if (g_strcmp0(command, "openbox --exit") == 0 ||
        g_strcmp0(command, "systemctl reboot") == 0 ||
        g_strcmp0(command, "systemctl poweroff") == 0) {
        show_confirmation_dialog(widget, (gpointer) command);
    } else if (g_strcmp0(command, "about") == 0) {
        show_about_dialog(widget);
    } else {
        execute_command(command);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape) {
        g_application_quit(G_APPLICATION(data));
        return TRUE;  // Event handled
    }
    return FALSE;  // Event not handled
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *image;
    GtkWidget *box;
    GtkWidget *label;
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
    const gchar *labels[] = {
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
    gtk_window_set_title(GTK_WINDOW(window), "Exit Openbox");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Load the icon from the resource and set it as the window icon
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_resource("/org/gtk/example/ssdd-icon.png", NULL);
    gtk_window_set_icon(GTK_WINDOW(window), icon_pixbuf);

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), app);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    for (int i = 0; i < 8; i++) {
        button = gtk_button_new();
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(button), box);

        image = gtk_image_new_from_icon_name(icons[i], GTK_ICON_SIZE_BUTTON);
        gtk_box_pack_start(GTK_BOX(box), image, TRUE, TRUE, 0);

        label = gtk_label_new(labels[i]);
        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

        g_object_set_data(G_OBJECT(button), "app", app);
        g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), (gpointer) buttons[i]);
        gtk_grid_attach(GTK_GRID(grid), button, i % 4, i / 4, 1, 1);
    }

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Register the resource
    g_resources_register(resources_get_resource());

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
