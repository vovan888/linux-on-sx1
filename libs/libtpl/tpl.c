/*
Copyright (c) 2005-2007, Troy Hanson     http://tpl.sourceforge.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

static const char id[]="$Id: tpl.c 121 2007-04-27 05:53:31Z thanson $";


#include <stdlib.h>  /* malloc */
#include <stdarg.h>  /* va_list */
#include <string.h>  /* memcpy, memset, strchr */
#include <stdio.h>   /* printf (tpl_hook.oops default function) */

#include <unistd.h>     /* for ftruncate */
#include <sys/types.h>  /* for 'open' */
#include <sys/stat.h>   /* for 'open' */
#include <fcntl.h>      /* for 'open' */
#include <errno.h>
#include <inttypes.h>   /* uint32_t, uint64_t, etc */

#if ( defined __CYGWIN__ || defined __MINGW32__ )
#include <win/mman.h>   /* mmap */
#else
#include <sys/mman.h>   /* mmap */
#endif

#include "tpl.h"

#define TPL_GATHER_BUFLEN 8192

/* Internal prototypes */
static tpl_node *tpl_node_new(tpl_node *parent);
static tpl_node *tpl_find_i(tpl_node *n, int i);
static void *tpl_cpv(void *datav, void *data, size_t sz);
static void *tpl_extend_backbone(tpl_node *n);
static char *tpl_fmt(tpl_node *r);
static void *tpl_dump_atyp(tpl_node *n, tpl_atyp* at, void *dv);
static size_t tpl_ser_osz(tpl_node *n);
static void tpl_free_atyp(tpl_node *n,tpl_atyp *atyp);
static int tpl_dump_to_mem(tpl_node *r, void *addr, size_t sz);
static int tpl_mmap_file(char *filename, tpl_mmap_rec *map_rec);
static int tpl_mmap_output_file(char *filename, size_t sz, void **text_out);
static int tpl_cpu_bigendian(void);
static int tpl_needs_endian_swap(void *);
static void tpl_byteswap(void *word, int len);
static void tpl_fatal(char *fmt, ...);
static int tpl_serlen(tpl_node *r, tpl_node *n, void *dv, size_t *serlen);
static int tpl_unpackA0(tpl_node *r);
static int tpl_oops(const char *fmt, ...);
static int tpl_gather_mem( char *buf, size_t len, tpl_gather_t **gs, tpl_gather_cb *cb, void *data);
static int tpl_gather_nonblocking( int fd, tpl_gather_t **gs, tpl_gather_cb *cb, void *data);
static int tpl_gather_blocking(int fd, void **img, size_t *sz);

/* This is used internally to help calculate padding when a 'double' 
 * follows a smaller datatype in a structure. Normally under gcc
 * on x86, d will be aligned at +4, however use of -malign-double
 * causes d to be aligned at +8 (this is actually faster on x86).
 * Also SPARC and x86_64 seem to align always on +8. 
 */
struct tpl_alignment_detector {
    char a;
    double d;  /* some platforms align this on +4, others on +8 */
};


/* Hooks for customizing tpl mem alloc, error handling, etc. Set defaults. */
tpl_hook_t tpl_hook = {
    .oops = tpl_oops,
    .malloc = malloc,
    .realloc = realloc,
    .free = free,
    .fatal = tpl_fatal,
    .gather_max = 0 /* max tpl size (bytes) for tpl_gather */
};

static const char tpl_fmt_chars[] = "AS(iucsBfIU#)"; /* all valid format chars */
static const char tpl_S_fmt_chars[] = "iucsfIU#";    /* valid within S(...) */
static const struct tpl_type_t tpl_types[] = {
    [TPL_TYPE_ROOT] =    {'r', 0},
    [TPL_TYPE_INT32] =   {'i', sizeof(int32_t)},
    [TPL_TYPE_UINT32] =  {'u', sizeof(uint32_t)},
    [TPL_TYPE_BYTE] =    {'c', sizeof(char)},
    [TPL_TYPE_STR] =     {'s', sizeof(char*)},
    [TPL_TYPE_ARY] =     {'A', 0},
    [TPL_TYPE_BIN] =     {'B', 0},
    [TPL_TYPE_DOUBLE] =  {'f', 8},  /* not sizeof(double) as that varies */
    [TPL_TYPE_INT64] =   {'I', sizeof(int64_t)},
    [TPL_TYPE_UINT64] =  {'U', sizeof(uint64_t)},
};

/* default error-reporting function. Just writes to stderr. */
static int tpl_oops(const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    va_end(ap);
    return 0;
}


static tpl_node *tpl_node_new(tpl_node *parent) {
    tpl_node *n;
    if ((n=tpl_hook.malloc(sizeof(tpl_node))) == NULL) {
        tpl_hook.fatal("out of memory\n");
    }
    n->addr=NULL;
    n->data=NULL;
    n->num=1;
    n->ser_osz=0;
    n->children=NULL;
    n->next=NULL;
    n->parent=parent;
    return n;
}

/* Used in S(..) formats to pack several fields from a structure based on 
 * only the structure address. We need to calculate field addresses 
 * manually taking into account the size of the fields and intervening padding.
 * The wrinkle is that double is not normally aligned on x86-32 but the
 * -malign-double compiler option causes it to be. Double are aligned
 * on Sparc, and apparently on 64 bit x86. We use a helper structure 
 * to detect whether double is aligned in this compilation environment.
 */
char *calc_field_addr(tpl_node *parent, int type,char *struct_addr, int ordinal) {
    tpl_node *prev;
    int padding=0;
    int align_sz;

    if (ordinal == 1) return struct_addr;  /* first field starts on structure address */

    /* generate enough padding so field addr is divisible by it's align_sz. 4, 8, etc */
    prev = parent->children->prev; 
    if (type != TPL_TYPE_DOUBLE) align_sz = tpl_types[type].sz;
    else align_sz = sizeof(struct tpl_alignment_detector) > 12 ? 8 : 4; 
    while ((((long)prev->addr + (tpl_types[prev->type].sz * prev->num) + padding) 
        % align_sz) != 0) padding++;
    return (char*)((long)prev->addr + (tpl_types[prev->type].sz * prev->num) + padding);
}

TPL_API tpl_node *tpl_map(char *fmt,...) {
    va_list ap;
    va_start(ap,fmt);

    return tpl_vmap(fmt, ap);
}

