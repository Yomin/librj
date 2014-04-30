/*
 * This source file is part of the librj c library.
 *
 * Copyright (c) 2012,2014 Martin Rödel aka Yomin
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

#define _GNU_SOURCE

#include "rj.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef NDEBUG
#   define DEBUG(x) x
#else
#   define DEBUG(x) while(0)
#endif

#define PREV_FIELD   1
#define PREV_COMMENT 2

#define MOD_MASK_DIR ((1<<4)-1)
#define MOD_THIS (1<<0)
#define MOD_NEXT (1<<1)
#define MOD_PREV (1<<2)
#define MOD_ONLY (1<<3)

#define MOD_MASK_METHOD (((1<<10)-1) & ~MOD_MASK_DIR)
#define MOD_GET (1<<4)
#define MOD_SET (1<<5)
#define MOD_APP (1<<6)
#define MOD_ADD (1<<7)
#define MOD_DEL (1<<8)
#define MOD_DEL_REC (1<<9)

int trim(char** str);
int escape(char** dest, const char* src, const char* delim);
int escape_rev(char** dest, const char* src, const char* delim);
char* mod(int mode, const char* key, const char* keyval,
    const char* field, const char* elem1, const char* elem2, struct recordjar* rj);

struct chain_field
{
    LIST_ENTRY(chain_field) chain;
    char *field, *value;
};
LIST_HEAD(record, chain_field);

struct chain_record
{
    CIRCLEQ_ENTRY(chain_record) chain;
    struct record rec;
};
CIRCLEQ_HEAD(jar, chain_record);


int rj_load(const char* file, struct recordjar* rj)
{
    FILE* fp = fopen(file, "r");
    if(!fp)
        return errno;
    
    struct jar* j = (struct jar*) malloc(sizeof(struct jar));
    rj->jar = j;
    CIRCLEQ_INIT(j);
    
    struct chain_record* cr = (struct chain_record*) malloc(sizeof(struct chain_record));
    CIRCLEQ_INSERT_TAIL(j, cr, chain);
    struct record* r = &cr->rec;
    rj->rec = cr;
    LIST_INIT(r);
    
    rj->size = 0;
    rj->field = 0;
    
    char* line = 0;
    size_t size;
    int count, prevtype = 0, encoding = 0;
    struct chain_field* f;
    
    while((count = getline(&line, &size, fp)) != -1)
    {
        if(!encoding)
        {
            encoding = 1;
            if(count >= 2 && line[0] == '%' && line[1] == '%')
            {
                DEBUG(printf("[RJ] check encoding\n"));
                char* enc = line+2;
                char* field = strtok(enc, ":");
                char* value = strtok(0, ":");
                if(field[0] == ':' || !value)
                    DEBUG(printf("  no encoding signature\n"));
                else
                {
                    trim(&field);
                    if(!strcmp(field, "encoding"))
                    {
                        if(trim(&value))
                        {
                            if(!strcmp(value, "US-ASCII"))
                            {
                                DEBUG(printf("  US-ASCII\n"));
                                continue;
                            }
                            else
                            {
                                DEBUG(printf("  no supported encoding signature\n"));
                                return RJ_ERROR_ENCODING_UNSUPPORTED;
                            }
                        }
                        else
                        {
                            DEBUG(printf("  invalid encoding signature\n"));
                            return RJ_ERROR_ENCODING_INVALID;
                        }
                    }
                    else
                        DEBUG(printf("  no encoding signature\n"));
                }
            }
        }
        
        if(count == 1 && line[0] == '\n')
            DEBUG(printf("[RJ] ignored newline\n"));
        else if(count >= 1 && (line[0] == ' ' || line[0] == '\t'))
        {
            DEBUG(printf("[RJ] fold line\n"));
            if(prevtype == PREV_FIELD)
            {
                char* value = line;
                int newlen = trim(&value);
                if(value[newlen-1] == '\\')
                {
                    DEBUG(printf("  continued\n"));
                    value[newlen-1] = 0;
                    --newlen;
                }
                escape_rev(&f->value, value, ""); // append
            }
            else
                DEBUG(printf("  error beginning fold line\n"));
        }
        else if(count >= 2 && line[0] == '%' && line[1] == '%')
        {
            DEBUG(printf("[RJ] comment\n"));
            if(prevtype == PREV_FIELD)
            {
                DEBUG(printf("  new record\n"));
                cr = (struct chain_record*) malloc(sizeof(struct chain_record));
                CIRCLEQ_INSERT_TAIL(j, cr, chain);
                r = &cr->rec;
                LIST_INIT(r);
            }
            prevtype = PREV_COMMENT;
        }
        else
        {
            DEBUG(printf("[RJ] field\n"));
            char* value;
            char* field = strtok_r(line, ":", &value);
            
            if(field[0] == ':' || !value)
                DEBUG(printf("  error no field name\n"));
            else
            {
                int fieldlen = trim(&field);
                int valuelen = trim(&value);
                if(!valuelen)
                    DEBUG(printf("  error no value\n"));
                else
                {
                    if(value[valuelen-1] == '\\')
                    {
                        DEBUG(printf("  continued\n"));
                        value[valuelen-1] = 0;
                        --valuelen;
                    }
                    
                    f = (struct chain_field*) malloc(sizeof(struct chain_field));
                    LIST_INSERT_HEAD(r, f, chain);
                    f->field = (char*) malloc((fieldlen+1)*sizeof(char));
                    strcpy(f->field, field);
                    f->value = 0;
                    escape_rev(&f->value, value, 0); // overwrite
                    
                    if(!rj->size || prevtype == PREV_COMMENT)
                        ++rj->size;
                    
                    prevtype = PREV_FIELD;
                }
            }
        }
    }
    
    if(prevtype == PREV_COMMENT)
    {
        DEBUG(printf("[RJ] remove empty last record\n"));
        struct chain_record* cr = j->cqh_last;
        CIRCLEQ_REMOVE(j, cr, chain);
        free(cr);
    }
    
    if(line)
        free(line);
    fclose(fp);
    return EXIT_SUCCESS;
}

int rj_save(const char* file, struct recordjar* rj)
{
    FILE* fp = fopen(file, "w");
    if(!fp)
        return errno;
    
    fprintf(fp, "%%%%encoding: US-ASCII\n");
    
    char* buf = 0;
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = j->cqh_first;
    while(1)
    {
        struct chain_field* f = r->rec.lh_first;
        while(f)
        {
            escape(&buf, f->value, 0); // overwrite
            fprintf(fp, "%s: %s\n", f->field, buf);
            f = f->chain.le_next;
        }
        r = r->chain.cqe_next;
        if(r == (void*)j)
            break;
        else
            fprintf(fp, "%%%%\n");
    }
    
    if(buf)
        free(buf);
    fclose(fp);
    return EXIT_SUCCESS;
}

void rj_free(struct recordjar* rj)
{
    struct jar* j = (struct jar*) rj->jar;
    while(j->cqh_first != (void*)j)
    {
        struct chain_record* cr = j->cqh_first;
        struct record* r = &cr->rec;
        struct chain_field* cf = r->lh_first;
        while(cf)
        {
            free(cf->field);
            free(cf->value);
            LIST_REMOVE(cf, chain);
            free(cf);
            cf = r->lh_first;
        }
        CIRCLEQ_REMOVE(j, cr, chain);
        free(cr);
    }
    free(j);
    memset(rj, 0, sizeof(struct recordjar));
}

const char* rj_strerror(int error)
{
    switch(error)
    {
    case RJ_ERROR_ENCODING_INVALID:     return "encoding invalid";
    case RJ_ERROR_ENCODING_UNSUPPORTED: return "encoding unsupported";
    default:                            return strerror(error);
    }
}

void rj_mapfold(rj_mapfold_func* func, void* state, struct recordjar* rj)
{
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = j->cqh_first;
    int rec_first = 1;
    void* tmp = rj->rec;
    
    while(1)
    {
        int fld_first = 1;
        int rec_last = r->chain.cqe_next == (void*)j;
        struct chain_field* f = r->rec.lh_first;
        rj->rec = r;
        while(f)
        {
            int fld_last = f->chain.le_next == 0;
            int info = rec_first | rec_last<<1 | fld_first<<2 | fld_last<<3;
            func(info, &f->field, &f->value, state, rj);
            f = f->chain.le_next;
            fld_first = 0;
        }
        r = r->chain.cqe_next;
        rec_first = 0;
        if(r == (void*)j)
            break;
    }
    rj->rec = tmp;
}

void rj_next(char** field, char** value, struct recordjar* rj)
{
    struct chain_record* cr = rj->rec;
    struct chain_field* cf = rj->field;
    
    if(!cf)
        cf = rj->field = cr->rec.lh_first;
    else
        cf = rj->field = cf->chain.le_next;
    
    if(cf)
    {
        *field = cf->field;
        *value = cf->value;
    }
    else
    {
        *field = 0;
        *value = 0;
    }
}

#define MET_GET(Name, Mode) \
    char* rj_##Name(const char* key, const char* keyval, \
        const char* field, const char* def, struct recordjar* rj) \
    { \
        return mod(MOD_##Mode|MOD_GET, key, keyval, field, def, 0, rj); \
    }

MET_GET(get, THIS)
MET_GET(get_next, NEXT)
MET_GET(get_prev, PREV)
MET_GET(get_only, ONLY)

#define MET_ADD(Name, Mode) \
    int rj_##Name(const char* key, const char* keyval, \
        const char* field, const char* value, struct recordjar* rj) \
    { \
        return !mod(MOD_##Mode|MOD_ADD, key, keyval, field, value, 0, rj); \
    }

MET_ADD(add, THIS)
MET_ADD(add_next, NEXT)
MET_ADD(add_prev, PREV)
MET_ADD(add_only, ONLY)

#define MET_SET(Name, Mode) \
    int rj_##Name(const char* key, const char* keyval, \
        const char* field, const char* value, struct recordjar* rj) \
    { \
        return !mod(MOD_##Mode|MOD_SET, key, keyval, field, value, 0, rj); \
    }

MET_SET(set, THIS)
MET_SET(set_next, NEXT)
MET_SET(set_prev, PREV)
MET_SET(set_only, ONLY)

#define MET_APP(Name, Mode) \
    int rj_##Name(const char* key, const char* keyval, const char* field, \
        const char* value, const char* delim, struct recordjar* rj) \
    { \
        const char* d = delim ? delim : ""; \
        return !mod(MOD_##Mode|MOD_APP, key, keyval, field, value, d, rj); \
    }

MET_APP(app, THIS)
MET_APP(app_next, NEXT)
MET_APP(app_prev, PREV)
MET_APP(app_only, ONLY)

#define MET_DEL_FIELD(Name, Mode) \
    int rj_##Name(const char* key, const char* keyval, \
        const char* field, struct recordjar* rj) \
    { \
        return !mod(MOD_##Mode|MOD_DEL, key, keyval, field, 0, 0, rj); \
    }

MET_DEL_FIELD(del_field, THIS)
MET_DEL_FIELD(del_field_next, NEXT)
MET_DEL_FIELD(del_field_prev, PREV)
MET_DEL_FIELD(del_field_only, ONLY)

#define MET_DEL_RECORD(Name, Mode) \
    int rj_##Name(const char* key, const char* keyval, struct recordjar* rj) \
    { \
        return !mod(MOD_##Mode|MOD_DEL_REC, key, keyval, 0, 0, 0, rj); \
    }

MET_DEL_RECORD(del_record, THIS)
MET_DEL_RECORD(del_record_next, NEXT)
MET_DEL_RECORD(del_record_prev, PREV)
MET_DEL_RECORD(del_record_only, ONLY)

// HELPER

int trim(char** str)
{
    while(**str == ' ' || **str == '\t') ++*str;
    if(**str)
    {
        int len = strlen(*str);
        char* last = *str+len-1;
        if(*last == ' ' || *last == '\t' || *last == '\n')
        {
            *last = 0;
            --last;
            while(last >= *str && (*last == ' ' || *last == '\t')) --last;
            last[1] = 0;
            return last-*str+1;
        }
        return len;
    }
    return 0;
}

// mode == 0 - append
// mode != 0 - overwrite

char* escape_alloc(char** dest, int elen, int mode)
{
    static int size;
    if(!*dest)
    {
        size = elen+1;
        *dest = (char*) malloc(size*sizeof(char));
        return *dest;
    }
    else
    {
        int len = strlen(*dest);
        if(mode)
        {
            size = len+elen+1;
            *dest = (char*) realloc(*dest, size*sizeof(char));
            return *dest+len;
        }
        else if(elen+1 > size)
        {
            size = elen+1;
            *dest = (char*) realloc(*dest, size*sizeof(char));
            return *dest;
        }
        return *dest;
    }
}

int escape_len(const char* str)
{
    int count = 0;
    while(*str)
    {
        switch(*str)
        {
            case '\n': case '\r': case '\t': case '&': case '\\':
                ++str;
                count += 2;
                break;
            default:
                ++str;
                ++count;
        }
    }
    if(str[-1] == ' ') // continuation
        ++count;
    return count;
}

int escape_len_rev(const char* str)
{
    int count = 0;
    while(str[0])
    {
        if(str[0] == '\\')
        {
            switch(str[1])
            {
                case 'n': case 'r': case 't': case '&': case '\\':
                    ++count;
                    ++str;
            }
            ++str;
        }
        else
        {
            ++count;
            ++str;
        }
    }
    return count;
}

char* escape_copy(char* dest, const char* src)
{
    while(*src)
    {
        switch(*src)
        {
            case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
            case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
            case '\t': *dest++ = '\\'; *dest++ = 't'; break;
            case '&':  *dest++ = '\\'; *dest++ = '&'; break;
            case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
            default: *dest++ = *src;
        }
        ++src;
    }
    *dest = 0;
    return dest;
}

// delim == 0: dest = src
// delim != 0: dest = dest delim src

int escape(char** dest, const char* src, const char* delim)
{
    int len;
    char* dptr;
    if(delim)
    {
        len = escape_len(src) + escape_len(delim);
        dptr = escape_alloc(dest, len, 1);
        dptr = escape_copy(dptr, delim);
    }
    else
    {
        len = escape_len(src);
        dptr = escape_alloc(dest, len, 0);
    }
    dptr = escape_copy(dptr, src);
    if(dptr[-1] == ' ') // continuation
    {
        dptr[0] = '\\';
        dptr[1] = '\0';
    }
    return len;
}

char* escape_rev_copy(char* dest, const char* src)
{
    while(src[0])
    {
        if(src[0] == '\\')
        {
            switch(src[1])
            {
                case 'n':  *dest++ = '\n'; ++src; break;
                case 'r':  *dest++ = '\r'; ++src; break;
                case 't':  *dest++ = '\t'; ++src; break;
                case '&':  *dest++ = '&';  ++src; break;
                case '\\': *dest++ = '\\'; ++src; break;
            }
            ++src;
        }
        else
            *dest++ = *src++;
    }
    *dest = 0;
    return dest;
}

// delim == 0: dest = src
// delim != 0: dest = dest delim src

int escape_rev(char** dest, const char* src, const char* delim)
{
    int len;
    char* dptr;
    if(delim)
    {
        len = escape_len_rev(src) + escape_len_rev(delim);
        dptr = escape_alloc(dest, len, 1);
        dptr = escape_rev_copy(dptr, delim);
    }
    else
    {
        len = escape_len_rev(src);
        dptr = escape_alloc(dest, len, 0);
    }
    escape_rev_copy(dptr, src);
    return len;
}

char* mod(int mode, const char* key, const char* keyval,
    const char* field, const char* elem1, const char* elem2, struct recordjar* rj)
{
    int found = 0;
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = (struct chain_record*) rj->rec;
    struct chain_field *f = 0, *modf;
    
    if(!r)
        goto notfound;
    
    while(1)
    {
        switch(mode & MOD_MASK_DIR)
        {
            case MOD_NEXT:
                r = r->chain.cqe_next;
                if(r == (void*)j)
                    r = j->cqh_first;
                break;
            case MOD_PREV:
                r = r->chain.cqe_prev;
                if(r == (void*)j)
                    r = j->cqh_last;
                break;
            case MOD_ONLY:
                if(f)
                    goto notfound;
                break;
        }
        if(!(mode & MOD_THIS) && !(mode & MOD_ONLY) && r == rj->rec)
            goto notfound;
        if(mode & MOD_THIS)
            mode = (mode & ~MOD_THIS) | MOD_NEXT;
        
        f = r->rec.lh_first;
        while(f)
        {
            if(found != 2 && (!field || !strcmp(f->field, field)))
            {
                modf = f;
                if(found)
                    goto found;
                found = 2;
            }
            if(found != 1 && (!key || !strcmp(f->field, key)) &&
                (!keyval || !strcmp(f->value, keyval)))
            {
                if(found || (mode & MOD_DEL_REC))
                    goto found;
                found = 1;
            }
            if(!f->chain.le_next && found == 1 && (mode & MOD_ADD))
                goto found;
            f = f->chain.le_next;
        }
        f = r->rec.lh_first; // stop of *_only
        found = 0;
    }
    
notfound:
    switch(mode & MOD_MASK_METHOD)
    {
        case MOD_GET:
            return (char*) elem1;
        case MOD_ADD:
            // add new record
            r = (struct chain_record*) malloc(sizeof(struct chain_record));
            CIRCLEQ_INSERT_HEAD(j, r, chain);
            LIST_INIT(&r->rec);
            // add key
            f = (struct chain_field*) malloc(sizeof(struct chain_field));
            LIST_INSERT_HEAD(&r->rec, f, chain);
            f->field = (char*) malloc((strlen(key)+1)*sizeof(char));
            strcpy(f->field, key);
            f->value = (char*) malloc((strlen(keyval)+1)*sizeof(char));
            strcpy(f->value, keyval);
            // add new elem
            goto found;
        default:
            return 0;
    }
    
found:
    rj->rec = r;
    rj->field = 0;
    switch(mode & MOD_MASK_METHOD)
    {
        case MOD_GET:
            return modf->value;
        case MOD_SET:
            modf->value = (char*) malloc((strlen(elem1)+1)*sizeof(char));
            strcpy(modf->value, elem1);
            return f->value;
        case MOD_APP:
        {
            int len = strlen(modf->value);
            int dlen = strlen(elem2);
            modf->value = (char*) realloc(modf->value, (len+dlen+strlen(elem1)+1)*sizeof(char));
            strcpy(modf->value+len, elem2);
            strcpy(modf->value+len+dlen, elem1);
            return f->value;
        }
        case MOD_DEL:
            free(modf->field);
            free(modf->value);
            LIST_REMOVE(modf, chain);
            free(modf);
            return (char*) key;
        case MOD_DEL_REC:
            f = r->rec.lh_first;
            while(f)
            {
                free(f->field);
                free(f->value);
                LIST_REMOVE(f, chain);
                free(f);
                f = r->rec.lh_first;
            }
            CIRCLEQ_REMOVE(j, r, chain);
            rj->rec = j->cqh_first;
            free(r);
            return (char*) key;
        case MOD_ADD:
            f = (struct chain_field*) malloc(sizeof(struct chain_field));
            LIST_INSERT_HEAD(&r->rec, f, chain);
            f->field = (char*) malloc((strlen(field)+1)*sizeof(char));
            strcpy(f->field, field);
            f->value = (char*) malloc((strlen(elem1)+1)*sizeof(char));
            strcpy(f->value, elem1);
            return f->value;
    }
    return 0;
}

// TEST

#ifdef TEST

struct show_state
{
    int rc, fc;
};

void show_func(int info, char** field, char** value,
    void* vstate, struct recordjar* rj)
{
    struct show_state* state = (struct show_state*) vstate;
    if(info & RJ_INFO_FLD_FIRST)
    {
        printf("record %i:\n", ++state->rc);
        state->fc = 0;
    }
    printf("    field %i: %s: %s\n", ++state->fc, *field, *value);
}

int main(int argc, char* argv[])
{
    char* file;
    int test = 0;
    switch(argc)
    {
        case 1: file = (char*) "test.rj"; test = 1; break;
        case 2: file = argv[1]; break;
        default: printf("Usage: %s [<file>]\n", argv[0]); return 1;
    }
    
    struct recordjar rj;
    if(rj_load(file, &rj))
        return -1;
    
    printf("%i records\n\n", rj.size);
    
    struct show_state state;
    state.rc = 0;
    state.fc = 0;
    rj_mapfold(show_func, &state, &rj);
    
    if(test)
    {
        printf("value1_r1: %s\n", rj_get("same", "bla", "field1", "not found", &rj));
        printf("value1_r1: %s\n", rj_get("same", "bla", "field1", "not found", &rj));
        printf("value1_r2: %s\n", rj_get_next("same", "bla", "field1", "not found", &rj));
        printf("value1_r2: %s\n", rj_get("same", "bla", "field1", "not found", &rj));
        printf("value1_r1: %s\n", rj_get_prev("same", "bla", "field1", "not found", &rj));
        printf("value1_r1: %s\n", rj_get("same", "bla", "field1", "not found", &rj));
        printf("value1_r2: %s\n", rj_get_prev("same", "bla", "field1", "not found", &rj));
        printf("value1_r1: %s\n", rj_get_only("same", "bla", "field1", "not found", &rj));
        printf("not found: %s\n", rj_get_only("field2", "value2", "field1", "not found", &rj));
        
        printf("not found: %s\n", rj_get("nothere", "bla", "field1", "not found", &rj));
        printf("not found: %s\n", rj_get("same", "nothere", "field1", "not found", &rj));
        printf("not found: %s\n", rj_get("same", "bla", "nothere", "not found", &rj));
        
        rj_add("field2", "value2", "new field", "new value", &rj);
        printf("new value: %s\n", rj_get("field2", "value2", "new field", "not found", &rj));
        rj_add("notexisting", "keyval", "new one", "new value", &rj);
        printf("keyval: %s\n", rj_get("new one", "new value", "notexisting", "not found", &rj));
        
        rj_set("field2", "value2", "new field", "other", &rj);
        printf("other: %s\n", rj_get("field2", "value2", "new field", "not found", &rj));
        
        rj_app("field2", "value2", "new field", "one", " \\ ", &rj);
        printf("other \\ one: %s\n", rj_get("field2", "value2", "new field", "not found", &rj));
        
        rj_del_field("field2", "value2", "field1", &rj);
        printf("not found: %s\n", rj_get("field2", "value2", "field1", "not found", &rj));
        rj_del_record("new one", "new value", &rj);
        printf("not found: %s\n", rj_get("new one", "new value", "notexisting", "not found", &rj));
        
        rj_save("test.test", &rj) ? printf("not saved\n") : printf("saved\n");
    }
    
    rj_free(&rj);
    
    return 0;
}

#endif
