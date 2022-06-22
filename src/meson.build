#
# target: libcini.so
#

libcini_symfile = join_paths(meson.current_source_dir(), 'libcini.sym')

libcini_deps = [
        dep_clist,
        dep_crbtree,
        dep_cstdaux,
        dep_cutf8,
]

libcini_both = both_libraries(
        'cini-'+major,
        [
                'c-ini.c',
                'c-ini-reader.c',
        ],
        c_args: [
                '-fvisibility=hidden',
                '-fno-common',
        ],
        dependencies: libcini_deps,
        install: not meson.is_subproject(),
        link_args: dep_cstdaux.get_variable('version-scripts') == 'yes' ? [
                '-Wl,--version-script=@0@'.format(libcini_symfile),
        ] : [],
        link_depends: libcini_symfile,
        soversion: 0,
)

libcini_dep = declare_dependency(
        dependencies: libcini_deps,
        include_directories: include_directories('.'),
        link_with: libcini_both.get_static_lib(),
        version: meson.project_version(),
)

if not meson.is_subproject()
        install_headers('c-ini.h')

        mod_pkgconfig.generate(
                description: project_description,
                filebase: 'libcini-'+major,
                libraries: libcini_both.get_shared_lib(),
                name: 'libcini',
                version: meson.project_version(),
        )
endif

#
# target: test-*
#

test_api = executable('test-api', ['test-api.c'], link_with: libcini_both.get_shared_lib())
test('API Symbol Visibility', test_api)

test_basic = executable('test-basic', ['test-basic.c'], dependencies: libcini_dep)
test('Basic API Behavior', test_basic)

test_reader = executable('test-reader', ['test-reader.c'], dependencies: libcini_dep)
test('Parser Capabilities', test_reader)
