/*
 * Ini-File Handling
 */

#include <c-list.h>
#include <c-rbtree.h>
#include <c-stdaux.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "c-ini.h"
#include "c-ini-private.h"

static int c_ini_entry_compare(CRBTree *t, void *k, CRBNode *rb) {
        CIniBytes *bytes = (CIniBytes *)k;
        CIniEntry *entry = c_rbnode_entry(rb, CIniEntry, rb_group);

        if (bytes->n_data < entry->n_key)
                return -1;
        else if (bytes->n_data > entry->n_key)
                return 1;
        else
                return memcmp(bytes->data, entry->key, bytes->n_data);
}

int c_ini_entry_new(CIniEntry **entryp,
                    const uint8_t *key,
                    size_t n_key,
                    const uint8_t *value,
                    size_t n_value) {
        _c_cleanup_(c_ini_entry_unrefp) CIniEntry *entry = NULL;
        _c_cleanup_(c_ini_freep) uint8_t *dup_key = NULL, *dup_value = NULL;

        dup_key = calloc(1, n_key + 1);
        if (!dup_key)
                return -ENOMEM;

        memcpy(dup_key, key, n_key);

        dup_value = calloc(1, n_value + 1);
        if (!dup_value)
                return -ENOMEM;

        memcpy(dup_value, value, n_value);

        entry = calloc(1, sizeof(*entry));
        if (!entry)
                return -ENOMEM;

        *entry = (CIniEntry)C_INI_ENTRY_NULL(*entry);
        entry->key = dup_key;
        dup_key = NULL;
        entry->n_key = n_key;
        entry->value = dup_value;
        dup_value = NULL;
        entry->n_value = n_value;

        *entryp = entry;
        entry = NULL;
        return 0;
}

static CIniEntry *c_ini_entry_free_internal(CIniEntry *entry) {
        if (!entry)
                return NULL;

        c_assert(!entry->group);
        c_assert(!c_list_is_linked(&entry->link_group));
        c_assert(!c_rbnode_is_linked(&entry->rb_group));

        free(entry->value);
        free(entry->key);
        free(entry);

        return NULL;
}

_c_public_ CIniEntry *c_ini_entry_ref(CIniEntry *entry) {
        if (entry)
                ++entry->n_refs;
        return entry;
}

_c_public_ CIniEntry *c_ini_entry_unref(CIniEntry *entry) {
        if (entry && !--entry->n_refs)
                c_ini_entry_free_internal(entry);
        return NULL;
}

void c_ini_entry_link(CIniEntry *entry, CIniGroup *group) {
        CIniBytes bytes = C_INI_BYTES_INIT((uint8_t *)entry->key, entry->n_key);
        CRBNode **slot, *parent;
        int r;

        c_assert(!entry->group);

        slot = &group->map_entries.root;
        parent = NULL;
        while (*slot) {
                parent = *slot;
                r = c_ini_entry_compare(&group->map_entries, &bytes, *slot);
                if (r < 0)
                        slot = &(*slot)->left;
                else
                        slot = &(*slot)->right;
        }

        c_ini_entry_ref(entry);
        entry->group = group;
        c_list_link_tail(&group->list_entries, &entry->link_group);
        c_rbtree_add(&group->map_entries, parent, slot, &entry->rb_group);
}

void c_ini_entry_unlink(CIniEntry *entry) {
        if (entry->group) {
                c_rbnode_unlink(&entry->rb_group);
                c_list_unlink(&entry->link_group);
                entry->group = NULL;
                entry = c_ini_entry_unref(entry);
                /* @entry might be gone here */
        }
}

_c_public_ CIniEntry *c_ini_entry_next(CIniEntry *entry) {
        if (!entry->group || entry->link_group.next == &entry->group->list_entries)
                return NULL;
        return c_list_entry(entry->link_group.next, CIniEntry, link_group);
}

_c_public_ CIniEntry *c_ini_entry_previous(CIniEntry *entry) {
        if (!entry->group || entry->link_group.prev == &entry->group->list_entries)
                return NULL;
        return c_list_entry(entry->link_group.prev, CIniEntry, link_group);
}

_c_public_ const char *c_ini_entry_get_key(CIniEntry *entry, size_t *n_keyp) {
        if (n_keyp)
                *n_keyp = entry->n_key;
        return (const char *)entry->key;
}

_c_public_ const char *c_ini_entry_get_value(CIniEntry *entry, size_t *n_valuep) {
        if (n_valuep)
                *n_valuep = entry->n_value;
        return (const char *)entry->value;
}

