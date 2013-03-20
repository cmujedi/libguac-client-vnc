 
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <rfb/rfbclient.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "audio.h"
#include "wav_encoder.h"
#ifdef ENABLE_OGG
#include "ogg_encoder.h"
#endif

#include "client.h"
#include "vnc_handlers.h"
#include "guac_handlers.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/introspect.h>

#define BUFSIZE 1024

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "read-only",
    "encodings",
    "password",
    "swap-red-blue",
    "color-depth",
    "disable-audio",
    NULL
};

char* __GUAC_CLIENT = "GUAC_CLIENT";

int guac_client_init(guac_client* client, int argc, char** argv) {

    rfbClient* rfb_client;

    vnc_guac_client_data* guac_client_data;

    int read_only;
    int i;

    /* Set up libvncclient logging */
    rfbClientLog = guac_vnc_client_log_info;
    rfbClientErr = guac_vnc_client_log_error;

    /*** PARSE ARGUMENTS ***/

    if (argc < 8) {
        guac_protocol_send_error(client->socket, "Wrong argument count received.");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    /* Set read-only flag */
    read_only = (strcmp(argv[2], "true") == 0);

    /* Set red/blue swap flag */
    guac_client_data->swap_red_blue = (strcmp(argv[5], "true") == 0);

    /* Freed after use by libvncclient */
    guac_client_data->password = strdup(argv[4]);

    /*** INIT RFB CLIENT ***/

    rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, __GUAC_CLIENT, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Do not handle clipboard and local cursor if read-only */
    if (read_only == 0) {
        /* Enable client-side cursor */
        rfb_client->GotCursorShape = guac_vnc_cursor;
        rfb_client->appData.useRemoteCursor = TRUE;

        /* Clipboard */
        rfb_client->GotXCutText = guac_vnc_cut_text;
    }

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;

    /* Depth */
    guac_vnc_set_pixel_format(rfb_client, atoi(argv[6]));

    guac_client_data->audio_enabled = (strcmp(argv[7], "true") != 0);
    
    printf("Audio created");
    
   /* If audio enabled, choose an encoder */
   if (guac_client_data->audio_enabled) {       

       /* Choose an encoding */
       for (i=0; client->info.audio_mimetypes[i] != NULL; i++) {

           const char* mimetype = client->info.audio_mimetypes[i];

#ifdef ENABLE_OGG
           /* If Ogg is supported, done. */
           if (strcmp(mimetype, ogg_encoder->mimetype) == 0) {
               guac_client_log_info(client, "Loading Ogg Vorbis encoder.");
               guac_client_data->audio = audio_stream_alloc(client,
                       ogg_encoder);
               break;
           }
#endif

           /* If wav is supported, done. */
           if (strcmp(mimetype, wav_encoder->mimetype) == 0) {
               guac_client_log_info(client, "Loading wav encoder.");
               guac_client_data->audio = audio_stream_alloc(client,
                       wav_encoder);
               break;
           }
       }

       /* If an encoding is available, load the sound plugin */
       if (guac_client_data->audio != NULL) {

           printf("Encoding available");
           
           /* Load sound plugin
           if (freerdp_channels_load_plugin(channels, instance->settings,
                       "guac_rdpsnd", guac_client_data->audio))
               guac_client_log_error(client,
                       "Failed to load guac_rdpsnd plugin."); */
           
           pthread_t pa_thread;
           audio_stream* audio = guac_client_data->audio;
           pa_thread_args* pa_args = malloc(sizeof(pa_thread_args*));
           pa_args->client = client;
           pa_args->audio  = audio;

           /* Create a plugin instead of the thread */
           if (pthread_create(&pa_thread, NULL, guac_client_pa_thread, (void*) pa_args)) {
               printf("Thread created");
               guac_protocol_send_error(client->socket, "Error initializing pulse audio thread");
               guac_socket_flush(client->socket);
               return 1;
           }
       }
       else
           guac_client_log_info(client,
                   "No available audio encoding. Sound disabled.");

   } /* end if audio enabled */

    /* Hook into allocation so we can handle resize. */
    guac_client_data->rfb_MallocFrameBuffer = rfb_client->MallocFrameBuffer;
    rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
    rfb_client->canHandleNewFBSize = 1;

    /* Set hostname and port */
    rfb_client->serverHost = strdup(argv[0]);
    rfb_client->serverPort = atoi(argv[1]);

    /* Set encodings if specified */
    if (argv[3][0] != '\0')
        rfb_client->appData.encodingsString = guac_client_data->encodings = strdup(argv[3]);
    else
        guac_client_data->encodings = NULL;

    /* Connect */
    if (!rfbInitClient(rfb_client, NULL, NULL)) {
        guac_protocol_send_error(client->socket, "Error initializing VNC client");
        guac_socket_flush(client->socket);
        return 1;
    }

    /* Set remaining client data */
    guac_client_data->rfb_client = rfb_client;
    guac_client_data->copy_rect_used = 0;
    guac_client_data->cursor = guac_client_alloc_buffer(client);

    /* Set handlers */
    client->handle_messages = vnc_guac_client_handle_messages;
    client->free_handler = vnc_guac_client_free_handler;
    if (read_only == 0) {
        /* Do not handle mouse/keyboard/clipboard if read-only */
        client->mouse_handler = vnc_guac_client_mouse_handler;
        client->key_handler = vnc_guac_client_key_handler;
        client->clipboard_handler = vnc_guac_client_clipboard_handler;
    }

    /* Send name */
    guac_protocol_send_name(client->socket, rfb_client->desktopName);

    /* Send size */
    guac_protocol_send_size(client->socket,
            GUAC_DEFAULT_LAYER, rfb_client->width, rfb_client->height);

    return 0;

}

void* guac_client_pa_thread(void* data) {
    pa_thread_args* args = (pa_thread_args*) data;
    guac_client* client = args->client;
    audio_stream* audio = args->audio;

    /***** We Stopped Here *****/

    /**** TODO: ****/
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

    pa_simple *s_in = NULL;
    int error;

    /* This is a command to get the name of the default source
    which should look like the following:
    alsa_output.pci-0000_00_05.0.analog-stereo.monitor
    */

    FILE *fp;
    char *command = "pactl list | grep -A2 'Source #' | grep 'Name: .*\\.monitor$' | cut -d\" \" -f2";
    char output[100];

    fp = popen(command,"r");

    /* read output from command */
    int result = fscanf(fp,"%s",output);
    if (result == EOF)
        goto finish;

    fclose(fp);

    char *device = output;

    /* Create a new playback stream */
    if (!(s_in = pa_simple_new(NULL, "Record from sound card", PA_STREAM_RECORD, device, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

    while (client->state == GUAC_CLIENT_RUNNING) {
        uint8_t buf[BUFSIZE];
        pa_usec_t latency;

        if ((latency = pa_simple_get_latency(s_in, &error)) == (pa_usec_t) -1) {
            fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
            goto finish;
        }

        if (pa_simple_read(s_in, buf, sizeof(buf), &error) < 0) {
            fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
            goto finish;
        }
        
        /* Init stream with requested format */
        audio_stream_begin(audio, 44100, 2, 16);

        /* Write initial 4 bytes of data */
        audio_stream_write_pcm(audio, buf, 4);

        /* Write pcm data to the audio stream buff */
        audio_stream_write_pcm(audio, buf, BUFSIZE);

        /* Flush encoded stream to guacamole */
        audio_stream_end(audio);
        
    }

finish:

    if (s_in)
        pa_simple_free(s_in);

    return NULL;
}



