#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources.h" // Assuming this header declares resources_get_resource()

// Function declarations
static void execute_command(const gchar *command, GtkWindow *parent);
static void show_settings_dialog(GtkWindow *parent);
static void show_about_tab(GtkWidget *box);
static void show_settings_tab(GtkWidget *box);
static void button_clicked(GtkWidget *widget, gpointer data); // data is unused now, could be NULL
static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void show_confirmation_dialog(GtkWindow *parent_window, const gchar *label, const gchar *command);
static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text, const gchar *icon_name, const gchar *command, int pos);
static void save_configuration(const gchar *commands[]);
static void load_configuration(gchar *commands[]);
static gchar *get_config_path(void);
static gchar *get_config_dir(void);
static void on_save_button_clicked(GtkButton *button, gpointer user_data);
static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data);

// Gets the configuration directory path (~/.config/ssdd)
static gchar *get_config_dir(void) {
    return g_build_filename(g_get_user_config_dir(), "ssdd", NULL);
}

// Gets the full configuration file path (~/.config/ssdd/config)
// Ensures the directory exists.
static gchar *get_config_path(void) {
    gchar *config_dir = get_config_dir();
    // Create the directory if it doesn't exist
    g_mkdir_with_parents(config_dir, 0755);
    gchar *config_path = g_build_filename(config_dir, "config", NULL);
    g_free(config_dir); // Free the dir path returned by get_config_dir
    return config_path;
}

// Executes a shell command asynchronously
static void execute_command(const gchar *command, GtkWindow *parent) {
    GError *error = NULL;
    // Use g_spawn_command_line_async for non-blocking execution
    gboolean ret = g_spawn_command_line_async(command, &error);
    if (!ret) {
        // Format an error message if spawning failed
        gchar *error_message = g_strdup_printf("Error executing command:\n'%s'\n\nReason: %s", command, error ? error->message : "Unknown error");

        // Show an error dialog
        GtkWidget *dialog = gtk_message_dialog_new(parent,
                                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "%s", error_message);
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_window_present(GTK_WINDOW(dialog));

        g_free(error_message);
        if (error) {
            g_error_free(error);
        }
    }
}

// Shows the settings dialog with multiple tabs
static void show_settings_dialog(GtkWindow *parent) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *notebook;
    GtkWidget *settings_tab;
    GtkWidget *about_tab;

    // Create the dialog window
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Settings");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent); // Set parent window
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);           // Make dialog modal
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE); // Destroy with parent

    // Get the content area of the dialog (GTK4 uses gtk_window_set_child)
    content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), content_area);

    // Create a notebook for tabs
    notebook = gtk_notebook_new();
    gtk_box_append(GTK_BOX(content_area), notebook);

    // Create and add Settings Tab
    settings_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // Box for settings content
    gtk_widget_set_margin_top(settings_tab, 10);
    gtk_widget_set_margin_bottom(settings_tab, 10);
    gtk_widget_set_margin_start(settings_tab, 10);
    gtk_widget_set_margin_end(settings_tab, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_tab, gtk_label_new("Settings"));
    show_settings_tab(settings_tab); // Populate the settings tab

    // Create and add About Tab
    about_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // Box for about content
    gtk_widget_set_margin_top(about_tab, 10);
    gtk_widget_set_margin_bottom(about_tab, 10);
    gtk_widget_set_margin_start(about_tab, 10);
    gtk_widget_set_margin_end(about_tab, 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_tab, gtk_label_new("About"));
    show_about_tab(about_tab); // Populate the about tab

    // Add a standard "Close" button to the dialog's action area
    gtk_dialog_add_button(GTK_DIALOG(dialog), "_Close", GTK_RESPONSE_CLOSE);
    // Connect the response signal to destroy the dialog
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);

    // Show the dialog
    gtk_window_present(GTK_WINDOW(dialog));
}

