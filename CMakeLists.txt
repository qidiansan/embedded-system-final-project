cmake_minimum_required(VERSION 3.16)
project(FileTransfer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# On Windows with custom Qt install, set CMAKE_PREFIX_PATH to the Qt directory.
# On Linux with system Qt6 (apt), CMake finds it automatically — leave this commented.
# set(CMAKE_PREFIX_PATH "E:/qt/6.5.3/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Widgets
)

set(SOURCES
    src/main.cpp
    src/protocol/MessageFramer.cpp
    src/protocol/MessageSerializer.cpp
    src/network/PeerServer.cpp
    src/network/PeerClient.cpp
    src/network/PeerConnection.cpp
    src/network/TransferManager.cpp
    src/transfer/FileSender.cpp
    src/transfer/FileReceiver.cpp
    src/transfer/DirectoryTransfer.cpp
    src/ui/MainWindow.cpp
    src/ui/ConnectionPanel.cpp
    src/ui/TransferPanel.cpp
    src/ui/LogPanel.cpp
    src/utils/Checksum.cpp
)

set(HEADERS
    src/protocol/ProtocolDefs.h
    src/protocol/MessageFramer.h
    src/protocol/MessageSerializer.h
    src/network/PeerServer.h
    src/network/PeerClient.h
    src/network/PeerConnection.h
    src/network/TransferManager.h
    src/transfer/FileSender.h
    src/transfer/FileReceiver.h
    src/transfer/DirectoryTransfer.h
    src/ui/MainWindow.h
    src/ui/ConnectionPanel.h
    src/ui/TransferPanel.h
    src/ui/LogPanel.h
    src/utils/Checksum.h
)

add_executable(FileTransfer ${SOURCES} ${HEADERS})

target_include_directories(FileTransfer PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(FileTransfer PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

# Windows: hide console window in release builds
if(WIN32)
    set_target_properties(FileTransfer PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# Install rule
install(TARGETS FileTransfer DESTINATION bin)
