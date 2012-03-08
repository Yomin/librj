/*
 * This source file is part of the librj c library.
 * 
 * Copyright (c) 2012 Martin Rödel aka Yomin
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

#ifndef __RJ_H__
#define __RJ_H__

#define RJ_INFO_REC_FIRST 1
#define RJ_INFO_REC_LAST  2
#define RJ_INFO_FLD_FIRST 4
#define RJ_INFO_FLD_LAST  8

struct recordjar
{
    int size;
    void *jar, *rec;
};

typedef void rj_mapfold_func(int info, char** field, char** value,
    void* state, struct recordjar* rj);

int  rj_load(const char* file, struct recordjar* rj);
int  rj_save(const char* file, struct recordjar* rj);
void rj_free(struct recordjar* rj);

void rj_mapfold(rj_mapfold_func* func, void* state, struct recordjar* rj);

#define RJ_GET(Name) \
    char* rj_##Name( \
        const char* key, const char* keyval, \
        const char* field, const char* def, \
        struct recordjar* rj);

RJ_GET(get)
RJ_GET(get_next)
RJ_GET(get_prev)
RJ_GET(get_only)

#define RJ_ADD(Name) \
    int rj_##Name( \
        const char* key, const char* keyval, \
        const char* field, const char* value, \
        struct recordjar* rj);

RJ_ADD(add)
RJ_ADD(add_next)
RJ_ADD(add_prev)
RJ_ADD(add_only)

#define RJ_SET(Name) \
    int rj_##Name( \
        const char* key, const char* keyval, \
        const char* field, const char* value, \
        struct recordjar* rj);

RJ_SET(set)
RJ_SET(set_next)
RJ_SET(set_prev)
RJ_SET(set_only)

#define RJ_APP(Name) \
    int rj_##Name( \
        const char* key, const char* keyval, \
        const char* field, const char* value, const char* delim, \
        struct recordjar* rj);

RJ_APP(app)
RJ_APP(app_next)
RJ_APP(app_prev)
RJ_APP(app_only)

#define RJ_DEL_RECORD(Name) \
    int rj_##Name(const char* key, const char* keyval, struct recordjar* rj);

RJ_DEL_RECORD(del_record)
RJ_DEL_RECORD(del_record_next)
RJ_DEL_RECORD(del_record_prev)
RJ_DEL_RECORD(del_record_only)

#define RJ_DEL_FIELD(Name) \
    int rj_##Name(const char* key, const char* keyval, const char* field, struct recordjar* rj);

RJ_DEL_FIELD(del_field)
RJ_DEL_FIELD(del_field_next)
RJ_DEL_FIELD(del_field_prev)
RJ_DEL_FIELD(del_field_only)

#endif
