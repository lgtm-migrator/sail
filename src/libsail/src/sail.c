/*  This file is part of SAIL (https://github.com/smoked-herring/sail)

    Copyright (c) 2020 Dmitry Baryshev

    The MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sail-common.h"
#include "sail.h"

sail_status_t sail_init_with_flags(int flags) {

    struct sail_context *context;
    SAIL_TRY(current_tls_context_with_flags(&context, flags));

    return SAIL_OK;
}

void sail_finish(void) {

    SAIL_LOG_INFO("Finish");

    control_tls_context(/* context - not needed */ NULL, SAIL_CONTEXT_DESTROY);
}

const struct sail_plugin_info_node* sail_plugin_info_list(void) {

    struct sail_context *context;
    SAIL_TRY_OR_EXECUTE(current_tls_context(&context),
                        /* on error */ return NULL);

    return context->plugin_info_node;
}

sail_status_t sail_plugin_info_from_path(const char *path, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_PATH_PTR(path);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    const char *dot = strrchr(path, '.');

    if (dot == NULL || *dot == '\0' || *(dot+1) == '\0') {
        return SAIL_ERROR_INVALID_ARGUMENT;
    }

    SAIL_LOG_DEBUG("Finding plugin info for path '%s'", path);

    SAIL_TRY(sail_plugin_info_from_extension(dot+1, plugin_info));

    return SAIL_OK;
}

sail_status_t sail_plugin_info_by_magic_number_from_path(const char *path, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_PATH_PTR(path);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    struct sail_io *io;
    SAIL_TRY(alloc_io_read_file(path, &io));

    SAIL_TRY_OR_CLEANUP(sail_plugin_info_by_magic_number_from_io(io, plugin_info),
                        /* cleanup */ sail_destroy_io(io));

    sail_destroy_io(io);

    return SAIL_OK;
}

sail_status_t sail_plugin_info_by_magic_number_from_mem(const void *buffer, size_t buffer_length, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_BUFFER_PTR(buffer);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    struct sail_io *io;
    SAIL_TRY(alloc_io_read_mem(buffer, buffer_length, &io));

    SAIL_TRY_OR_CLEANUP(sail_plugin_info_by_magic_number_from_io(io, plugin_info),
                        /* cleanup */ sail_destroy_io(io));

    sail_destroy_io(io);

    return SAIL_OK;
}

sail_status_t sail_plugin_info_by_magic_number_from_io(struct sail_io *io, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_IO_PTR(io);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    struct sail_context *context;
    SAIL_TRY(current_tls_context(&context));

    size_t nbytes;
    unsigned char buffer[SAIL_MAGIC_BUFFER_SIZE];

    SAIL_TRY(io->read(io->stream, buffer, 1, SAIL_MAGIC_BUFFER_SIZE, &nbytes));

    if (nbytes != SAIL_MAGIC_BUFFER_SIZE) {
        SAIL_LOG_ERROR("Failed to read %d bytes from the I/O source", SAIL_MAGIC_BUFFER_SIZE);
        return SAIL_ERROR_READ_IO;
    }

    /* Seek back. */
    SAIL_TRY(io->seek(io->stream, 0, SEEK_SET));

    /* \xFF\xDD => "FF DD" + string terminator. */
    char hex_numbers[SAIL_MAGIC_BUFFER_SIZE * 3 + 1];
    char *hex_numbers_ptr = hex_numbers;

    for (size_t i = 0; i < nbytes; i++, hex_numbers_ptr += 3) {
#ifdef SAIL_WIN32
        sprintf_s(hex_numbers_ptr, 4, "%02x ", buffer[i]);
#else
        sprintf(hex_numbers_ptr, "%02x ", buffer[i]);
#endif
    }

    hex_numbers_ptr--;
    *hex_numbers_ptr = '\0';

    SAIL_LOG_DEBUG("Read magic number: '%s'", hex_numbers);

    /* Find the plugin info. */
    struct sail_plugin_info_node *node = context->plugin_info_node;

    while (node != NULL) {
        struct sail_string_node *string_node = node->plugin_info->magic_number_node;

        while (string_node != NULL) {
            if (strncmp(hex_numbers, string_node->value, strlen(string_node->value)) == 0) {
                *plugin_info = node->plugin_info;
                SAIL_LOG_DEBUG("Found plugin info: '%s'", (*plugin_info)->name);
                return SAIL_OK;
            }

            string_node = string_node->next;
        }

        node = node->next;
    }

    return SAIL_ERROR_PLUGIN_NOT_FOUND;
}

sail_status_t sail_plugin_info_from_extension(const char *extension, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_EXTENSION_PTR(extension);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    SAIL_LOG_DEBUG("Finding plugin info for extension '%s'", extension);

    struct sail_context *context;
    SAIL_TRY(current_tls_context(&context));

    char *extension_copy;
    SAIL_TRY(sail_strdup(extension, &extension_copy));

    /* Will compare in lower case. */
    sail_to_lower(extension_copy);

    struct sail_plugin_info_node *node = context->plugin_info_node;

    while (node != NULL) {
        struct sail_string_node *string_node = node->plugin_info->extension_node;

        while (string_node != NULL) {
            if (strcmp(string_node->value, extension_copy) == 0) {
                sail_free(extension_copy);
                *plugin_info = node->plugin_info;
                SAIL_LOG_DEBUG("Found plugin info: '%s'", (*plugin_info)->name);
                return SAIL_OK;
            }

            string_node = string_node->next;
        }

        node = node->next;
    }

    sail_free(extension_copy);
    return SAIL_ERROR_PLUGIN_NOT_FOUND;
}

sail_status_t sail_plugin_info_from_mime_type(const char *mime_type, const struct sail_plugin_info **plugin_info) {

    SAIL_CHECK_PTR(mime_type);
    SAIL_CHECK_PLUGIN_INFO_PTR(plugin_info);

    SAIL_LOG_DEBUG("Finding plugin info for mime type '%s'", mime_type);

    struct sail_context *context;
    SAIL_TRY(current_tls_context(&context));

    char *mime_type_copy;
    SAIL_TRY(sail_strdup(mime_type, &mime_type_copy));

    /* Will compare in lower case. */
    sail_to_lower(mime_type_copy);

    struct sail_plugin_info_node *node = context->plugin_info_node;

    while (node != NULL) {
        struct sail_string_node *string_node = node->plugin_info->mime_type_node;

        while (string_node != NULL) {
            if (strcmp(string_node->value, mime_type_copy) == 0) {
                sail_free(mime_type_copy);
                *plugin_info = node->plugin_info;
                SAIL_LOG_DEBUG("Found plugin info: '%s'", (*plugin_info)->name);
                return SAIL_OK;
            }

            string_node = string_node->next;
        }

        node = node->next;
    }

    sail_free(mime_type_copy);
    return SAIL_ERROR_PLUGIN_NOT_FOUND;
}

sail_status_t sail_unload_plugins(void) {

    SAIL_LOG_DEBUG("Unloading cached plugins");

    struct sail_context *context;
    SAIL_TRY(current_tls_context(&context));

    struct sail_plugin_info_node *node = context->plugin_info_node;
    int counter = 0;

    while (node != NULL) {
        if (node->plugin != NULL) {
            destroy_plugin(node->plugin);
            counter++;
        }

        node->plugin = NULL;

        node = node->next;
    }

    SAIL_LOG_DEBUG("Unloaded plugins: %d", counter);

    return SAIL_OK;
}