// Populates the "Settings" tab with command entries
static void show_settings_tab(GtkWidget *box) {
    const gchar *labels[] = {
        "Logout Command:",
        "Reboot Command:",
        "Shutdown Command:",
        "Switch User Command:",
        "Suspend Command:",
        "Hibernate Command:"
    };

    gchar *commands[6] = { NULL }; // Initialize command array to NULL
    load_configuration(commands); // Load commands from config file

    // Create a grid layout for labels and entries
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_append(GTK_BOX(box), grid);

    // Array to hold the GtkEntry widgets
    GtkWidget **entries = g_new(GtkWidget*, 6);

    // Create label and entry for each command
    for (int i = 0; i < 6; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        GtkWidget *entry = gtk_entry_new();
        // Set entry text (handle potential NULL from load_configuration)
        gtk_editable_set_text(GTK_EDITABLE(entry), commands[i] ? commands[i] : "");
        gtk_widget_set_hexpand(entry, TRUE); // Make entry expand horizontally
        gtk_widget_set_halign(label, GTK_ALIGN_END); // Align label to the right

        // Attach label and entry to the grid
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry, 1, i, 1, 1);

        entries[i] = entry; // Store entry widget
        g_free(commands[i]); // Free the command string loaded earlier
    }

    // Create the "Save" button
    GtkWidget *save_button = gtk_button_new_with_label("Save");
    gtk_widget_set_halign(save_button, GTK_ALIGN_END); // Align button to the right
    gtk_widget_set_margin_top(save_button, 10);
    // Store the entries array with the button, ensuring it's freed when button is destroyed
    g_object_set_data_full(G_OBJECT(save_button), "entries", entries, (GDestroyNotify)g_free);
    // Connect the save button's clicked signal
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), NULL);
    gtk_box_append(GTK_BOX(box), save_button); // Add button to the tab's box
}

// Callback function for the "Save" button in settings
static void on_save_button_clicked(GtkButton *button, gpointer user_data) {
    // Retrieve the GtkEntry widgets array stored with the button
    GtkWidget **entries = g_object_get_data(G_OBJECT(button), "entries");
    const gchar *commands_to_save[6]; // Array to hold command strings from entries

    // Get text from each entry
    for (int i = 0; i < 6; i++) {
        // This pointer is owned by the GtkEditable, do not free it here.
        commands_to_save[i] = gtk_editable_get_text(GTK_EDITABLE(entries[i]));
    }

    // Save the retrieved commands to the configuration file
    save_configuration(commands_to_save);

    // Show an information dialog confirming the save
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));
    GtkWidget *info_dialog = gtk_message_dialog_new(parent_window,
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_INFO,
                                           GTK_BUTTONS_OK,
                                           "Settings saved successfully.");
    g_signal_connect(info_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_window_present(GTK_WINDOW(info_dialog));
}

// Populates the "About" tab
static void show_about_tab(GtkWidget *box) {
    GtkWidget *label;
    GtkWidget *image;
    const gchar *about_text =
        "\n<b>Simple ShutDown Dialog</b>\n\n"
        "<b>Version:</b> 2.1\n"
        "<b>Author:</b> kekePower\n"
        "<b>URL: </b><a href=\"https://git.kekepower.com/kekePower/ssdd\">https://git.kekepower.com/kekePower/ssdd</a>\n"
        "<b>Description:</b> A Simple ShutDown Dialog for session management.\n";

    // Load icon from GResource
    image = gtk_image_new_from_resource("/org/gtk/ssdd/ssdd-icon.png");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 128); // Set icon size
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER); // Center align image
    gtk_widget_set_margin_bottom(image, 10);
    gtk_box_append(GTK_BOX(box), image);

    // Create and configure the about text label
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), about_text); // Use markup for formatting
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);   // Allow text selection
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);         // Wrap text if needed
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);     // Center align text
    gtk_widget_set_valign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);
}

