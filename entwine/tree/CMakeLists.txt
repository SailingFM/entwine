set(MODULE tree)
set(BASE "${CMAKE_CURRENT_SOURCE_DIR}")

set(
    SOURCES
    "${BASE}/branch.cpp"
    "${BASE}/multi-batcher.cpp"
    "${BASE}/point-info.cpp"
    "${BASE}/registry.cpp"
    "${BASE}/roller.cpp"
    "${BASE}/sleepy-tree.cpp"
)

set(
    HEADERS
    "${BASE}/branch.hpp"
    "${BASE}/multi-batcher.hpp"
    "${BASE}/point-info.hpp"
    "${BASE}/registry.hpp"
    "${BASE}/roller.hpp"
    "${BASE}/sleepy-tree.hpp"
)

add_subdirectory(branches)

install(FILES ${HEADERS} DESTINATION include/entwine/${MODULE})
add_library(${MODULE} OBJECT ${SOURCES} ${BRANCHES_SOURCES})
