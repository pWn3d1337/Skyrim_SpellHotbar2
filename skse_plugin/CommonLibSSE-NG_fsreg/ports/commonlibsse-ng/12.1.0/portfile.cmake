# alandtse's fork of CommonLibSSE-NG (the repository was called CommonLibVR before), tag v12.1.0
vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO alandtse/CommonLibSSE-NG
        REF 8ffc2b74af015f837de53cc02b0cb470a15678e4
        SHA512 a9024e40778c74ec025647132b59a27a35f5229a1fb6c69caad139b342f0a35588d6fee371e7f9212d84916b7ed98f65eacc595e3f94cc230cb9575834465f77
        HEAD_REF ng
)

# extern/openvr is a git submodule, the source archive does not contain it. Commit the tag points at.
vcpkg_from_github(
        OUT_SOURCE_PATH OPENVR_SOURCE_PATH
        REPO ValveSoftware/openvr
        REF 60eb187801956ad277f1cae6680e3a410ee0873b
        SHA512 bb85b4705e7095ac65df9969112b2df8930cee7917cc5f14231c5a0ffeed7a73ffa60727fd32f8786a403656f95a3ec0f80bf3ceabc5b8ede964aefb920bc718
        HEAD_REF master
)
file(COPY "${OPENVR_SOURCE_PATH}/headers" DESTINATION "${SOURCE_PATH}/extern/openvr")
file(COPY "${OPENVR_SOURCE_PATH}/lib/win64" DESTINATION "${SOURCE_PATH}/extern/openvr/lib")

# The build fetches the instruction length decoder of MinHook (hde64) with FetchContent, vcpkg builds run without network.
vcpkg_from_github(
        OUT_SOURCE_PATH MINHOOK_SOURCE_PATH
        REPO TsudaKageyu/minhook
        REF c3fcafdc10146beb5919319d0683e44e3c30d537 # v1.3.4
        SHA512 72eede39e2a0ae5b9024769a9aad3878106526f434268b1cff0c1389a015656905b827449a05a5c16df47aa27cd6e4c28959368b710cc499d8cb9a04c53863a5
        HEAD_REF master
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DBUILD_TESTS=OFF
            -DSKSE_SUPPORT_XBYAK=ON
            -DCOMMONLIB_PREBUILT=OFF
            "-DFETCHCONTENT_SOURCE_DIR_HDE64=${MINHOOK_SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake)
vcpkg_copy_pdbs()

file(GLOB CMAKE_CONFIGS "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE/*.cmake")
file(INSTALL ${CMAKE_CONFIGS} DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
# the exported target links Microsoft::DirectXTK, the config of the library only looks for spdlog
file(APPEND "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSEConfig.cmake" "find_dependency(directxtk CONFIG)\n")

# The RE headers of the VR runtime include openvr.h, the library links openvr_api.lib from its source tree.
# Install both and point the exported target at the installed library instead of the build tree.
file(INSTALL "${OPENVR_SOURCE_PATH}/headers/openvr.h" "${OPENVR_SOURCE_PATH}/headers/openvr_capi.h" "${OPENVR_SOURCE_PATH}/headers/openvr_driver.h"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(INSTALL "${OPENVR_SOURCE_PATH}/lib/win64/openvr_api.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(INSTALL "${OPENVR_SOURCE_PATH}/lib/win64/openvr_api.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
file(GLOB TARGET_FILES "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/*-targets*.cmake")
foreach(TARGET_FILE IN LISTS TARGET_FILES)
    vcpkg_replace_string("${TARGET_FILE}" "${SOURCE_PATH}/extern/openvr/lib/win64/openvr_api.lib" "\${_IMPORT_PREFIX}/lib/openvr_api.lib")
endforeach()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING.txt" "${SOURCE_PATH}/EXCEPTIONS.md" "${SOURCE_PATH}/licenses/LICENSE-MIT.txt" "${SOURCE_PATH}/licenses/LICENSE-hde64.txt" "${OPENVR_SOURCE_PATH}/LICENSE")