// Callback function for main window button clicks
static void button_clicked(GtkWidget *widget, gpointer data) {
    // Retrieve data stored with the button
    const gchar *command = g_object_get_data(G_OBJECT(widget), "command");
    const gchar *label = g_object_get_data(G_OBJECT(widget), "label");
    GtkApplication *app = g_object_get_data(G_OBJECT(widget), "app");
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_root(widget));

    // Safety check for command data
    if (!command) {
        g_warning("Command data not found for button '%s'", label ? label : "(unknown)");
        return;
    }

    // Handle special commands: "exit" and "settings"
    if (g_strcmp0(command, "exit") == 0) {
        g_application_quit(G_APPLICATION(app)); // Quit the application
        return;
    }

    if (g_strcmp0(command, "settings") == 0) {
        show_settings_dialog(parent_window); // Show the settings dialog
    } else {
        // For other commands, show a confirmation dialog
        show_confirmation_dialog(parent_window, label, command);
    }
}

// Callback function for key press events on the main window
static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    // Check if the Escape key was pressed
    if (keyval == GDK_KEY_Escape) {
        GtkApplication *app = GTK_APPLICATION(user_data);
        g_application_quit(G_APPLICATION(app)); // Quit the application
        return TRUE; // Event handled
    }
    return FALSE; // Event not handled
}

// Shows a confirmation dialog before executing a command
static void show_confirmation_dialog(GtkWindow *parent_window, const gchar *label, const gchar *command) {
    GtkWidget *dialog;
    // Create the confirmation message string
    gchar *message = g_strdup_printf("Are you sure you want to %s?", label ? label : "perform this action");

    // Create the message dialog
    dialog = gtk_message_dialog_new(parent_window,
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION, // Question icon
                                    GTK_BUTTONS_YES_NO,   // Yes/No buttons
                                    "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation");

    // Store a copy of the command with the dialog, freed on destroy
    g_object_set_data_full(G_OBJECT(dialog), "command", g_strdup(command), g_free);

    // Connect the response signal
    g_signal_connect(dialog, "response", G_CALLBACK(on_confirmation_response), NULL);

    gtk_window_present(GTK_WINDOW(dialog));
    g_free(message); // Free the message string
}

// Callback function for confirmation dialog responses
static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    // Check if the "Yes" button was clicked
    if (response_id == GTK_RESPONSE_YES) {
        // Retrieve the command stored with the dialog
        const gchar *command = g_object_get_data(G_OBJECT(dialog), "command");
        // Get the parent window
        GtkWindow *parent = GTK_WINDOW(gtk_window_get_transient_for(GTK_WINDOW(dialog)));
        // Execute the command if valid
        if(command && parent) {
             execute_command(command, parent);
        } else if (!command) {
            g_warning("Command data missing in confirmation dialog response.");
        }
    }
    // Destroy the dialog regardless of the response
    gtk_window_destroy(GTK_WINDOW(dialog));
}

// Creates a button with an icon and label, and sets up its signal
static void create_button(GtkWidget *grid, GtkApplication *app, const gchar *label_text,
                          const gchar *icon_name, const gchar *command, int pos) {
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *image;
    GtkWidget *label;

    // Create the button
    button = gtk_button_new();
    gtk_widget_set_hexpand(button, TRUE); // Allow horizontal expansion
    gtk_widget_set_vexpand(button, TRUE); // Allow vertical expansion

    // Create a vertical box for the icon and label inside the button
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 5px spacing
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_button_set_child(GTK_BUTTON(button), box);

    // Create and add the icon
    image = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_LARGE); // Use a standard large icon size
    gtk_box_append(GTK_BOX(box), image);

    // Create and add the label
    label = gtk_label_new(label_text);
    gtk_box_append(GTK_BOX(box), label);

    // Store necessary data with the button widget
    g_object_set_data(G_OBJECT(button), "app", app); // Store app pointer
    // Store copies of label and command, freed automatically when button is destroyed
    g_object_set_data_full(G_OBJECT(button), "label", g_strdup(label_text), g_free);
    g_object_set_data_full(G_OBJECT(button), "command", g_strdup(command), g_free);

    // Connect the button's clicked signal to the callback function
    g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), NULL); // Pass NULL as data

    // Attach the button to the main grid layout
    gtk_grid_attach(GTK_GRID(grid), button, pos % 4, pos / 4, 1, 1); // 4 columns grid
}

