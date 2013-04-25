
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
 * The Original Code is pa_handlers.
 *
 * The Initial Developers of the Original Code are
 *   Craig Hokanson <craig.hokanson@sv.cmu.edu>
 *   Sion Chaudhuri <sion.chaudhuri@sv.cmu.edu>
 *   Gio Perez <gio.perez@sv.cmu.edu>
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developers. All Rights Reserved.
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

#include <errno.h>
#include <time.h>

#include <guacamole/audio.h>
#include <guacamole/client.h>

#include "buffer.h"
#include "client.h"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/introspect.h>
#include "pa_handlers.h"

buffer* guac_pa_buffer_alloc() {
    buffer* audio_buffer = malloc(sizeof(buffer));   
    buffer_init(audio_buffer, sizeof(unsigned char) * BUF_DATA_SIZE);

    return audio_buffer;
}

void guac_pa_buffer_free(buffer* audio_buffer) {
    buffer_free(audio_buffer);
    free(audio_buffer);
}

void* guac_pa_read_audio(void* data) {
    audio_args* args = (audio_args*) data;
    buffer* audio_buffer = args->audio_buffer;
    guac_client* client = args->audio->client;
    
    guac_client_log_info(client, "Starting audio read thread...");
    
    pa_simple* s_in = NULL;
    int error;
    
    static const pa_sample_spec ss = {
              .format = PA_SAMPLE_S16LE,
              .rate = SAMPLE_RATE,
              .channels = CHANNELS
          };
    
    /* Create a new record stream */
    if (!(s_in = pa_simple_new(NULL, "Record from sound card", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        guac_client_log_info(client, "Failed to create record stream using pa_simple_new(): %s\n", pa_strerror(error));
        goto finish;
    }

    while (client->state == GUAC_CLIENT_RUNNING) {
        uint8_t buf[BUF_DATA_SIZE];
        pa_usec_t latency;

        if ((latency = pa_simple_get_latency(s_in, &error)) == (pa_usec_t) -1) {
            guac_client_log_info(client, "Failed to get latency using pa_simple_get_latency(): %s\n", pa_strerror(error));
            goto finish;
        }

        if (pa_simple_read(s_in, buf, sizeof(buf), &error) < 0) {
            guac_client_log_info(client, "Failed to read audio buffer using pa_simple_read(): %s\n", pa_strerror(error));
            goto finish;
        }
        
        buffer_insert(audio_buffer, (void*) buf, sizeof(unsigned char) * BUF_DATA_SIZE);
    }

finish:
    if (s_in)
        pa_simple_free(s_in);

    guac_client_log_info(client, "Stopping audio read thread...");

    return NULL;
}

void* guac_pa_send_audio(void* data) {
    audio_args* args = (audio_args*) data;
    audio_stream* audio = args->audio; 
    buffer* audio_buffer = args->audio_buffer;
    guac_client* client = audio->client;
    
    guac_client_log_info(client, "Starting audio send thread...");

    while (client->state == GUAC_CLIENT_RUNNING) {
        unsigned char* buffer_data = malloc(sizeof(unsigned char) * BUF_DATA_SIZE);
        int counter = 0;

        audio_stream_begin(audio, SAMPLE_RATE, CHANNELS, BPS);
         
        while (counter < BUF_LENGTH) {
            buffer_remove(audio_buffer, (void *) buffer_data, sizeof(unsigned char) * BUF_DATA_SIZE, client);
            audio_stream_write_pcm(audio, buffer_data, BUF_DATA_SIZE);  
            counter++;
          
            if (client->state != GUAC_CLIENT_RUNNING)
                break;
        }

        audio_stream_end(audio); 
                
        guac_pa_sleep(SEND_INTERVAL);              
    }
  
    guac_client_log_info(client, "Stopping audio send thread...");
    return NULL;
}

void guac_pa_sleep(int millis) {

    struct timespec sleep_period;

    sleep_period.tv_sec =   millis / 1000;
    sleep_period.tv_nsec = (millis % 1000) * 1000000L;

    nanosleep(&sleep_period, NULL);
}
