function(add_all_subdirectories)
    file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
            CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*)
    foreach(child ${children})
        if(NOT child MATCHES "^_"
           AND IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child}
           AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
            add_subdirectory(${child})
        endif()
    endforeach()
endfunction()
