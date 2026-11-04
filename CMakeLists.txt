cmake_minimum_required(VERSION 3.16)

project(OpenGG
    VERSION 1.0.0
    DESCRIPTION "OpenGG - unofficial SteelSeries GG alternative for Linux (Apex 3 TKL, Rival 3 Wireless Gen 2)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SSGG_BUILD_TESTS "Build unit tests" ON)
option(SSGG_ENABLE_DBUS "Enable D-Bus based idle detection (QtDBus)" ON)

# --- Dependencies -----------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Svg Network)
find_package(Qt6 6.2 QUIET OPTIONAL_COMPONENTS LinguistTools Test DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi-hidraw)

set(SSGG_APP_ID "opengg")

# --- Sources ----------------------------------------------------------------
set(SSGG_SOURCES
    src/main.cpp
    src/core/Theme.cpp
    src/core/Icons.cpp
    src/core/ConfigStore.cpp
    src/core/Autostart.cpp
    src/core/IdleMonitor.cpp
    src/core/SingleInstance.cpp
    src/hid/HidTransport.cpp
    src/devices/Device.cpp
    src/devices/DeviceManager.cpp
    src/devices/Apex3Tkl.cpp
    src/devices/Rival3WirelessGen2.cpp
    src/devices/Protocol.cpp
    src/models/KeyboardConfig.cpp
    src/models/MouseConfig.cpp
    src/lighting/LightingEngine.cpp
    src/ui/widgets/ToggleSwitch.cpp
    src/ui/widgets/IconButton.cpp
    src/ui/widgets/NavTab.cpp
    src/ui/widgets/TopBar.cpp
    src/ui/widgets/Card.cpp
    src/ui/widgets/SectionLabel.cpp
    src/ui/widgets/SegmentedControl.cpp
    src/ui/widgets/ValueSlider.cpp
    src/ui/widgets/ColorPicker.cpp
    src/ui/widgets/KeyboardPreview.cpp
    src/ui/widgets/MousePreview.cpp
    src/ui/widgets/DeviceCard.cpp
    src/ui/widgets/ConfigList.cpp
    src/ui/widgets/Toast.cpp
    src/ui/widgets/EmptyState.cpp
    src/ui/widgets/FlowLayout.cpp
    src/ui/pages/HomePage.cpp
    src/ui/pages/EnginePage.cpp
    src/ui/pages/DevicePage.cpp
    src/ui/pages/KeyboardIlluminationTab.cpp
    src/ui/pages/KeyboardSettingsTab.cpp
    src/ui/pages/MouseButtonsTab.cpp
    src/ui/pages/MouseSensitivityTab.cpp
    src/ui/pages/MouseSettingsTab.cpp
    src/ui/pages/PlaceholderPage.cpp
    src/ui/pages/SettingsPage.cpp
    src/ui/DeviceController.cpp
    src/ui/MainWindow.cpp
    resources/resources.qrc
)

add_executable(${SSGG_APP_ID} ${SSGG_SOURCES})

target_include_directories(${SSGG_APP_ID} PRIVATE src)
target_compile_definitions(${SSGG_APP_ID} PRIVATE
    SSGG_VERSION="${PROJECT_VERSION}"
    SSGG_APP_ID="${SSGG_APP_ID}"
)
target_link_libraries(${SSGG_APP_ID} PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Network
    PkgConfig::HIDAPI)

if(SSGG_ENABLE_DBUS AND TARGET Qt6::DBus)
    target_link_libraries(${SSGG_APP_ID} PRIVATE Qt6::DBus)
    target_compile_definitions(${SSGG_APP_ID} PRIVATE SSGG_HAVE_DBUS=1)
    message(STATUS "QtDBus found: idle detection enabled")
else()
    message(STATUS "QtDBus not found or disabled: idle detection disabled")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${SSGG_APP_ID} PRIVATE -Wall -Wextra)
endif()

# --- Translations -----------------------------------------------------------
set(SSGG_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/opengg_tr.ts
)

