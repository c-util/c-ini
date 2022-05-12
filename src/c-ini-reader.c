/*
 * Ini-File Reader
 */

#include <c-stdaux.h>
#include <c-utf8.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "c-ini.h"
#include "c-ini-private.h"

int c_ini_reader_init(CIniReader *reader) {
        *reader = (CIniReader)C_INI_READER_NULL(*reader);
        return 0;
}

void c_ini_reader_deinit(CIniReader *reader) {
        free(reader->line);
        c_ini_group_unref(reader->current);
        c_ini_domain_unref(reader->domain);
        *reader = (CIniReader)C_INI_READER_NULL(*reader);
}

_c_public_ int c_ini_reader_new(CIniReader **readerp) {
        _c_cleanup_(c_ini_reader_freep) CIniReader *reader = NULL;
        int r;

        reader = calloc(1, sizeof(*reader));
        if (!reader)
                return -ENOMEM;

        *reader = (CIniReader)C_INI_READER_NULL(*reader);

        r = c_ini_reader_init(reader);
        if (r)
                return r;

        *readerp = reader;
        reader = NULL;
        return 0;
}

_c_public_ void c_ini_reader_set_mode(CIniReader *reader, unsigned int mode) {
        /* make sure no invalid modes are passed */
        c_assert(!(mode & ~(C_INI_MODE_EXTENDED_WHITESPACE |
                            C_INI_MODE_KEEP_DUPLICATE_GROUPS |
                            C_INI_MODE_MERGE_GROUPS |
                            C_INI_MODE_KEEP_DUPLICATE_ENTRIES |
                            C_INI_MODE_OVERRIDE_ENTRIES)));
        /* KEEP_DUPLICATE_GROUPS cannot be combined with MERGE_GROUPS */
        c_assert(!(mode & C_INI_MODE_KEEP_DUPLICATE_GROUPS) ||
                 !(mode & C_INI_MODE_MERGE_GROUPS));
        /* KEEP_DUPLICATE_ENTRIES cannot be combined with OVERRIDE_ENTRIES */
        c_assert(!(mode & C_INI_MODE_KEEP_DUPLICATE_ENTRIES) ||
               !(mode & C_INI_MODE_OVERRIDE_ENTRIES));

        reader->mode = mode;
}

_c_public_ unsigned int c_ini_reader_get_mode(CIniReader *reader) {
        return reader->mode;
}

_c_public_ CIniReader *c_ini_reader_free(CIniReader *reader) {
        if (!reader)
                return NULL;

        c_ini_reader_deinit(reader);
        free(reader);

        return NULL;
}

static int c_ini_reader_parse_entry(CIniReader *reader,
                                    CIniRaw *raw,
                                    size_t i_key,
                                    size_t i_assignment,
                                    size_t n) {
        _c_cleanup_(c_ini_entry_unrefp) CIniEntry *entry = NULL;
        const uint8_t *key = raw->data + i_key;
        const uint8_t *value = raw->data + i_assignment + 1;
        size_t n_key = i_assignment - i_key;
        size_t n_value = n - (i_assignment - i_key + 1);
        CIniGroup *group;
        CIniEntry *dup;
        int r;

        /*
         * The caller verified that this line is a normal assignment. Leading
         * and trailing whitespace are stripped already. @i_key points to the
         * start of the key, @i_assignment to the first assignment operator,
         * and @n is the length from @i_key to the end of the entire
         * assignment.
         */

        /*
         * Spaces around assignment operators are always allowed. In extended
         * mode, all whitespace are allowed. Skip it here. Note that leading
         * whitespace are already skipped by the caller.
         */
        if (reader->mode & C_INI_MODE_EXTENDED_WHITESPACE) {
                while (n_key > 0 && c_ini_is_whitespace(key[n_key - 1]))
                        --n_key;
                while (n_value > 0 && c_ini_is_whitespace(value[0])) {
                        ++value;
                        --n_value;
                }
        } else {
                while (n_key > 0 && key[n_key - 1] == ' ')
                        --n_key;
                while (n_value > 0 && value[0] == ' ') {
                        ++value;
                        --n_value;
                }
        }

        /*
         * Create a new entry. Always do this, even if we discard it later. We
         * want to perform validations regardless whether we keep it or not.
         */
        r = c_ini_entry_new(&entry, key, n_key, value, n_value);
        if (r)
                return r;

        /*
         * If there is no open group, it means the file started with
         * assignments without a prior group header. In this case, we always
         * append the entries to the NULL group.
         */
        group = reader->current ?: reader->domain->null_group;

        /*
         * Now link the entry into the group. If OVERRIDE is active, late
         * entries override previous entries. If duplicates are kept, then we
         * don't merge entries. If neither is set, duplicates are discarded.
         */
        dup = c_ini_group_find(group, (const char *)key, n_key);
        if (dup && reader->mode & C_INI_MODE_OVERRIDE_ENTRIES) {
                c_ini_entry_unlink(dup);
                dup = NULL; /* unref'ed during unlink */
                c_ini_entry_link(entry, group);
        } else if (!dup || reader->mode & C_INI_MODE_KEEP_DUPLICATE_ENTRIES) {
                c_ini_entry_link(entry, group);
        }

        return 0;
}

