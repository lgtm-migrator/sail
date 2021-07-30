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

#include "sail-common.h"
#include "sail-c++.h"

namespace sail
{

sail_status_t print_errno(const char *format)
{
    SAIL_TRY(sail_print_errno(format));

    return SAIL_OK;
}

uint64_t now()
{
    return sail_now();
}

bool path_exists(const std::string_view path)
{
    return sail_path_exists(path.data());
}

bool is_dir(const std::string_view path)
{
    return sail_is_dir(path.data());
}

bool is_file(const std::string_view path)
{
    return sail_is_file(path.data());
}

sail_status_t file_size(const std::string_view path, size_t *size) {

    SAIL_TRY(sail_file_size(path.data(), size));

    return SAIL_OK;
}

sail_status_t read_file_contents(const std::string_view path, sail::arbitrary_data *contents) {

    SAIL_CHECK_PTR(contents);

    size_t size;
    SAIL_TRY(file_size(path, &size));

    contents->resize(size);
    SAIL_TRY(sail_file_contents_into_data(path.data(), contents->data()));

    return SAIL_OK;
}

}
