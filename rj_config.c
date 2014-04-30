/*
 * This source file is part of the librj c library.
 *
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "rj_config.h"

#include <string.h>

char* rj_config_get(const char *section, const char *field, const char *def, struct recordjar *rj)
{
    return rj_get("section", section, field, def, rj);
}

void rj_config_set(const char *section, const char *field, const char *value, struct recordjar *rj)
{
    if(rj_get("section", section, 0, 0, rj))
    {
        if(!rj_set_only(0, 0, field, value, rj))
            rj_add(0, 0, field, value, rj);
    }
    else
        rj_add("section", section, field, value, rj);
}

int rj_config_list(const char *section, struct recordjar *rj)
{
    return !rj_get("section", section, 0, 0, rj);
}

void rj_config_next(char **field, char **value, struct recordjar *rj)
{
    rj_next(field, value, rj);
    
    if(!strcmp("section", *field))
        rj_next(field, value, rj);
}
