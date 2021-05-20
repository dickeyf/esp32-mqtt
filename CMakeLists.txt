# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ap_config)

file(MAKE_DIRECTORY webconfig/dist)
spiffs_create_partition_image(www webconfig/dist FLASH_IN_PROJECT DEPENDS WEBCONFIG)

target_add_binary_data(ap_config.elf "main/trust_store.cer" TEXT)
