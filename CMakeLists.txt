cmake_minimum_required( VERSION 2.8 )

project( img2dds )

find_library( FREEIMAGEPLUS_LIBRARY freeimageplus )
find_library( SQUISH_LIBRARY squish )

add_executable( img2dds main.cc img2dds.cc )
target_link_libraries( img2dds ${FREEIMAGEPLUS_LIBRARY} ${SQUISH_LIBRARY} )

install( TARGETS img2dds RUNTIME DESTINATION bin )