// Saves the current command configuration to the file using GKeyFile
static void save_configuration(const gchar *commands[]) {
    GError *error = NULL;
    gchar *config_path = get_config_path(); // Get config file path (ensures dir exists)

    // Use GKeyFile for robust configuration handling
    GKeyFile *key_file = g_key_file_new();

    // Set command strings in the key file under the [Commands] group
    g_key_file_set_string(key_file, "Commands", "LOGOUT_COMMAND", commands[0] ? commands[0] : "");
    g_key_file_set_string(key_file, "Commands", "REBOOT_COMMAND", commands[1] ? commands[1] : "");
    g_key_file_set_string(key_file, "Commands", "SHUTDOWN_COMMAND", commands[2] ? commands[2] : "");
    g_key_file_set_string(key_file, "Commands", "SWITCH_USER_COMMAND", commands[3] ? commands[3] : "");
    g_key_file_set_string(key_file, "Commands", "SUSPEND_COMMAND", commands[4] ? commands[4] : "");
    g_key_file_set_string(key_file, "Commands", "HIBERNATE_COMMAND", commands[5] ? commands[5] : "");

    // Convert key file data to string format
    gchar *config_data = g_key_file_to_data(key_file, NULL, &error);

    if (error) {
        g_warning("Failed to generate configuration data: %s", error->message);
        g_error_free(error);
        g_key_file_free(key_file);
        g_free(config_path);
        return;
    }

    // Save the configuration string data to the file
    if (!g_file_set_contents(config_path, config_data, -1, &error)) {
        g_warning("Failed to save configuration to '%s': %s", config_path, error->message);
        g_error_free(error);
    }

    // Clean up allocated memory
    g_free(config_data);
    g_key_file_free(key_file);
    g_free(config_path);
}

