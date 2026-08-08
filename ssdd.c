#include <errno.h>

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define CONFIG_DIR_NAME "ssdd"
#define CONFIG_FILE_NAME "config"
#define CONFIG_GROUP "Commands"
#define APP_VERSION "2.2"

typedef enum {
  ACTION_LOGOUT,
  ACTION_REBOOT,
  ACTION_SHUTDOWN,
  ACTION_SWITCH_USER,
  ACTION_SUSPEND,
  ACTION_HIBERNATE,
  N_CONFIG_ACTIONS,
  ACTION_SETTINGS = N_CONFIG_ACTIONS,
  ACTION_EXIT,
  N_ACTIONS
} ActionId;

typedef struct {
  const gchar *label;
  const gchar *icon_name;
  const gchar *config_key;
  const gchar *default_command;
} ActionSpec;

typedef struct {
  gchar *commands[N_CONFIG_ACTIONS];
} AppState;

typedef struct {
  AppState *state;
  GtkWidget *entries[N_CONFIG_ACTIONS];
} SettingsData;

static const ActionSpec actions[N_ACTIONS] = {
    [ACTION_LOGOUT] = {"Logout", "system-log-out-symbolic", "LOGOUT_COMMAND",
                       "openbox --exit"},
    [ACTION_REBOOT] = {"Reboot", "system-reboot-symbolic", "REBOOT_COMMAND",
                       "systemctl reboot"},
    [ACTION_SHUTDOWN] = {"Shutdown", "system-shutdown-symbolic",
                         "SHUTDOWN_COMMAND", "systemctl poweroff"},
    [ACTION_SWITCH_USER] = {"Switch User", "system-users-symbolic",
                            "SWITCH_USER_COMMAND", "dm-tool switch-to-greeter"},
    [ACTION_SUSPEND] = {"Suspend", "media-playback-pause-symbolic",
                        "SUSPEND_COMMAND", "systemctl suspend"},
    [ACTION_HIBERNATE] = {"Hibernate", "document-save-symbolic",
                          "HIBERNATE_COMMAND", "systemctl hibernate"},
    [ACTION_SETTINGS] = {"Settings", "preferences-system-symbolic", NULL, NULL},
    [ACTION_EXIT] = {"Exit", "window-close-symbolic", NULL, NULL}};

G_STATIC_ASSERT(G_N_ELEMENTS(actions) == N_ACTIONS);

static void activate(GtkApplication *app, gpointer user_data);
static void app_state_free(gpointer data);
static void button_clicked(GtkButton *button, gpointer user_data);
static void create_button(GtkWidget *grid, ActionId action_id);
static void execute_command(const gchar *command, GtkWindow *parent);
static gchar *get_config_path(GError **error);
static void load_configuration(AppState *state);
static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval,
                               guint keycode, GdkModifierType state,
                               gpointer user_data);
static void on_confirmation_response(GtkDialog *dialog, gint response_id,
                                     gpointer user_data);
static void on_save_button_clicked(GtkButton *button, gpointer user_data);
static gboolean save_configuration(const gchar *const commands[],
                                   GError **error);
static void show_about_tab(GtkWidget *box);
static void show_confirmation_dialog(GtkWindow *parent, const gchar *label,
                                     const gchar *command);
static void show_message_dialog(GtkWindow *parent, GtkMessageType type,
                                GtkButtonsType buttons, const gchar *title,
                                const gchar *message);
static void show_settings_dialog(GtkWindow *parent, AppState *state);
static void show_settings_tab(GtkWidget *box, AppState *state);

static void app_state_free(gpointer data) {
  AppState *state = data;

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++)
    g_free(state->commands[i]);

  g_free(state);
}

static void app_state_set_command(AppState *state, guint index,
                                  const gchar *command) {
  g_return_if_fail(index < N_CONFIG_ACTIONS);

  g_free(state->commands[index]);
  state->commands[index] = g_strdup(command);
}

