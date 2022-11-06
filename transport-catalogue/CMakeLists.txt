cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)

set(TR_CATALOGUE_FILES
json_reader.cpp   serialization.h
domain.cpp        json_reader.h        svg.cpp
domain.h          main.cpp             svg.h
geo.cpp           map_renderer.cpp     transport_catalogue.cpp
geo.h             map_renderer.h       transport_catalogue.h
graph.h           ranges.h             
json.cpp          request_handler.cpp  transport_router.cpp
json.h            request_handler.h    transport_router.h
json_builder.cpp  router.h
json_builder.h    serialization.cpp    transport_catalogue.proto)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TR_CATALOGUE_FILES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)