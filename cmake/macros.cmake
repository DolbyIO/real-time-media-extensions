# Macro for stripping library from debug symbols and moving them to a separate file
macro(dump_debug_symbols_and_strip __target)
if(LINUX OR ANDROID)
	add_custom_command(TARGET ${__target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${__target}> $<TARGET_FILE:${__target}>.debug
		COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${__target}>
		COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:${__target}>.debug $<TARGET_FILE:${__target}>
		COMMENT "Striping ${__target} and generating debug symbols"
	)
elseif(MACOS)
	if(NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
		# XCode will generate dsyms by its configuration; ninja generator must strip manually:
		add_custom_command(TARGET ${__target} POST_BUILD
			COMMAND dsymutil $<TARGET_FILE:${__target}> -o $<TARGET_FILE:${__target}>.dSYM
			COMMAND ${CMAKE_STRIP} -S $<TARGET_FILE:${__target}>
			COMMENT "Striping ${__target} and generating debug symbols"
		)
	endif()
endif()
endmacro()

function(install_debuginfo __target __destination)
	if(WINDOWS)
		install(
			FILES $<TARGET_PDB_FILE:${__target}>
			DESTINATION ${__destination}
		)
	else(WINDOWS)
		if(LINUX OR ANDROID)
			set(DEBUG_SYMBOLS_EXTENSION "debug")
		elseif(MACOS)
			set(DEBUG_SYMBOLS_EXTENSION "dSYM")
		endif()
		install(
			FILES $<TARGET_FILE:${__target}>.${DEBUG_SYMBOLS_EXTENSION}
			DESTINATION ${__destination}
		)
	endif(WINDOWS)
endfunction()