static int c_ini_reader_parse_group(CIniReader *reader,
                                    CIniRaw *raw,
                                    size_t i_label,
                                    size_t n_label) {
        _c_cleanup_(c_ini_group_unrefp) CIniGroup *group = NULL;
        const uint8_t *label = raw->data + i_label;
        CIniGroup *dup;
        int r;

        /*
         * The caller verified that the current line opens a new group. The
         * opening and closing brackets where already stripped, the remaining
         * bits are indexed from @i_label with length @n_label.
         * All parsers treat the content inside of the brackets as literal
         * group label. Not whitespaces are stripped, nor are any other
         * conversions applied. Hence, we simply create the new group and
         * append it.
         *
         * Note that even if in strict mode, we must always open a new group
         * here. That is, this line definitely opens a new group. If the writer
         * violated the format, we should mark the line as such, but we must
         * open the group so following assignments will be linked to the
         * correct group. IOW, we always represent the entire file in our
         * dataset, but we might mark invalid entries as such and exclude them
         * from the lookup trees (similar to discarded duplicates).
         */

        dup = c_ini_domain_find(reader->domain, (const char *)label, n_label);
        if (dup && reader->mode & C_INI_MODE_MERGE_GROUPS) {
                /* ref/unref in right order, both might be the same */
                c_ini_group_ref(dup);
                c_ini_group_unref(reader->current);
                reader->current = dup;
        } else {
                r = c_ini_group_new(&group, label, n_label);
                if (r)
                        return r;

                if (!dup || reader->mode & C_INI_MODE_KEEP_DUPLICATE_GROUPS)
                        c_ini_group_link(group, reader->domain);

                c_ini_group_unref(reader->current);
                reader->current = c_ini_group_ref(group);
        }

        return 0;
}

static int c_ini_reader_parse_line(CIniReader *reader, CIniRaw *raw) {
        const uint8_t *end, *data = raw->data;
        size_t n = raw->n_data;

        /*
         * Lines must be separated by a '\n' character, and that character
         * only. The last line is not required to be terminated by a newline.
         * Lets cut off the newline here, if it is present.
         */
        if (n > 0 && data[n - 1] == '\n')
                --n;

        /*
         * Trailing carriage returns are ignored, if the specific quirk is
         * enabled.
         */
        if (reader->mode & C_INI_MODE_EXTENDED_WHITESPACE) {
                if (n > 0 && data[n - 1] == '\r')
                        --n;
        }

        /*
         * If requested, skip any leading whitespace.
         */
        if (reader->mode & C_INI_MODE_EXTENDED_WHITESPACE) {
                while (n > 0 && c_ini_is_whitespace(data[0])) {
                        ++data;
                        --n;
                }
        }

        /*
         * Blank lines, and lines starting with '#' are considered comments and
         * are ignored. Bail out early, if a comment is detected.
         */
        if (n < 1 || data[0] == '#')
                return 0;

        /*
         * If a line starts with '[' and ends with ']', we always treat it as a
         * group, regardless whether it is correctly formatted. This avoids
         * accidentally merging two groups just because the group-header is
         * malformatted.
         */
        if (data[0] == '[') {
                const uint8_t *tmp = data + 1;
                size_t n_tmp = n - 1;

                end = memchr(tmp, ']', n_tmp);
                if (end) {
                        /* If requested, skip trailing whitespace */
                        if (reader->mode & C_INI_MODE_EXTENDED_WHITESPACE) {
                                while (n_tmp > 0 && c_ini_is_whitespace(tmp[n_tmp - 1]))
                                        --n_tmp;
                        }

                        if (end - tmp + 1 == (ssize_t)n_tmp)
                                return c_ini_reader_parse_group(reader,
                                                                raw,
                                                                tmp - raw->data,
                                                                n_tmp - 1);
                }
        }

        /*
         * If the line contains any assignment, we parse it into a key-value
         * pair.
         */
        end = memchr(data, '=', n);
        if (end)
                return c_ini_reader_parse_entry(reader,
                                                raw,
                                                data - raw->data,
                                                end - raw->data,
                                                n);

        /*
         * We couldn't detect this line, so ignore it. We keep it around, so a
         * serializer will include it later on, but our parsers will ignore it.
         */
        reader->malformed = true;
        return 0;
}

