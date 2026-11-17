cmake_minimum_required(VERSION 3.28)
project(gitprofiles VERSION 0.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Concurrent)
find_package(KF6 6.0 REQUIRED COMPONENTS Kirigami I18n CoreAddons)
# KF6::I18nQml target ships with the I18n package (no separate component).

# REQUIRES 6.8 opts into the 6.8 policy baseline (incl. QTP0004=NEW, so
# qt_add_qml_module manages the qmldir for the qml/ subdirectory).
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(gitprofiles
    src/main.cpp
)

qt_add_qml_module(gitprofiles
    URI org.gitprofiles
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/GitMissingPage.qml
        qml/ProfilesPage.qml
        qml/ProfileEditor.qml
        qml/DirectoriesPage.qml
        qml/ApplyPage.qml
        qml/SettingsPage.qml
        qml/DependenciesPage.qml
        qml/AboutPage.qml
    SOURCES
        src/Requirements.h
        src/Requirements.cpp
        src/SshConfig.h
        src/SshConfig.cpp
        src/ProfileModel.h
        src/ProfileModel.cpp
        src/RepoScanner.h
        src/RepoScanner.cpp
        src/RemoteSwitcher.h
        src/RemoteSwitcher.cpp
        src/Secrets.h
        src/Secrets.cpp
        src/ProviderCli.h
        src/ProviderCli.cpp
        src/Config.h
        src/Config.cpp
        src/SshTester.h
        src/SshTester.cpp
)

# Single source of truth for the version: the CMake project() version.
target_compile_definitions(gitprofiles PRIVATE GITPROFILES_VERSION="${PROJECT_VERSION}")

qt_add_resources(gitprofiles "appicon"
    PREFIX "/"
    FILES data/icons/gitprofiles.svg
)

target_include_directories(gitprofiles PRIVATE src)

target_link_libraries(gitprofiles PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Concurrent
    KF6::Kirigami
    KF6::I18n
    KF6::I18nQml
    KF6::CoreAddons
)

install(TARGETS gitprofiles ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES data/io.github.hernique33comiitei.GitProfiles.desktop
    DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES data/io.github.hernique33comiitei.GitProfiles.metainfo.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES data/icons/gitprofiles.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(FILES data/icons/gitprofiles-symbolic.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)

# Translations: compile po/<lang>.po -> <locale>/<lang>/LC_MESSAGES/gitprofiles.mo
find_program(MSGFMT_EXECUTABLE msgfmt)
if(MSGFMT_EXECUTABLE)
    file(GLOB PO_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/po/*.po)
    set(MO_FILES)
    foreach(po ${PO_FILES})
        get_filename_component(lang ${po} NAME_WE)
        set(mo ${CMAKE_CURRENT_BINARY_DIR}/locale/${lang}/gitprofiles.mo)
        add_custom_command(OUTPUT ${mo}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/locale/${lang}
            COMMAND ${MSGFMT_EXECUTABLE} -o ${mo} ${po}
            DEPENDS ${po})
        install(FILES ${mo} DESTINATION ${KDE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES)
        list(APPEND MO_FILES ${mo})
    endforeach()
    if(MO_FILES)
        add_custom_target(translations ALL DEPENDS ${MO_FILES})
    endif()
endif()
