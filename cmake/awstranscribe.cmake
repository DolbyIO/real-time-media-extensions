set(AWS_SDK_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../ext-lib/aws-sdk-install/")
set(AWS_SDK_LIBRARY_DIR "${AWS_SDK_ROOT_DIR}/lib")
set(AWS_SDK_INCLUDE_DIR "${AWS_SDK_ROOT_DIR}/include")

set(AWS_SDK_CORE_PATH
	"${AWS_SDK_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_SHARED_LIBRARY_SUFFIX}"
)
set(AWS_SDK_TRANSCRIBESTREAMING_PATH
	"${AWS_SDK_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}aws-cpp-sdk-transcribestreaming${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

add_library(aws-cpp-sdk-core SHARED IMPORTED)
set_target_properties(aws-cpp-sdk-core PROPERTIES
	IMPORTED_LOCATION ${AWS_SDK_CORE_PATH}
	INTERFACE_INCLUDE_DIRECTORIES ${AWS_SDK_INCLUDE_DIR}
)
add_library(aws-cpp-sdk-transcribestreaming SHARED IMPORTED)
set_target_properties(aws-cpp-sdk-transcribestreaming PROPERTIES
	IMPORTED_LOCATION ${AWS_SDK_TRANSCRIBESTREAMING_PATH}
	INTERFACE_INCLUDE_DIRECTOIRES ${AWS_SDK_INCLUDE_DIR}
)

add_library(aws-sdk-cpp INTERFACE)
target_link_libraries(aws-sdk-cpp INTERFACE aws-cpp-sdk-core aws-cpp-sdk-transcribestreaming)

