/*  This file is part of SAIL (https://github.com/smoked-herring/sail)

    Copyright (c) 2021 Dmitry Baryshev

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <webp/decode.h>
#include <webp/demux.h>

#include "sail-common.h"

#include "helpers.h"

/*
 * Codec-specific state.
 */
struct webp_state {
    struct sail_read_options *read_options;
    struct sail_write_options *write_options;

    WebPDemuxer *webp_demux;
    WebPIterator *webp_iterator;
    unsigned frame_number;
    uint32_t background_color;
    uint32_t frame_count;
    unsigned canvas_width;
    unsigned canvas_height;
    unsigned canvas_bytes_per_line;
    unsigned bytes_per_pixel;
    uint8_t *canvas_pixels;
    unsigned prev_x;
    unsigned prev_y;
    unsigned prev_width;
    unsigned prev_height;
    WebPMuxAnimDispose prev_dispose_method;
    WebPMuxAnimBlend prev_blend_method;

    void *image_data;
    size_t image_data_size;
};

static sail_status_t alloc_webp_state(struct webp_state **webp_state) {

    void *ptr;
    SAIL_TRY(sail_malloc(sizeof(struct webp_state), &ptr));
    *webp_state = ptr;

    (*webp_state)->read_options  = NULL;
    (*webp_state)->write_options = NULL;

    (*webp_state)->webp_demux            = NULL;
    (*webp_state)->webp_iterator         = NULL;
    (*webp_state)->frame_number          = 0;
    (*webp_state)->background_color      = 0;
    (*webp_state)->frame_count           = 0;
    (*webp_state)->canvas_width          = 0;
    (*webp_state)->canvas_height         = 0;
    (*webp_state)->canvas_bytes_per_line = 0;
    (*webp_state)->bytes_per_pixel       = 0;
    (*webp_state)->canvas_pixels         = NULL;
    (*webp_state)->prev_x                = 0;
    (*webp_state)->prev_y                = 0;
    (*webp_state)->prev_width            = 0;
    (*webp_state)->prev_height           = 0;
    (*webp_state)->prev_dispose_method   = WEBP_MUX_DISPOSE_NONE;
    (*webp_state)->prev_blend_method     = WEBP_MUX_NO_BLEND;

    (*webp_state)->image_data      = NULL;
    (*webp_state)->image_data_size = 0;

    return SAIL_OK;
}

static void destroy_webp_state(struct webp_state *webp_state) {

    if (webp_state == NULL) {
        return;
    }

    if (webp_state->webp_iterator != NULL) {
        WebPDemuxReleaseIterator(webp_state->webp_iterator);
        sail_free(webp_state->webp_iterator);
    }

    sail_free(webp_state->canvas_pixels);
    sail_free(webp_state->image_data);

    WebPDemuxDelete(webp_state->webp_demux);

    sail_destroy_read_options(webp_state->read_options);
    sail_destroy_write_options(webp_state->write_options);

    sail_free(webp_state);
}

/*
 * Decoding functions.
 */

