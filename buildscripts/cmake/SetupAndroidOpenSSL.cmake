# SPDX-License-Identifier: GPL-3.0-only
#
# Qt's TLS backend loads OpenSSL at runtime, and Android does not provide it to apps.
# Prebuilt OpenSSL 3 libraries (Apache-2.0) from KDAB/android_openssl, the source recommended by Qt,
# are downloaded at a pinned commit, verified, and packaged into the APK.
# Only the two libraries for the ABI being built are downloaded (the repository is ~200 MB).

set(VS_ANDROID_OPENSSL_COMMIT b71f1470962019bd89534a2919f5925f93bc5779) # OpenSSL 3.1.8, 16 KB pages

set(VS_ANDROID_OPENSSL_SHA256_arm64-v8a_libcrypto_3.so 1e6c12ae0c2dadfe9d178d7f80f0ab248a1877a234066098bf5594e5205e5740)
set(VS_ANDROID_OPENSSL_SHA256_arm64-v8a_libssl_3.so 01d2bd0baac626efd3309f35f99c4b826dd9b885a7e4d14d5b12b3603d3a407f)
set(VS_ANDROID_OPENSSL_SHA256_x86_64_libcrypto_3.so bc4bdd31835659e639c2dc8f0a722342f51230074c23ee3dd6d1fa2353d72db1)
set(VS_ANDROID_OPENSSL_SHA256_x86_64_libssl_3.so 2af32571317adf90fb7a007756a17dd15cd2469da9ebbd4e973883f8753369c1)

function(vs_add_android_openssl target)
    set(abi ${ANDROID_ABI})
    set(dir "${CMAKE_BINARY_DIR}/android_openssl/${abi}")

    set(libs)
    foreach(lib libcrypto_3.so libssl_3.so)
        set(hash ${VS_ANDROID_OPENSSL_SHA256_${abi}_${lib}})
        if (NOT hash)
            message(WARNING "No OpenSSL for ${abi}: HTTPS will not work")
            return()
        endif()

        set(file "${dir}/${lib}")
        if (NOT EXISTS "${file}")
            message(STATUS "Downloading ${lib} for ${abi}")
            file(DOWNLOAD
                "https://raw.githubusercontent.com/KDAB/android_openssl/${VS_ANDROID_OPENSSL_COMMIT}/ssl_3/${abi}/${lib}"
                "${file}"
                EXPECTED_HASH SHA256=${hash}
                TLS_VERIFY ON
            )
        endif()

        list(APPEND libs "${file}")
    endforeach()

    set_property(TARGET ${target} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${libs})
endfunction()
