SET(PACKAGE_NAME SEEDLINK4)
SET(APP_NAME seedlink4)

SET(
	${PACKAGE_NAME}_SOURCES
		main.cpp
		app.cpp
		format.cpp
		mseed24.cpp
		record.cpp
		server.cpp
		session.cpp
		settings.cpp
		storage.cpp
)


INCLUDE_DIRECTORIES(${LIBMSEED_INCLUDE_DIR})
#SC_INSTALL_INIT(${APP_NAME} ../../templates/initd.py)

FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})

SC_ADD_EXECUTABLE(${PACKAGE_NAME} ${APP_NAME})
SC_ADD_VERSION(${PACKAGE_NAME} ${APP_NAME})
SC_LINK_LIBRARIES_INTERNAL(${APP_NAME} client)