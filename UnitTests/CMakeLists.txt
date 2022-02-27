
if (NOT (CMAKE_BUILD_TYPE STREQUAL "Ubsan"))
	create_test(
			NAME
				cNpyTests
			SOURCES
				main.cpp
				cnpyUnitTests.cpp
			DEPENDENCIES
				cnpy
			SYSTEM_DEPENDENCIES
				gtest pthread
	)
endif()

create_test(
		NAME
			NpyTests
		SOURCES
			main.cpp
			MmapNpyUnitTests.cpp
			NpyUnitTests.cpp
			NpzUnitTests.cpp
		DEPENDENCIES
			Npy++ cnpy
		SYSTEM_DEPENDENCIES
			gtest pthread
)

add_custom_command(TARGET NpyTests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/UnitTests/0123.npz ${CMAKE_BINARY_DIR}/0123.npz
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/UnitTests/0123.npz ${CMAKE_BINARY_DIR}/UnitTests/0123.npz
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/UnitTests/0123.npz ${CMAKE_BINARY_DIR}/bin/0123.npz
)
