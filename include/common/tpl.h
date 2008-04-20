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

#ifndef TPL_H
#define TPL_H 

#include <sys/types.h>  /* size_t */
#include <inttypes.h>   /* uint32_t */

#if defined __cplusplus
extern "C" {
#endif

#define TPL_API

#define TPL_MAGIC "tpl"

/* macro to add a structure to a doubly-linked list */
#define DL_ADD(head,add)                                        \
    do {                                                        \
        if (head) {                                             \
            (add)->prev = (head)->prev;                         \
            (head)->prev->next = (add);                         \
            (head)->prev = (add);                               \
            (add)->next = NULL;                                 \
        } else {                                                \
            (head)=(add);                                       \
            (head)->prev = (head);                              \
            (head)->next = NULL;                                \
        }                                                       \
    } while (0);

/* values for the flags byte that appears after the magic prefix */
#define TPL_SUPPORTED_BITFLAGS 1
#define TPL_FL_BIGENDIAN  (1 << 0)

/* bit flags used at runtime, not stored in the tpl */
#define TPL_FILE    (1 << 0)
#define TPL_MEM     (1 << 1)
#define TPL_FD      (1 << 2)
#define TPL_RDONLY  (1 << 3)  /* tpl was loaded (for unpacking) */
#define TPL_WRONLY  (1 << 4)  /* app has initiated tpl packing  */
#define TPL_XENDIAN (1 << 5)  /* swap endianness when unpacking */
#define TPL_UFREE   (1 << 6)  /* free mem img when freeing tpl  */
#define TPL_INFER_FMT (1 << 7)  /* infer format string from image */

/* char values for node type */
#define TPL_TYPE_ROOT   0
#define TPL_TYPE_INT32  1
#define TPL_TYPE_UINT32 2
#define TPL_TYPE_BYTE   3
#define TPL_TYPE_STR    4
#define TPL_TYPE_ARY    5
#define TPL_TYPE_BIN    6
#define TPL_TYPE_DOUBLE 7
#define TPL_TYPE_INT64  8
#define TPL_TYPE_UINT64 9

/* flags for tpl_gather mode */
#define TPL_GATHER_BLOCKING    1
#define TPL_GATHER_NONBLOCKING 2
#define TPL_GATHER_MEM         3

/* error codes */
#define ERR_NOT_MINSIZE        (-1)
#define ERR_MAGIC_MISMATCH     (-2)
#define ERR_INCONSISTENT_SZ    (-3)
#define ERR_FMT_INVALID        (-4)
#define ERR_FMT_MISSING_NUL    (-5)
#define ERR_FMT_MISMATCH       (-6)
#define ERR_FLEN_MISMATCH      (-7)
#define ERR_INCONSISTENT_SZ2   (-8)
#define ERR_INCONSISTENT_SZ3   (-9)
#define ERR_INCONSISTENT_SZ4   (-10)
#define ERR_UNSUPPORTED_FLAGS  (-11)

/* Hooks for error logging, memory allocation functions and other config 
 * parameters to allow customization.
 */
typedef int (tpl_print_fcn)(const char *fmt, ...);
typedef void *(tpl_malloc_fcn)(size_t sz);
typedef void *(tpl_realloc_fcn)(void *ptr, size_t sz);
typedef void (tpl_free_fcn)(void *ptr);
typedef void (tpl_fatal_fcn)(char *fmt, ...);

typedef struct tpl_hook_t {
    tpl_print_fcn *oops;
    tpl_malloc_fcn *malloc;
    tpl_realloc_fcn *realloc;
    tpl_free_fcn *free;
    tpl_fatal_fcn *fatal;
    size_t gather_max;
} tpl_hook_t;

typedef struct tpl_pidx {
    struct tpl_node *node;
    struct tpl_pidx *next,*prev;
} tpl_pidx;

typedef struct tpl_node {
    char type;
    void *addr;
    void *data;                  /* r:tpl_root_data*. A:tpl_atyp*. ow:szof type */
    int num;                     /* length of type if its a C array */
    size_t ser_osz;              /* serialization output size for subtree */
    struct tpl_node *children;   /* my children; linked-list */
    struct tpl_node *next,*prev; /* my siblings (next child of my parent) */
    struct tpl_node *parent;     /* my parent */
} tpl_node;

typedef struct tpl_atyp {
    uint32_t num;    /* num elements */
    size_t sz;  /* size of each backbone's datum */
    struct tpl_backbone *bb,*bbtail; 
    void *cur;                       
} tpl_atyp;

typedef struct tpl_backbone {
    struct tpl_backbone *next;
    /* when this structure is malloc'd, extra space is alloc'd at the
     * end to store the backbone "datum", and data points to it. */
    char data[];
} tpl_backbone;

typedef struct tpl_mmap_rec {
    int fd;
    void *text;
    size_t text_sz;
} tpl_mmap_rec;

typedef struct tpl_root_data {
    int flags;
    tpl_pidx *pidx;
    tpl_mmap_rec mmap;
    char *fmt;
    int *fxlens, num_fxlens;
} tpl_root_data;

struct tpl_type_t {
    char c;
    int sz;
};

/* used when un/packing 'B' type (binary buffers) */

typedef struct tpl_bin {
    void *addr;
    uint32_t sz;
} tpl_bin;

/* The next structure is used for async/piecemeal reading of tpl images */

typedef struct tpl_gather_t {
    char *img;
    int len;
} tpl_gather_t;

/* Callback used when tpl_gather has read a full tpl image */
typedef int (tpl_gather_cb)(void *img, size_t sz, void *data);

/* Prototypes */
TPL_API tpl_node *tpl_map(char *fmt,...);       /* define tpl using format */
TPL_API void tpl_free(tpl_node *r);             /* free a tpl map */
TPL_API int tpl_pack(tpl_node *r, int i);       /* pack the n'th packable */
TPL_API int tpl_unpack(tpl_node *r, int i);     /* unpack the n'th packable */
TPL_API int tpl_dump(tpl_node *r, int mode, ...); /* serialize to mem/file */
TPL_API int tpl_load(tpl_node *r, int mode, ...); /* set mem/file to unpack */
TPL_API int tpl_Alen(tpl_node *r, int i);      /* array len of packable i */
TPL_API char* tpl_peek(int mode, ...);         /* sneak peek at format string */
TPL_API int tpl_gather( int mode, ...);        /* non-blocking image gather */

#if defined __cplusplus
    }
#endif

#endif /* TPL_H */