TPL_API tpl_node *tpl_vmap(char *fmt, va_list ap) {
    int lparen_level=0,expect_lparen=0,t,in_structure=0,ordinal,infer=0;
    char *c, *struct_addr;
/*    va_list ap; */
    tpl_node *root,*parent,*n,*preceding;
    tpl_pidx *pidx;
    int *fxlens, num_fxlens;
    void *fxlens_img; 
    uint32_t f;
    size_t spn;

    /* the internal infer_2 mode is is used to implement the load-stage of
     * 'structure-wildcard' unpacking. (see below). Not the normal case. */
    if (!strcmp(fmt,"#infer_2")) {
        infer=1;
        root = (tpl_node*)va_arg(ap,void*);  /* arg #1*/
        fmt = va_arg(ap,char*);              /* arg #2 */
        fxlens_img = va_arg(ap,void*);       /* arg #3 (not uint32_t aligned) */

        /* quick check that the fmt =~ /^S\([iucsfIU#]+\)$/ */
        if (strncmp(fmt,"S(",2)) return NULL;
        spn = strspn(fmt+2, tpl_S_fmt_chars);
        if (strcmp(fmt+2+spn,")")) return NULL;

        struct_addr = root->addr;
        root->addr = NULL;
        ((tpl_root_data*)root->data)->flags &= ~TPL_INFER_FMT;
    } else {
        /* normal case */
        root = tpl_node_new(NULL);
        root->type = TPL_TYPE_ROOT; 
        root->data = (tpl_root_data*)tpl_hook.malloc(sizeof(tpl_root_data));
        if (!root->data) tpl_hook.fatal("out of memory\n");
        memset((tpl_root_data*)root->data,0,sizeof(tpl_root_data));
    }

    /* Inferred-format mode is invoked by passing "S(*)" as the format
     * and the only vararg is a structure address. Later when tpl_load() is
     * invoked, the real format string [which must be of the form S(...)] will
     * be read from the image by tpl_sanity, at which point this function will
     * be re-invoked in #infer_2 mode to properly build the node tree. */
    if (!strcmp(fmt,"S(*)")) {
        root->addr = va_arg(ap,void*);       /* struct addr */
        ((tpl_root_data*)root->data)->flags |= TPL_INFER_FMT;
        return root;
    }

    /* set up root nodes special ser_osz to reflect overhead of preamble */
    root->ser_osz =  sizeof(uint32_t); /* tpl leading length */
    root->ser_osz += strlen(fmt) + 1;  /* fmt + NUL-terminator */
    root->ser_osz += 4;                /* 'tpl' magic prefix + flags byte */

    parent=root;

    c=fmt;
    while (*c != '\0') {
        switch (*c) {
            case 'c':
            case 'i':
            case 'u':
            case 'I':
            case 'U':
            case 'f':
                if (*c=='c') t=TPL_TYPE_BYTE;
                if (*c=='i') t=TPL_TYPE_INT32;
                if (*c=='u') t=TPL_TYPE_UINT32;
                if (*c=='I') t=TPL_TYPE_INT64;
                if (*c=='U') t=TPL_TYPE_UINT64;
                if (*c=='f') t=TPL_TYPE_DOUBLE;

                if (expect_lparen) goto fail;
                n = tpl_node_new(parent);
                n->type = t;
                if (in_structure) {
                    n->addr = calc_field_addr(parent,n->type,struct_addr,ordinal++);
                } else n->addr = (void*)va_arg(ap,void*);
                n->data = tpl_hook.malloc(tpl_types[t].sz);
                if (!n->data) tpl_hook.fatal("out of memory\n");
                if (n->parent->type == TPL_TYPE_ARY) 
                    ((tpl_atyp*)(n->parent->data))->sz += tpl_types[t].sz;
                DL_ADD(parent->children,n);
                break;
            case '#':
                /* apply a 'num' to preceding atom */
                if (!parent->children) goto fail;
                preceding = parent->children->prev; /* first child's prev is 'last child'*/
                t = preceding->type;
                if (!(t == TPL_TYPE_BYTE   || t == TPL_TYPE_INT32 ||
                      t == TPL_TYPE_UINT32 || t == TPL_TYPE_DOUBLE ||
                      t == TPL_TYPE_UINT64 || t == TPL_TYPE_INT64 )) goto fail;
                if (infer) {
                    memcpy(&f, fxlens_img, sizeof(uint32_t)); /* copy to aligned */
                    fxlens_img = (void*)((long)fxlens_img + sizeof(uint32_t)); 
                    if (((tpl_root_data*)root->data)->flags & TPL_XENDIAN)
                        tpl_byteswap(&f,sizeof(uint32_t));
                    preceding->num = f;
                } else {
                    preceding->num = va_arg(ap,int);
                }
                root->ser_osz += sizeof(uint32_t); 

                num_fxlens = ++(((tpl_root_data*)root->data)->num_fxlens);
                fxlens = ((tpl_root_data*)root->data)->fxlens;
                fxlens = tpl_hook.realloc(fxlens, sizeof(int) * num_fxlens);
                if (!fxlens) tpl_hook.fatal("out of memory\n");
                ((tpl_root_data*)root->data)->fxlens = fxlens;
                fxlens[num_fxlens-1] = preceding->num;

                preceding->data = tpl_hook.realloc(preceding->data, 
                    tpl_types[t].sz * preceding->num);
                if (!preceding->data) tpl_hook.fatal("out of memory\n");
                if (n->parent->type == TPL_TYPE_ARY) 
                    ((tpl_atyp*)(n->parent->data))->sz += tpl_types[t].sz * (preceding->num-1);
                break;
            case 'B':
                if (infer) goto fail;
                if (expect_lparen) goto fail;
                if (in_structure) goto fail;
                n = tpl_node_new(parent);
                n->type = TPL_TYPE_BIN;
                n->addr = (tpl_bin*)va_arg(ap,void*);
                n->data = tpl_hook.malloc(sizeof(tpl_bin*));
                if (!n->data) tpl_hook.fatal("out of memory\n");
                *((tpl_bin**)n->data) = NULL;
                if (n->parent->type == TPL_TYPE_ARY) 
                    ((tpl_atyp*)(n->parent->data))->sz += sizeof(tpl_bin);
                DL_ADD(parent->children,n);
                break;
            case 's':
                if (expect_lparen) goto fail;
                n = tpl_node_new(parent);
                n->type = TPL_TYPE_STR;
                if (in_structure) {
                    n->addr = calc_field_addr(parent,n->type,struct_addr,ordinal++);
                } else n->addr = (void*)va_arg(ap,void*);
                n->data = tpl_hook.malloc(sizeof(char*));
                if (!n->data) tpl_hook.fatal("out of memory\n");
                *(char**)(n->data) = NULL;
                if (n->parent->type == TPL_TYPE_ARY) 
                    ((tpl_atyp*)(n->parent->data))->sz += sizeof(void*);
                DL_ADD(parent->children,n);
                break;
            case 'A':
                if (infer) goto fail;
                if (in_structure) goto fail;
                n = tpl_node_new(parent);
                n->type = TPL_TYPE_ARY;
                DL_ADD(parent->children,n);
                parent = n;
                expect_lparen=1;
                pidx = (tpl_pidx*)tpl_hook.malloc(sizeof(tpl_pidx));
                if (!pidx) tpl_hook.fatal("out of memory\n");
                pidx->node = n;
                pidx->next = NULL;
                DL_ADD(((tpl_root_data*)(root->data))->pidx,pidx);
                /* set up the A's tpl_atyp */
                n->data = (tpl_atyp*)tpl_hook.malloc(sizeof(tpl_atyp));
                if (!n->data) tpl_hook.fatal("out of memory\n");
                ((tpl_atyp*)(n->data))->num = 0;
                ((tpl_atyp*)(n->data))->sz = 0;
                ((tpl_atyp*)(n->data))->bb = NULL;
                ((tpl_atyp*)(n->data))->bbtail = NULL;
                ((tpl_atyp*)(n->data))->cur = NULL;
                if (n->parent->type == TPL_TYPE_ARY) 
                    ((tpl_atyp*)(n->parent->data))->sz += sizeof(void*);
                break;
            case 'S':
                if (in_structure) goto fail;
                expect_lparen=1;
                ordinal=1;  /* index upcoming atoms in S(..) */
                in_structure=1+lparen_level; /* so we can tell where S fmt ends */
                struct_addr = infer ? struct_addr : (char*)va_arg(ap,void*);
                break;
            case ')':
                lparen_level--;
                if (lparen_level < 0) goto fail;
                if (*(c-1) == '(') goto fail;
                if (in_structure && (in_structure-1 == lparen_level)) in_structure=0;
                else parent = parent->parent; /* rparen ends A() type, not S() type */
                break;
            case '(':
                if (!expect_lparen) goto fail;
                expect_lparen=0;
                lparen_level++;
                break;
            default:
                tpl_hook.oops("unsupported option %c\n", *c);
                goto fail;
        }
        c++;
    }
    if (lparen_level != 0) goto fail;
    va_end(ap);

    /* copy the format string, save for convenience */
    ((tpl_root_data*)(root->data))->fmt = tpl_hook.malloc(strlen(fmt)+1);
    if (((tpl_root_data*)(root->data))->fmt == NULL) 
        tpl_hook.fatal("out of memory\n");
    memcpy(((tpl_root_data*)(root->data))->fmt,fmt,strlen(fmt)+1);

    return root;

fail:
    va_end(ap);
    tpl_hook.oops("failed to parse %s\n", fmt);
    tpl_free(root);
    return NULL;
}

static int tpl_unmap_file( tpl_mmap_rec *mr) {

    if ( munmap( mr->text, mr->text_sz ) == -1 ) {
        tpl_hook.oops("Failed to munmap: %s\n", strerror(errno));
    }
    close(mr->fd);
    mr->text = NULL;
    mr->text_sz = 0;
    return 0;
}

static void tpl_free_keep_map(tpl_node *r) {
    int mmap_bits = (TPL_RDONLY|TPL_FILE);
    int ufree_bits = (TPL_MEM|TPL_UFREE);
    tpl_node *nxtc,*c;
    int find_next_node=0,looking;
    size_t sz;

    /* For mmap'd files, or for 'ufree' memory images , do appropriate release */
    if ((((tpl_root_data*)(r->data))->flags & mmap_bits) == mmap_bits) {
        tpl_unmap_file( &((tpl_root_data*)(r->data))->mmap); 
    } else if ((((tpl_root_data*)(r->data))->flags & ufree_bits) == ufree_bits) {
        tpl_hook.free( ((tpl_root_data*)(r->data))->mmap.text );
    }

    c = r->children;
    if (c) {
        while(c->type != TPL_TYPE_ROOT) {    /* loop until we come back to root node */
            switch (c->type) {
                case TPL_TYPE_BIN:
                    /* free any binary buffer hanging from tpl_bin */
                    if ( *((tpl_bin**)(c->data)) ) {
                        if ( (*((tpl_bin**)(c->data)))->addr ) {
                            tpl_hook.free( (*((tpl_bin**)(c->data)))->addr );
                        }
                        *((tpl_bin**)c->data) = NULL; /* reset tpl_bin */
                    }
                    find_next_node=1;
                    break;
                case TPL_TYPE_STR:
                    /* free any packed (copied) string */
                    if (*(char**)(c->data)) tpl_hook.free(*(char**)(c->data)); 
                    *(char**)(c->data) = NULL;  /* reset string */
                    find_next_node=1;
                    break;
                case TPL_TYPE_INT32:
                case TPL_TYPE_UINT32:
                case TPL_TYPE_INT64:
                case TPL_TYPE_UINT64:
                case TPL_TYPE_BYTE:
                case TPL_TYPE_DOUBLE:
                    find_next_node=1;
                    break;
                case TPL_TYPE_ARY:
                    c->ser_osz = 0; /* zero out the serialization output size */

                    sz = ((tpl_atyp*)(c->data))->sz;  /* save sz to use below */
                    tpl_free_atyp(c,c->data);

                    /* make new atyp */
                    c->data = (tpl_atyp*)tpl_hook.malloc(sizeof(tpl_atyp));
                    if (!c->data) tpl_hook.fatal("out of memory\n");
                    ((tpl_atyp*)(c->data))->num = 0;
                    ((tpl_atyp*)(c->data))->sz = sz;  /* restore bb datum sz */
                    ((tpl_atyp*)(c->data))->bb = NULL;
                    ((tpl_atyp*)(c->data))->bbtail = NULL;
                    ((tpl_atyp*)(c->data))->cur = NULL;

                    c = c->children; 
                    break;
                default:
                    tpl_hook.fatal("unsupported format character\n");
                    break;
            }

            if (find_next_node) {
                find_next_node=0;
                looking=1;
                while(looking) {
                    if (c->next) {
                        nxtc=c->next;
                        c=nxtc;
                        looking=0;
                    } else {
                        if (c->type == TPL_TYPE_ROOT) break; /* root node */
                        else {
                            nxtc=c->parent;
                            c=nxtc;
                        }
                    }
                }
            }
        }
    }

    ((tpl_root_data*)(r->data))->flags = 0;  /* reset flags */
}

