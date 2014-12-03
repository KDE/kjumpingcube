project(kjumpingcube)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.3.0")

find_package(ECM 1.3.0 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Qml Quick QuickWidgets Svg Test)
find_package(KF5 REQUIRED COMPONENTS 
    CoreAddons
    Config
    ItemModels
    WidgetsAddons
    WindowSystem
    Config
    DBusAddons
    I18n
    ConfigWidgets
    TextWidgets
    XmlGui
    KIO
    NotifyConfig
    NewStuff
    KDELibs4Support)

find_package(KF5KDEGames 4.9.0 REQUIRED)
find_package(Phonon4Qt5 CONFIG REQUIRED)

include(FeatureSummary)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DTRANSLATION_DOMAIN="kjumpingcube")

add_subdirectory( pics )
add_subdirectory( doc )

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
   ai_box.cpp )

ki18n_wrap_ui(kjumpingcube_SRCS settings.ui )

kconfig_add_kcfg_files(kjumpingcube_SRCS prefs.kcfgc )

kde4_add_app_icon(kjumpingcube_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-apps-kjumpingcube.png")
add_executable(kjumpingcube ${kjumpingcube_SRCS})

target_link_libraries(kjumpingcube  KF5::KIOCore KF5KDEGames KF5::XmlGui KF5::KDELibs4Support Qt5::Svg)

install(TARGETS kjumpingcube  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS org.kde.kjumpingcube.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kjumpingcube.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES kjumpingcubeui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kjumpingcube )

ecm_install_icons( ICONS hi128-apps-kjumpingcube.png  hi16-apps-kjumpingcube.png  hi22-apps-kjumpingcube.png  hi32-apps-kjumpingcube.png  hi48-apps-kjumpingcube.png  hi64-apps-kjumpingcube.png DESTINATION ${ICON_INSTALL_DIR}   )