static int c_ini_group_compare(CRBTree *t, void *k, CRBNode *rb) {
        CIniBytes *bytes = (CIniBytes *)k;
        CIniGroup *group = c_rbnode_entry(rb, CIniGroup, rb_domain);

        if (bytes->n_data < group->n_label)
                return -1;
        else if (bytes->n_data > group->n_label)
                return 1;
        else
                return memcmp(bytes->data, group->label, group->n_label);
}

int c_ini_group_new(CIniGroup **groupp, const uint8_t *label, size_t n_label) {
        _c_cleanup_(c_ini_group_unrefp) CIniGroup *group = NULL;
        _c_cleanup_(c_ini_freep) uint8_t *dup = NULL;

        dup = calloc(1, n_label + 1);
        if (!dup)
                return -ENOMEM;

        memcpy(dup, label, n_label);

        group = calloc(1, sizeof(*group));
        if (!group)
                return -ENOMEM;

        *group = (CIniGroup)C_INI_GROUP_NULL(*group);
        group->label = dup;
        dup = NULL;
        group->n_label = n_label;

        *groupp = group;
        group = NULL;
        return 0;
}

static CIniGroup *c_ini_group_free_internal(CIniGroup *group) {
        CIniEntry *entry, *t_entry;

        if (!group)
                return NULL;

        c_list_for_each_entry_safe(entry, t_entry, &group->list_entries, link_group)
                c_ini_entry_unlink(entry);

        c_assert(!c_list_is_linked(&group->link_domain));
        c_assert(!c_rbnode_is_linked(&group->rb_domain));
        c_assert(!group->domain);

        free(group->label);
        free(group);

        return NULL;
}

_c_public_ CIniGroup *c_ini_group_ref(CIniGroup *group) {
        if (group)
                ++group->n_refs;
        return group;
}

_c_public_ CIniGroup *c_ini_group_unref(CIniGroup *group) {
        if (group && !--group->n_refs)
                c_ini_group_free_internal(group);
        return NULL;
}

void c_ini_group_link(CIniGroup *group, CIniDomain *domain) {
        CIniBytes bytes = C_INI_BYTES_INIT((uint8_t *)group->label, group->n_label);
        CRBNode **slot, *parent;
        int r;

        c_assert(!group->domain);

        slot = &domain->map_groups.root;
        parent = NULL;
        while (*slot) {
                parent = *slot;
                r = c_ini_group_compare(&domain->map_groups, &bytes, *slot);
                if (r < 0)
                        slot = &(*slot)->left;
                else
                        slot = &(*slot)->right;
        }

        c_ini_group_ref(group);
        group->domain = domain;
        c_list_link_tail(&domain->list_groups, &group->link_domain);
        c_rbtree_add(&domain->map_groups, parent, slot, &group->rb_domain);
}

void c_ini_group_unlink(CIniGroup *group) {
        if (group->domain) {
                c_rbnode_unlink(&group->rb_domain);
                c_list_unlink(&group->link_domain);
                group->domain = NULL;
                group = c_ini_group_unref(group);
                /* @group might be gone here */
        }
}

_c_public_ CIniGroup *c_ini_group_next(CIniGroup *group) {
        if (!group->domain || group->link_domain.next == &group->domain->list_groups)
                return NULL;
        return c_list_entry(group->link_domain.next, CIniGroup, link_domain);
}

_c_public_ CIniGroup *c_ini_group_previous(CIniGroup *group) {
        if (!group->domain || group->link_domain.prev == &group->domain->list_groups)
                return NULL;
        return c_list_entry(group->link_domain.prev, CIniGroup, link_domain);
}

_c_public_ const char *c_ini_group_get_label(CIniGroup *group, size_t *n_labelp) {
        if (n_labelp)
                *n_labelp = group->n_label;
        return (const char *)group->label;
}

_c_public_ CIniEntry *c_ini_group_iterate(CIniGroup *group) {
        return c_list_first_entry(&group->list_entries, CIniEntry, link_group);
}

_c_public_ CIniEntry *c_ini_group_find(CIniGroup *group, const char *label, ssize_t n_label) {
        CIniBytes bytes;
        CRBNode *iter;
        int r;

        if (n_label < 0)
                n_label = strlen(label);

        bytes = (CIniBytes)C_INI_BYTES_INIT((uint8_t *)label, n_label);

        iter = group->map_entries.root;
        while (iter) {
                r = c_ini_entry_compare(&group->map_entries, &bytes, iter);
                if (r < 0)
                        iter = iter->left;
                else if (r > 0)
                        iter = iter->right;
                else
                        break;
        }

        if (iter) {
                /*
                 * If there are multiple entries with the same key, always make
                 * sure to return the leftmost (earliest addition). This
                 * guarantees our API is consistent and deterministic regarding
                 * duplicates.
                 */
                while (iter->left &&
                       !c_ini_entry_compare(&group->map_entries, &bytes, iter->left))
                        iter = iter->left;
        }

        return c_rbnode_entry(iter, CIniEntry, rb_group);
}