TPL_API void tpl_free(tpl_node *r) {
    int mmap_bits = (TPL_RDONLY|TPL_FILE);
    int ufree_bits = (TPL_MEM|TPL_UFREE);
    tpl_node *nxtc,*c;
    int find_next_node=0,looking;
    tpl_pidx *pidx,*pidx_nxt;

    /* For mmap'd files, or for 'ufree' memory images , do appropriate release */
    if ((((tpl_root_data*)(r->data))->flags & mmap_bits) == mmap_bits) {
        tpl_unmap_file( &((tpl_root_data*)(r->data))->mmap); 
    } else if ((((tpl_root_data*)(r->data))->flags & ufree_bits) == ufree_bits) {
        tpl_hook.free( ((tpl_root_data*)(r->data))->mmap.text );
    }

    c = r->children;
    if (c) {
        while(c->type != TPL_TYPE_ROOT) {    /* loop until we come back to root node */
            switch (c->type) {
                case TPL_TYPE_BIN:
                    /* free any binary buffer hanging from tpl_bin */
                    if ( *((tpl_bin**)(c->data)) ) {
                        if ( (*((tpl_bin**)(c->data)))->sz != 0 ) {
                            tpl_hook.free( (*((tpl_bin**)(c->data)))->addr );
                        }
                        tpl_hook.free(*((tpl_bin**)c->data)); /* free tpl_bin */
                    }
                    tpl_hook.free(c->data);  /* free tpl_bin* */
                    find_next_node=1;
                    break;
                case TPL_TYPE_STR:
                    /* free any packed (copied) string */
                    if (*(char**)(c->data)) tpl_hook.free(*(char**)(c->data)); 
                    tpl_hook.free(c->data);
                    find_next_node=1;
                    break;
                case TPL_TYPE_INT32:
                case TPL_TYPE_UINT32:
                case TPL_TYPE_INT64:
                case TPL_TYPE_UINT64:
                case TPL_TYPE_BYTE:
                case TPL_TYPE_DOUBLE:
                    tpl_hook.free(c->data);
                    find_next_node=1;
                    break;
                case TPL_TYPE_ARY:
                    tpl_free_atyp(c,c->data);
                    if (c->children) c = c->children; /* normal case */
                    else find_next_node=1; /* edge case, handle bad format A() */
                    break;
                default:
                    tpl_hook.fatal("unsupported format character\n");
                    break;
            }

            if (find_next_node) {
                find_next_node=0;
                looking=1;
                while(looking) {
                    if (c->next) {
                        nxtc=c->next;
                        tpl_hook.free(c);
                        c=nxtc;
                        looking=0;
                    } else {
                        if (c->type == TPL_TYPE_ROOT) break; /* root node */
                        else {
                            nxtc=c->parent;
                            tpl_hook.free(c);
                            c=nxtc;
                        }
                    }
                }
            }
        }
    }

    /* free root */
    for(pidx=((tpl_root_data*)(r->data))->pidx; pidx; pidx=pidx_nxt) {
        pidx_nxt = pidx->next;
        tpl_hook.free(pidx);
    }
    tpl_hook.free(((tpl_root_data*)(r->data))->fmt);
    if (((tpl_root_data*)(r->data))->num_fxlens > 0) {
        tpl_hook.free(((tpl_root_data*)(r->data))->fxlens);
    }
    tpl_hook.free(r->data);  /* tpl_root_data */
    tpl_hook.free(r);
}


/* Find the i'th packable ('A' node) */
static tpl_node *tpl_find_i(tpl_node *n, int i) {
    int j=0;
    tpl_pidx *pidx;
    if (n->type != TPL_TYPE_ROOT) return NULL;
    if (i == 0) return n;  /* packable 0 is root */
    for(pidx=((tpl_root_data*)(n->data))->pidx; pidx; pidx=pidx->next) {
        if (++j == i) return pidx->node;
    }
    return NULL;
}

static void *tpl_cpv(void *datav, void *data, size_t sz) {
    if (sz>0) memcpy(datav,data,sz);
    return (void*)((long)datav + sz);
}

static void *tpl_extend_backbone(tpl_node *n) {
    tpl_backbone *bb;
    bb = (tpl_backbone*)tpl_hook.malloc(sizeof(tpl_backbone) +
      ((tpl_atyp*)(n->data))->sz );  /* datum hangs on coattails of bb */
    if (!bb) tpl_hook.fatal("out of memory\n");
    memset(bb->data,0,((tpl_atyp*)(n->data))->sz);
    bb->next = NULL;
    /* Add the new backbone to the tail, also setting head if necessary  */
    if (((tpl_atyp*)(n->data))->bb == NULL) {
        ((tpl_atyp*)(n->data))->bb = bb;
        ((tpl_atyp*)(n->data))->bbtail = bb;
    } else {
        ((tpl_atyp*)(n->data))->bbtail->next = bb;
        ((tpl_atyp*)(n->data))->bbtail = bb;
    }

    ((tpl_atyp*)(n->data))->num++;
    return bb->data;
}

/* Get the format string corresponding to a given tpl (root node) */
static char *tpl_fmt(tpl_node *r) {
    return ((tpl_root_data*)(r->data))->fmt;
}

/* Get the fmt # lengths as a contiguous buffer of ints (length num_fxlens) */
static int *tpl_fxlens(tpl_node *r, int *num_fxlens) {
    *num_fxlens = ((tpl_root_data*)(r->data))->num_fxlens;
    return ((tpl_root_data*)(r->data))->fxlens;
}

/* called when serializing an 'A' type node into a buffer which has
 * already been set up with the proper space. The backbone is walked
 * which was obtained from the tpl_atyp header passed in. 
 */
static void *tpl_dump_atyp(tpl_node *n, tpl_atyp* at, void *dv) {
    tpl_backbone *bb;
    tpl_node *c;
    void *datav;
    uint32_t slen;
    tpl_bin *binp;
    char *strp;
    tpl_atyp *atypp;

    /* handle 'A' nodes */
    dv = tpl_cpv(dv,&at->num,sizeof(uint32_t));  /* array len */
    for(bb=at->bb; bb; bb=bb->next) {
        datav = bb->data;
        for(c=n->children; c; c=c->next) {
            switch (c->type) {
                case TPL_TYPE_BYTE:
                case TPL_TYPE_DOUBLE:
                case TPL_TYPE_INT32:
                case TPL_TYPE_UINT32:
                case TPL_TYPE_INT64:
                case TPL_TYPE_UINT64:
                    dv = tpl_cpv(dv,datav,tpl_types[c->type].sz * c->num);
                    datav = (void*)((long)datav + tpl_types[c->type].sz * c->num);
                    break;
                case TPL_TYPE_BIN:
                    /* dump the buffer length followed by the buffer */
                    memcpy(&binp,datav,sizeof(tpl_bin*)); /* cp to aligned */
                    slen = binp->sz;
                    dv = tpl_cpv(dv,&slen,sizeof(uint32_t));
                    dv = tpl_cpv(dv,binp->addr,slen);
                    datav = (void*)((long)datav + sizeof(tpl_bin*));
                    break;
                case TPL_TYPE_STR:
                    /* dump the string length followed by the string */
                    memcpy(&strp,datav,sizeof(char*)); /* cp to aligned */
                    slen = strlen(strp);
                    dv = tpl_cpv(dv,&slen,sizeof(uint32_t));
                    dv = tpl_cpv(dv,strp,slen);
                    datav = (void*)((long)datav + sizeof(char*));
                    break;
                case TPL_TYPE_ARY:
                    memcpy(&atypp,datav,sizeof(tpl_atyp*)); /* cp to aligned */
                    dv = tpl_dump_atyp(c,atypp,dv);
                    datav = (void*)((long)datav + sizeof(void*));
                    break;
                default:
                    tpl_hook.fatal("unsupported format character\n");
                    break;
            }
        }
    }
    return dv;
}

/* figure the serialization output size needed for tpl whose root is n*/
static size_t tpl_ser_osz(tpl_node *n) {
    tpl_node *c;
    size_t sz;
    tpl_bin *binp;
    char *strp;

    /* handle the root node ONLY (subtree's ser_osz have been bubbled-up) */
    if (n->type != TPL_TYPE_ROOT) {
        tpl_hook.fatal("internal error: tpl_ser_osz on non-root node\n");
    }

    sz = n->ser_osz;    /* start with fixed overhead, already stored */
    for(c=n->children; c; c=c->next) {
        switch (c->type) {
            case TPL_TYPE_BYTE:
            case TPL_TYPE_DOUBLE:
            case TPL_TYPE_INT32:
            case TPL_TYPE_UINT32:
            case TPL_TYPE_INT64:
            case TPL_TYPE_UINT64:
                sz += tpl_types[c->type].sz * c->num;
                break;
            case TPL_TYPE_BIN:
                sz += sizeof(uint32_t);  /* binary buf len */
                memcpy(&binp,c->data,sizeof(tpl_bin*)); /* cp to aligned */
                sz += binp->sz; 
                break;
            case TPL_TYPE_STR:
                sz += sizeof(uint32_t);  /* string len */
                memcpy(&strp,c->data,sizeof(char*)); /* cp to aligned */
                sz += strlen(strp);
                break;
            case TPL_TYPE_ARY:
                sz += sizeof(uint32_t);  /* array len */
                sz += c->ser_osz;        /* bubbled-up child array ser_osz */
                break;
            default:
                tpl_hook.fatal("unsupported format character\n");
                break;
        }
    }
    return sz;
}


