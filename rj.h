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

char* rj_get(
    const char* key, const char* keyval,
    const char* field, const char* def,
    struct recordjar* rj);

char* rj_get_next(
    const char* key, const char* keyval,
    const char* field, const char* def,
    struct recordjar* rj);

char* rj_get_prev(
    const char* key, const char* keyval,
    const char* field, const char* def,
    struct recordjar* rj);

int rj_add(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_add_next(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_add_prev(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_set(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_set_next(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_set_prev(
    const char* key, const char* keyval,
    const char* field, const char* value,
    struct recordjar* rj);

int rj_app(
    const char* key, const char* keyval,
    const char* field, const char* value, const char* delim,
    struct recordjar* rj);

int rj_app_next(
    const char* key, const char* keyval,
    const char* field, const char* value, const char* delim,
    struct recordjar* rj);

int rj_app_prev(
    const char* key, const char* keyval,
    const char* field, const char* value, const char* delim,
    struct recordjar* rj);

int rj_del_record(
    const char* key, const char* keyval, struct recordjar* rj);

int rj_del_record_next(
    const char* key, const char* keyval, struct recordjar* rj);

int rj_del_record_prev(
    const char* key, const char* keyval, struct recordjar* rj);

int rj_del_field(
    const char* key, const char* keyval, const char* field, struct recordjar* rj);

int rj_del_field_next(
    const char* key, const char* keyval, const char* field, struct recordjar* rj);

int rj_del_field_prev(
    const char* key, const char* keyval, const char* field, struct recordjar* rj);

#endif
