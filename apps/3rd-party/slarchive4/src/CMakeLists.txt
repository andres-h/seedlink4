SET(SLARCHIVE_SOURCES
	dsarchive.c
	slarchive.c
	strutils.c
)

INCLUDE_DIRECTORIES(../../../../libs/3rd-party/libslink4)
INCLUDE_DIRECTORIES(../../../../../../system/libs/3rd-party/libmseed)

ADD_EXECUTABLE(slarchive4 ${SLARCHIVE_SOURCES})

TARGET_LINK_LIBRARIES(slarchive4 slink4 mseed)

INSTALL(TARGETS slarchive4
	RUNTIME DESTINATION ${SC3_PACKAGE_BIN_DIR})
