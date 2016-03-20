project(kjumpingcube)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.3.0")
set (KF5_MIN_VERSION "5.15.0")

find_package(ECM 1.7.0 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Svg)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    Crash
    WidgetsAddons
    DBusAddons
    I18n
    ConfigWidgets
    XmlGui
    KDELibs4Support
)

find_package(KF5KDEGames 4.9.0 REQUIRED)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

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

ki18n_wrap_ui(kjumpingcube_SRCS settings.ui)

kconfig_add_kcfg_files(kjumpingcube_SRCS prefs.kcfgc)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-kjumpingcube.png")
ecm_add_app_icon(kjumpingcube_SRCS ICONS ${ICONS_SRCS})
add_executable(kjumpingcube ${kjumpingcube_SRCS})

target_link_libraries(kjumpingcube
    KF5KDEGames
    KF5::XmlGui
    KF5::DBusAddons
    KF5::KDELibs4Support
    Qt5::Svg
)

install(TARGETS kjumpingcube  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(PROGRAMS org.kde.kjumpingcube.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES kjumpingcube.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})
install(FILES kjumpingcubeui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kjumpingcube)

ecm_install_icons(ICONS
    128-apps-kjumpingcube.png
    16-apps-kjumpingcube.png
    22-apps-kjumpingcube.png
    32-apps-kjumpingcube.png
    48-apps-kjumpingcube.png
    64-apps-kjumpingcube.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