TPL_API int tpl_dump(tpl_node *r, int mode, ...) {
    va_list ap;
    char *filename, *bufv;
    void **addr_out,*buf;
    int fd,rc=0;
    size_t sz,*sz_out;

    if (((tpl_root_data*)(r->data))->flags & TPL_RDONLY) {  /* unusual */
        tpl_hook.oops("error: tpl_dump called for a loaded tpl\n");
        /* tpl image is already loaded, could just write it out (TODO) */
        return -1;
    }

    sz = tpl_ser_osz(r); /* compute the size needed to serialize  */

    va_start(ap,mode);
    if (mode & TPL_FILE) {
        filename = va_arg(ap,char*);
        fd = tpl_mmap_output_file(filename, sz, &buf);
        if (fd == -1) rc = -1;
        else {
            rc = tpl_dump_to_mem(r,buf,sz);
            if (msync(buf,sz,MS_SYNC) == -1) {
                tpl_hook.oops("msync failed on fd %d: %s\n", fd, strerror(errno));
            }
            if (munmap(buf, sz) == -1) {
                tpl_hook.oops("munmap failed on fd %d: %s\n", fd, strerror(errno));
            }
            close(fd);
        }
    } else if (mode & TPL_FD) {
        fd = va_arg(ap, int);
        if ( (buf = tpl_hook.malloc(sz)) == NULL) tpl_hook.fatal("out of memory\n");
        tpl_dump_to_mem(r,buf,sz);
        bufv = buf;
        do {
            rc = write(fd,bufv,sz);
            if (rc > 0) {
                sz -= rc;
                bufv += rc;
            } else if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN) continue;
                tpl_hook.oops("error writing to fd %d: %s\n", fd, strerror(errno));
                free(buf);
                return -1;
            }
        } while (sz > 0);
        free(buf);
    } else if (mode & TPL_MEM) {
        addr_out = (void**)va_arg(ap, void*);
        sz_out = va_arg(ap, size_t*);
        if ( (buf = tpl_hook.malloc(sz)) == NULL) tpl_hook.fatal("out of memory\n");
        *sz_out = sz;
        *addr_out = buf;
        rc=tpl_dump_to_mem(r,buf,sz);
    } else {
        tpl_hook.oops("unsupported tpl_dump mode %d\n", mode);
        rc=-1;
    }
    va_end(ap);
    return rc;
}

/* This function expects the caller to have set up a memory buffer of 
 * adequate size to hold the serialized tpl. The sz parameter must be
 * the result of tpl_ser_osz(r).
 */
static int tpl_dump_to_mem(tpl_node *r,void *addr,size_t sz) {
    uint32_t slen;
    int *fxlens, num_fxlens;
    void *dv;
    char *fmt,flags;
    tpl_node *c;

    flags = 0;
    if (tpl_cpu_bigendian()) flags |= TPL_FL_BIGENDIAN;

    dv = addr;
    dv = tpl_cpv(dv,TPL_MAGIC,3);         /* copy tpl magic prefix */
    dv = tpl_cpv(dv,&flags,1);            /* copy flags byte */
    dv = tpl_cpv(dv,&sz,sizeof(uint32_t));/* overall length (inclusive) */
    fmt = tpl_fmt(r);
    dv = tpl_cpv(dv,fmt,strlen(fmt)+1);   /* copy format with NUL-term */
    fxlens = tpl_fxlens(r,&num_fxlens);
    dv = tpl_cpv(dv,fxlens,num_fxlens*sizeof(uint32_t));/* fmt # lengths */

    /* serialize the tpl content, iterating over direct children of root */
    c = r->children;
    while (c) {
        switch (c->type) {
            case TPL_TYPE_BYTE:
            case TPL_TYPE_DOUBLE:
            case TPL_TYPE_INT32:
            case TPL_TYPE_UINT32:
            case TPL_TYPE_INT64:
            case TPL_TYPE_UINT64:
                dv = tpl_cpv(dv,c->data,tpl_types[c->type].sz * c->num);
                break;
            case TPL_TYPE_BIN:
                slen = (*(tpl_bin**)(c->data))->sz;
                dv = tpl_cpv(dv,&slen,sizeof(uint32_t));  /* buffer len */
                dv = tpl_cpv(dv,(*(tpl_bin**)(c->data))->addr,slen); /* buf */
                break;
            case TPL_TYPE_STR:
                slen = strlen(*(char**)c->data);
                dv = tpl_cpv(dv,&slen,sizeof(uint32_t));  /* string len */
                dv = tpl_cpv(dv,*(char**)(c->data),slen); /* string */
                break;
            case TPL_TYPE_ARY:
                dv = tpl_dump_atyp(c,(tpl_atyp*)c->data,dv);
                break;
            default:
                tpl_hook.fatal("unsupported format character\n");
                break;
        }
        c = c->next;
    }

    return 0;
}

static int tpl_cpu_bigendian() {
   unsigned i = 1;
   char *c;
   c = (char*)&i;
   return (c[0] == 1 ? 0 : 1);
}


/*
 * algorithm for sanity-checking a tpl image:
 * scan the tpl whilst not exceeding the buffer size (bufsz) ,
 * formulating a calculated (expected) size of the tpl based
 * on walking its data. When calcsize has been calculated it
 * should exactly match the buffer size (bufsz) and the internal
 * recorded size (intlsz)
 */
static int tpl_sanity(tpl_node *r) {
    uint32_t intlsz;
    int found_nul=0,rc, octothorpes=0, num_fxlens, *fxlens, flen;
    void *d, *dv;
    char intlflags, *fmt, c, *mapfmt;
    size_t bufsz, serlen;

    d = ((tpl_root_data*)(r->data))->mmap.text;
    bufsz = ((tpl_root_data*)(r->data))->mmap.text_sz;

    dv = d;
    if (bufsz < (4 + sizeof(uint32_t) + 1)) return ERR_NOT_MINSIZE; /* min sz: magic+flags+len+nul */
    if (memcmp(dv,TPL_MAGIC, 3) != 0) return ERR_MAGIC_MISMATCH; /* missing tpl magic prefix */
    if (tpl_needs_endian_swap(dv)) ((tpl_root_data*)(r->data))->flags |= TPL_XENDIAN;
    dv = (void*)((long)dv + 3);
    memcpy(&intlflags,dv,sizeof(char));  /* extract flags */
    if (intlflags & ~TPL_SUPPORTED_BITFLAGS) return ERR_UNSUPPORTED_FLAGS;
    dv = (void*)((long)dv + 1);
    memcpy(&intlsz,dv,sizeof(uint32_t));  /* extract internal size */
    if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN) tpl_byteswap(&intlsz, sizeof(uint32_t));
    if (intlsz != bufsz) return ERR_INCONSISTENT_SZ;  /* inconsisent buffer/internal size */
    dv = (void*)((long)dv + sizeof(uint32_t));

    /* dv points to the start of the format string. Look for nul w/in buf sz */
    fmt = (char*)dv;
    while ((long)dv-(long)d < bufsz && !found_nul) {
        if ( (c = *(char*)dv) != '\0') {
            if (strchr(tpl_fmt_chars,c) == NULL) 
               return ERR_FMT_INVALID;  /* invalid char in format string */
            if ( (c = *(char*)dv) == '#') octothorpes++;
            dv = (void*)((long)dv + 1);
        }
        else found_nul = 1;
    }
    if (!found_nul) return ERR_FMT_MISSING_NUL;  /* runaway format string */
    dv = (void*)((long)dv + 1);   /* advance to octothorpe lengths buffer */
    
    /* Take a momentary diversion if the tpl is in "inferred-format" mode.
       This mode means that the format string should be inferred from the
       image. This is only supported for a simple format of type "S(...)". */
    if (((tpl_root_data*)(r->data))->flags & TPL_INFER_FMT) {
        if (!tpl_map( "#infer_2", r, fmt, dv)) return ERR_FMT_MISMATCH;
    }

    /* compare the map format to the format of this tpl image */
    mapfmt = tpl_fmt(r);
    rc = strcmp(mapfmt,fmt);
    if (rc != 0) return ERR_FMT_MISMATCH; 

    /* compare octothorpe lengths in image to the mapped values */
    if ((((long)dv + (octothorpes * 4)) - (long)d) > bufsz) return ERR_INCONSISTENT_SZ4;
    fxlens = tpl_fxlens(r,&num_fxlens);  /* mapped fxlens */
    while(num_fxlens--) {
        memcpy(&flen,dv,sizeof(uint32_t)); /* stored flen */
        if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN) tpl_byteswap(&flen, sizeof(uint32_t));
        if (flen != *fxlens) return ERR_FLEN_MISMATCH;
        dv = (void*)((long)dv + sizeof(uint32_t));
        fxlens++;
    }

    /* dv now points to beginning of data */
    rc = tpl_serlen(r,r,dv,&serlen);  /* get computed serlen of data part */
    if (rc == -1) return ERR_INCONSISTENT_SZ2; /* internal inconsitency in tpl image */
    serlen += ((long)dv - (long)d);   /* add back serlen of preamble part */
    if (serlen != bufsz) return ERR_INCONSISTENT_SZ3;  /* buffer/internal sz exceeds serlen */
    return 0;
}

