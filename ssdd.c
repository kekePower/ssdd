#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources.h"

// Function declarations
static void execute_command(const gchar *command, GtkWindow *parent);
static void show_settings_dialog(GtkWindow *parent);
static void show_about_tab(GtkWidget *box);
static void show_settings_tab(GtkWidget *box);
static void button_clicked(GtkWidget *widget, gpointer data);
static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void show_confirmation_dialog(GtkWindow *parent_window, const gchar *label, const gchar *command);
static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text, const gchar *icon_name, const gchar *command, int pos);
static void save_configuration(const gchar *commands[]);
static void load_configuration(gchar *commands[]);
static gchar *get_config_path(void);
static gchar *get_config_dir(void);
static void on_save_button_clicked(GtkButton *button, gpointer user_data);
static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data);

static gchar *get_config_dir(void) {
    return g_build_filename(g_get_user_config_dir(), "ssdd", NULL);
}

static gchar *get_config_path(void) {
    return g_build_filename(g_get_user_config_dir(), "ssdd", "config", NULL);
}

static void execute_command(const gchar *command, GtkWindow *parent) {
    GError *error = NULL;
    gboolean ret = g_spawn_command_line_async(command, &error);
    if (!ret) {
        gchar *error_message = g_strdup_printf("Error executing command '%s': %s", command, error->message);

        GtkWidget *dialog = gtk_message_dialog_new(parent,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "%s", error_message);
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_window_present(GTK_WINDOW(dialog));

        g_free(error_message);
        g_error_free(error);
    }
}

static void show_settings_dialog(GtkWindow *parent) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *notebook;
    GtkWidget *settings_tab;
    GtkWidget *about_tab;

    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Settings");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

    content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), content_area);

    notebook = gtk_notebook_new();
    gtk_box_append(GTK_BOX(content_area), notebook);

    settings_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(settings_tab, 10);
    gtk_widget_set_margin_bottom(settings_tab, 10);
    gtk_widget_set_margin_start(settings_tab, 10);
    gtk_widget_set_margin_end(settings_tab, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_tab, gtk_label_new("Settings"));

    about_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(about_tab, 10);
    gtk_widget_set_margin_bottom(about_tab, 10);
    gtk_widget_set_margin_start(about_tab, 10);
    gtk_widget_set_margin_end(about_tab, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_tab, gtk_label_new("About"));

    show_settings_tab(settings_tab);
    show_about_tab(about_tab);

    GtkWidget *close_button = gtk_button_new_with_label("Close");
    gtk_box_append(GTK_BOX(content_area), close_button);
    g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void show_settings_tab(GtkWidget *box) {
    const gchar *labels[] = {
        "Logout Command",
        "Reboot Command",
        "Shutdown Command",
        "Switch User Command",
        "Suspend Command",
        "Hibernate Command"
    };

    gchar *commands[6];
    load_configuration(commands);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_append(GTK_BOX(box), grid);

    GtkWidget **entries = g_new(GtkWidget*, 6);

    for (int i = 0; i < 6; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        GtkWidget *entry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(entry), commands[i]);
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_widget_set_halign(label, GTK_ALIGN_END);

        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry, 1, i, 1, 1);

        entries[i] = entry;
    }

    GtkWidget *save_button = gtk_button_new_with_label("Save");
    g_object_set_data_full(G_OBJECT(save_button), "entries", entries, (GDestroyNotify)g_free);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    gtk_box_append(GTK_BOX(box), save_button);

    for (int i = 0; i < 6; i++) {
        g_free(commands[i]);
    }
}

static void on_save_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget **entries = g_object_get_data(G_OBJECT(button), "entries");
    const gchar *commands[6];

    for (int i = 0; i < 6; i++) {
        commands[i] = gtk_editable_get_text(GTK_EDITABLE(entries[i]));
    }

    save_configuration(commands);
}

static void show_about_tab(GtkWidget *box) {
    GtkWidget *label;
    GtkWidget *image;
    const gchar *about_text =
        "\n<b>About Simple ShutDown Dialog</b>\n\n"
        "<b>Version:</b> 2.0\n"
        "<b>Author:</b> kekePower\n"
        "<b>URL: </b><a href=\"https://git.kekepower.com/kekePower/ssdd\">https://git.kekepower.com/kekePower/ssdd</a>\n"
        "<b>Description:</b> A Simple ShutDown Dialog for Openbox.\n";

    image = gtk_image_new_from_resource("/org/gtk/ssdd/ssdd-icon.png");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 250);
    gtk_box_append(GTK_BOX(box), image);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), about_text);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);
}

