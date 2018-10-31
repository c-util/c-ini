#pragma once

#include <c-list.h>
#include <c-rbtree.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include "c-ini.h"

#define _public_ __attribute__((__visibility__("default")))
#define _cleanup_(_x) __attribute__((__cleanup__(_x)))

typedef struct CIniBytes CIniBytes;
typedef struct CIniRaw CIniRaw;

/* initial size of the line buffer */
#define C_INI_INITIAL_LINE_SIZE (4096U)

struct CIniBytes {
        uint8_t *data;
        size_t n_data;
};

#define C_INI_BYTES_INIT(_data, _n_data) {                                      \
                .data = (_data),                                                \
                .n_data = (_n_data),                                            \
        }

struct CIniEntry {
        unsigned long n_refs;
        CIniGroup *group;
        CList link_group;
        CRBNode rb_group;

        uint8_t *key;
        size_t n_key;
        uint8_t *value;
        size_t n_value;
};

#define C_INI_ENTRY_NULL(_x) {                                                  \
                .n_refs = 1,                                                    \
                .link_group = C_LIST_INIT((_x).link_group),                     \
                .rb_group = C_RBNODE_INIT((_x).rb_group),                       \
        }

struct CIniGroup {
        unsigned long n_refs;
        CList link_domain;
        CRBNode rb_domain;
        CIniDomain *domain;

        uint8_t *label;
        size_t n_label;

        CList list_entries;
        CRBTree map_entries;
};

#define C_INI_GROUP_NULL(_x) {                                                  \
                .n_refs = 1,                                                    \
                .link_domain = C_LIST_INIT((_x).link_domain),                   \
                .rb_domain = C_RBNODE_INIT((_x).rb_domain),                     \
                .list_entries = C_LIST_INIT((_x).list_entries),                 \
                .map_entries = C_RBTREE_INIT,                                   \
        }

struct CIniRaw {
        unsigned long n_refs;
        CList link_domain;
        CIniDomain *domain;

        size_t n_data;
        uint8_t data[];
};

#define C_INI_RAW_NULL(_x) {                                                    \
                .n_refs = 1,                                                    \
                .link_domain = C_LIST_INIT((_x).link_domain),                   \
        }

struct CIniDomain {
        unsigned long n_refs;
        CIniGroup *null_group;

        CList list_raws;
        CList list_groups;
        CRBTree map_groups;
};

#define C_INI_DOMAIN_NULL(_x) {                                                 \
                .n_refs = 1,                                                    \
                .list_raws = C_LIST_INIT((_x).list_raws),                       \
                .list_groups = C_LIST_INIT((_x).list_groups),                   \
                .map_groups = C_RBTREE_INIT,                                    \
        }

struct CIniReader {
        unsigned int mode;

        CIniDomain *domain;
        CIniGroup *current;

        bool malformed : 1;

        uint8_t *line;
        size_t n_line;
        size_t z_line;
};

#define C_INI_READER_NULL(_x) {                                                 \
        }

/* entries */

int c_ini_entry_new(CIniEntry **entryp, const uint8_t *key, size_t n_key, const uint8_t *value, size_t n_value);

void c_ini_entry_link(CIniEntry *entry, CIniGroup *group);
void c_ini_entry_unlink(CIniEntry *entry);

/* groups */

int c_ini_group_new(CIniGroup **groupp, const uint8_t *label, size_t n_label);

void c_ini_group_link(CIniGroup *group, CIniDomain *domain);
void c_ini_group_unlink(CIniGroup *group);

/* raws */

int c_ini_raw_new(CIniRaw **rawp, const uint8_t *data, size_t n_data);
CIniRaw *c_ini_raw_ref(CIniRaw *raw);
CIniRaw *c_ini_raw_unref(CIniRaw *raw);

void c_ini_raw_link(CIniRaw *raw, CIniDomain *domain);
void c_ini_raw_unlink(CIniRaw *raw);

/* domains */

int c_ini_domain_new(CIniDomain **domainp);

/* readers */

int c_ini_reader_init(CIniReader *reader);
void c_ini_reader_deinit(CIniReader *reader);

int c_ini_reader_parse(CIniDomain **domainp,
                       unsigned int mode,
                       const uint8_t *data,
                       size_t n_data);

/* inline helpers */

static inline void c_ini_freep(void *p) {
        free(*(void **)p);
}

static inline bool c_ini_is_whitespace(char c) {
        return c == 0x09 || /* horizontal tab */
               c == 0x0a || /* line feed */
               c == 0x0c || /* form feed */
               c == 0x0d || /* carriage return */
               c == 0x20;   /* white space */
}

static inline void c_ini_raw_unrefp(CIniRaw **raw) {
        if (*raw)
                c_ini_raw_unref(*raw);
}