static void *tpl_find_data_start(void *d) {
    int octothorpes=0;
    d = (void*)((long)d + 4); /* skip TPL_MAGIC and flags byte */
    d = (void*)((long)d + 4); /* skip int32 overall len */
    while(*(char*)d != '\0') {
        if (*(char*)d == '#') octothorpes++;
        d = (void*)((long)d + 1);
    }
    d = (void*)((long)d +  1);  /* skip NUL */
    d = (void*)((long)d +  (octothorpes * sizeof(uint32_t)));  /* skip # array lens */
    return d;
}

static int tpl_needs_endian_swap(void *d) {
    char *c;
    int cpu_is_bigendian;
    c = (char*)d;
    cpu_is_bigendian = tpl_cpu_bigendian();
    return ((c[3] & TPL_FL_BIGENDIAN) == cpu_is_bigendian) ? 0 : 1;
}

TPL_API char* tpl_peek(int mode, ...) {
    va_list ap;
    int xendian=0,found_nul=0;
    char *filename;
    void *addr, *dv;
    size_t sz, fmt_len;
    tpl_mmap_rec mr;
    char *fmt,*fmt_cpy,c;
    uint32_t intlsz;

    va_start(ap,mode);
    if (mode & TPL_FILE) filename = va_arg(ap,char *);
    else if (mode & TPL_MEM) {
        addr = va_arg(ap,void *);
        sz = va_arg(ap,size_t);
    } else {
        tpl_hook.oops("unsupported tpl_peek mode %d\n", mode);
        return NULL;
    }
    va_end(ap);

    if (mode & TPL_FILE) {
        if (tpl_mmap_file(filename, &mr) != 0) {
            tpl_hook.oops("tpl_peek failed for file %s\n", filename);
            return NULL;
        }
        addr = mr.text;
        sz = mr.text_sz;
    }

    dv = addr;
    if (sz < (4 + sizeof(uint32_t) + 1)) goto fail; /* min sz */
    if (memcmp(dv,TPL_MAGIC, 3) != 0) goto fail; /* missing tpl magic prefix */
    if (tpl_needs_endian_swap(dv)) xendian=1;
    dv = (void*)((long)dv + 4);
    memcpy(&intlsz,dv,sizeof(uint32_t));  /* extract internal size */
    if (xendian) tpl_byteswap(&intlsz, sizeof(uint32_t));
    if (intlsz != sz) goto fail;  /* inconsisent buffer/internal size */
    dv = (void*)((long)dv + sizeof(uint32_t));

    /* dv points to the start of the format string. Look for nul w/in buf sz */
    fmt = (char*)dv;
    while ((long)dv-(long)addr < sz && !found_nul) {
        if ( (c = *(char*)dv) != '\0') {
            dv = (void*)((long)dv + 1);
        }
        else found_nul = 1;
    }
    if (!found_nul) goto fail;  /* runaway format string */
    fmt_len = (char*)dv - fmt + 1; /* include NUL */
    fmt_cpy = tpl_hook.malloc(fmt_len);
    if (fmt_cpy == NULL) {
        tpl_hook.fatal("out of memory");
    }
    memcpy(fmt_cpy, fmt, fmt_len);
    if (mode & TPL_FILE) tpl_unmap_file( &mr );
    return fmt_cpy;

fail:
    if (mode & TPL_FILE) tpl_unmap_file( &mr );
    return NULL;
}

TPL_API int tpl_load(tpl_node *r, int mode, ...) {
    va_list ap;
    int rc=0,fd;
    char *filename;
    void *addr;
    size_t sz;

    va_start(ap,mode);
    if (mode & TPL_FILE) filename = va_arg(ap,char *);
    else if (mode & TPL_MEM) {
        addr = va_arg(ap,void *);
        sz = va_arg(ap,size_t);
    } else if (mode & TPL_FD) {
        fd = va_arg(ap,int);
    } else {
        tpl_hook.oops("unsupported tpl_load mode %d\n", mode);
        return -1;
    }
    va_end(ap);

    if (r->type != TPL_TYPE_ROOT) {
        tpl_hook.oops("error: tpl_load to non-root node\n");
        return -1;
    }
    if (((tpl_root_data*)(r->data))->flags & (TPL_WRONLY|TPL_RDONLY)) {
        /* already packed or loaded, so reset it as if newly mapped */
        tpl_free_keep_map(r);
    }
    if (mode & TPL_FILE) {
        if (tpl_mmap_file(filename, &((tpl_root_data*)(r->data))->mmap) != 0) {
            tpl_hook.oops("tpl_load failed for file %s\n", filename);
            return -1;
        }
        if ( (rc = tpl_sanity(r)) != 0) {
            if (rc == ERR_FMT_MISMATCH) {
                tpl_hook.oops("%s: format signature mismatch\n", filename);
            } else if (rc == ERR_FLEN_MISMATCH) { 
                tpl_hook.oops("%s: array lengths mismatch\n", filename);
            } else { 
                tpl_hook.oops("%s: not a valid tpl file\n", filename); 
            }
            tpl_unmap_file( &((tpl_root_data*)(r->data))->mmap );
            return -1;
        }
        ((tpl_root_data*)(r->data))->flags = (TPL_FILE | TPL_RDONLY);
    } else if (mode & TPL_MEM) {
        ((tpl_root_data*)(r->data))->mmap.text = addr;
        ((tpl_root_data*)(r->data))->mmap.text_sz = sz;
        if ( (rc = tpl_sanity(r)) != 0) {
            if (rc == ERR_FMT_MISMATCH) {
                tpl_hook.oops("format signature mismatch\n");
            } else { 
                tpl_hook.oops("not a valid tpl file\n"); 
            }
            return -1;
        }
        ((tpl_root_data*)(r->data))->flags = (TPL_MEM | TPL_RDONLY);
        if (mode & TPL_UFREE) ((tpl_root_data*)(r->data))->flags |= TPL_UFREE;
    } else if (mode & TPL_FD) {
        /* if fd read succeeds, resulting mem img is used for load */
        if (tpl_gather(TPL_GATHER_BLOCKING,fd,&addr,&sz) > 0) {
            return tpl_load(r, TPL_MEM|TPL_UFREE, addr, sz);
        } else return -1;
    } else {
        tpl_hook.oops("invalid tpl_load mode %d\n", mode);
        return -1;
    }
    /* this applies to TPL_MEM or TPL_FILE */
    if (tpl_needs_endian_swap(((tpl_root_data*)(r->data))->mmap.text))
        ((tpl_root_data*)(r->data))->flags |= TPL_XENDIAN;
    tpl_unpackA0(r);   /* prepare root A nodes for use */
    return 0;
}

TPL_API int tpl_Alen(tpl_node *r, int i) {
    tpl_node *n;

    n = tpl_find_i(r,i);
    if (n == NULL) {
        tpl_hook.oops("invalid index %d to tpl_unpack\n", i);
        return -1;
    }
    if (n->type != TPL_TYPE_ARY) return -1;
    return ((tpl_atyp*)(n->data))->num;
}

static void tpl_free_atyp(tpl_node *n, tpl_atyp *atyp) {
    tpl_backbone *bb,*bbnxt;
    tpl_node *c;
    void *dv;
    tpl_bin *binp;
    tpl_atyp *atypp;
    char *strp;

    bb = atyp->bb;
    while (bb) {
        bbnxt = bb->next;
        dv = bb->data;
        for(c=n->children; c; c=c->next) {
            switch (c->type) {
                case TPL_TYPE_BYTE:
                case TPL_TYPE_DOUBLE:
                case TPL_TYPE_INT32:
                case TPL_TYPE_UINT32:
                case TPL_TYPE_INT64:
                case TPL_TYPE_UINT64:
                    dv = (void*)((long)dv + tpl_types[c->type].sz);
                    break;
                case TPL_TYPE_BIN:
                    memcpy(&binp,dv,sizeof(tpl_bin*)); /* cp to aligned */
                    if (binp->addr) tpl_hook.free( binp->addr ); /* free buf */
                    tpl_hook.free(binp);  /* free tpl_bin */
                    dv = (void*)((long)dv + sizeof(tpl_bin*));
                    break;
                case TPL_TYPE_STR:
                    memcpy(&strp,dv,sizeof(char*)); /* cp to aligned */
                    tpl_hook.free(strp); /* free string */
                    dv = (void*)((long)dv + sizeof(char*));
                    break;
                case TPL_TYPE_ARY:
                    memcpy(&atypp,dv,sizeof(tpl_atyp*)); /* cp to aligned */
                    tpl_free_atyp(c,atypp);  /* free atyp */
                    dv = (void*)((long)dv + sizeof(void*));
                    break;
                default:
                    tpl_hook.fatal("unsupported format character\n");
                    break;
            }
        }
        tpl_hook.free(bb);
        bb = bbnxt;
    }
    tpl_hook.free(atyp);
}

/* determine (by walking) byte length of serialized r/A node at address dv 
 * returns 0 on success, or -1 if the tpl isn't trustworthy (fails consistency)
 */
