cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "03")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump KJUMPINGCUBE_BASE_VERSION once new features are added
set(KJUMPINGCUBE_BASE_VERSION "2.1")
set(KJUMPINGCUBE_VERSION "${KJUMPINGCUBE_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(kjumpingcube VERSION ${KJUMPINGCUBE_VERSION})

set (QT_MIN_VERSION "5.12.0")
set (KF5_MIN_VERSION "5.68.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Svg)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    ConfigWidgets
    Crash
    DBusAddons
    DocTools
    KIO
    I18n
    XmlGui
    WidgetsAddons
)

find_package(KF5KDEGames 7.3.0 REQUIRED)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)

add_definitions(
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_FOREACH
    -DQT_NO_KEYWORDS
    -DQT_STRICT_ITERATORS
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050C00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054400
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

add_subdirectory(pics)
add_subdirectory(doc)

########### next target ###############

set(kjumpingcube_SRCS 
   kjumpingcube.cpp 
   kcubeboxwidget.cpp 
   main.cpp 
   kcubewidget.cpp 
   game.cpp
   ai_main.cpp
   ai_kepler.cpp
   ai_newton.cpp
   ai_box.cpp
)

ecm_setup_version(${KJUMPINGCUBE_VERSION}
    VARIABLE_PREFIX KJUMPINGCUBE
    VERSION_HEADER kjumpingcube_version.h
)

ecm_qt_declare_logging_category(kjumpingcube_SRCS
    HEADER kjumpingcube_debug.h
    IDENTIFIER KJUMPINGCUBE_LOG
    CATEGORY_NAME org.kde.kjumpingcube
    DESCRIPTION "KJumpingCube"
    EXPORT KJUMPINGCUBE
)

qt5_add_resources(kjumpingcube_SRCS kjumpingcube.qrc)
ki18n_wrap_ui(kjumpingcube_SRCS settings.ui)

kconfig_add_kcfg_files(kjumpingcube_SRCS prefs.kcfgc)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-kjumpingcube.png")
ecm_add_app_icon(kjumpingcube_SRCS ICONS ${ICONS_SRCS})
add_executable(kjumpingcube ${kjumpingcube_SRCS})

target_link_libraries(kjumpingcube
    KF5KDEGames
    KF5::CoreAddons
    KF5::Crash
    KF5::DBusAddons
    KF5::KIOWidgets
    KF5::XmlGui
    Qt5::Svg
)

install(TARGETS kjumpingcube  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(PROGRAMS org.kde.kjumpingcube.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kjumpingcube.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR})
install(FILES kjumpingcube.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

ecm_install_icons(ICONS
    128-apps-kjumpingcube.png
    16-apps-kjumpingcube.png
    22-apps-kjumpingcube.png
    32-apps-kjumpingcube.png
    48-apps-kjumpingcube.png
    64-apps-kjumpingcube.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)

ecm_qt_install_logging_categories(
    EXPORT KJUMPINGCUBE
    FILE kjumpingcube.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
