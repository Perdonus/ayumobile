# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

function(generate_lang target_name lang_file)
    set(gen_dst ${CMAKE_CURRENT_BINARY_DIR}/gen)
    file(MAKE_DIRECTORY ${gen_dst})

    set(gen_timestamp ${gen_dst}/lang_auto.timestamp)
    set(gen_files
        ${gen_dst}/lang_auto.cpp
        ${gen_dst}/lang_auto.h
    )

    set(codegen_lang_command "$<TARGET_FILE:codegen_lang>")
    if (TDESKTOP_CODEGEN_LANG)
        set(codegen_lang_command "${TDESKTOP_CODEGEN_LANG}")
    endif()

    add_custom_command(
    OUTPUT
        ${gen_timestamp}
    BYPRODUCTS
        ${gen_files}
    COMMAND
        ${codegen_lang_command}
        -o${gen_dst}
        ${lang_file}
    COMMENT "Generating lang (${target_name})"
    DEPENDS
        codegen_lang
        ${lang_file}
    )
    generate_target(${target_name} lang ${gen_timestamp} "${gen_files}" ${gen_dst})
endfunction()
