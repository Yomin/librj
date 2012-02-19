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

#include "librj.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef NDEBUG
#   define DEBUG(x) x
#else
#   define DEBUG(x) while(0)
#endif

#define PREV_FIELD   1
#define PREV_COMMENT 2

#define ESCAPE_APPEND    0
#define ESCAPE_OVERWRITE 1

#define MOD_MASK_DIR ((1<<3)-1)
#define MOD_THIS (1<<0)
#define MOD_NEXT (1<<1)
#define MOD_PREV (1<<2)

#define MOD_MASK_METHOD (((1<<8)-1) & ~MOD_MASK_DIR)
#define MOD_GET (1<<3)
#define MOD_SET (1<<4)
#define MOD_ADD (1<<5)
#define MOD_DEL (1<<6)
#define MOD_DEL_REC (1<<7)

int trim(char** str);
int escape(char** dest, const char* src, int mode);
int escape_rev(char** dest, const char* src, int mode);
char* mod(int mode, const char* key, const char* keyval,
    const char* field, const char* elem, struct recordjar* rj);

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
        return -1;
    
    struct jar* j = (struct jar*) malloc(sizeof(struct jar));
    rj->jar = j;
    CIRCLEQ_INIT(j);
    
    struct chain_record* cr = (struct chain_record*) malloc(sizeof(struct chain_record));
    CIRCLEQ_INSERT_HEAD(j, cr, chain);
    struct record* r = &cr->rec;
    rj->rec = r;
    LIST_INIT(r);
    
    rj->size = 0;
    
    char* line = 0;
    size_t size;
    int count, prevtype = 0, encoding = 0;
    struct chain_field* f;
    struct chain_record* prevrec = 0;
    while((count = getline(&line, &size, fp)) != -1)
    {
        if(!encoding)
        {
            encoding = 1;
            if(count >= 2 && line[0] == '%' && line[1] == '%')
            {
                DEBUG(printf("check encoding\n"));
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
                                return -3;
                            }
                        }
                        else
                        {
                            DEBUG(printf("  invalid encoding signature\n"));
                            return -2;
                        }
                    }
                    else
                        DEBUG(printf("  no encoding signature\n"));
                }
            }
        }
        
        if(count == 1 && line[0] == '\n')
            DEBUG(printf("ignored newline\n"));
        else if(count >= 1 && (line[0] == ' ' || line[0] == '\t'))
        {
            DEBUG(printf("fold line\n"));
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
                escape_rev(&f->value, value, ESCAPE_APPEND);
            }
            else
                DEBUG(printf("  error beginning fold line\n"));
        }
        else if(count >= 2 && line[0] == '%' && line[1] == '%')
        {
            DEBUG(printf("comment\n"));
            if(prevtype == PREV_FIELD)
            {
                DEBUG(printf("  new record\n"));
                prevrec = cr;
                cr = (struct chain_record*) malloc(sizeof(struct chain_record));
                CIRCLEQ_INSERT_HEAD(j, cr, chain);
                r = &cr->rec;
                LIST_INIT(r);
                rj->rec = cr;
            }
            prevtype = PREV_COMMENT;
        }
        else
        {
            DEBUG(printf("field\n"));
            char* field = strtok(line, ":");
            char* value = strtok(0, ":");
            
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
                    escape_rev(&f->value, value, ESCAPE_OVERWRITE);
                    
                    if(!rj->size || prevtype == PREV_COMMENT)
                        ++rj->size;
                    
                    prevtype = PREV_FIELD;
                }
            }
        }
    }
    
    if(prevtype == PREV_COMMENT)
    {
        DEBUG(printf("remove empty last record\n"));
        struct chain_record* cr = j->cqh_first;
        CIRCLEQ_REMOVE(j, cr, chain);
        free(cr);
        rj->rec = prevrec;
    }
    
    if(line)
        free(line);
    fclose(fp);
    return 0;
}

