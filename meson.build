project(
        'c-ini',
        'c',
        default_options: [
                'c_std=c11',
        ],
        license: 'Apache',
        meson_version: '>=0.60.0',
        version: '1.0.0',
)
major = meson.project_version().split('.')[0]
project_description = 'Ini-File Handling'

mod_pkgconfig = import('pkgconfig')

dep_clist = dependency('libclist-3')
dep_crbtree = dependency('libcrbtree-3')
dep_cstdaux = dependency('libcstdaux-1')
dep_cutf8 = dependency('libcutf8-1')
add_project_arguments(dep_cstdaux.get_variable('cflags').split(' '), language: 'c')

subdir('src')

meson.override_dependency('libcini-'+major, libcini_dep, static: true)