SAIL_EXPORT sail_status_t sail_codec_read_init_v5_webp(struct sail_io *io, const struct sail_read_options *read_options, void **state) {

    SAIL_CHECK_STATE_PTR(state);
    *state = NULL;

    SAIL_TRY(sail_check_io_valid(io));
    SAIL_CHECK_READ_OPTIONS_PTR(read_options);

    /* Allocate a new state. */
    struct webp_state *webp_state;
    SAIL_TRY(alloc_webp_state(&webp_state));
    *state = webp_state;

    /* Deep copy read options. */
    SAIL_TRY(sail_copy_read_options(read_options, &webp_state->read_options));

    /* Read the entire image. */
    char signature_and_size[8];
    SAIL_TRY(io->strict_read(io->stream, signature_and_size, sizeof(signature_and_size)));
    webp_state->image_data_size = *(uint32_t *)(signature_and_size + 4) + sizeof(signature_and_size);

    SAIL_TRY(io->seek(io->stream, SEEK_SET, 0));

    void *ptr;
    SAIL_TRY(sail_malloc(webp_state->image_data_size, &ptr));
    webp_state->image_data = ptr;

    SAIL_TRY(io->strict_read(io->stream, webp_state->image_data, webp_state->image_data_size));

    /* Construct a WebP demuxer. */
    const WebPData data = { webp_state->image_data, webp_state->image_data_size };

    webp_state->webp_demux = WebPDemux(&data);

    SAIL_TRY(sail_malloc(sizeof(WebPIterator), &ptr));
    webp_state->webp_iterator = ptr;

    /* Frame count and other image info. */
    webp_state->background_color = WebPDemuxGetI(webp_state->webp_demux, WEBP_FF_BACKGROUND_COLOR);
    webp_state->frame_count      = WebPDemuxGetI(webp_state->webp_demux, WEBP_FF_FRAME_COUNT);
    webp_state->canvas_width     = WebPDemuxGetI(webp_state->webp_demux, WEBP_FF_CANVAS_WIDTH);
    webp_state->canvas_height    = WebPDemuxGetI(webp_state->webp_demux, WEBP_FF_CANVAS_HEIGHT);

    SAIL_TRY(sail_bytes_per_line(webp_state->canvas_width, SAIL_PIXEL_FORMAT_BPP32_RGBA, &webp_state->canvas_bytes_per_line));
    webp_state->bytes_per_pixel = webp_state->canvas_bytes_per_line / webp_state->canvas_width;

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_seek_next_frame_v5_webp(void *state, struct sail_io *io, struct sail_image **image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_CHECK_IMAGE_PTR(image);

    struct webp_state *webp_state = (struct webp_state *)state;

    /* Start demuxing. */
    if (webp_state->frame_number == 0) {
        if (WebPDemuxGetFrame(webp_state->webp_demux, 1, webp_state->webp_iterator) == 0) {
            SAIL_LOG_ERROR("WEBP: Failed to get the first frame");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_UNDERLYING_CODEC);
        }

        /* Allocate a canvas frame to apply disposal later. */
        size_t image_size = webp_state->canvas_bytes_per_line * webp_state->canvas_height;

        void *ptr;
        SAIL_TRY(sail_malloc(image_size, &ptr));
        webp_state->canvas_pixels = ptr;

        /* Fill background. */
        webp_private_fill_color(webp_state->canvas_pixels, webp_state->canvas_bytes_per_line, webp_state->bytes_per_pixel,
                                webp_state->background_color, 0, 0, webp_state->canvas_width, webp_state->canvas_height);
    } else {
        switch (webp_state->prev_dispose_method) {
            case WEBP_MUX_DISPOSE_BACKGROUND: {
                webp_private_fill_color(webp_state->canvas_pixels, webp_state->canvas_bytes_per_line, webp_state->bytes_per_pixel,
                                        webp_state->background_color, webp_state->prev_x, webp_state->prev_y,
                                        webp_state->prev_width, webp_state->prev_height);
                break;
            }
            case WEBP_MUX_DISPOSE_NONE: {
                break;
            }
            default: {
                SAIL_LOG_ERROR("WEBP: Unknown disposal method");
                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNDERLYING_CODEC);
            }
        }

        if (WebPDemuxNextFrame(webp_state->webp_iterator) == 0) {
            SAIL_LOG_AND_RETURN(SAIL_ERROR_NO_MORE_FRAMES);
        }
    }

    webp_state->frame_number++;
    webp_state->prev_x              = webp_state->webp_iterator->x_offset;
    webp_state->prev_y              = webp_state->webp_iterator->y_offset;
    webp_state->prev_width          = webp_state->webp_iterator->width;
    webp_state->prev_height         = webp_state->webp_iterator->height;
    webp_state->prev_dispose_method = webp_state->webp_iterator->dispose_method;
    webp_state->prev_blend_method   = webp_state->webp_iterator->blend_method;

    struct sail_image *image_local;
    SAIL_TRY(sail_alloc_image(&image_local));
    SAIL_TRY_OR_CLEANUP(sail_alloc_source_image(&image_local->source_image),
                        /* cleanup */ sail_destroy_image(image_local));

    image_local->source_image->pixel_format = webp_state->webp_iterator->has_alpha ? SAIL_PIXEL_FORMAT_BPP32_YUVA : SAIL_PIXEL_FORMAT_BPP24_YUV;
    image_local->source_image->chroma_subsampling = SAIL_CHROMA_SUBSAMPLING_420;

    image_local->width = webp_state->canvas_width;
    image_local->height = webp_state->canvas_height;
    image_local->bytes_per_line = webp_state->canvas_bytes_per_line;
    image_local->pixel_format = SAIL_PIXEL_FORMAT_BPP32_RGBA;
    if (webp_state->frame_count > 1) {
        /* Fall back to 100 ms. when the duration is <= 0. */
        image_local->delay = webp_state->webp_iterator->duration <= 0 ? 100 : webp_state->webp_iterator->duration;
    }

