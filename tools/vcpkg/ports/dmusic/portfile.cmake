vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/GothicKit/dmusic.git
    REF b73b270b94714b8ec8d9d0b2a9f3a5c445da52d1
    PATCHES cultures-pattern-tracks.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DDM_BUILD_STATIC=ON
        -DDM_BUILD_EXAMPLES=OFF
        -DDM_ENABLE_ASAN=OFF
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