static void button_clicked(GtkWidget *widget, gpointer data) {
    const gchar *command = (const gchar *)data;
    const gchar *label = g_object_get_data(G_OBJECT(widget), "label");
    GtkApplication *app = g_object_get_data(G_OBJECT(widget), "app");
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(widget));

    if (g_strcmp0(command, "exit") == 0) {
        g_application_quit(G_APPLICATION(app));
        return;
    }

    if (g_strcmp0(command, "settings") == 0) {
        show_settings_dialog(parent_window);
    } else {
        show_confirmation_dialog(parent_window, label, command);
    }
}

static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    if (keyval == GDK_KEY_Escape) {
        GtkApplication *app = GTK_APPLICATION(user_data);
        g_application_quit(G_APPLICATION(app));
        return TRUE;
    }
    return FALSE;
}

static void show_confirmation_dialog(GtkWindow *parent_window, const gchar *label, const gchar *command) {
    GtkWidget *dialog;
    gchar *message = g_strdup_printf("Are you sure you want to %s?", label);

    dialog = gtk_message_dialog_new(parent_window,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "%s", message);

    // Pass the command via g_object_set_data
    g_object_set_data_full(G_OBJECT(dialog), "command", g_strdup(command), g_free);

    g_signal_connect(dialog, "response", G_CALLBACK(on_confirmation_response), NULL);

    gtk_window_present(GTK_WINDOW(dialog));
    g_free(message);
}

static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_YES) {
        const gchar *command = g_object_get_data(G_OBJECT(dialog), "command");
        GtkWindow *parent = GTK_WINDOW(gtk_window_get_transient_for(GTK_WINDOW(dialog)));
        execute_command(command, parent);
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text,
                          const gchar *icon_name, const gchar *command, int pos) {
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *image;
    GtkWidget *label;

    button = gtk_button_new();
    gtk_widget_set_hexpand(button, TRUE); // Allow button to expand horizontally
    gtk_widget_set_vexpand(button, TRUE); // Allow button to expand vertically

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Set spacing to 0 for tighter layout
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);   // Center the content vertically
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);   // Center the content horizontally
    gtk_button_set_child(GTK_BUTTON(button), box);

    image = gtk_image_new_from_icon_name(icon_name);
    gtk_box_append(GTK_BOX(box), image);

    label = gtk_label_new(label_text);
    gtk_box_append(GTK_BOX(box), label);

    // Reduce margins around the button content
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    gtk_widget_set_margin_start(box, 5);
    gtk_widget_set_margin_end(box, 5);

    g_object_set_data(G_OBJECT(button), "app", app);
    g_object_set_data(G_OBJECT(button), "label", (gpointer)label_text);
    g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), (gpointer)command);

    gtk_grid_attach(GTK_GRID(grid), button, pos % 4, pos / 4, 1, 1);
}

static void save_configuration(const gchar *commands[]) {
    GError *error = NULL;
    gchar *config_dir = get_config_dir();
    gchar *config_path = get_config_path();

    g_mkdir_with_parents(config_dir, 0755);

    GString *config_data = g_string_new(NULL);
    g_string_append_printf(config_data, "LOGOUT_COMMAND=%s\n", commands[0]);
    g_string_append_printf(config_data, "REBOOT_COMMAND=%s\n", commands[1]);
    g_string_append_printf(config_data, "SHUTDOWN_COMMAND=%s\n", commands[2]);
    g_string_append_printf(config_data, "SWITCH_USER_COMMAND=%s\n", commands[3]);
    g_string_append_printf(config_data, "SUSPEND_COMMAND=%s\n", commands[4]);
    g_string_append_printf(config_data, "HIBERNATE_COMMAND=%s\n", commands[5]);

    g_file_set_contents(config_path, config_data->str, -1, &error);

    if (error) {
        g_warning("Failed to save configuration: %s", error->message);
        g_error_free(error);
    }

    g_string_free(config_data, TRUE);
    g_free(config_dir);
    g_free(config_path);
}

