#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(TFLiteHost_RTApp_MT3620_BareMetal C)
add_library( libtflitemcu_speech_test STATIC IMPORTED)
SET_TARGET_PROPERTIES( libtflitemcu_speech_test PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libtflitemcu_speech_test.a)
# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} libtflitemcu_speech_test stdc++ supc++ m c gcc nosys)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} libtflitemcu_speech_test stdc++ supc++ m c gcc nosys)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)
include_directories(${CMAKE_SOURCE_DIR}/ )

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