int rj_save(const char* file, struct recordjar* rj)
{
    FILE* fp = fopen(file, "w");
    if(!fp)
        return -1;
    
    fprintf(fp, "%%%%encoding: US-ASCII\n");
    
    char* buf = 0;
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = j->cqh_first;
    while(1)
    {
        struct chain_field* f = r->rec.lh_first;
        while(f)
        {
            escape(&buf, f->value, ESCAPE_OVERWRITE);
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
    return 0;
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
    rj->jar = 0;
    rj->rec = 0;
    rj->size = 0;
}

void rj_mapfold(rj_mapfold_func* func, void* state, struct recordjar* rj)
{
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = j->cqh_first;
    while(1)
    {
        int next = 1;
        struct chain_field* f = r->rec.lh_first;
        while(f)
        {
            func(next, &f->field, &f->value, state);
            f = f->chain.le_next;
            next = 0;
        }
        r = r->chain.cqe_next;
        if(r == (void*)j)
            break;
    }
}

char* rj_get(const char* key, const char* keyval,
    const char* field, const char* def, struct recordjar* rj)
{
    return mod(MOD_THIS|MOD_GET, key, keyval, field, def, rj);
}

char* rj_get_next(const char* key, const char* keyval,
    const char* field, const char* def, struct recordjar* rj)
{
    return mod(MOD_NEXT|MOD_GET, key, keyval, field, def, rj);
}

char* rj_get_prev(const char* key, const char* keyval,
    const char* field, const char* def, struct recordjar* rj)
{
    return mod(MOD_PREV|MOD_GET, key, keyval, field, def, rj);
}

int rj_add(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_THIS|MOD_ADD, key, keyval, field, value, rj);
}

int rj_add_next(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_NEXT|MOD_ADD, key, keyval, field, value, rj);
}

int rj_add_prev(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_PREV|MOD_ADD, key, keyval, field, value, rj);
}

int rj_set(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_THIS|MOD_SET, key, keyval, field, value, rj);
}

int rj_set_next(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_NEXT|MOD_SET, key, keyval, field, value, rj);
}

int rj_set_prev(const char* key, const char* keyval,
    const char* field, const char* value, struct recordjar* rj)
{
    return !mod(MOD_PREV|MOD_SET, key, keyval, field, value, rj);
}

int rj_del_field(const char* key, const char* keyval,
    const char* field, struct recordjar* rj)
{
    return !mod(MOD_THIS|MOD_DEL, key, keyval, field, 0, rj);
}

int rj_del_field_next(const char* key, const char* keyval,
    const char* field, struct recordjar* rj)
{
    return !mod(MOD_NEXT|MOD_DEL, key, keyval, field, 0, rj);
}

int rj_del_field_prev(const char* key, const char* keyval,
    const char* field, struct recordjar* rj)
{
    return !mod(MOD_PREV|MOD_DEL, key, keyval, field, 0, rj);
}

int rj_del_record(const char* key, const char* keyval, struct recordjar* rj)
{
    return !mod(MOD_THIS|MOD_DEL_REC, key, keyval, 0, 0, rj);
}

int rj_del_record_next(const char* key, const char* keyval, struct recordjar* rj)
{
    return !mod(MOD_NEXT|MOD_DEL_REC, key, keyval, 0, 0, rj);
}

int rj_del_record_prev(const char* key, const char* keyval, struct recordjar* rj)
{
    return !mod(MOD_PREV|MOD_DEL_REC, key, keyval, 0, 0, rj);
}

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
        if(mode == ESCAPE_APPEND)
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

int escape(char** dest, const char* src, int mode)
{
    int elen = escape_len(src);
    char* dptr = escape_alloc(dest, elen, mode);
    while(*src)
    {
        switch(*src)
        {
            case '\n': *dptr++ = '\\'; *dptr++ = 'n'; break;
            case '\r': *dptr++ = '\\'; *dptr++ = 'r'; break;
            case '\t': *dptr++ = '\\'; *dptr++ = 't'; break;
            case '&':  *dptr++ = '\\'; *dptr++ = '&'; break;
            case '\\': *dptr++ = '\\'; *dptr++ = '\\'; break;
            default: *dptr++ = *src;
        }
        ++src;
    }
    if(src[-1] == ' ') // continuation
        *dptr++ = '\\';
    *dptr = 0;
    return elen;
}