static int tpl_serlen(tpl_node *r, tpl_node *n, void *dv, size_t *serlen) {
    uint32_t slen;
    int num,fidx;
    tpl_node *c;
    size_t len=0, alen, buf_past;

    buf_past = ((long)((tpl_root_data*)(r->data))->mmap.text + 
                      ((tpl_root_data*)(r->data))->mmap.text_sz);

    if (n->type == TPL_TYPE_ROOT) num = 1;
    else if (n->type == TPL_TYPE_ARY) {
        if ((long)dv + sizeof(uint32_t) > buf_past) return -1;
        memcpy(&num,dv,sizeof(uint32_t));
        if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
             tpl_byteswap(&num, sizeof(uint32_t));
        dv = (void*)((long)dv + sizeof(uint32_t));
        len += sizeof(uint32_t);
    } else tpl_hook.fatal("internal error in tpl_serlen\n");

    while (num-- > 0) {
        for(c=n->children; c; c=c->next) {
            switch (c->type) {
                case TPL_TYPE_BYTE:
                case TPL_TYPE_DOUBLE:
                case TPL_TYPE_INT32:
                case TPL_TYPE_UINT32:
                case TPL_TYPE_INT64:
                case TPL_TYPE_UINT64:
                    for(fidx=0; fidx < c->num; fidx++) {  /* octothorpe support */
                        if ((long)dv + tpl_types[c->type].sz > buf_past) return -1;
                        dv = (void*)((long)dv + tpl_types[c->type].sz);
                        len += tpl_types[c->type].sz;
                    }
                    break;
                case TPL_TYPE_BIN:
                case TPL_TYPE_STR:
                    len += sizeof(uint32_t);
                    if ((long)dv + sizeof(uint32_t) > buf_past) return -1;
                    memcpy(&slen,dv,sizeof(uint32_t));
                    if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                        tpl_byteswap(&slen, sizeof(uint32_t));
                    len += slen;
                    dv = (void*)((long)dv + sizeof(uint32_t));
                    if ((long)dv + slen > buf_past) return -1;
                    dv = (void*)((long)dv + slen);
                    break;
                case TPL_TYPE_ARY:
                    if ( tpl_serlen(r,c,dv, &alen) == -1) return -1;
                    dv = (void*)((long)dv + alen);
                    len += alen;
                    break;
                default:
                    tpl_hook.fatal("unsupported format character\n");
                    break;
            }
        }
    }
    *serlen = len;
    return 0;
}

