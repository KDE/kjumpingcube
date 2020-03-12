project(kjumpingcube)

cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
set (QT_MIN_VERSION "5.7.0")
set (KF5_MIN_VERSION "5.30.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

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

find_package(KF5KDEGames 4.9.0 REQUIRED)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMQtDeclareLoggingCategory)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
   add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054400)
endif()

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

ecm_qt_declare_logging_category(kjumpingcube_SRCS HEADER kjumpingcube_debug.h IDENTIFIER KJUMPINGCUBE_LOG CATEGORY_NAME org.kde.kjumpingcube)

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

if (${ECM_VERSION} STRGREATER "5.58.0")
    install(FILES kjumpingcube.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES kjumpingcube.categories  DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
