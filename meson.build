project(
        'c-ini',
        'c',
        version: '1',
        license: 'Apache',
        default_options: [
                'c_std=c11',
        ],
)
project_description = 'Ini-File Handling'

add_project_arguments('-D_GNU_SOURCE', language: 'c')
mod_pkgconfig = import('pkgconfig')

sub_clist = subproject('c-list')
sub_crbtree = subproject('c-rbtree')
sub_cutf8 = subproject('c-utf8')

dep_clist = sub_clist.get_variable('libclist_dep')
dep_crbtree = sub_crbtree.get_variable('libcrbtree_dep')
dep_cutf8 = sub_cutf8.get_variable('libcutf8_dep')

subdir('src')
