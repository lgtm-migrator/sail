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

#ifndef SAIL_CODEC_LAYOUT_FUNCTIONS_POINTERS_H
#define SAIL_CODEC_LAYOUT_FUNCTIONS_POINTERS_H

#ifdef SAIL_BUILD
#include "sail-common.h"
#else
#include <sail-common/sail-common.h>
#endif

/*
 * Decoding functions.
 */

typedef sail_status_t (*sail_codec_load_init_v8_t)(struct sail_io *io, const struct sail_load_options *load_options, void **state);
typedef sail_status_t (*sail_codec_load_seek_next_frame_v8_t)(void *state, struct sail_image **image);
typedef sail_status_t (*sail_codec_load_frame_v8_t)(void *state, struct sail_image *image);
typedef sail_status_t (*sail_codec_load_finish_v8_t)(void **state);

/*
 * Encoding functions.
 */

typedef sail_status_t (*sail_codec_save_init_v8_t)(struct sail_io *io, const struct sail_save_options *save_options, void **state);
typedef sail_status_t (*sail_codec_save_seek_next_frame_v8_t)(void *state, const struct sail_image *image);
typedef sail_status_t (*sail_codec_save_frame_v8_t)(void *state, const struct sail_image *image);
typedef sail_status_t (*sail_codec_save_finish_v8_t)(void **state);

#endif
