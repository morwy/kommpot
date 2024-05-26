#ifndef EXPORT_DEFINITIONS_H
#define EXPORT_DEFINITIONS_H

#pragma once

//
// Code below is licensed under MIT license.
//

//
// MIT License
//
// Copyright (c) 2018 Elise Navennec
// URL: https://atomheartother.github.io/c++/2018/07/12/CPPDynLib.html
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef IS_COMPILING_STATIC

#    if defined _WIN32 || defined __CYGWIN__

#        ifdef WIN_EXPORT

#            ifdef __GNUC__
#                define EXPORTED __attribute__((dllexport))
#                define EXPORTED_TEMPLATE
#            else
#                define EXPORTED __declspec(dllexport)
#                define EXPORTED_TEMPLATE
#            endif

#        else

#            ifdef __GNUC__
#                define EXPORTED __attribute__((dllimport))
#                define EXPORTED_TEMPLATE extern
#            else
#                define EXPORTED __declspec(dllimport)
#                define EXPORTED_TEMPLATE extern
#            endif

#        endif

#        define NOT_EXPORTED

#    else

#        if __GNUC__ >= 4
#            define EXPORTED __attribute__((visibility("default")))
#            define EXPORTED_TEMPLATE
#            define NOT_EXPORTED __attribute__((visibility("hidden")))
#        else
#            define EXPORTED
#            define EXPORTED_TEMPLATE
#            define NOT_EXPORTED
#        endif

#    endif

#else

#    define EXPORTED
#    define EXPORTED_TEMPLATE
#    define NOT_EXPORTED

#endif

#endif // EXPORT_DEFINITIONS_H
