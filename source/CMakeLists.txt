#==============================================================================
#    _____ _    _ ______  _____ _____
#   / ____| |  | |  ____|/ ____/ ____|
#  | |    | |__| | |__  | (___| (___ 
#  | |    |  __  |  __|  \___ \\___ \
#  | |____| |  | | |____ ____) |___) |
#   \_____|_|  |_|______|_____/_____/
#
#  /source/CMakeLists.txt
#
#==============================================================================

cmake_minimum_required (VERSION 2.6)

project (chess2 CXX)

add_executable(chess2 serialib.cpp chess.cpp user_interface.cpp main.cpp )

set_property(TARGET chess2 PROPERTY CXX_STANDARD 11)
set_property(TARGET chess2 PROPERTY CXX_STANDARD_REQUIRED ON) 
