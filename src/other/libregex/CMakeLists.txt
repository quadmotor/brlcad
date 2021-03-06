include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${${CMAKE_PROJECT_NAME}_SOURCE_DIR}/include)

set(LIBREGEX_PUBLIC_HDRS
  regex.h
  )

set(LIBREGEX_SOURCES
  regcomp.c
  regerror.c
  regexec.c
  regfree.c
  )

add_definitions(
  -DREGEX_EXPORT_DLL
  -DBRLCAD_DLL
  )

add_library(regex SHARED ${LIBREGEX_SOURCES})
set_target_properties(regex PROPERTIES VERSION 1.0.4 SOVERSION 1)
install(TARGETS regex
  RUNTIME DESTINATION ${BIN_DIR}
  LIBRARY DESTINATION ${LIB_DIR}
  ARCHIVE DESTINATION ${LIB_DIR})
add_library(regex-static STATIC ${LIBREGEX_SOURCES})
install(TARGETS regex-static
  RUNTIME DESTINATION ${BIN_DIR}
  LIBRARY DESTINATION ${LIB_DIR}
  ARCHIVE DESTINATION ${LIB_DIR})

IF(NOT WIN32)
  SET_TARGET_PROPERTIES(regex-static PROPERTIES OUTPUT_NAME "regex")
ENDIF(NOT WIN32)
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  install(FILES ${LIBREGEX_PUBLIC_HDRS} DESTINATION include)
endif()

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

