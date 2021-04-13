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

#include "sail-manip.h"

void spread_gray8_to_rgba32(uint8_t value, sail_rgba32_t *rgba32) {

    rgba32->component1 = rgba32->component2 = rgba32->component3 = value;
    rgba32->component4 = 255;
}

void spread_gray16_to_rgba32(uint16_t value, sail_rgba32_t *rgba32) {

    rgba32->component1 = rgba32->component2 = rgba32->component3 = (uint8_t)(value / 257.0);
    rgba32->component4 = 255;
}

void spread_gray8_to_rgba64(uint8_t value, sail_rgba64_t *rgba64) {

    rgba64->component1 = rgba64->component2 = rgba64->component3 = (uint16_t)value * 257;
    rgba64->component4 = 65535;
}

void spread_gray16_to_rgba64(uint16_t value, sail_rgba64_t *rgba64) {

    rgba64->component1 = rgba64->component2 = rgba64->component3 = value;
    rgba64->component4 = 65535;
}

sail_status_t get_palette_rgba32(const struct sail_palette *palette, unsigned index, sail_rgba32_t *rgba32) {

    if (index >= palette->color_count) {
        SAIL_LOG_ERROR("Palette index %u is out of range [0; %u)", index, palette->color_count);
        SAIL_LOG_AND_RETURN(SAIL_ERROR_BROKEN_IMAGE);
    }

    switch (palette->pixel_format) {
        case SAIL_PIXEL_FORMAT_BPP24_RGB: {
            const uint8_t *entry = (uint8_t *)palette->data + index * 3;

            rgba32->component1 = *(entry+0);
            rgba32->component2 = *(entry+1);
            rgba32->component3 = *(entry+2);
            rgba32->component4 = 255;
            break;
        }
        case SAIL_PIXEL_FORMAT_BPP32_RGBA: {
            const uint8_t *entry = (uint8_t *)palette->data + index * 4;

            rgba32->component1 = *(entry+0);
            rgba32->component2 = *(entry+1);
            rgba32->component3 = *(entry+2);
            rgba32->component4 = *(entry+3);
            break;
        }
        default: {
            const char *pixel_format_str = NULL;
            SAIL_TRY_OR_SUPPRESS(sail_pixel_format_to_string(palette->pixel_format, &pixel_format_str));
            SAIL_LOG_ERROR("Palette pixel format %s is not currently supported", pixel_format_str);
        }
    }

    return SAIL_OK;
}

void fill_rgba32_pixel_from_uint8_values(const sail_rgba32_t *rgba32, uint8_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options) {

    if (a < 0 && options != NULL && (options->options & SAIL_CONVERSION_OPTION_BLEND_ALPHA)) {
        const double opacity = rgba32->component4 / 255.0;

        *(scan+r) = (uint8_t)(opacity * rgba32->component1 + (1 - opacity) * options->background24.component1);
        *(scan+g) = (uint8_t)(opacity * rgba32->component2 + (1 - opacity) * options->background24.component2);
        *(scan+b) = (uint8_t)(opacity * rgba32->component3 + (1 - opacity) * options->background24.component3);
    } else {
        *(scan+r) = rgba32->component1;
        *(scan+g) = rgba32->component2;
        *(scan+b) = rgba32->component3;
    }

    if (a >= 0) {
        *(scan+a) = rgba32->component4;
    }
}

void fill_rgba32_pixel_from_uint16_values(const sail_rgba64_t *rgba64, uint8_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options) {

    if (a < 0 && options != NULL && (options->options & SAIL_CONVERSION_OPTION_BLEND_ALPHA)) {
        const double opacity = rgba64->component4 / 65535.0;

        *(scan+r) = (uint8_t)((opacity * rgba64->component1 + (1 - opacity) * options->background48.component1) / 257.0);
        *(scan+g) = (uint8_t)((opacity * rgba64->component2 + (1 - opacity) * options->background48.component2) / 257.0);
        *(scan+b) = (uint8_t)((opacity * rgba64->component3 + (1 - opacity) * options->background48.component3) / 257.0);
    } else {
        *(scan+r) = (uint8_t)(rgba64->component1 / 257.0);
        *(scan+g) = (uint8_t)(rgba64->component2 / 257.0);
        *(scan+b) = (uint8_t)(rgba64->component3 / 257.0);
    }

    if (a >= 0) {
        *(scan+a) = (uint8_t)(rgba64->component4 / 257.0);
    }
}

void fill_rgba64_pixel_from_uint8_values(const sail_rgba32_t *rgba32, uint16_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options) {

    if (a < 0 && options != NULL && (options->options & SAIL_CONVERSION_OPTION_BLEND_ALPHA)) {
        const double opacity = rgba32->component4 / 255.0;

        *(scan+r) = (uint16_t)(opacity * (rgba32->component1 * 257) + (1 - opacity) * options->background48.component1);
        *(scan+g) = (uint16_t)(opacity * (rgba32->component2 * 257) + (1 - opacity) * options->background48.component2);
        *(scan+b) = (uint16_t)(opacity * (rgba32->component3 * 257) + (1 - opacity) * options->background48.component3);
    } else {
        *(scan+r) = rgba32->component1 * 257;
        *(scan+g) = rgba32->component2 * 257;
        *(scan+b) = rgba32->component3 * 257;
    }

    if (a >= 0) {
        *(scan+a) = rgba32->component4 * 257;
    }
}

void fill_rgba64_pixel_from_uint16_values(const sail_rgba64_t *rgba64, uint16_t *scan, int r, int g, int b, int a, const struct sail_conversion_options *options) {

    if (a < 0 && options != NULL && (options->options & SAIL_CONVERSION_OPTION_BLEND_ALPHA)) {
        const double opacity = rgba64->component4 / 65535.0;

        *(scan+r) = (uint16_t)(opacity * rgba64->component1 + (1 - opacity) * options->background48.component1);
        *(scan+g) = (uint16_t)(opacity * rgba64->component2 + (1 - opacity) * options->background48.component2);
        *(scan+b) = (uint16_t)(opacity * rgba64->component3 + (1 - opacity) * options->background48.component3);
    } else {
        *(scan+r) = rgba64->component1;
        *(scan+g) = rgba64->component2;
        *(scan+b) = rgba64->component3;
    }

    if (a >= 0) {
        *(scan+a) = rgba64->component4;
    }
}
