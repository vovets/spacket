include("../../functions.cmake")

set(SRCS serial_device_impl.cpp)

PREPEND_CUR_DIR(PLATFORM_SRCS ${SRCS})

set(PLATFORM_SRCS ${PLATFORM_SRCS} PARENT_SCOPE)
