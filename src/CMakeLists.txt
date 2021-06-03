ecm_setup_version(${KJUMPINGCUBE_VERSION}
    VARIABLE_PREFIX KJUMPINGCUBE
    VERSION_HEADER kjumpingcube_version.h
)

add_executable(kjumpingcube)

target_sources(kjumpingcube PRIVATE
    kjumpingcube.cpp 
    kcubeboxwidget.cpp 
    main.cpp 
    kcubewidget.cpp 
    game.cpp
    ai_main.cpp
    ai_kepler.cpp
    ai_newton.cpp
    ai_box.cpp

    kjumpingcube.qrc
)

ecm_qt_declare_logging_category(kjumpingcube
    HEADER kjumpingcube_debug.h
    IDENTIFIER KJUMPINGCUBE_LOG
    CATEGORY_NAME org.kde.kjumpingcube
    DESCRIPTION "KJumpingCube"
    EXPORT KJUMPINGCUBE
)

ki18n_wrap_ui(kjumpingcube settings.ui)

kconfig_add_kcfg_files(kjumpingcube prefs.kcfgc)

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/icons/*-apps-kjumpingcube.png")
ecm_add_app_icon(kjumpingcube ICONS ${ICONS_SRCS})

target_link_libraries(kjumpingcube
    KF5KDEGames
    KF5::CoreAddons
    KF5::Crash
    KF5::DBusAddons
    KF5::KIOWidgets
    KF5::XmlGui
    Qt::Svg
)

install(TARGETS kjumpingcube ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.kjumpingcube.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kjumpingcube.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR})
install(FILES kjumpingcube.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

ecm_qt_install_logging_categories(
    EXPORT KJUMPINGCUBE
    FILE kjumpingcube.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
