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

/*
 * This is a codec layout definition file.
 *
 * It's intedened to be used as a reference how codecs V8 are organized. It's also could
 * be used by codecs' developers to compile their codecs directly into a test application
 * to simplify debugging.
 *
 * Include guards are not used as the header may be included multiple times with different
 * SAIL_CODEC_NAME definitions.
 */

#ifdef SAIL_BUILD
#include "error.h"
#else
#include <sail-common/error.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SAIL_CODEC_NAME
/*
 * Generate syntax error.
 *
 * Usage:
 *
 * #define SAIL_CODEC_NAME jpeg
 * #include <sail/layouts/v8.h>
 */
Please define SAIL_CODEC_NAME before including this header.
#endif

#define SAIL_CONSTRUCT_CODEC_FUNC_CONCAT2_IMPL(a, b) a##_##b
#define SAIL_CONSTRUCT_CODEC_FUNC_CONCAT2(a, b)      SAIL_CONSTRUCT_CODEC_FUNC_CONCAT2_IMPL(a, b)
#define SAIL_CONSTRUCT_CODEC_FUNC(name)              SAIL_CONSTRUCT_CODEC_FUNC_CONCAT2(name, SAIL_CODEC_NAME)

/*
 * Decoding functions.
 */

/*
 * Starts decoding the specified io stream using the specified options.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The IO is valid and open.
 *   - The load options is not NULL.
 *
 * This function MUST:
 *   - Allocate an internal state object with internal data structures necessary to decode a file,
 *     and assign its value to the state.
 *
 * STATE explanation: Pass the address of a local void* pointer. Codecs will store an internal state
 * in it and destroy it in sail_codec_load_finish_vx(). States must be used per image. DO NOT use the same state
 * to load multiple images in the same time.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_load_init_v8)(struct sail_io *io, const struct sail_load_options *load_options, void **state);

/*
 * Seeks to the next frame. The frame is NOT immediately loaded or decoded by most SAIL codecs.
 * SAIL uses this method in loading and probing operations.
 *
 * SAIL uses sail_codec_load_frame_vx() to actually load the frame.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state points to the state allocated by sail_codec_load_init_vx().
 *
 * This function MUST:
 *   - Allocate the image and the source image (sail_image.sail_source_image).
 *   - Fill the expected image properties (width, height, pixel format, image properties etc.) and meta data.
 *     The image pixel format must be as close to the source as possible.
 *   - Seek to the next image frame.
 *
 * This function MUST NOT:
 *   - Allocate the image pixels. They will be allocated by libsail and will be available in
 *     sail_codec_load_frame_vx().
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_load_seek_next_frame_v8)(void *state, struct sail_image **image);

/*
 * Reads the next frame of the current image in the current pass. The image pixels are pre-allocated by libsail.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state is valid and points to the state allocated by sail_codec_load_init_vx().
 *   - The image points to the image allocated by sail_codec_load_seek_next_frame_vx().
 *   - The image pixels are allocated.
 *
 * This function MUST:
 *   - Read the image pixels into sail_image.pixels.
 *   - Output pixels with the origin in the top left corner (i.e. not flipped).
 *   - Output pixels in format as close to the source as possible.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_load_frame_v8)(void *state, struct sail_image *image);

/*
 * Finilizes loading operation. No more loadings are possible after calling this function.
 * This function doesn't close the io stream. It just stops decoding. Use io->close() or sail_destroy_io()
 * to actually close the io stream.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state points to the state allocated by sail_codec_load_init_vx().
 *
 * This function MUST:
 *   - Destroy the state and set it to NULL.
 *
 * This function MUST NOT:
 *   - Close the IO.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_load_finish_v8)(void **state);

/*
 * Encoding functions.
 */

/*
 * Starts encoding the specified io stream using the specified options. The specified save options
 * will be deep copied into an internal buffer.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The IO is valid and open.
 *   - The save options is not NULL.
 *
 * This function MUST:
 *   - Allocate an internal state object with internal data structures necessary to decode a file,
 *     and assign its value to the state.
 *
 * STATE explanation: Pass the address of a local void* pointer. Codecs will store an internal state
 * in it and destroy it in sail_codec_save_finish_vx(). States must be used per image. DO NOT use the same state
 * to save multiple images to different targets in the same time.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_save_init_v8)(struct sail_io *io, const struct sail_save_options *save_options, void **state);

/*
 * Seeks to a next frame before saving it. The frame is NOT immediately written. Use sail_codec_save_frame_vx()
 * to actually save a frame.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state points to the state allocated by sail_codec_save_init_vx().
 *   - The image is valid.
 *
 * This function MUST:
 *   - Seek to the right position before saving the next image frame.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_save_seek_next_frame_v8)(void *state, const struct sail_image *image);

/*
 * Writes a next frame of the current image in the current pass.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state is valid and points to the state allocated by sail_codec_save_init_vx().
 *   - The image is valid.
 *
 * This function MUST:
 *   - Write the image pixels and meta data into the IO.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_save_frame_v8)(void *state, const struct sail_image *image);

/*
 * Finilizes saving operation. No more savings are possible after calling this function.
 * This function doesn't close the io stream. Use io->close() or sail_destroy_io() to actually
 * close the io stream.
 *
 * libsail, a caller of this function, guarantees the following:
 *   - The state points to the state allocated by sail_codec_save_init_vx().
 *
 * This function MUST:
 *   - Destroy the state and set it to NULL.
 *
 * This function MUST NOT:
 *   - Close the IO.
 *
 * Returns SAIL_OK on success.
 */
sail_status_t SAIL_CONSTRUCT_CODEC_FUNC(sail_codec_save_finish_v8)(void **state);

/* extern "C" */
#ifdef __cplusplus
}
#endif