static gchar *get_config_path(GError **error) {
  g_autofree gchar *config_dir =
      g_build_filename(g_get_user_config_dir(), CONFIG_DIR_NAME, NULL);

  if (g_mkdir_with_parents(config_dir, 0755) == -1) {
    gint saved_errno = errno;

    g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(saved_errno),
                "Could not create configuration directory '%s': %s", config_dir,
                g_strerror(saved_errno));
    return NULL;
  }

  return g_build_filename(config_dir, CONFIG_FILE_NAME, NULL);
}

static gboolean save_configuration(const gchar *const commands[],
                                   GError **error) {
  g_autofree gchar *config_path = get_config_path(error);
  g_autoptr(GKeyFile) key_file = NULL;

  if (config_path == NULL)
    return FALSE;

  key_file = g_key_file_new();

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++) {
    g_key_file_set_string(key_file, CONFIG_GROUP, actions[i].config_key,
                          commands[i] != NULL ? commands[i] : "");
  }

  return g_key_file_save_to_file(key_file, config_path, error);
}

static void load_configuration(AppState *state) {
  g_autoptr(GError) error = NULL;
  g_autofree gchar *config_path = get_config_path(&error);
  g_autoptr(GKeyFile) key_file = g_key_file_new();
  gboolean needs_save = FALSE;

  if (config_path == NULL ||
      !g_key_file_load_from_file(
          key_file, config_path,
          G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
    g_warning("Could not load configuration%s%s. Using defaults.",
              error != NULL ? ": " : "", error != NULL ? error->message : "");

    for (guint i = 0; i < N_CONFIG_ACTIONS; i++)
      app_state_set_command(state, i, actions[i].default_command);

    if (config_path != NULL) {
      const gchar *commands[N_CONFIG_ACTIONS];
      g_autoptr(GError) save_error = NULL;

      for (guint i = 0; i < N_CONFIG_ACTIONS; i++)
        commands[i] = state->commands[i];

      if (!save_configuration(commands, &save_error))
        g_warning("Could not save default configuration: %s",
                  save_error->message);
    }
    return;
  }

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++) {
    g_autofree gchar *command = g_key_file_get_string(
        key_file, CONFIG_GROUP, actions[i].config_key, NULL);

    if (command == NULL || *command == '\0') {
      g_warning(
          "Configuration key '%s' is missing or empty; using default '%s'",
          actions[i].config_key, actions[i].default_command);
      app_state_set_command(state, i, actions[i].default_command);
      needs_save = TRUE;
    } else {
      app_state_set_command(state, i, command);
    }
  }

  if (needs_save) {
    const gchar *commands[N_CONFIG_ACTIONS];
    g_autoptr(GError) save_error = NULL;

    for (guint i = 0; i < N_CONFIG_ACTIONS; i++)
      commands[i] = state->commands[i];

    if (!save_configuration(commands, &save_error))
      g_warning("Could not repair configuration: %s", save_error->message);
  }
}

static void show_message_dialog(GtkWindow *parent, GtkMessageType type,
                                GtkButtonsType buttons, const gchar *title,
                                const gchar *message) {
  GtkWidget *dialog = gtk_message_dialog_new(
      parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type, buttons,
      "%s", message);

  gtk_window_set_title(GTK_WINDOW(dialog), title);
  g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void execute_command(const gchar *command, GtkWindow *parent) {
  g_autoptr(GError) error = NULL;

  if (!g_spawn_command_line_async(command, &error)) {
    g_autofree gchar *message = g_strdup_printf(
        "Could not execute:\n'%s'\n\nReason: %s", command, error->message);

    show_message_dialog(parent, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                        "Error Executing Command", message);
  }
}

static void show_about_tab(GtkWidget *box) {
  static const gchar about_text[] =
      "\n<b>Simple ShutDown Dialog</b>\n\n"
      "<b>Version:</b> " APP_VERSION "\n"
      "<b>Author:</b> kekePower\n"
      "<b>URL: </b><a href=\"https://git.kekepower.com/kekePower/ssdd\">"
      "https://git.kekepower.com/kekePower/ssdd</a>\n"
      "<b>Description:</b> A Simple ShutDown Dialog for session management.\n";
  GtkWidget *image = gtk_image_new_from_resource("/org/gtk/ssdd/ssdd-icon.png");
  GtkWidget *label = gtk_label_new(NULL);

  gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
  gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_bottom(image, 10);
  gtk_box_append(GTK_BOX(box), image);

  gtk_label_set_markup(GTK_LABEL(label), about_text);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), label);
}

