# -----------------------------------------------------------------------------------------------------
# Copyright (c) 2006-2022, Knut Reinert & Freie Universität Berlin
# Copyright (c) 2016-2022, Knut Reinert & MPI für molekulare Genetik
# This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
# shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
# -----------------------------------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.10)

### Find doxygen and dependency to DOT tool
message (STATUS "Searching for doxygen.")
find_package (Doxygen REQUIRED)

if (NOT ${DOXYGEN_FOUND})
    message (FATAL_ERROR "Could not find doxygen. Not building documentation.")
endif ()

if (NOT ${DOXYGEN_HAVE_DOT})
    message (STATUS "Could not find dot tool. Disabling dot support.")
    set (SEQAN3_DOXYGEN_HAVE_DOT "NO")
else ()
    message (STATUS "Found dot tool. Enabling dot support.")
    set (SEQAN3_DOXYGEN_HAVE_DOT "YES")
endif ()

### Use mathjax instead of latex to render formulas.
find_package (LATEX QUIET)

if (${LATEX_FOUND})
    set (SEQAN3_DOXYGEN_USE_MATHJAX "NO")
else ()
    set (SEQAN3_DOXYGEN_USE_MATHJAX "YES")
endif ()

### Number of threads to use for dot. Doxygen's default is 0 (all threads).
set (SEQAN3_DOXYGEN_DOT_NUM_THREADS "0")

### Configure doc/developer targets.
set (SEQAN3_DOXYGEN_SOURCE_DIR "${SEQAN3_CLONE_DIR}")
set (SEQAN3_DOXYFILE_IN ${SEQAN3_DOXYGEN_INPUT_DIR}/seqan3_doxygen_cfg.in)
set (SEQAN3_FOOTER_HTML_IN ${SEQAN3_DOXYGEN_INPUT_DIR}/seqan3_footer.html.in)
# DoxygenLayout.xml.in is created by seqan3-doxygen-layout.cmake
set (SEQAN3_LAYOUT_IN ${CMAKE_CURRENT_BINARY_DIR}/DoxygenLayout.xml.in)

option (SEQAN3_USER_DOC "Create build target and test for user documentation." ON)
option (SEQAN3_DEV_DOC "Create build target and test for developer documentation." ON)
option (SEQAN3_VERCEL_PREVIEW_DOC "Is this a preview build by vercel.com?" OFF)

if (SEQAN3_VERCEL_PREVIEW_DOC)
    set (SEQAN3_DOXYGEN_USE_MATHJAX "YES")
    set (SEQAN3_DOXYGEN_DOT_NUM_THREADS "2")
    set (SEQAN3_DOXYFILE_OPTION_POWERED_BY_VERCEL
         "HTML_EXTRA_FILES       += ${SEQAN3_DOXYGEN_SOURCE_DIR}/test/documentation/.vercel/powered-by-vercel.svg")
    set (SEQAN3_FOOTER_HTML_OPTION_POWERED_BY_VERCEL
         "<li class='footer'><a href='https://vercel.com/?utm_source=seqan&utm_campaign=oss'><img class='footer' src='$relpath^powered-by-vercel.svg' height='31' alt='Powered by Vercel' style='width:unset;'/></a></li>"
    )
endif ()

### Download and extract cppreference-doxygen-web.tag.xml for std:: documentation links
set (SEQAN3_DOXYGEN_STD_TAGFILE "${PROJECT_BINARY_DIR}/cppreference-doxygen-web.tag.xml")
include (ExternalProject)
ExternalProject_Add (
    download-cppreference-doxygen-web-tag
    URL "https://github.com/PeterFeicht/cppreference-doc/releases/download/v20220201/html-book-20220201.tar.xz"
    URL_HASH SHA256=b41960e7ec9c5433b31b1b33db5854f97770ae49535c81e7647f618003102d1a
    TLS_VERIFY ON
    DOWNLOAD_DIR "${PROJECT_BINARY_DIR}"
    DOWNLOAD_NAME "html-book.tar.xz"
    DOWNLOAD_NO_EXTRACT YES
    BINARY_DIR "${PROJECT_BINARY_DIR}"
    CONFIGURE_COMMAND /bin/sh -c "xzcat html-book.tar.xz | tar -xf - cppreference-doxygen-web.tag.xml"
    BUILD_COMMAND rm "html-book.tar.xz"
    INSTALL_COMMAND "")

### TEST HELPER

# doxygen does not show any warnings (doxygen prints warnings / errors to cerr)
set (SEQAN3_TEST_DOXYGEN_FAIL_ON_WARNINGS
     "${DOXYGEN_EXECUTABLE} > doxygen.cout 2> doxygen.cerr; cat \"doxygen.cerr\"; test ! -s \"doxygen.cerr\""
     CACHE INTERNAL "The doxygen test command")

### install helper

# make sure that prefix path is /usr/local/share/doc/seqan3/
if (NOT DEFINED CMAKE_SIZEOF_VOID_P)
    # we need this to suppress GNUInstallDirs AUTHOR_WARNING:
    #   CMake Warning (dev) at /usr/share/cmake-3.19/Modules/GNUInstallDirs.cmake:223 (message):
    #     Unable to determine default CMAKE_INSTALL_LIBDIR directory because no
    #     target architecture is known.  Please enable at least one language before
    #     including GNUInstallDirs.
    set (CMAKE_SIZEOF_VOID_P 8)
endif ()
include (GNUInstallDirs) # this is needed to prefix the install paths
