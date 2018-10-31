#pragma once

/**
 * Ini-File Handling
 *
 * XXX
 *
 * Possible extensions:
 *
 *  * Several different INI formats exist. For now, we only support the
 *    desktop-entry-specification as released by XDG / freedesktop.org. Other
 *    formats should be straightforward to add.
 *
 *  * The glib-keyfile implementation has a set of extensions over the XDG
 *    specification. Several of these are already implemented, but we should
 *    better document the details:
 *
 *     * Ignore a single trailing '\r'. This is ignored both as "\r\n" as well
 *       as at the end of the file in case there is no missing newline.
 *
 *     * Whitespace are ignored at the beginning of all lines (including
 *       'blank' lines and comments). Additionally, trailing a group SPACE and
 *       HORIZONTAL TAB are ignored (possibly a bug and meant to ignore all
 *       whitespace).
 *
 *       Whitespace according to glib are independent of any locale, and they
 *       currently include:
 *
 *          0x09: HORIZONTAL TAB (\t)
 *          0x0a: LINE FEED (\n)
 *          0x0c: FORM FEED (\x0c)
 *          0x0d: CARRIAGE RETURN (\r)
 *          0x20: SPACE ( )
 *
 *     * An 'Encoding=' key is interpreted in the first-group and first-group
 *       only. Anything but 'UTF-8' is rejected.
 *       There should be little reason to support this feature.
 *
 *     * Identifiers are not restricted to ASCII. The restrictions are:
 *
 *        * Group names must be non-empty, must not contain '[', ']', nor ASCII
 *          control characters. Invalid UTF-8 is accepted, though.
 *
 *        * Keys must be non-empty, must not contain '[', ']', '=', nor spaces.
 *          Invalid UTF-8 is accepted, though.
 *          A key might have a trailing locale specifier enclosed in brackets,
 *          which must only contain alphanumeric codepoints, as well as '@',
 *          '.', '_', '-'.
 *
 *  * The current parsers assume the data source is trusted. Meaning, while it
 *    does correctly verify validity of all content, it does not enforce limits
 *    on data lengths in any way. If the data source is not trusted, you would
 *    want to extend the parsers to refuse files with overlong entities, or
 *    overlong extents.
 *
 *  * For now the API does not include serializers / writers, but was written
 *    to allow for seamless addition of such calls. Ini-Files are
 *    human-readable files, and as such there rarely is the need to write them
 *    programmatically. If the need ever arises, we will have to extent the API
 *    to provide serializers and modifiers.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdlib.h>

typedef struct CIniDomain CIniDomain;
typedef struct CIniEntry CIniEntry;
typedef struct CIniGroup CIniGroup;
typedef struct CIniReader CIniReader;

enum {
        C_INI_MODE_EXTENDED_WHITESPACE                          = (1 <<  0),
        C_INI_MODE_KEEP_DUPLICATE_GROUPS                        = (1 <<  1),
        C_INI_MODE_MERGE_GROUPS                                 = (1 <<  2),
        C_INI_MODE_KEEP_DUPLICATE_ENTRIES                       = (1 <<  3),
        C_INI_MODE_OVERRIDE_ENTRIES                             = (1 <<  4),
};

/* entries */

CIniEntry *c_ini_entry_ref(CIniEntry *entry);
CIniEntry *c_ini_entry_unref(CIniEntry *entry);

CIniEntry *c_ini_entry_next(CIniEntry *entry);
CIniEntry *c_ini_entry_previous(CIniEntry *entry);
const char *c_ini_entry_get_key(CIniEntry *entry, size_t *n_keyp);
const char *c_ini_entry_get_value(CIniEntry *entry, size_t *n_valuep);

/* groups */

CIniGroup *c_ini_group_ref(CIniGroup *group);
CIniGroup *c_ini_group_unref(CIniGroup *group);

CIniGroup *c_ini_group_next(CIniGroup *group);
CIniGroup *c_ini_group_previous(CIniGroup *group);
const char *c_ini_group_get_label(CIniGroup *group, size_t *n_groupp);

CIniEntry *c_ini_group_iterate(CIniGroup *group);
CIniEntry *c_ini_group_find(CIniGroup *group, const char *label, ssize_t n_label);

/* domains */

CIniDomain *c_ini_domain_ref(CIniDomain *domain);
CIniDomain *c_ini_domain_unref(CIniDomain *domain);

CIniGroup *c_ini_domain_get_null_group(CIniDomain *domain);

CIniGroup *c_ini_domain_iterate(CIniDomain *domain);
CIniGroup *c_ini_domain_find(CIniDomain *domain, const char *label, ssize_t n_label);

/* readers */

int c_ini_reader_new(CIniReader **readerp);
CIniReader *c_ini_reader_free(CIniReader *reader);

void c_ini_reader_set_mode(CIniReader *reader, unsigned int mode);
unsigned int c_ini_reader_get_mode(CIniReader *reader);

int c_ini_reader_feed(CIniReader *reader, const uint8_t *data, size_t n_data);
int c_ini_reader_seal(CIniReader *reader, CIniDomain **domainp);

/* inline helpers */

static inline void c_ini_entry_unrefp(CIniEntry **entry) {
        if (*entry)
                c_ini_entry_unref(*entry);
}

static inline void c_ini_group_unrefp(CIniGroup **group) {
        if (*group)
                c_ini_group_unref(*group);
}

static inline void c_ini_domain_unrefp(CIniDomain **domain) {
        if (*domain)
                c_ini_domain_unref(*domain);
}

static inline void c_ini_reader_freep(CIniReader **reader) {
        if (*reader)
                c_ini_reader_free(*reader);
}

#ifdef __cplusplus
}
#endif