static void show_settings_tab(GtkWidget *box, AppState *state) {
  GtkWidget *grid = gtk_grid_new();
  GtkWidget *save_button = gtk_button_new_with_label("Save");
  SettingsData *settings = g_new0(SettingsData, 1);

  settings->state = state;

  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(box), grid);

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++) {
    g_autofree gchar *label_text =
        g_strdup_printf("%s Command:", actions[i].label);
    GtkWidget *label = gtk_label_new(label_text);
    GtkWidget *entry = gtk_entry_new();

    gtk_editable_set_text(GTK_EDITABLE(entry), state->commands[i]);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, (gint)i, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, (gint)i, 1, 1);
    settings->entries[i] = entry;
  }

  gtk_widget_set_halign(save_button, GTK_ALIGN_END);
  gtk_widget_set_margin_top(save_button, 10);
  g_object_set_data_full(G_OBJECT(save_button), "settings", settings, g_free);
  g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked),
                   NULL);
  gtk_box_append(GTK_BOX(box), save_button);
}

static void show_settings_dialog(GtkWindow *parent, AppState *state) {
  GtkWidget *dialog = gtk_dialog_new();
  GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *notebook = gtk_notebook_new();
  GtkWidget *settings_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *about_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  gtk_window_set_title(GTK_WINDOW(dialog), "Settings");
  gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_child(GTK_WINDOW(dialog), content);
  gtk_box_append(GTK_BOX(content), notebook);

  gtk_widget_set_margin_top(settings_tab, 10);
  gtk_widget_set_margin_bottom(settings_tab, 10);
  gtk_widget_set_margin_start(settings_tab, 10);
  gtk_widget_set_margin_end(settings_tab, 10);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), settings_tab,
                           gtk_label_new("Settings"));
  show_settings_tab(settings_tab, state);

  gtk_widget_set_margin_top(about_tab, 10);
  gtk_widget_set_margin_bottom(about_tab, 10);
  gtk_widget_set_margin_start(about_tab, 10);
  gtk_widget_set_margin_end(about_tab, 10);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_tab,
                           gtk_label_new("About"));
  show_about_tab(about_tab);

  gtk_dialog_add_button(GTK_DIALOG(dialog), "_Close", GTK_RESPONSE_CLOSE);
  g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_save_button_clicked(GtkButton *button,
                                   gpointer user_data G_GNUC_UNUSED) {
  SettingsData *settings = g_object_get_data(G_OBJECT(button), "settings");
  const gchar *commands[N_CONFIG_ACTIONS];
  g_autoptr(GError) error = NULL;
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++) {
    GtkEditable *entry = GTK_EDITABLE(settings->entries[i]);
    const gchar *command = gtk_editable_get_text(entry);

    if (*command == '\0') {
      gtk_editable_set_text(entry, actions[i].default_command);
      command = gtk_editable_get_text(entry);
    }

    commands[i] = command;
  }

  if (!save_configuration(commands, &error)) {
    g_autofree gchar *message = g_strdup_printf(
        "Could not save the settings.\n\nReason: %s", error->message);
    show_message_dialog(parent, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                        "Error Saving Settings", message);
    return;
  }

  for (guint i = 0; i < N_CONFIG_ACTIONS; i++)
    app_state_set_command(settings->state, i, commands[i]);

  show_message_dialog(parent, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                      "Settings Saved", "Settings saved successfully.");
}