static int c_ini_reader_commit(CIniReader *reader) {
        _c_cleanup_(c_ini_raw_unrefp) CIniRaw *raw = NULL;
        int r;

        /*
         * So far no format supports multi-line entries. If support for
         * multi-line entries is added, it needs to be detected here, and the
         * line-buffer should simply stay untouched (concatenating the entire
         * entry into a single CIniRaw). Furthermore, the final commit must
         * tell us so, so we don't wait for more input.
         * Once we allocate the CIniRaw entry here, we assume the entry is
         * complete and ready to be parsed.
         */

        r = c_ini_raw_new(&raw, reader->line, reader->n_line);
        if (r)
                return r;

        reader->n_line = 0;
        c_ini_raw_link(raw, reader->domain);

        return c_ini_reader_parse_line(reader, raw);
}

static int c_ini_reader_append(CIniReader *reader, const uint8_t *data, size_t n_data) {
        size_t n;
        void *p;

        if (!n_data)
                return 0;

        if (reader->z_line - reader->n_line < n_data) {
                n = reader->n_line + n_data;
                if (n < C_INI_INITIAL_LINE_SIZE)
                        n = C_INI_INITIAL_LINE_SIZE;
                if (n < reader->n_line)
                        return -E2BIG;

                p = realloc(reader->line, n);
                if (!p)
                        return -ENOMEM;

                reader->line = p;
                reader->z_line = n;
        }

        c_memcpy(reader->line + reader->n_line, data, n_data);
        reader->n_line += n_data;
        return 0;
}

_c_public_ int c_ini_reader_feed(CIniReader *reader, const uint8_t *data, size_t n_data) {
        const uint8_t *end;
        size_t n;
        int r;

        if (!n_data)
                return 0;

        if (!reader->domain) {
                /*
                 * This is the first data-set being pushed into the reader.
                 * Allocate a new domain that we use to collect all the data.
                 */
                r = c_ini_domain_new(&reader->domain);
                if (r)
                        return r;
        }

        /*
         * All currently supported formats have in common that they are
         * line-based. Hence, read the provided data into a line-buffer and
         * commit it at every newline.
         * If a specific format needs to merge multiple lines, it should not
         * discard the line-buffer. Further input will be appended to the
         * line-buffer until the next newline.
         */
        while ((end = memchr(data, '\n', n_data))) {
                n = end - data + 1;

                r = c_ini_reader_append(reader, data, n);
                if (r)
                        return r;

                r = c_ini_reader_commit(reader);
                if (r)
                        return r;

                n_data -= n;
                data += n;
        }

        /*
         * The remaining data has no more newlines. Simply append it to the
         * line-buffer. The next call will continue where we left off.
         */
        return c_ini_reader_append(reader, data, n_data);
}

_c_public_ int c_ini_reader_seal(CIniReader *reader, CIniDomain **domainp) {
        int r;

        if (!reader->domain) {
                /*
                 * Nothing was pushed into the reader. This means the file or
                 * data-set was empty. Lets create an empty domain, so we can
                 * return it to the caller later.
                 */
                r = c_ini_domain_new(&reader->domain);
                if (r)
                        return r;
        }

        /*
         * There might be data in the line-buffer. No trailing newline
         * is required, so simply commit the last line.
         */
        r = c_ini_reader_commit(reader);
        if (r)
                return r;

        /*
         * Reset internal state so we are prepared for the next parsing round.
         * The entire state must be reset, unless requested otherwise.
         */
        reader->current = c_ini_group_unref(reader->current);
        reader->malformed = false;

        /*
         * Hand the entire domain to the caller (including the ref-count). It
         * is now completely owned by the caller. The next parsing round will
         * allocate a new domain.
         */
        *domainp = reader->domain;
        reader->domain = NULL;
        return 0;
}

_c_public_ int c_ini_reader_parse(CIniDomain **domainp,
                                  unsigned int mode,
                                  const uint8_t *data,
                                  size_t n_data) {
        _c_cleanup_(c_ini_reader_deinit) CIniReader reader = C_INI_READER_NULL(reader);
        int r;

        r = c_ini_reader_init(&reader);
        if (r)
                return r;

        reader.mode = mode;

        r = c_ini_reader_feed(&reader, data, n_data);
        if (r)
                return r;

        return c_ini_reader_seal(&reader, domainp);
}
