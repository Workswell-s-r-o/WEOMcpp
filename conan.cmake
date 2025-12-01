# Install third-party dependencies
set(W_CONAN_INSTALL_DIR "${PROJECT_BINARY_DIR}/_deps")

function(conan_install)
    if(DEFINED CMAKE_CXX_COMPILER_ID)
        set(_compiler ${CMAKE_CXX_COMPILER_ID})
        set(_compiler_version ${CMAKE_CXX_COMPILER_VERSION})
    else()
        if(NOT DEFINED CMAKE_C_COMPILER_ID)
            message(FATAL_ERROR "C or C++ compiler not defined")
        endif()
        set(_compiler ${CMAKE_C_COMPILER_ID})
        set(_compiler_version ${CMAKE_C_COMPILER_VERSION})
    endif()

    if(_compiler MATCHES MSVC)
        set(_compiler "msvc")
        string(SUBSTRING ${MSVC_VERSION} 0 3 _compiler_version)
    elseif(_compiler MATCHES Clang)
        set(_compiler "clang")
        string(REPLACE "." ";" VERSION_LIST ${_compiler_version})
        list(GET VERSION_LIST 0 _compiler_version)
    elseif(_compiler MATCHES GNU)
        set(_compiler "gcc")
        string(REPLACE "." ";" VERSION_LIST ${_compiler_version})
        list(GET VERSION_LIST 0 _compiler_version)
    endif()

    if (LINUX)
        set(W_CONAN_ADDITIONAL_INSTALL_ARGS -s:a compiler.libcxx=libstdc++11 ${W_CONAN_ADDITIONAL_INSTALL_ARGS}) 
    endif()

    if (APPLE)
        set(W_CONAN_ADDITIONAL_INSTALL_ARGS -s:a compiler.libcxx=libc++ ${W_CONAN_ADDITIONAL_INSTALL_ARGS})
    endif()

    message(STATUS "CMake-Conan: compiler=${_compiler}")
    message(STATUS "CMake-Conan: compiler.version=${_compiler_version}")

    if(NOT CMAKE_BUILD_TYPE)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set!")
    endif()

    execute_process(COMMAND conan install ${PROJECT_SOURCE_DIR}
        --output-folder=${W_CONAN_INSTALL_DIR}
        -s:a compiler=${_compiler}
        -s:a compiler.version=${_compiler_version}
        -s:a build_type=${CMAKE_BUILD_TYPE}
        --build=missing
        ${W_CONAN_ADDITIONAL_INSTALL_ARGS})
endfunction()

conan_install()

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
list(PREPEND CMAKE_MODULE_PATH ${W_CONAN_INSTALL_DIR})
list(PREPEND CMAKE_PREFIX_PATH ${W_CONAN_INSTALL_DIR})