int c_ini_raw_new(CIniRaw **rawp, const uint8_t *data, size_t n_data) {
        _c_cleanup_(c_ini_raw_unrefp) CIniRaw *raw = NULL;

        raw = calloc(1, sizeof(*raw) + n_data + 1);
        if (!raw)
                return -ENOMEM;

        *raw = (CIniRaw)C_INI_RAW_NULL(*raw);
        raw->n_data = n_data;
        memcpy(raw->data, data, n_data);

        *rawp = raw;
        raw = NULL;
        return 0;
}

static CIniRaw *c_ini_raw_free_internal(CIniRaw *raw) {
        if (!raw)
                return NULL;

        c_assert(!c_list_is_linked(&raw->link_domain));

        free(raw);

        return NULL;
}

CIniRaw *c_ini_raw_ref(CIniRaw *raw) {
        if (raw)
                ++raw->n_refs;
        return raw;
}

CIniRaw *c_ini_raw_unref(CIniRaw *raw) {
        if (raw && !--raw->n_refs)
                c_ini_raw_free_internal(raw);
        return NULL;
}

void c_ini_raw_link(CIniRaw *raw, CIniDomain *domain) {
        c_assert(!raw->domain);

        c_ini_raw_ref(raw);
        raw->domain = domain;
        c_list_link_tail(&domain->list_raws, &raw->link_domain);
}

void c_ini_raw_unlink(CIniRaw *raw) {
        if (raw->domain) {
                c_list_unlink(&raw->link_domain);
                raw->domain = NULL;
                c_ini_raw_unref(raw);
        }
}

int c_ini_domain_new(CIniDomain **domainp) {
        _c_cleanup_(c_ini_domain_unrefp) CIniDomain *domain = NULL;
        int r;

        domain = calloc(1, sizeof(*domain));
        if (!domain)
                return -ENOMEM;

        *domain = (CIniDomain)C_INI_DOMAIN_NULL(*domain);

        r = c_ini_group_new(&domain->null_group, NULL, 0);
        if (r)
                return r;

        *domainp = domain;
        domain = NULL;
        return 0;
}

static CIniDomain *c_ini_domain_free_internal(CIniDomain *domain) {
        CIniGroup *group, *t_group;
        CIniRaw *raw, *t_raw;

        if (!domain)
                return NULL;

        c_list_for_each_entry_safe(raw, t_raw, &domain->list_raws, link_domain)
                c_ini_raw_unlink(raw);

        c_list_for_each_entry_safe(group, t_group, &domain->list_groups, link_domain)
                c_ini_group_unlink(group);

        c_assert(c_list_is_empty(&domain->list_raws));
        c_assert(c_list_is_empty(&domain->list_groups));
        c_assert(c_rbtree_is_empty(&domain->map_groups));

        c_ini_group_unref(domain->null_group);
        free(domain);

        return NULL;
}

_c_public_ CIniDomain *c_ini_domain_ref(CIniDomain *domain) {
        if (domain)
                ++domain->n_refs;
        return domain;
}

_c_public_ CIniDomain *c_ini_domain_unref(CIniDomain *domain) {
        if (domain && !--domain->n_refs)
                c_ini_domain_free_internal(domain);
        return NULL;
}

_c_public_ CIniGroup *c_ini_domain_get_null_group(CIniDomain *domain) {
        return domain->null_group;
}

_c_public_ CIniGroup *c_ini_domain_iterate(CIniDomain *domain) {
        return c_list_first_entry(&domain->list_groups, CIniGroup, link_domain);
}

_c_public_ CIniGroup *c_ini_domain_find(CIniDomain *domain, const char *label, ssize_t n_label) {
        CIniBytes bytes;
        CRBNode *iter;
        int r;

        if (n_label < 0)
                n_label = strlen(label);

        bytes = (CIniBytes)C_INI_BYTES_INIT((uint8_t *)label, n_label);

        iter = domain->map_groups.root;
        while (iter) {
                r = c_ini_group_compare(&domain->map_groups, &bytes, iter);
                if (r < 0)
                        iter = iter->left;
                else if (r > 0)
                        iter = iter->right;
                else
                        break;
        }

        if (iter) {
                /*
                 * If there are multiple entries with the same key, always make
                 * sure to return the leftmost (earliest addition). This
                 * guarantees our API is consistent and deterministic regarding
                 * duplicates.
                 */
                while (iter->left &&
                       !c_ini_group_compare(&domain->map_groups, &bytes, iter->left))
                        iter = iter->left;
        }

        return c_rbnode_entry(iter, CIniGroup, rb_domain);
}
