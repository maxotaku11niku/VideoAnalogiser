# CMakeList.txt : CMake project for VideoAnalogiser, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (VideoAnalogiser "VideoAnalogiser.cpp" "VideoAnalogiser.h" "BroadcastStandard.cpp" "BroadcastStandard.h" "ColourSystem.cpp" "ColourSystem.h" "PALSystem.cpp" "PALSystem.h" "Utils.cpp" "Utils.h" "ConversionEngine.cpp" "ConversionEngine.h" "NTSCSystem.cpp" "NTSCSystem.h" "SECAMSystem.cpp" "SECAMSystem.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VideoAnalogiser PROPERTY CXX_STANDARD 20)
endif()

if (MSVC)
  set_property(TARGET VideoAnalogiser PROPERTY COMPILE_OPTIONS -openmp)
else()
  set_property(TARGET VideoAnalogiser PROPERTY COMPILE_OPTIONS -fopenmp)
  target_link_libraries (VideoAnalogiser PUBLIC "${CMAKE_FIND_LIBRARY_PREFIXES}/libgomp")
endif()

# TODO: Add tests and install targets if needed.

target_link_libraries (VideoAnalogiser PUBLIC "${PROJECT_SOURCE_DIR}/lib/avcodec.lib")
target_link_libraries (VideoAnalogiser PUBLIC "${PROJECT_SOURCE_DIR}/lib/avformat.lib")
target_link_libraries (VideoAnalogiser PUBLIC "${PROJECT_SOURCE_DIR}/lib/avutil.lib")
target_link_libraries (VideoAnalogiser PUBLIC "${PROJECT_SOURCE_DIR}/lib/swresample.lib")
target_link_libraries (VideoAnalogiser PUBLIC "${PROJECT_SOURCE_DIR}/lib/swscale.lib")