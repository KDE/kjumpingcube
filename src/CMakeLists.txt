ecm_setup_version(${KJUMPINGCUBE_VERSION}
    VARIABLE_PREFIX KJUMPINGCUBE
    VERSION_HEADER kjumpingcube_version.h
)

add_executable(kjumpingcube)

target_sources(kjumpingcube PRIVATE
    ai_base.h
    ai_box.cpp
    ai_box.h
    ai_globals.h
    ai_kepler.cpp
    ai_kepler.h
    ai_main.cpp
    ai_main.h
    ai_newton.cpp
    ai_newton.h
    game.cpp
    game.h
    kcubeboxwidget.cpp
    kcubeboxwidget.h
    kcubewidget.cpp
    kcubewidget.h
    kjumpingcube.cpp
    kjumpingcube.h
    main.cpp
    settingswidget.h

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

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(kjumpingcube KDEGames6)
else()
    target_link_libraries(kjumpingcube KF5KDEGames)
endif()

target_link_libraries(kjumpingcube
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::KIOWidgets
    KF${KF_MAJOR_VERSION}::XmlGui
    Qt${QT_MAJOR_VERSION}::Svg
)

if (TARGET Qt::Core5Compat)
    target_link_libraries(kjumpingcube Qt::Core5Compat)
endif()

install(TARGETS kjumpingcube ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.kjumpingcube.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kjumpingcube.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR})
install(FILES kjumpingcube.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

ecm_qt_install_logging_categories(
    EXPORT KJUMPINGCUBE
    FILE kjumpingcube.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