int escape_rev(char** dest, const char* src, int mode)
{
    int elen = escape_len_rev(src);
    char* dptr = escape_alloc(dest, elen, mode);
    while(src[0])
    {
        if(src[0] == '\\')
        {
            switch(src[1])
            {
                case 'n': *dptr++ = '\n'; ++src; break;
                case 'r': *dptr++ = '\r'; ++src; break;
                case 't': *dptr++ = '\t'; ++src; break;
                case '&': *dptr++ = '&';  ++src; break;
                case '\\': *dptr++ = '\\'; ++src; break;
            }
            ++src;
        }
        else
            *dptr++ = *src++;
    }
    *dptr = 0;
    return elen;
}

char* mod(int mode, const char* key, const char* keyval,
    const char* field, const char* elem, struct recordjar* rj)
{
    int found = 0;
    struct jar* j = (struct jar*) rj->jar;
    struct chain_record* r = (struct chain_record*) rj->rec;
    struct chain_field *f, *modf;
    
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
        }
        if(!(mode & MOD_THIS) && r == rj->rec)
            goto notfound;
        if(mode & MOD_THIS)
            mode = (mode & ~MOD_THIS) | MOD_NEXT;
        
        f = r->rec.lh_first;
        while(f)
        {
            if(!strcmp(f->field, key) && !strcmp(f->value, keyval))
            {
                if(found || (mode & MOD_DEL_REC))
                    goto found;
                found = 1;
            }
            else if(field && !strcmp(f->field, field))
            {
                modf = f;
                if(found)
                    goto found;
                found = 2;
            }
            if(!f->chain.le_next && found == 1 && (mode & MOD_ADD))
                goto found;
            f = f->chain.le_next;
        }
        found = 0;
    }
    
notfound:
    switch(mode & MOD_MASK_METHOD)
    {
        case MOD_GET:
            return (char*) elem;
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
            f->value = 0;
            escape_rev(&f->value, keyval, ESCAPE_OVERWRITE);
            // add new elem
            goto found;
        default:
            return 0;
    }
    
found:
    rj->rec = r;
    switch(mode & MOD_MASK_METHOD)
    {
        case MOD_GET:
            return modf->value;
        case MOD_SET:
            escape_rev(&modf->value, elem, ESCAPE_OVERWRITE);
            return f->value;
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
            f->value = 0;
            escape_rev(&f->value, elem, ESCAPE_OVERWRITE);
            return f->value;
    }
    return 0;
}

// DEBUG

#ifndef NDEBUG

struct show_state
{
    int rc, fc;
};

void show_func(int next, char** field, char** value, void* vstate)
{
    struct show_state* state = (struct show_state*) vstate;
    if(next)
    {
        printf("record %i:\n", state->rc++);
        state->fc = 0;
    }
    printf("    field %i: %s: %s\n", state->fc++, *field, *value);
}

int main(int argc, char* argv[])
{
    struct recordjar rj;
    if(rj_load("test.rj", &rj))
        return -1;
    
    printf("%i records\n", rj.size);
    
    struct show_state state;
    state.rc = 0;
    state.fc = 0;
    rj_mapfold(show_func, &state, &rj);
    
    rj_save("test.test", &rj) ? printf("not saved\n") : printf("saved\n");
    
    printf("%s\n", rj_get("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get_next("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get_prev("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get("same", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get_prev("same", "bla", "field1", "not found", &rj));
    
    printf("%s\n", rj_get("nothere", "bla", "field1", "not found", &rj));
    printf("%s\n", rj_get("same", "nothere", "field1", "not found", &rj));
    printf("%s\n", rj_get("same", "bla", "nothere", "not found", &rj));
    
    rj_add("field2", "value2", "new field", "new value", &rj);
    printf("%s\n", rj_get("field2", "value2", "new field", "not found", &rj));
    rj_add("notexisting", "keyval", "new one", "new value", &rj);
    printf("%s\n", rj_get("new one", "new value", "notexisting", "not found", &rj));
    
    rj_set("field2", "value2", "new field", "other", &rj);
    printf("%s\n", rj_get("field2", "value2", "new field", "not found", &rj));
    
    rj_del_field("field2", "value2", "new field", &rj);
    printf("%s\n", rj_get("field2", "value2", "new field", "not found", &rj));
    rj_del_record("new one", "new value", &rj);
    printf("%s\n", rj_get("new one", "new value", "notexisting", "not found", &rj));
    
    rj_free(&rj);
    
    return 0;
}

#endif
