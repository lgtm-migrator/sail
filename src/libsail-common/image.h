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

#ifndef SAIL_IMAGE_H
#define SAIL_IMAGE_H

#include <stdbool.h>

#ifdef SAIL_BUILD
    #include "error.h"
    #include "export.h"
#else
    #include <sail-common/error.h>
    #include <sail-common/export.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct sail_iccp;
struct sail_meta_data_node;
struct sail_palette;
struct sail_resolution;
struct sail_source_image;

/*
 * sail_image represents an image. Fields set by SAIL when loading images are marked with LOAD.
 * Fields that must be set by a caller when saving images are marked with SAVE.
 */
struct sail_image {

    /*
     * Image pixels.
     *
     * LOAD: Set by SAIL to an allocated array of pixels.
     * SAVE: Must be set by a caller to an allocated array of pixels.
     */
    void *pixels;

    /*
     * Image width.
     *
     * LOAD: Set by SAIL to a positive image width in pixels.
     * SAVE: Must be set by a caller to a positive image width in pixels.
     */
    unsigned width;

    /*
     * Image height.
     *
     * LOAD: Set by SAIL to a positive image height in pixels.
     * SAVE: Must be set by a caller to a positive image height in pixels.
     */
    unsigned height;

    /*
     * Bytes per line.
     *
     * LOAD: Set by SAIL to a positive length of a row of pixels in bytes.
     * SAVE: Must be set by a caller to a positive number of bytes per line. A caller could set
     *       it to sail_bytes_per_line() if scan lines are not padded to a certain boundary.
     */
    unsigned bytes_per_line;

    /*
     * Image resolution.
     *
     * LOAD: Set by SAIL to a valid resolution or to NULL if this information is not available.
     * SAVE: Must be set by a caller to a valid image resolution if necessary.
     */
    struct sail_resolution *resolution;

    /*
     * Image pixel format. See SailPixelFormat.
     *
     * LOAD: Set by SAIL to a valid image pixel format.
     * SAVE: Must be set by a caller to a valid input image pixel format. Pixels in this format will be supplied
     *       to the codec by a caller later. The list of supported input pixel formats by a certain codec
     *       can be obtained from sail_save_features.pixel_formats.
     */
    enum SailPixelFormat pixel_format;

    /*
     * Image gamma.
     *
     * LOAD: Set by SAIL to a valid gamma if it's available. 1 by default.
     * SAVE: Must be set by a caller to a valid gamma. Not all codecs support saving
     *       gamma.
     */
    double gamma;

    /*
     * Delay in milliseconds to display the image on the screen if the image is a frame
     * in an animation or -1 otherwise.
     *
     * LOAD: Set by SAIL to a non-negative number of milliseconds if the image is a frame
     *       in an animation or to -1 otherwise.
     *       For animations, it's guaranteed that all the frames have non-negative delays.
     *       For multi-paged sequences, it's guaranteed that all the pages have delays equal to -1.
     * SAVE: Must be set by a caller to a non-negative number of milliseconds if the image is a frame
     *       in an animation.
     */
    int delay;

    /*
     * Palette if the image has a palette and the requested pixel format assumes having a palette.
     * Destroyed by sail_destroy_image().
     *
     * LOAD: Set by SAIL to a valid palette if the image is indexed and the requested pixel format
     *       assumes having a palette. NULL otherwise.
     * SAVE: Must be allocated and set by a caller to a valid palette if the image is indexed.
     */
    struct sail_palette *palette;

    /*
     * Image meta data. Codecs guarantee that values are non-NULL.
     *
     * LOAD: Set by SAIL to a valid linked list with meta data (like JPEG comments) or to NULL.
     * SAVE: Must be allocated and set by a caller to a valid linked list with meta data
     *       (like JPEG comments) if necessary.
     */
    struct sail_meta_data_node *meta_data_node;

    /*
     * Embedded ICC profile.
     *
     * Note for animated/multi-paged images: only the first image in an animated/multi-paged
     * sequence might have an ICC profile.
     *
     * LOAD: Set by SAIL to a valid ICC profile or to NULL.
     * SAVE: Must be allocated and set by a caller to a valid ICC profile if necessary.
     */
    struct sail_iccp *iccp;

    /*
     * Source image properties which are usually lost during decoding.
     * For example, one might want to know the source image pixel format.
     *
     * LOAD: Set by SAIL to valid source image properties of the original image.
     * SAVE: Ignored.
     */
    struct sail_source_image *source_image;
};

typedef struct sail_image sail_image_t;

/*
 * Allocates a new image.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_alloc_image(struct sail_image **image);

/*
 * Destroys the specified image and all its internal allocated memory buffers. The image MUST NOT be used anymore
 * after calling this function. Does nothing if the image is NULL.
 */
SAIL_EXPORT void sail_destroy_image(struct sail_image *image);

/*
 * Makes a deep copy of the specified image.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_copy_image(const struct sail_image *source, struct sail_image **target);

/*
 * Makes a deep copy of the specified image without its pixels and palette.
 *
 * This function could be used in pixel conversion procedures when you need to preserve all the image info
 * except pixels.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_copy_image_skeleton(const struct sail_image *source, struct sail_image **target);

/*
 * Returns SAIL_OK if the given image has valid pixel_format, dimensions, and bytes per line.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_check_image_skeleton_valid(const struct sail_image *image);

/*
 * Returns SAIL_OK if the given image has valid dimensions and pixels.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_check_image_valid(const struct sail_image *image);

/*
 * Mirrors the image vertically.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_mirror_vertically(struct sail_image *image);

/*
 * Mirrors the image horizontally.
 *
 * Returns SAIL_OK on success.
 */
SAIL_EXPORT sail_status_t sail_mirror_horizontally(struct sail_image *image);

/* extern "C" */
#ifdef __cplusplus
}
#endif

#endif
