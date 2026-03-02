cmake_minimum_required(VERSION 3.16)
project(CryptexChat LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

qt_add_executable(CryptexChat
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    setup_db.h
    chatdbhandler.h chatdbhandler.cpp
    privatechatwidget.h privatechatwidget.cpp
    usersystem.h
    usersystem.cpp
    encryptionutils.h
    encryptionutils.cpp
    crypto_helpers.h
    crypto_helpers.cpp
    tiny_aes.c tiny_aes.h
    message.h
    messagequeue.h
    sessionmanager.cpp sessionmanager.h





)

# Resources: embed app logo
qt_add_resources(CryptexChat app_resources
    PREFIX "/images"
    FILES
        CryptexChat_Logo.png
)

target_link_libraries(CryptexChat
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt${QT_VERSION_MAJOR}::Sql  # Linking QtSql for SQLite
        Qt::Network)

include(GNUInstallDirs)

install(TARGETS CryptexChat
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET CryptexChat
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
