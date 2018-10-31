/*
 * Tests for Basic Operations
 * This test does some basic operations and verifies their correctness. This
 * includes testing for API guarantees, as well as functional correctness.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-ini.h"
#include "c-ini-private.h"

static void test_basic_reader(void) {
        char input[] = "\n"
                       "key=value\n"
                       "\n"
                       "[group_0]\n"
                       "# comment\n"
                       "key0_0=value0_0\n"
                       "key0_1=value0_1\n"
                       "key0_2=value0_2\n"
                       "\n"
                       "[group_1]\n"
                       "# comment\n"
                       "key1_0=value1_0\n"
                       "key1_1=value1_1\n"
                       "key1_2=value1_2\n"
                       "";
        _cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;
        CIniGroup *group, *prev_group;
        CIniEntry *entry, *prev_entry;
        size_t i, j;
        int r;

        /* parse data */

        r = c_ini_reader_parse(&domain,
                               0,
                               (const uint8_t *)input,
                               strlen(input));
        assert(!r);

        /* verify null-group */

        group = c_ini_domain_get_null_group(domain);
        assert(group);
        assert(!c_ini_group_previous(group));
        assert(!c_ini_group_next(group));

        entry = c_ini_group_iterate(group);
        assert(entry);
        assert(!c_ini_entry_previous(entry));
        assert(!c_ini_entry_next(entry));
        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key"));
        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value"));

        /* iterate all groups recursively */

        i = 0;
        prev_group = NULL;
        for (group = c_ini_domain_iterate(domain);
             group;
             group = c_ini_group_next(group)) {
                assert(c_ini_group_previous(group) == prev_group);

                switch (i++) {
                case 0:
                        assert(!strcmp(c_ini_group_get_label(group, NULL), "group_0"));

                        j = 0;
                        prev_entry = NULL;
                        for (entry = c_ini_group_iterate(group);
                             entry;
                             entry = c_ini_entry_next(entry)) {
                                assert(c_ini_entry_previous(entry) == prev_entry);

                                switch (j++) {
                                case 0:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key0_0"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value0_0"));
                                        break;
                                case 1:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key0_1"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value0_1"));
                                        break;
                                case 2:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key0_2"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value0_2"));
                                        break;
                                default:
                                        assert(0);
                                        break;
                                }

                                prev_entry = entry;
                        }
                        assert(j == 3);

                        break;
                case 1:
                        assert(!strcmp(c_ini_group_get_label(group, NULL), "group_1"));

                        j = 0;
                        prev_entry = NULL;
                        for (entry = c_ini_group_iterate(group);
                             entry;
                             entry = c_ini_entry_next(entry)) {
                                assert(c_ini_entry_previous(entry) == prev_entry);

                                switch (j++) {
                                case 0:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key1_0"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value1_0"));
                                        break;
                                case 1:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key1_1"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value1_1"));
                                        break;
                                case 2:
                                        assert(!strcmp(c_ini_entry_get_key(entry, NULL), "key1_2"));
                                        assert(!strcmp(c_ini_entry_get_value(entry, NULL), "value1_2"));
                                        break;
                                default:
                                        assert(0);
                                        break;
                                }

                                prev_entry = entry;
                        }
                        assert(j == 3);

                        break;
                default:
                        assert(0);
                        break;
                }

                prev_group = group;
        }
        assert(i == 2);
}

int main(int argc, char *argv[]) {
        test_basic_reader();
        return 0;
}