static void load_configuration(gchar *commands[]) {
    GError *error = NULL;
    gchar *config_data = NULL;
    gchar *config_dir = get_config_dir();
    gchar *config_path = get_config_path();

    g_mkdir_with_parents(config_dir, 0755);

    if (!g_file_test(config_path, G_FILE_TEST_EXISTS)) {
        g_warning("Configuration file not found. Generating a default configuration.");
        const gchar *default_commands[] = {
            "openbox --exit",
            "systemctl reboot",
            "systemctl poweroff",
            "dm-tool switch-to-greeter",
            "systemctl suspend",
            "systemctl hibernate"
        };
        save_configuration(default_commands);
    }

    if (!g_file_get_contents(config_path, &config_data, NULL, &error)) {
        g_warning("Failed to load configuration: %s", error->message);
        g_error_free(error);
        g_free(config_dir);
        g_free(config_path);
        return;
    }

    gchar **lines = g_strsplit(config_data, "\n", -1);
    for (int i = 0; i < 6; i++) {
        gchar **key_value = g_strsplit(lines[i], "=", 2);

        if (!key_value[0] || !key_value[1]) {
            g_warning("Invalid entry in configuration file at line %d. Using default command.", i + 1);
            const gchar *default_commands[] = {
                "openbox --exit",
                "systemctl reboot",
                "systemctl poweroff",
                "dm-tool switch-to-greeter",
                "systemctl suspend",
                "systemctl hibernate"
            };
            commands[i] = g_strdup(default_commands[i]);
            g_strfreev(key_value);
            continue;
        }

        if (g_strcmp0(key_value[0], "LOGOUT_COMMAND") == 0) {
            commands[0] = g_strdup(key_value[1]);
        } else if (g_strcmp0(key_value[0], "REBOOT_COMMAND") == 0) {
            commands[1] = g_strdup(key_value[1]);
        } else if (g_strcmp0(key_value[0], "SHUTDOWN_COMMAND") == 0) {
            commands[2] = g_strdup(key_value[1]);
        } else if (g_strcmp0(key_value[0], "SWITCH_USER_COMMAND") == 0) {
            commands[3] = g_strdup(key_value[1]);
        } else if (g_strcmp0(key_value[0], "SUSPEND_COMMAND") == 0) {
            commands[4] = g_strdup(key_value[1]);
        } else if (g_strcmp0(key_value[0], "HIBERNATE_COMMAND") == 0) {
            commands[5] = g_strdup(key_value[1]);
        } else {
            g_warning("Unknown key in configuration: %s", key_value[0]);
        }

        g_strfreev(key_value);
    }

    for (int i = 0; i < 6; i++) {
        if (commands[i] == NULL || g_strcmp0(commands[i], "") == 0) {
            g_warning("Command at index %d is invalid. Assigning default value.", i);
            const gchar *default_commands[] = {
                "openbox --exit",
                "systemctl reboot",
                "systemctl poweroff",
                "dm-tool switch-to-greeter",
                "systemctl suspend",
                "systemctl hibernate"
            };
            commands[i] = g_strdup(default_commands[i]);
        }
    }

    g_strfreev(lines);
    g_free(config_data);
    g_free(config_dir);
    g_free(config_path);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    gchar *commands[6];
    load_configuration(commands);

    const gchar *icons[] = {
        "system-log-out",
        "view-refresh",
        "system-shutdown",
        "system-users",
        "media-playback-pause",
        "media-playback-stop",
        "preferences-system",
        "application-exit"
    };
    const gchar *labels[] = {
        "Logout",
        "Reboot",
        "Shutdown",
        "Switch User",
        "Suspend",
        "Hibernate",
        "Settings",
        "Exit"
    };

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple ShutDown Dialog");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 2);    // Reduce vertical spacing between grid rows
    gtk_grid_set_column_spacing(GTK_GRID(grid), 2); // Reduce horizontal spacing between grid columns
    gtk_widget_set_margin_top(grid, 10);
    gtk_widget_set_margin_bottom(grid, 10);
    gtk_widget_set_margin_start(grid, 10);
    gtk_widget_set_margin_end(grid, 10);
    gtk_window_set_child(GTK_WINDOW(window), grid);

    for (int i = 0; i < 6; i++) {
        create_button(grid, app, labels[i], icons[i], commands[i], i);
        g_free(commands[i]);
    }
    create_button(grid, app, labels[6], icons[6], "settings", 6);
    create_button(grid, app, labels[7], icons[7], "exit", 7);

    // Key event handling
    GtkEventController *key_controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(window, key_controller);
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), app);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    g_resources_register(resources_get_resource());

    app = gtk_application_new("org.gtk.ssdd", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
