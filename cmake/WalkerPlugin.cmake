function(walker_add_plugin)
    cmake_parse_arguments(ARG "" "NAME;PRODUCT_NAME;PLUGIN_CODE" "LINK" ${ARGN})

    # Folder name, e.g. plugins/tape_delay -> "tape_delay"
    get_filename_component(folder ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Default NAME: CamelCase folder name with Walker prefix (tape_delay -> WalkerTapeDelay)
    if(NOT ARG_NAME)
        string(REGEX REPLACE "[_-]" ";" words ${folder})
        set(camel "")
        foreach(w ${words})
            string(SUBSTRING ${w} 0 1 first)
            string(SUBSTRING ${w} 1 -1 rest)
            string(TOUPPER ${first} first)
            string(APPEND camel ${first}${rest})
        endforeach()
        set(ARG_NAME Walker${camel})
    endif()

    # Default PRODUCT_NAME: "Walker Tape Delay"
    if(NOT ARG_PRODUCT_NAME)
        string(REGEX REPLACE "([a-z])([A-Z])" "\\1 \\2" spaced ${ARG_NAME})
        set(ARG_PRODUCT_NAME "${spaced}")
    endif()

    # Plugin codes must be unique, so require them explicitly
    if(NOT ARG_PLUGIN_CODE)
        message(FATAL_ERROR "walker_add_plugin(${ARG_NAME}): PLUGIN_CODE is required (4 chars, unique per plugin)")
    endif()

    juce_add_plugin(${ARG_NAME}
        COMPANY_NAME "Walker"
        PLUGIN_MANUFACTURER_CODE Wlkr
        PLUGIN_CODE ${ARG_PLUGIN_CODE}
        FORMATS VST3 Standalone
        PRODUCT_NAME "${ARG_PRODUCT_NAME}"
        COPY_PLUGIN_AFTER_BUILD TRUE)

    # Collect sources, skipping any file or folder starting with "_"
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    list(FILTER sources EXCLUDE REGEX "(^|/)_")
    target_sources(${ARG_NAME} PRIVATE ${sources})

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/inc)
        target_include_directories(${ARG_NAME} PRIVATE inc)
    endif()

    target_compile_definitions(${ARG_NAME} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${ARG_NAME}
        PRIVATE
            juce::juce_audio_utils
            ${ARG_LINK}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

function(walker_add_component)
    cmake_parse_arguments(ARG "" "NAME" "PUBLIC_LINK;PRIVATE_LINK" ${ARGN})

    # Default name = folder name (components/dsp -> dsp)
    if(NOT ARG_NAME)
        get_filename_component(ARG_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    endif()
    set(target walker_${ARG_NAME})

    # Collect sources, skipping any file or folder starting with "_"
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    list(FILTER sources EXCLUDE REGEX "(^|/)_")

    if(sources)
        add_library(${target} STATIC ${sources})
        target_include_directories(${target} PUBLIC inc)
        target_link_libraries(${target}
            PUBLIC  ${ARG_PUBLIC_LINK}
            PRIVATE ${ARG_PRIVATE_LINK}
                    juce::juce_recommended_config_flags
                    juce::juce_recommended_warning_flags)
    else()
        # Header-only component
        add_library(${target} INTERFACE)
        target_include_directories(${target} INTERFACE inc)
        target_link_libraries(${target} INTERFACE ${ARG_PUBLIC_LINK})
    endif()

    add_library(walker::${ARG_NAME} ALIAS ${target})
endfunction()
