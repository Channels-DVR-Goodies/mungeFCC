cmake_minimum_required(VERSION 3.10)
project( mungeFCC )

file(GLOB HASHES "*.hash")
set(OUTFILES)
foreach (HASH ${HASHES})

    string(REGEX REPLACE "(.*).hash$" "\\1.h" OUTPUT_FILE_NAME ${HASH})

    add_custom_command(
            OUTPUT "${OUTPUT_FILE_NAME}"
            COMMAND hashstrings ${HASH}
            DEPENDS "${HASH}")

    set(OUTFILES ${OUTFILES} "${OUTPUT_FILE_NAME}")

endforeach (HASH)

add_custom_target(hashes ALL DEPENDS ${OUTFILES})

add_executable( mungeFCC
                mungeFCC.c mungeFCC.h
                affiliate.h)

target_link_libraries(mungeFCC hashstrings)