// Loads command configuration from the file using GKeyFile
// Populates the `commands` array (caller must free elements).
// Uses defaults if file/keys are missing or invalid.
static void load_configuration(gchar *commands[]) {
    GError *error = NULL;
    gchar *config_path = get_config_path(); // Get config file path (ensures dir exists)

    // Default commands used if config is missing or invalid
    const gchar *default_commands[] = {
        "openbox --exit",           // Default Logout
        "systemctl reboot",         // Default Reboot
        "systemctl poweroff",       // Default Shutdown
        "dm-tool switch-to-greeter",// Default Switch User
        "systemctl suspend",        // Default Suspend
        "systemctl hibernate"       // Default Hibernate
    };

    GKeyFile *key_file = g_key_file_new();

    // Attempt to load the configuration file
    if (!g_key_file_load_from_file(key_file, config_path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
        // If loading fails (file not found, invalid format, etc.)
        g_warning("Configuration file '%s' not found or invalid (%s). Using defaults.", config_path, error ? error->message : "unknown error");
        if (error) g_error_free(error);
        error = NULL; // Reset error status

        // Save a default configuration file for next time
        save_configuration(default_commands);

        // Load default commands directly into the output array
        for (int i = 0; i < 6; i++) {
            commands[i] = g_strdup(default_commands[i]); // Duplicate default strings
        }
    } else {
        // File loaded successfully, read values
        commands[0] = g_key_file_get_string(key_file, "Commands", "LOGOUT_COMMAND", NULL);
        commands[1] = g_key_file_get_string(key_file, "Commands", "REBOOT_COMMAND", NULL);
        commands[2] = g_key_file_get_string(key_file, "Commands", "SHUTDOWN_COMMAND", NULL);
        commands[3] = g_key_file_get_string(key_file, "Commands", "SWITCH_USER_COMMAND", NULL);
        commands[4] = g_key_file_get_string(key_file, "Commands", "SUSPEND_COMMAND", NULL);
        commands[5] = g_key_file_get_string(key_file, "Commands", "HIBERNATE_COMMAND", NULL);

        // Replace NULL (missing keys) or empty strings with defaults
        for (int i = 0; i < 6; i++) {
            if (commands[i] == NULL || g_strcmp0(commands[i], "") == 0) {
                 g_free(commands[i]); // Free if empty string was returned or NULL
                 commands[i] = g_strdup(default_commands[i]); // Use default
                 g_warning("Configuration key missing or empty for index %d, using default: '%s'", i, default_commands[i]);
            }
        }
    }

    // Clean up
    g_key_file_free(key_file);
    g_free(config_path);
}

// Activation function called when the application starts
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    gchar *commands[6] = { NULL }; // Array to hold loaded commands
    load_configuration(commands); // Load commands from config file

    // Icon names (using symbolic for better theme integration)
    const gchar *icons[] = {
        "system-log-out-symbolic",
        "system-reboot-symbolic",
        "system-shutdown-symbolic",
        "system-users-symbolic",
        "media-playback-pause-symbolic", // Consider alternatives if needed
        "document-save-symbolic",        // Hibernate icon choice
        "preferences-system-symbolic",   // Settings icon
        "window-close-symbolic"          // Exit icon
    };
    // Button labels
    const gchar *labels[] = {
        "Logout", "Reboot", "Shutdown", "Switch User",
        "Suspend", "Hibernate", "Settings", "Exit"
    };
    // Special command identifiers for settings and exit
    const gchar *special_commands[] = {
        "settings", "exit"
    };

    // Create the main application window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple ShutDown Dialog");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE); // Disable resizing

    // --- Obsolete GTK3 calls removed ---
    // gtk_window_set_position(...) removed - Handled by WM
    // gtk_window_set_icon(...) removed - Handled by .desktop file

    // Create the main grid layout
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_widget_set_margin_top(grid, 15);
    gtk_widget_set_margin_bottom(grid, 15);
    gtk_widget_set_margin_start(grid, 15);
    gtk_widget_set_margin_end(grid, 15);
    gtk_window_set_child(GTK_WINDOW(window), grid); // Set grid as child of window

    // Create buttons for the 6 main actions (Logout, Reboot, etc.)
    for (int i = 0; i < 6; i++) {
        // Pass the loaded command; create_button makes a copy. Handle NULL.
        create_button(grid, app, labels[i], icons[i], commands[i] ? commands[i] : "", i);
        g_free(commands[i]); // Free the original string from load_configuration
    }

    // Create Settings and Exit buttons
    create_button(grid, app, labels[6], icons[6], special_commands[0], 6); // "settings"
    create_button(grid, app, labels[7], icons[7], special_commands[1], 7); // "exit"

    // Add key event controller to handle Escape key press
    GtkEventController *key_controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(window, key_controller);
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), app);

    // Show the main window
    gtk_window_present(GTK_WINDOW(window));
}

// Main function
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // Register GResources (ensure resources.c/h are compiled and linked)
    // Verify 'resources_get_resource()' matches the function in generated resources.c
    g_resources_register(resources_get_resource());

    // Create the GTK application instance
    // Use reverse DNS notation for the application ID
    app = gtk_application_new("com.kekepower.ssdd", G_APPLICATION_DEFAULT_FLAGS);
    // Connect the activate signal to the activate function
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    // Run the application
    status = g_application_run(G_APPLICATION(app), argc, argv);
    // Clean up the application object
    g_object_unref(app);

    // Optional: Unregister resources (usually handled automatically on exit)
    // g_resources_unregister(resources_get_resource());

    return status; // Return the application exit status
}