if(TARGET Qt6::lrelease)
    set(SSGG_QM_DIR ${CMAKE_CURRENT_BINARY_DIR}/i18n)
    file(MAKE_DIRECTORY ${SSGG_QM_DIR})
    set(SSGG_QM_FILES)
    foreach(ts_file IN LISTS SSGG_TS_FILES)
        get_filename_component(ts_base ${ts_file} NAME_WE)
        set(qm_file ${SSGG_QM_DIR}/${ts_base}.qm)
        add_custom_command(
            OUTPUT ${qm_file}
            COMMAND Qt6::lrelease -silent ${ts_file} -qm ${qm_file}
            DEPENDS ${ts_file}
            COMMENT "Compiling translation ${ts_base}"
            VERBATIM)
        list(APPEND SSGG_QM_FILES ${qm_file})
    endforeach()
    set_source_files_properties(${SSGG_QM_FILES} PROPERTIES GENERATED TRUE)
    qt_add_resources(${SSGG_APP_ID} "i18n"
        PREFIX "/i18n"
        BASE ${SSGG_QM_DIR}
        FILES ${SSGG_QM_FILES})
    message(STATUS "Translations enabled (lrelease found)")
else()
    message(WARNING "Qt LinguistTools (lrelease) not found: the app will be English only. Install qt6-l10n-tools / qt6-tools-dev-tools.")
endif()

# --- Tests ------------------------------------------------------------------
if(SSGG_BUILD_TESTS AND TARGET Qt6::Test)
    enable_testing()
    add_executable(protocol_tests
        tests/ProtocolTests.cpp
        src/devices/Protocol.cpp
        src/models/KeyboardConfig.cpp
        src/models/MouseConfig.cpp)
    target_include_directories(protocol_tests PRIVATE src)
    target_link_libraries(protocol_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
    add_test(NAME protocol_tests COMMAND protocol_tests)
endif()

# --- Install ----------------------------------------------------------------
include(GNUInstallDirs)
set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${SSGG_APP_ID}")

install(TARGETS ${SSGG_APP_ID} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/${SSGG_APP_ID}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/icons/${SSGG_APP_ID}.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES packaging/70-${SSGG_APP_ID}.rules
        DESTINATION lib/udev/rules.d)
install(FILES packaging/${SSGG_APP_ID}.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES README.md LICENSE
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES packaging/debian/copyright
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Compressed changelog and manual page (Debian policy)
find_program(GZIP_EXECUTABLE gzip)
if(GZIP_EXECUTABLE)
    set(SSGG_CHANGELOG_GZ ${CMAKE_CURRENT_BINARY_DIR}/changelog.gz)
    add_custom_command(OUTPUT ${SSGG_CHANGELOG_GZ}
        COMMAND ${GZIP_EXECUTABLE} -9 -n -c ${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/changelog > ${SSGG_CHANGELOG_GZ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/changelog VERBATIM)
    set(SSGG_MAN_GZ ${CMAKE_CURRENT_BINARY_DIR}/${SSGG_APP_ID}.1.gz)
    add_custom_command(OUTPUT ${SSGG_MAN_GZ}
        COMMAND ${GZIP_EXECUTABLE} -9 -n -c ${CMAKE_CURRENT_SOURCE_DIR}/packaging/${SSGG_APP_ID}.1 > ${SSGG_MAN_GZ}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/packaging/${SSGG_APP_ID}.1 VERBATIM)
    add_custom_target(ssgg_docs ALL DEPENDS ${SSGG_CHANGELOG_GZ} ${SSGG_MAN_GZ})
    install(FILES ${SSGG_CHANGELOG_GZ} DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES ${SSGG_MAN_GZ} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

# --- CPack (.deb) -----------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "${SSGG_APP_ID}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "fevz10 <fvzdrl@gmail.com>")
set(CPACK_PACKAGE_VENDOR "fevz10")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unofficial SteelSeries GG alternative for Linux")
set(CPACK_PACKAGE_DESCRIPTION
"OpenGG is an unofficial, community made desktop application that brings the
SteelSeries GG experience to Linux. Configures the SteelSeries Apex 3 TKL
keyboard (8-zone illumination, brightness) and the Rival 3 Wireless Gen 2
mouse (CPI presets, polling rate, button mapping, battery) over HID.
Not affiliated with or endorsed by SteelSeries.")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "udev, qt6-qpa-plugins")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-wayland, libqt6svg6")
# The project was briefly packaged as "steelseries-gg"; replace it cleanly.
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "steelseries-gg")
set(CPACK_DEBIAN_PACKAGE_REPLACES "steelseries-gg")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postrm")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
include(CPack)