#if 0
    /* Fetch ICC profile. */
    if (webp_state->read_options->io_options & SAIL_IO_OPTION_ICCP) {
        SAIL_TRY_OR_CLEANUP(webp_private_fetch_iccp(&webp_image->icc, &image_local->iccp),
                            /* cleanup */ sail_destroy_image(image_local));
    }
#endif

    *image = image_local;

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_seek_next_pass_v5_webp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_TRY(sail_check_image_skeleton_valid(image));

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_frame_v5_webp(void *state, struct sail_io *io, struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_TRY(sail_check_image_skeleton_valid(image));

    struct webp_state *webp_state = (struct webp_state *)state;

    switch (webp_state->prev_blend_method) {
        case WEBP_MUX_NO_BLEND: {
            if (WebPDecodeRGBAInto(webp_state->webp_iterator->fragment.bytes,
                                    webp_state->webp_iterator->fragment.size,
                                    webp_state->canvas_pixels + webp_state->canvas_bytes_per_line * webp_state->prev_y +
                                        webp_state->prev_x * webp_state->bytes_per_pixel,
                                    webp_state->canvas_bytes_per_line * webp_state->canvas_height,
                                    webp_state->canvas_bytes_per_line) == NULL) {
                SAIL_LOG_ERROR("WEBP: Failed to decode image");
                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNDERLYING_CODEC);
            }
            break;
        }
        case WEBP_MUX_BLEND: {
            if (WebPDecodeRGBAInto(webp_state->webp_iterator->fragment.bytes,
                                    webp_state->webp_iterator->fragment.size,
                                    image->pixels,
                                    image->bytes_per_line * image->height,
                                    webp_state->prev_width * webp_state->bytes_per_pixel) == NULL) {
                SAIL_LOG_ERROR("WEBP: Failed to decode image");
                SAIL_LOG_AND_RETURN(SAIL_ERROR_UNDERLYING_CODEC);
            }

            uint8_t *dst_scanline = webp_state->canvas_pixels + webp_state->prev_y * image->bytes_per_line + webp_state->prev_x * webp_state->bytes_per_pixel;
            uint8_t *src_scanline = image->pixels;

            for (unsigned row = 0; row < webp_state->prev_height; row++, dst_scanline += webp_state->canvas_bytes_per_line, src_scanline += webp_state->prev_width * webp_state->bytes_per_pixel) {
                SAIL_TRY(webp_private_blend_over(dst_scanline, 0, src_scanline, webp_state->prev_width, webp_state->bytes_per_pixel));
            }
            break;
        }
        default: {
            SAIL_LOG_ERROR("WEBP: Unknown blending method");
            SAIL_LOG_AND_RETURN(SAIL_ERROR_UNDERLYING_CODEC);
        }
    }

    memcpy(image->pixels, webp_state->canvas_pixels, image->bytes_per_line * image->height);

    return SAIL_OK;
}

SAIL_EXPORT sail_status_t sail_codec_read_finish_v5_webp(void **state, struct sail_io *io) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));

    struct webp_state *webp_state = (struct webp_state *)(*state);

    /* Subsequent calls to finish() will expectedly fail in the above line. */
    *state = NULL;

    destroy_webp_state(webp_state);

    return SAIL_OK;
}

/*
 * Encoding functions.
 */

SAIL_EXPORT sail_status_t sail_codec_write_init_v5_webp(struct sail_io *io, const struct sail_write_options *write_options, void **state) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_CHECK_WRITE_OPTIONS_PTR(write_options);

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_seek_next_frame_v5_webp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_TRY(sail_check_image_valid(image));

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_seek_next_pass_v5_webp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_TRY(sail_check_image_valid(image));

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_frame_v5_webp(void *state, struct sail_io *io, const struct sail_image *image) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));
    SAIL_TRY(sail_check_image_valid(image));

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}

SAIL_EXPORT sail_status_t sail_codec_write_finish_v5_webp(void **state, struct sail_io *io) {

    SAIL_CHECK_STATE_PTR(state);
    SAIL_TRY(sail_check_io_valid(io));

    SAIL_LOG_AND_RETURN(SAIL_ERROR_NOT_IMPLEMENTED);
}
