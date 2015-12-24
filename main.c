#include <pthread.h>
#include <gtk/gtk.h>

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>

#include <pocketsphinx.h>

#include <stdio.h>
#include <string.h>

#include "continuous.h"

#define WINDOW_SIZE 300

/* GtkWidget is the storage type for widgets */
GtkWidget *window;

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data ) {

    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete-event". */

    return FALSE;
}

/* Another callback */
static void destroy( GtkWidget *widget, gpointer data ) {
    gtk_main_quit ();
}

void *gui_background_service() {

    /* This is called in all GTK applications. Arguments are parsed
     * from the command line and are returned to the application. */
    gtk_init (NULL, NULL);

    /* create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
    g_signal_connect (window, "delete-event",
              G_CALLBACK (delete_event), NULL);

    /* Here we connect the "destroy" event to a signal handler.
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete-event" callback. */
    g_signal_connect (window, "destroy",
              G_CALLBACK (destroy), NULL);

    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window), WINDOW_SIZE);

    /* and the window */
    gtk_widget_show (window);

    /* All GTK applications must have a gtk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */
    fprintf(stderr, "gtk main running...\n");
    gtk_main();

    return 0;
}

/*
 * Callback function by kws trigger.
 */
static GdkColor Yellow = {0, 0xffff, 0xffff, 0x0000};
static GdkColor Blue = {0, 0x0000, 0x0000, 0xffff};
static GdkColor Green = {0, 0x0000, 0xffff, 0x0000};
static GdkColor Black = {0, 0x0000, 0x0000, 0x0000};
static GdkColor White = {0, 0xffff, 0xffff, 0xffff};
static GdkColor Red = {0, 0x0ffff, 0x0000, 0x0000};

void change_color_command(const char *color) {

    if (strstr(color, "YELLOW") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &Yellow);
    } else if (strstr(color, "BLUE") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &Blue);
    } else if (strstr(color, "GREEN") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &Green);
    } else if (strstr(color, "BLACK") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &Black);
    } else if (strstr(color, "WHITE") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &White);
    } else if (strstr(color, "RED") != NULL) {
        gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &Red);
    } else {
        fprintf(stderr, "color not supported\n");
    }

    fprintf(stderr, "color changed to: %s\n", color);

}

static ps_decoder_t *ps;
static cmd_ln_t *config;
static FILE *rawfd;

void kws_main(int argc, char *argv[]) {

    char const *cfg;
    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }

    if (config == NULL || (cmd_ln_str_r(config, "-infile") == NULL && cmd_ln_boolean_r(config, "-inmic") == FALSE)) {
	E_INFO("Specify '-infile <file.wav>' to recognize from file or '-inmic yes' to recognize from microphone.\n");
        cmd_ln_free_r(config);
        return;
    }

    ps_default_search_args(config);
    ps = ps_init(config);
    if (ps == NULL) {
        cmd_ln_free_r(config);
        return;
    }

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (cmd_ln_str_r(config, "-infile") != NULL) {
        recognize_from_file(ps, config, rawfd);
    } else if (cmd_ln_boolean_r(config, "-inmic")) {
        recognize_from_microphone(ps, config, change_color_command);
    }

    ps_free(ps);
    cmd_ln_free_r(config);

}


int main(int argc, char *argv[]) {

    /* This variable is our reference to the second thread */
    pthread_t gui_background_thread;

    /* Create a second thread which executes inc_x(&x) */
    if(pthread_create(&gui_background_thread, NULL, gui_background_service, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return -1;
    }

    /* Do my works here */
    kws_main(argc, argv);

    /* Wait for the second thread to finish */
    if(pthread_join(gui_background_thread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }

    return 0;
}
