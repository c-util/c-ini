/*
 * Tests for Basic Operations
 * This test does some basic operations and verifies their correctness. This
 * includes testing for API guarantees, as well as functional correctness.
 */

#undef NDEBUG
#include <assert.h>
#include <c-stdaux.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-ini.h"
#include "c-ini-private.h"

static void test_reader_assert_empty(CIniDomain *domain) {
        c_assert(!c_ini_group_iterate(c_ini_domain_get_null_group(domain)));
        c_assert(!c_ini_domain_iterate(domain));
}

static void test_reader_assert_null_singleton(CIniDomain *domain,
                                              const char *key,
                                              const char *value) {
        CIniGroup *group;
        CIniEntry *entry;

        c_assert(!c_ini_domain_iterate(domain));

        group = c_ini_domain_get_null_group(domain);
        entry = c_ini_group_iterate(group);
        c_assert(entry);
        c_assert(!c_ini_entry_next(entry));
        c_assert(!c_ini_entry_previous(entry));
        c_assert(!strcmp(key, c_ini_entry_get_key(entry, NULL)));
        c_assert(!strcmp(value, c_ini_entry_get_value(entry, NULL)));
}

static void test_reader_assert_singleton(CIniDomain *domain,
                                         const char *label,
                                         const char *key,
                                         const char *value) {
        CIniGroup *group;
        CIniEntry *entry;

        c_assert(!c_ini_group_iterate(c_ini_domain_get_null_group(domain)));

        group = c_ini_domain_iterate(domain);
        c_assert(group);
        c_assert(!c_ini_group_next(group));
        c_assert(!c_ini_group_previous(group));
        c_assert(!strcmp(label, c_ini_group_get_label(group, NULL)));

        entry = c_ini_group_iterate(group);
        c_assert(entry);
        c_assert(!c_ini_entry_next(entry));
        c_assert(!c_ini_entry_previous(entry));
        c_assert(!strcmp(key, c_ini_entry_get_key(entry, NULL)));
        c_assert(!strcmp(value, c_ini_entry_get_value(entry, NULL)));
}

static void test_reader_normal_whitespace(void) {
        const char *input[] = {
                /* normalized input */
                "[group]\n"
                "key=value\n"
                "",

                /* no newline at the end (must correctly parse last line) */
                "[group]\n"
                "key=value"
                "",

                /* bunch of newlines everywhere (must be ignored) */
                "\n\n"
                "[group]\n\n"
                "key=value\n\n"
                "",

                /* spaces around assignments (must be stripped) */
                "[group]\n"
                "key = value\n"
                "",

                /* more spaces around assignments (must be stripped) */
                "[group]\n"
                "key    =    value\n"
                "",

                /* spaces in labels (must NOT be stripped, except around '=') */
                "[  group  ]\n"
                "  key=value  \n"
                "",

                /* spaces before groups (must discard the line as invalid) */
                " [group]\n"
                "key=value\n"
                "",

                /* spaces after groups (must discard the line as invalid) */
                "[group] \n"
                "key=value\n"
                "",

                /* no group (put assignment in null-group) */
                "key=value\n"
                "",

                /* comments must not start with spaces (they become normal lines otherwise)*/
                "[group]\n"
                "# valid = comment\n"
                " # key = value\n"
                "",
        };
        const struct {
                const char *label;
                const char *key;
                const char *value;
        } output[] = {
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "  group  ",
                        .key = "  key",
                        .value = "value  ",
                },
                {
                        .key = "key",
                        .value = "value",
                },
                {
                        .key = "key",
                        .value = "value",
                },
                {
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = " # key",
                        .value = "value",
                },
        };
        size_t i;
        int r;

        c_assert(sizeof(input) / sizeof(*input) == sizeof(output) / sizeof(*output));

        for (i = 0; i < sizeof(input) / sizeof(*input); ++i) {
                _c_cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;

                r = c_ini_reader_parse(&domain, 0, (void*)input[i], strlen(input[i]));
                c_assert(!r);

                if (output[i].label)
                        test_reader_assert_singleton(domain,
                                                     output[i].label,
                                                     output[i].key,
                                                     output[i].value);
                else if (output[i].key)
                        test_reader_assert_null_singleton(domain,
                                                          output[i].key,
                                                          output[i].value);
                else
                        test_reader_assert_empty(domain);
        }
}

static void test_reader_extended_whitespace(void) {
        const char *input[] = {
                /* normalized input */
                "[group]\n"
                "key=value\n"
                "",

                /* spaces around group are ignored */
                "  [group]  \n"
                "key=value\n"
                "",

                /* spaces around key/value are ignored, except after value */
                "[group]\n"
                "  key  =  value  \n"
                "",

                /* spaces include more than just \x20 */
                "[group]\n"
                "\x09\x0a\x0c\x0d\x20\n"
                "key=value\n"
                "",

                /* \r\n line-ending are supported */
                "[group]\r\n"
                "key=value\r\n"
                "",

                /* comments can now be indented */
                "[group]\n"
                " # foo = bar\n"
                "key=value\n"
                "",
        };
        const struct {
                const char *label;
                const char *key;
                const char *value;
        } output[] = {
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value  ",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
                {
                        .label = "group",
                        .key = "key",
                        .value = "value",
                },
        };
        size_t i;
        int r;

        c_assert(sizeof(input) / sizeof(*input) == sizeof(output) / sizeof(*output));

        for (i = 0; i < sizeof(input) / sizeof(*input); ++i) {
                _c_cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;

                r = c_ini_reader_parse(&domain,
                                       C_INI_MODE_EXTENDED_WHITESPACE,
                                       (void*)input[i],
                                       strlen(input[i]));
                c_assert(!r);

                if (output[i].label)
                        test_reader_assert_singleton(domain,
                                                     output[i].label,
                                                     output[i].key,
                                                     output[i].value);
                else if (output[i].key)
                        test_reader_assert_null_singleton(domain,
                                                          output[i].key,
                                                          output[i].value);
                else
                        test_reader_assert_empty(domain);
        }
}

int main(int argc, char *argv[]) {
        test_reader_normal_whitespace();
        test_reader_extended_whitespace();
        return 0;
}
