add_custom_command(
	OUTPUT libslink4/libslink.a
	COMMAND make -C libslink4
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(buildslink4 DEPENDS libslink4/libslink.a)

ADD_LIBRARY(slink4 STATIC IMPORTED GLOBAL)
add_dependencies(slink4 buildslink4)
set_target_properties(slink4 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libslink4/libslink.a")
