#######################################
#        FuzzyLite support            #
#######################################

if(NOT WIN32 AND NOT APPLE)
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" ON)
else()
	option(FORCE_BUNDLED_FL "Force to use FuzzyLite included into VCMI's source tree" OFF)
endif()

#FuzzyLite uses MSVC pragmas in headers, so, we need to disable -Wunknown-pragmas
if(MINGW)
    add_compile_options(-Wno-unknown-pragmas)
endif()

if(NOT FORCE_BUNDLED_FL)
	find_package(fuzzylite)
else()
	set(fuzzylite_FOUND FALSE)
endif()

if(NOT fuzzylite_FOUND)
	set(FL_BUILD_BINARY OFF CACHE BOOL "")
	set(FL_BUILD_SHARED OFF CACHE BOOL "")
	set(FL_BUILD_TESTS OFF CACHE BOOL "")
	if(ANDROID)
		set(FL_BACKTRACE OFF CACHE BOOL "" FORCE)
	endif()

	add_subdirectory(FuzzyLite EXCLUDE_FROM_ALL)
	add_library(fuzzylite::fuzzylite ALIAS staticTarget)
endif()

#######################################
#        Add subdirectories           #
#######################################

# Combat AI's
if(ENABLE_STUPID_AI)
	add_subdirectory(StupidAI)
endif()

if(ENABLE_BATTLE_AI)
	add_subdirectory(BattleAI)
endif()

if(ENABLE_MMAI)
	add_subdirectory(MMAI)
endif()

# Adventure AI's
add_subdirectory(EmptyAI)

if(ENABLE_NULLKILLER_AI)
	add_subdirectory(Nullkiller)
endif()

if(ENABLE_NULLKILLER2_AI)
	add_subdirectory(Nullkiller2)
endif()

# AI's living outside the VCMI tree, e.g. -D EXTRA_AI_DIRS="/path/to/MyAI;/path/to/OtherAI"
set(EXTRA_AI_DIRS "" CACHE STRING "Semicolon-separated list of external AI source directories")
foreach(EXTRA_AI_DIR ${EXTRA_AI_DIRS})
	get_filename_component(EXTRA_AI_NAME ${EXTRA_AI_DIR} NAME)
	add_subdirectory(${EXTRA_AI_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_AI_NAME})
endforeach()
