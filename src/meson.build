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

libcini_private = static_library(
        'cini-private',
        [
                'c-ini.c',
                'c-ini-reader.c',
        ],
        c_args: [
                '-fvisibility=hidden',
                '-fno-common',
        ],
        dependencies: libcini_deps,
        pic: true,
)

libcini_shared = shared_library(
        'cini',
        objects: libcini_private.extract_all_objects(),
        dependencies: libcini_deps,
        install: not meson.is_subproject(),
        soversion: 0,
        link_depends: libcini_symfile,
        link_args: [
                '-Wl,--no-undefined',
                '-Wl,--version-script=@0@'.format(libcini_symfile),
        ],
)

libcini_dep = declare_dependency(
        include_directories: include_directories('.'),
        link_with: libcini_private,
        dependencies: libcini_deps,
        version: meson.project_version(),
)

if not meson.is_subproject()
        install_headers('c-ini.h')

        mod_pkgconfig.generate(
                libraries: libcini_shared,
                version: meson.project_version(),
                name: 'libcini',
                filebase: 'libcini',
                description: project_description,
        )
endif

#
# target: test-*
#

test_api = executable('test-api', ['test-api.c'], link_with: libcini_shared)
test('API Symbol Visibility', test_api)

test_basic = executable('test-basic', ['test-basic.c'], dependencies: libcini_dep)
test('Basic API Behavior', test_basic)

test_reader = executable('test-reader', ['test-reader.c'], dependencies: libcini_dep)
test('Parser Capabilities', test_reader)
