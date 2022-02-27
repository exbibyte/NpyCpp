
# sudo apt install zlib1g zlib1g-dev
create_library(
    NAME
        Npy++
    SOURCES
        dummy.cpp
    PUBLIC_INCLUDE_DIRECTORIES
        . ../MemoryMapping
    SYSTEM_DEPENDENCIES
        z
)