static void show_confirmation_dialog(GtkWindow *parent, const gchar *label,
                                     const gchar *command) {
  g_autofree gchar *message =
      g_strdup_printf("Are you sure you want to %s?", label);
  GtkWidget *dialog = gtk_message_dialog_new(
      parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", message);

  gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation");
  g_object_set_data_full(G_OBJECT(dialog), "command", g_strdup(command),
                         g_free);
  g_signal_connect(dialog, "response", G_CALLBACK(on_confirmation_response),
                   NULL);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_confirmation_response(GtkDialog *dialog, gint response_id,
                                     gpointer user_data G_GNUC_UNUSED) {
  if (response_id == GTK_RESPONSE_YES) {
    const gchar *command = g_object_get_data(G_OBJECT(dialog), "command");
    GtkWindow *parent = gtk_window_get_transient_for(GTK_WINDOW(dialog));

    if (command != NULL && parent != NULL)
      execute_command(command, parent);
    else
      g_warning("Confirmation dialog is missing its command or parent");
  }

  gtk_window_destroy(GTK_WINDOW(dialog));
}

static void button_clicked(GtkButton *button,
                           gpointer user_data G_GNUC_UNUSED) {
  ActionId action_id = (ActionId)(GPOINTER_TO_UINT(g_object_get_data(
                                      G_OBJECT(button), "action-id")) -
                                  1);
  GtkWindow *window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));
  GtkApplication *app = gtk_window_get_application(window);
  AppState *state = g_object_get_data(G_OBJECT(app), "app-state");

  g_return_if_fail(action_id >= 0 && action_id < N_ACTIONS);

  if (action_id == ACTION_EXIT) {
    g_application_quit(G_APPLICATION(app));
  } else if (action_id == ACTION_SETTINGS) {
    show_settings_dialog(window, state);
  } else {
    show_confirmation_dialog(window, actions[action_id].label,
                             state->commands[action_id]);
  }
}

static gboolean on_key_pressed(GtkEventControllerKey *controller G_GNUC_UNUSED,
                               guint keyval, guint keycode G_GNUC_UNUSED,
                               GdkModifierType state G_GNUC_UNUSED,
                               gpointer user_data) {
  if (keyval != GDK_KEY_Escape)
    return FALSE;

  g_application_quit(G_APPLICATION(user_data));
  return TRUE;
}

static void create_button(GtkWidget *grid, ActionId action_id) {
  const ActionSpec *action = &actions[action_id];
  GtkWidget *button = gtk_button_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *image = gtk_image_new_from_icon_name(action->icon_name);
  GtkWidget *label = gtk_label_new(action->label);

  gtk_widget_set_hexpand(button, TRUE);
  gtk_widget_set_vexpand(button, TRUE);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_button_set_child(GTK_BUTTON(button), box);
  gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_LARGE);
  gtk_box_append(GTK_BOX(box), image);
  gtk_box_append(GTK_BOX(box), label);

  g_object_set_data(G_OBJECT(button), "action-id",
                    GUINT_TO_POINTER((guint)action_id + 1));
  g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), NULL);
  gtk_grid_attach(GTK_GRID(grid), button, (gint)action_id % 4,
                  (gint)action_id / 4, 1, 1);
}

static void activate(GtkApplication *app, gpointer user_data G_GNUC_UNUSED) {
  GtkWindow *existing_window = gtk_application_get_active_window(app);

  if (existing_window != NULL) {
    gtk_window_present(existing_window);
    return;
  }

  GtkWidget *window = gtk_application_window_new(app);
  GtkWidget *grid = gtk_grid_new();

  gtk_window_set_title(GTK_WINDOW(window), "Simple ShutDown Dialog");
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
  gtk_widget_set_margin_top(grid, 15);
  gtk_widget_set_margin_bottom(grid, 15);
  gtk_widget_set_margin_start(grid, 15);
  gtk_widget_set_margin_end(grid, 15);
  gtk_window_set_child(GTK_WINDOW(window), grid);

  for (guint i = 0; i < N_ACTIONS; i++)
    create_button(grid, (ActionId)i);

  GtkEventController *key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(window, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed),
                   app);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  g_autoptr(GtkApplication) app =
      gtk_application_new("com.kekepower.ssdd", G_APPLICATION_DEFAULT_FLAGS);
  AppState *state = g_new0(AppState, 1);

  load_configuration(state);
  g_object_set_data_full(G_OBJECT(app), "app-state", state, app_state_free);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  return g_application_run(G_APPLICATION(app), argc, argv);
}