static int tpl_mmap_output_file(char *filename, size_t sz, void **text_out) {
    void *text;
    int fd,perms;

    perms = S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH;  /* ug+w o+r */
    fd=open(filename,O_CREAT|O_TRUNC|O_RDWR,perms);
    if ( fd == -1 ) {
        tpl_hook.oops("Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    text = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (text == MAP_FAILED) {
        tpl_hook.oops("Failed to mmap %s: %s\n", filename, strerror(errno));
        close(fd);
        return -1;
    }
    if (ftruncate(fd,sz) == -1) {
        tpl_hook.oops("ftruncate failed: %s\n", strerror(errno));
        munmap( text, sz );
        close(fd);
        return -1;
    }
    *text_out = text;
    return fd;
}

static int tpl_mmap_file(char *filename, tpl_mmap_rec *mr) {
    struct stat stat_buf;

    if ( (mr->fd = open(filename, O_RDONLY)) == -1 ) {
        tpl_hook.oops("Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    if ( fstat(mr->fd, &stat_buf) == -1) {
        close(mr->fd);
        tpl_hook.oops("Couldn't stat file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    mr->text_sz = (size_t)stat_buf.st_size;  
    mr->text = mmap(0, stat_buf.st_size, PROT_READ, MAP_PRIVATE, mr->fd, 0);
    if (mr->text == MAP_FAILED) {
        close(mr->fd);
        tpl_hook.oops("Failed to mmap %s: %s\n", filename, strerror(errno));
        return -1;
    }

    return 0;
}

TPL_API int tpl_pack(tpl_node *r, int i) {
    tpl_node *n, *child;
    void *datav=NULL;
    size_t sz;
    uint32_t slen;
    char *str;
    tpl_bin *bin;

    n = tpl_find_i(r,i);
    if (n == NULL) {
        tpl_hook.oops("invalid index %d to tpl_pack\n", i);
        return -1;
    }

    if (((tpl_root_data*)(r->data))->flags & TPL_RDONLY) {
        /* convert to an writeable tpl, initially empty */
        tpl_free_keep_map(r);
    }

    ((tpl_root_data*)(r->data))->flags |= TPL_WRONLY;

    if (n->type == TPL_TYPE_ARY) datav = tpl_extend_backbone(n);
    child = n->children;
    while(child) {
        switch(child->type) {
            case TPL_TYPE_BYTE:
            case TPL_TYPE_DOUBLE:
            case TPL_TYPE_INT32:
            case TPL_TYPE_UINT32:
            case TPL_TYPE_INT64:
            case TPL_TYPE_UINT64:
                /* no need to use fidx iteration here; we can copy multiple values in one memcpy */
                memcpy(child->data,child->addr,tpl_types[child->type].sz * child->num);
                if (datav) datav = tpl_cpv(datav,child->data,tpl_types[child->type].sz * child->num);
                if (n->type == TPL_TYPE_ARY) n->ser_osz += tpl_types[child->type].sz * child->num;
                break;
            case TPL_TYPE_BIN:
                /* copy the buffer to be packed */ 
                slen = ((tpl_bin*)child->addr)->sz;
                if (slen >0) {
                    str = tpl_hook.malloc(slen);
                    if (!str) tpl_hook.fatal("out of memory\n");
                    memcpy(str,((tpl_bin*)child->addr)->addr,slen);
                } else str = NULL;
                /* and make a tpl_bin to point to it */
                bin = tpl_hook.malloc(sizeof(tpl_bin));
                if (!bin) tpl_hook.fatal("out of memory\n");
                bin->addr = str;
                bin->sz = slen;
                /* now pack its pointer, first deep freeing any pre-existing bin */
                if (*(tpl_bin**)(child->data) != NULL) {
                    if ((*(tpl_bin**)(child->data))->sz != 0) {
                            tpl_hook.free( (*(tpl_bin**)(child->data))->addr );
                    }
                    tpl_hook.free(*(tpl_bin**)(child->data));  
                }
                memcpy(child->data,&bin,sizeof(tpl_bin*));
                if (datav) {
                    datav = tpl_cpv(datav, &bin, sizeof(tpl_bin*));
                    *(tpl_bin**)(child->data) = NULL;  
                }
                if (n->type == TPL_TYPE_ARY) {
                    n->ser_osz += sizeof(uint32_t); /* binary buf len word */
                    n->ser_osz += bin->sz;          /* binary buf */
                }
                break;
            case TPL_TYPE_STR:
                /* copy the string to be packed */
                slen = strlen(*(char**)(child->addr));
                str = tpl_hook.malloc(slen+1);
                if (!str) tpl_hook.fatal("out of memory\n");
                memcpy(str,*(char**)(child->addr),slen);
                str[slen] = '\0';   /* nul terminate */
                /* now pack its pointer, first freeing any pre-existing string */
                if (*(char**)(child->data) != NULL) {
                    tpl_hook.free(*(char**)(child->data));  
                }
                memcpy(child->data,&str,sizeof(char*));
                if (datav) {
                    datav = tpl_cpv(datav, &str, sizeof(char*));
                    *(char**)(child->data) = NULL;  
                }
                if (n->type == TPL_TYPE_ARY) {
                    n->ser_osz += sizeof(uint32_t); /* string len word */
                    n->ser_osz += slen;             /* string (without nul) */
                }
                break;
            case TPL_TYPE_ARY:
                /* copy the child's tpl_atype* and reset it to empty */
                if (datav) {
                    sz = ((tpl_atyp*)(child->data))->sz;
                    datav = tpl_cpv(datav, &child->data, sizeof(void*));
                    child->data = tpl_hook.malloc(sizeof(tpl_atyp));
                    if (!child->data) tpl_hook.fatal("out of memory\n");
                    ((tpl_atyp*)(child->data))->num = 0;
                    ((tpl_atyp*)(child->data))->sz = sz;
                    ((tpl_atyp*)(child->data))->bb = NULL;
                    ((tpl_atyp*)(child->data))->bbtail = NULL;
                }
                /* parent is array? then bubble up child array's ser_osz */
                if (n->type == TPL_TYPE_ARY) {
                    n->ser_osz += sizeof(uint32_t); /* array len word */
                    n->ser_osz += child->ser_osz;   /* child array ser_osz */
                    child->ser_osz = 0;             /* reset child array ser_osz */
                }
                break;
            default:
                tpl_hook.fatal("unsupported format character\n");
                break;
        }
        child=child->next;
    }
    return 0;
}

TPL_API int tpl_unpack(tpl_node *r, int i) {
    tpl_node *n, *c;
    uint32_t slen;
    int rc=1, fidx;
    char *str;
    void *dv, *caddr;
    size_t A_bytes;

    void *img;
    size_t sz;

    /* handle unusual case of tpl_pack,tpl_unpack without an 
     * intervening tpl_dump. do a dump/load implicitly. */
    if (((tpl_root_data*)(r->data))->flags & TPL_WRONLY) {
        if (tpl_dump(r,TPL_MEM,&img,&sz) != 0) return -1;
        if (tpl_load(r,TPL_MEM|TPL_UFREE,img,sz) != 0) {
            tpl_hook.free(img);
            return -1;
        };
    }

    n = tpl_find_i(r,i);
    if (n == NULL) {
        tpl_hook.oops("invalid index %d to tpl_unpack\n", i);
        return -1;
    }

    /* either root node or an A node */
    if (n->type == TPL_TYPE_ROOT) {
        dv = tpl_find_data_start( ((tpl_root_data*)(n->data))->mmap.text );
    } else if (n->type == TPL_TYPE_ARY) {
        if (((tpl_atyp*)(n->data))->num <= 0) return 0; /* array consumed */
        else rc = ((tpl_atyp*)(n->data))->num--;
        dv = ((tpl_atyp*)(n->data))->cur;
        if (!dv) tpl_hook.fatal("must unpack parent of node before node itself\n");
    }

    for(c=n->children; c; c=c->next) {
        switch (c->type) {
            case TPL_TYPE_BYTE:
            case TPL_TYPE_DOUBLE:
            case TPL_TYPE_INT32:
            case TPL_TYPE_UINT32:
            case TPL_TYPE_INT64:
            case TPL_TYPE_UINT64:
                /* unpack elements of cross-endian octothorpic array individually */
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN) {
                    for(fidx=0; fidx < c->num; fidx++) {
                        caddr = (void*)((long)c->addr + (fidx * tpl_types[c->type].sz));
                        memcpy(caddr,dv,tpl_types[c->type].sz);
                        tpl_byteswap(caddr, tpl_types[c->type].sz);
                        dv = (void*)((long)dv + tpl_types[c->type].sz);
                    }
                } else {
                    /* bulk unpack ok if not cross-endian */
                    memcpy(c->addr, dv, tpl_types[c->type].sz * c->num);
                    dv = (void*)((long)dv + tpl_types[c->type].sz * c->num);
                }
                break;
            case TPL_TYPE_BIN:
                memcpy(&slen,dv,sizeof(uint32_t));
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                    tpl_byteswap(&slen, sizeof(uint32_t));
                if (slen > 0) {
                    str = (char*)tpl_hook.malloc(slen);
                    if (!str) tpl_hook.fatal("out of memory\n");
                } else str=NULL;
                dv = (void*)((long)dv + sizeof(uint32_t));
                if (slen>0) memcpy(str,dv,slen);
                memcpy(&(((tpl_bin*)c->addr)->addr),&str,sizeof(void*));
                memcpy(&(((tpl_bin*)c->addr)->sz),&slen,sizeof(uint32_t));
                dv = (void*)((long)dv + slen);
                break;
            case TPL_TYPE_STR:
                memcpy(&slen,dv,sizeof(uint32_t));
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                    tpl_byteswap(&slen, sizeof(uint32_t));
                str = (char*)tpl_hook.malloc(slen+1);
                if (!str) tpl_hook.fatal("out of memory\n");
                dv = (void*)((long)dv + sizeof(uint32_t));
                memcpy(str,dv,slen);
                str[slen] = '\0'; /* nul terminate */
                memcpy((char**)(c->addr),&str,sizeof(char*));
                dv = (void*)((long)dv + slen);
                break;
            case TPL_TYPE_ARY:
                if (tpl_serlen(r,c,dv, &A_bytes) == -1) 
                    tpl_hook.fatal("internal error in unpack\n");
                memcpy( &((tpl_atyp*)(c->data))->num, dv, sizeof(uint32_t));
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                    tpl_byteswap(&((tpl_atyp*)(c->data))->num, sizeof(uint32_t));
                ((tpl_atyp*)(c->data))->cur = (void*)((long)dv+sizeof(uint32_t));
                dv = (void*)((long)dv + A_bytes);
                break;
            default:
                tpl_hook.fatal("unsupported format character\n");
                break;
        }
    }
    if (n->type == TPL_TYPE_ARY) ((tpl_atyp*)(n->data))->cur = dv; /* next element */
    return rc;
}

/* Specialized function that unpacks only the root's A nodes, after tpl_load  */
static int tpl_unpackA0(tpl_node *r) {
    tpl_node *n, *c;
    uint32_t slen;
    int rc=1,fidx;
    void *dv;
    size_t A_bytes;

    n = r;
    dv = tpl_find_data_start( ((tpl_root_data*)(r->data))->mmap.text);

    for(c=n->children; c; c=c->next) {
        switch (c->type) {
            case TPL_TYPE_BYTE:
            case TPL_TYPE_DOUBLE:
            case TPL_TYPE_INT32:
            case TPL_TYPE_UINT32:
            case TPL_TYPE_INT64:
            case TPL_TYPE_UINT64:
                for(fidx=0;fidx < c->num; fidx++) {
                    dv = (void*)((long)dv + tpl_types[c->type].sz);
                }
                break;
            case TPL_TYPE_BIN:
            case TPL_TYPE_STR:
                memcpy(&slen,dv,sizeof(uint32_t));
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                    tpl_byteswap(&slen, sizeof(uint32_t));
                dv = (void*)((long)dv + sizeof(uint32_t));
                dv = (void*)((long)dv + slen);
                break;
            case TPL_TYPE_ARY:
                if ( tpl_serlen(r,c,dv, &A_bytes) == -1) 
                    tpl_hook.fatal("internal error in unpackA0\n");
                memcpy( &((tpl_atyp*)(c->data))->num, dv, sizeof(uint32_t));
                if (((tpl_root_data*)(r->data))->flags & TPL_XENDIAN)
                    tpl_byteswap(&((tpl_atyp*)(c->data))->num, sizeof(uint32_t));
                ((tpl_atyp*)(c->data))->cur = (void*)((long)dv+sizeof(uint32_t));
                dv = (void*)((long)dv + A_bytes);
                break;
            default:
                tpl_hook.fatal("unsupported format character\n");
                break;
        }
    }
    return rc;
}

/* In-place byte order swapping of a word of length "len" bytes */
static void tpl_byteswap(void *word, int len) {
    int i;
    char c, *w;
    w = (char*)word;
    for(i=0; i<len/2; i++) {
        c = w[i];
        w[i] = w[len-1-i];
        w[len-1-i] = c;
    }
}

static void tpl_fatal(char *fmt, ...) {
    va_list ap;
    char exit_msg[100];

    va_start(ap,fmt);
    vsnprintf(exit_msg, 100, fmt, ap);
    va_end(ap);

    tpl_hook.oops("%s", exit_msg);
    exit(-1);
}

TPL_API int tpl_gather(int mode, ...) {
    va_list ap;
    int fd,rc;
    size_t *szp,sz;
    void **img,*addr,*data;
    tpl_gather_t **gs;
    tpl_gather_cb *cb;

    va_start(ap,mode);
    switch (mode) {
        case TPL_GATHER_BLOCKING:
            fd = va_arg(ap,int);
            img = va_arg(ap,void*);
            szp = va_arg(ap,size_t*);
            rc = tpl_gather_blocking(fd,img,szp);
            break;
        case TPL_GATHER_NONBLOCKING:
            fd = va_arg(ap,int);
            gs = (tpl_gather_t**)va_arg(ap,void*);
            cb = (tpl_gather_cb*)va_arg(ap,void*);
            data = va_arg(ap,void*);
            rc = tpl_gather_nonblocking(fd,gs,cb,data);
            break;
        case TPL_GATHER_MEM:
            addr = va_arg(ap,void*);
            sz = va_arg(ap,size_t);
            gs = (tpl_gather_t**)va_arg(ap,void*);
            cb = (tpl_gather_cb*)va_arg(ap,void*);
            data = va_arg(ap,void*);
            rc = tpl_gather_mem(addr,sz,gs,cb,data);
            break;
        default:
            tpl_hook.fatal("unsupported tpl_gather mode %d\n",mode);
            break;
    }
    va_end(ap);
    return rc;
}

/* dequeue a tpl by reading until one full tpl image is obtained.
 * We take care not to read past the end of the tpl.
 * This is intended as a blocking call i.e. for use with a blocking fd.
 * It can be given a non-blocking fd, but the read spins if we have to wait.
 */
static int tpl_gather_blocking(int fd, void **img, size_t *sz) {
    char preamble[8];
    int i=0, rc;
    uint32_t tpllen;

    do { 
        rc = read(fd,&preamble[i],8-i);
        i += (rc>0) ? rc : 0;
    } while ((rc==-1 && (errno==EINTR||errno==EAGAIN)) || (rc>0 && i<8));

    if (rc<0) {
        tpl_hook.oops("tpl_gather_fd_blocking failed: %s\n", strerror(errno));
        return -1;
    } else if (rc == 0) {
        /* tpl_hook.oops("tpl_gather_fd_blocking: eof\n"); */
        return 0;
    } else if (i != 8) {
        tpl_hook.oops("internal error\n");
        return -1;
    }

    if (preamble[0] == 't' && preamble[1] == 'p' && preamble[2] == 'l') {
        memcpy(&tpllen,&preamble[4],4);
        if (tpl_needs_endian_swap(preamble)) tpl_byteswap(&tpllen,4);
    } else {
        tpl_hook.oops("tpl_gather_fd_blocking: non-tpl input\n");
        return -1;
    }

    /* malloc space for remainder of tpl image (overall length tpllen) 
     * and read it in
     */
    if (tpl_hook.gather_max > 0 && 
        tpllen > tpl_hook.gather_max) {
        tpl_hook.oops("tpl exceeds max length %d\n", 
            tpl_hook.gather_max);
        return -2;
    }
    *sz = tpllen;
    if ( (*img = tpl_hook.malloc(tpllen)) == NULL) {
        tpl_hook.fatal("out of memory\n");
    }

    memcpy(*img,preamble,8);  /* copy preamble to output buffer */
    i=8;
    do { 
        rc = read(fd,&((*(char**)img)[i]),tpllen-i);
        i += (rc>0) ? rc : 0;
    } while ((rc==-1 && (errno==EINTR||errno==EAGAIN)) || (rc>0 && i<tpllen));

    if (rc<0) {
        tpl_hook.oops("tpl_gather_fd_blocking failed: %s\n", strerror(errno));
        tpl_hook.free(*img);
        return -1;
    } else if (rc == 0) {
        /* tpl_hook.oops("tpl_gather_fd_blocking: eof\n"); */
        tpl_hook.free(*img);
        return 0;
    } else if (i != tpllen) {
        tpl_hook.oops("internal error\n");
        tpl_hook.free(*img);
        return -1;
    }

    return 1;
}

/* Used by select()-driven apps which want to gather tpl images piecemeal */
/* the file descriptor must be non-blocking for this functino to work. */
static int tpl_gather_nonblocking( int fd, tpl_gather_t **gs, tpl_gather_cb *cb, void *data) {
    char buf[TPL_GATHER_BUFLEN], *img, *tpl;
    int rc, keep_looping, cbrc=0;
    size_t catlen;
    uint32_t tpllen;

    while (1) {
        rc = read(fd,buf,TPL_GATHER_BUFLEN);
        if (rc == -1) {
            if (errno == EINTR) continue;  /* got signal during read, ignore */
            if (errno == EAGAIN) return 1; /* nothing to read right now */
            else {
                tpl_hook.oops("tpl_gather failed: %s\n", strerror(errno));
                if (*gs) {
                    tpl_hook.free((*gs)->img);
                    tpl_hook.free(*gs);
                    *gs = NULL;
                }
                return -1;                 /* error, caller should close fd  */
            }
        } else if (rc == 0) {
            if (*gs) {
                tpl_hook.oops("tpl_gather: partial tpl image precedes EOF\n");
                tpl_hook.free((*gs)->img);
                tpl_hook.free(*gs);
                *gs = NULL;
            }
            return 0;                      /* EOF, caller should close fd */
        } else {
            /* concatenate any partial tpl from last read with new buffer */
            if (*gs) {
                catlen = (*gs)->len + rc;
                if (tpl_hook.gather_max > 0 && 
                    catlen > tpl_hook.gather_max) {
                    tpl_hook.free( (*gs)->img );
                    tpl_hook.free( (*gs) );
                    *gs = NULL;
                    tpl_hook.oops("tpl exceeds max length %d\n", 
                        tpl_hook.gather_max);
                    return -2;              /* error, caller should close fd */
                }
                if ( (img = tpl_hook.realloc((*gs)->img, catlen)) == NULL) {
                    tpl_hook.fatal("out of memory\n");
                }
                memcpy(img + (*gs)->len, buf, rc);
                tpl_hook.free(*gs);
                *gs = NULL;
            } else {
                img = buf;
                catlen = rc;
            }
            /* isolate any full tpl(s) in img and invoke cb for each */
            tpl = img;
            keep_looping = (tpl+8 < img+catlen) ? 1 : 0;
            while (keep_looping) {
                if (strncmp("tpl", tpl, 3) != 0) {
                    tpl_hook.oops("tpl prefix invalid\n");
                    if (img != buf) tpl_hook.free(img);
                    tpl_hook.free(*gs);
                    *gs = NULL;
                    return -3; /* error, caller should close fd */
                }
                memcpy(&tpllen,&tpl[4],4);
                if (tpl_needs_endian_swap(tpl)) tpl_byteswap(&tpllen,4);
                if (tpl+tpllen <= img+catlen) {
                    cbrc = (cb)(tpl,tpllen,data);  /* invoke cb for tpl image */
                    tpl += tpllen;                 /* point to next tpl image */
                    if (cbrc < 0) keep_looping = 0;
                    else keep_looping = (tpl+8 < img+catlen) ? 1 : 0;
                } else keep_looping=0;
            } 
            /* check if app callback requested closure of tpl source */
            if (cbrc < 0) {
                tpl_hook.oops("tpl_fd_gather aborted by app callback\n");
                if (img != buf) tpl_hook.free(img);
                if (*gs) tpl_hook.free(*gs);
                *gs = NULL;
                return -4;
            }
            /* store any leftover, partial tpl fragment for next read */
            if (tpl == img && img != buf) {  
                /* consumed nothing from img!=buf */
                if ( (*gs = tpl_hook.malloc(sizeof(tpl_gather_t))) == NULL ) {
                    tpl_hook.fatal("out of memory\n");
                }
                (*gs)->img = tpl;
                (*gs)->len = catlen;
            } else if (tpl < img+catlen) {  
                /* consumed 1+ tpl(s) from img!=buf or 0 from img==buf */
                if ( (*gs = tpl_hook.malloc(sizeof(tpl_gather_t))) == NULL ) {
                    tpl_hook.fatal("out of memory\n");
                }
                if ( ((*gs)->img = tpl_hook.malloc(img+catlen - tpl)) == NULL ) {
                    tpl_hook.fatal("out of memory\n");
                }
                (*gs)->len = img+catlen - tpl;
                memcpy( (*gs)->img, tpl, img+catlen - tpl);
                /* free partially consumed concat buffer if used */
                if (img != buf) tpl_hook.free(img); 
            } else {                        /* tpl(s) fully consumed */
                /* free consumed concat buffer if used */
                if (img != buf) tpl_hook.free(img); 
            }
        }
    } 
}

/* gather tpl piecemeal from memory buffer (not fd) e.g., from a lower-level api */
static int tpl_gather_mem( char *buf, size_t len, tpl_gather_t **gs, tpl_gather_cb *cb, void *data) {
    char *img, *tpl;
    int keep_looping, cbrc=0;
    size_t catlen;
    uint32_t tpllen;

    /* concatenate any partial tpl from last read with new buffer */
    if (*gs) {
        catlen = (*gs)->len + len;
        if (tpl_hook.gather_max > 0 && 
            catlen > tpl_hook.gather_max) {
            tpl_hook.free( (*gs)->img );
            tpl_hook.free( (*gs) );
            *gs = NULL;
            tpl_hook.oops("tpl exceeds max length %d\n", 
                tpl_hook.gather_max);
            return -2;              /* error, caller should stop accepting input from source*/
        }
        if ( (img = tpl_hook.realloc((*gs)->img, catlen)) == NULL) {
            tpl_hook.fatal("out of memory\n");
        }
        memcpy(img + (*gs)->len, buf, len);
        tpl_hook.free(*gs);
        *gs = NULL;
    } else {
        img = buf;
        catlen = len;
    }
    /* isolate any full tpl(s) in img and invoke cb for each */
    tpl = img;
    keep_looping = (tpl+8 < img+catlen) ? 1 : 0;
    while (keep_looping) {
        if (strncmp("tpl", tpl, 3) != 0) {
            tpl_hook.oops("tpl prefix invalid\n");
            if (img != buf) tpl_hook.free(img);
            tpl_hook.free(*gs);
            *gs = NULL;
            return -3; /* error, caller should stop accepting input from source*/
        }
        memcpy(&tpllen,&tpl[4],4);
        if (tpl_needs_endian_swap(tpl)) tpl_byteswap(&tpllen,4);
        if (tpl+tpllen <= img+catlen) {
            cbrc = (cb)(tpl,tpllen,data);  /* invoke cb for tpl image */
            tpl += tpllen;               /* point to next tpl image */
            if (cbrc < 0) keep_looping = 0;
            else keep_looping = (tpl+8 < img+catlen) ? 1 : 0;
        } else keep_looping=0;
    } 
    /* check if app callback requested closure of tpl source */
    if (cbrc < 0) {
        tpl_hook.oops("tpl_mem_gather aborted by app callback\n");
        if (img != buf) tpl_hook.free(img);
        if (*gs) tpl_hook.free(*gs);
        *gs = NULL;
        return -4;
    }
    /* store any leftover, partial tpl fragment for next read */
    if (tpl == img && img != buf) {  
        /* consumed nothing from img!=buf */
        if ( (*gs = tpl_hook.malloc(sizeof(tpl_gather_t))) == NULL ) {
            tpl_hook.fatal("out of memory\n");
        }
        (*gs)->img = tpl;
        (*gs)->len = catlen;
    } else if (tpl < img+catlen) {  
        /* consumed 1+ tpl(s) from img!=buf or 0 from img==buf */
        if ( (*gs = tpl_hook.malloc(sizeof(tpl_gather_t))) == NULL ) {
            tpl_hook.fatal("out of memory\n");
        }
        if ( ((*gs)->img = tpl_hook.malloc(img+catlen - tpl)) == NULL ) {
            tpl_hook.fatal("out of memory\n");
        }
        (*gs)->len = img+catlen - tpl;
        memcpy( (*gs)->img, tpl, img+catlen - tpl);
        /* free partially consumed concat buffer if used */
        if (img != buf) tpl_hook.free(img); 
    } else {                        /* tpl(s) fully consumed */
        /* free consumed concat buffer if used */
        if (img != buf) tpl_hook.free(img); 
    }
    return 1;
}
