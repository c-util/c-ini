/*
 * Tests for Public API
 * This test, unlikely the others, is linked against the real, distributed,
 * shared library. Its sole purpose is to test for symbol availability.
 */

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-ini.h"

#define _cleanup_(_x) __attribute__((__cleanup__(_x)))

static void test_api(void) {
        _cleanup_(c_ini_reader_freep) CIniReader *reader = NULL;
        _cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;
        _cleanup_(c_ini_group_unrefp) CIniGroup *group = NULL;
        _cleanup_(c_ini_entry_unrefp) CIniEntry *entry = NULL;
        int r;

        r = c_ini_reader_new(&reader);
        assert(!r);

        /* readers */

        assert(C_INI_MODE_EXTENDED_WHITESPACE |
               C_INI_MODE_KEEP_DUPLICATE_GROUPS |
               C_INI_MODE_MERGE_GROUPS |
               C_INI_MODE_KEEP_DUPLICATE_ENTRIES |
               C_INI_MODE_OVERRIDE_ENTRIES);
        c_ini_reader_set_mode(reader, 0);
        c_ini_reader_get_mode(reader);

        r = c_ini_reader_feed(reader, (const uint8_t *)"x=y", 3);
        assert(!r);

        r = c_ini_reader_seal(reader, &domain);
        assert(!r);

        reader = c_ini_reader_free(reader);

        /* domains */

        c_ini_domain_unref(c_ini_domain_ref(domain));

        assert(!c_ini_domain_iterate(domain));
        assert(!c_ini_domain_find(domain, "foobar", -1));

        group = c_ini_group_ref(c_ini_domain_get_null_group(domain));

        domain = c_ini_domain_unref(domain);

        /* groups */

        c_ini_group_unref(c_ini_group_ref(group));

        assert(!c_ini_group_next(group));
        assert(!c_ini_group_previous(group));
        assert(c_ini_group_get_label(group, NULL));
        assert(c_ini_group_iterate(group));

        entry = c_ini_entry_ref(c_ini_group_find(group, "x", -1));

        group = c_ini_group_unref(group);

        /* entries */

        c_ini_entry_unref(c_ini_entry_ref(entry));

        assert(!c_ini_entry_next(entry));
        assert(!c_ini_entry_previous(entry));
        assert(c_ini_entry_get_key(entry, NULL));
        assert(c_ini_entry_get_value(entry, NULL));

        entry = c_ini_entry_unref(entry);
}

int main(int argc, char **argv) {
        test_api();
        return 0;
}
