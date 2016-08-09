############################################
#
# How to build mtree
#
############################################
IF(ENABLE_MTREE)

  SET(mtree_SOURCES
    only.c
    mtree.c
    mtree.h
    mtree_platform.h
    ../libarchive_fe/err.c
    ../libarchive_fe/err.h
  )
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../libarchive_fe)

  # Include fgetln() if the system does not provide it
  IF(NOT HAVE_FGETLN)
    SET(mtree_SOURCES ${mtree_SOURCES} fgetln.c)
  ENDIF(NOT HAVE_FGETLN)

  # Include fparseln() if the system does not provide it
  IF(NOT HAVE_FPARSELN)
    SET(mtree_SOURCES ${mtree_SOURCES} fparseln.c)
  ENDIF(NOT HAVE_FPARSELN)

  # Include strsep() if the system does not provide it
  IF(NOT HAVE_STRSEP)
    SET(mtree_SOURCES ${mtree_SOURCES} strsep.c)
  ENDIF(NOT HAVE_STRSEP)

  # Only add the user/group parsing functions when pwcache routines are
  # supported
  IF(HAVE_PWCACHE)
    SET(mtree_SOURCES ${mtree_SOURCES} getid.c)
  ENDIF(HAVE_PWCACHE)

  # mtree documentation
  SET(mtree_MANS mtree.1)

  # How to build mtree
  ADD_EXECUTABLE(mtree ${mtree_SOURCES})
  IF(ENABLE_MTREE_SHARED)
    TARGET_LINK_LIBRARIES(mtree archive ${ADDITIONAL_LIBS})
  ELSE(ENABLE_MTREE_SHARED)
    TARGET_LINK_LIBRARIES(mtree archive_static ${ADDITIONAL_LIBS})
    SET_TARGET_PROPERTIES(mtree PROPERTIES COMPILE_DEFINITIONS
    				 LIBARCHIVE_STATIC)
  ENDIF(ENABLE_MTREE_SHARED)

  # Installation rules
  INSTALL(TARGETS mtree RUNTIME DESTINATION bin)
  INSTALL_MAN(${mtree_MANS})
ENDIF(ENABLE_MTREE)

# Test suite
add_subdirectory(test)
