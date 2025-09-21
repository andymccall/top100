cmake_minimum_required(VERSION 3.11)
# Adopt modern CMake policies up to 3.30 to silence developer warnings (e.g., CMP0167 FindBoost)
cmake_policy(VERSION 3.30)
project(Top100)

# --- Dependencies ---
include(FetchContent)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# HTTP client (cpr)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5
)
set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
# NOTE: Disable SSL to avoid requiring OpenSSL/MbedTLS on the system.
# If you install libssl-dev later, switch this back to ON and use https URLs.
# Re-enable SSL now that OpenSSL is available on the system.
set(CPR_ENABLE_SSL ON CACHE INTERNAL "")
FetchContent_MakeAvailable(cpr)

# Prefer Boost CMake package (CONFIG); fall back to FindBoost module if not available
option(TOP100_ENABLE_TESTS "Build unit tests" ON)

# Optional UI frontends
option(TOP100_UI_QT "Build the Qt (cross-platform) UI" OFF)
option(TOP100_UI_KDE "Build the KDE-enhanced (Kirigami) UI" OFF)
option(TOP100_UI_GTK "Build the GTK (gtkmm) UI" OFF)
option(TOP100_UI_HAIKU "Build the Haiku (BeAPI) UI" OFF)

# --- Core Library ---
# Build as STATIC to avoid STL crossing shared library boundaries in tests/CLI
add_library(top100 STATIC lib/top100.cpp)
target_include_directories(top100 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(top100 PUBLIC nlohmann_json::nlohmann_json)

# Config helpers
add_library(top100_config STATIC lib/config.cpp)
target_include_directories(top100_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(top100_config PUBLIC nlohmann_json::nlohmann_json)

add_library(top100_config_utils STATIC lib/config_utils.cpp)
target_include_directories(top100_config_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(top100_config_utils PUBLIC top100_config)

# --- CLI Executable ---
add_executable(top100_cli 
    cli/main.cpp 
    cli/displaymenu.cpp
    cli/addmovie.cpp
    cli/removemovie.cpp
    cli/listmovies.cpp
    cli/viewdetails.cpp
    cli/dup_policy.h
    lib/omdb.cpp
    lib/bluesky.cpp
    lib/mastodon.cpp
    cli/addfromomdb.cpp
    cli/comparemovies.cpp
    cli/comparemovies.h)

# include paths for CLI
target_include_directories(top100_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cli ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# link libraries for CLI
target_link_libraries(top100_cli PRIVATE top100 top100_config top100_config_utils cpr::cpr nlohmann_json::nlohmann_json top100_services)

# Services (BlueSky + Mastodon) in a reusable library for UIs
add_library(top100_services STATIC lib/bluesky.cpp lib/mastodon.cpp lib/posting.cpp lib/omdb.cpp)
target_include_directories(top100_services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(top100_services PUBLIC cpr::cpr nlohmann_json::nlohmann_json)

if(TOP100_ENABLE_TESTS)
  # Prefer Boost CMake package (CONFIG); fall back to FindBoost module if not available
  find_package(Boost CONFIG QUIET COMPONENTS unit_test_framework)
  if(NOT Boost_FOUND)
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)
  endif()

  # --- Tests ---
  enable_testing()

  # New granular tests
  add_executable(test_core tests/test_core.cpp)
  target_link_libraries(test_core PRIVATE top100 Boost::unit_test_framework)
  # Per-case tests for CoreSuite
  add_test(NAME core_add_and_get_movie COMMAND test_core --run_test=CoreSuite/add_and_get_movie)
  add_test(NAME core_remove_movie COMMAND test_core --run_test=CoreSuite/remove_movie)
  add_test(NAME core_save_and_load COMMAND test_core --run_test=CoreSuite/save_and_load)

  add_executable(test_sorting tests/test_sorting.cpp)
  target_link_libraries(test_sorting PRIVATE top100 Boost::unit_test_framework)
  # Per-case tests for SortingSuite
  add_test(NAME sorting_by_year COMMAND test_sorting --run_test=SortingSuite/sort_by_year)
  add_test(NAME sorting_alphabetical COMMAND test_sorting --run_test=SortingSuite/sort_alphabetical)

  add_executable(test_movie_json tests/test_movie_json.cpp)
  target_link_libraries(test_movie_json PRIVATE top100 Boost::unit_test_framework nlohmann_json::nlohmann_json)
  # Standalone case (no explicit suite); use wildcard to match root
  add_test(NAME movie_json_round_trip COMMAND test_movie_json --run_test=json_round_trip_with_ratings)

  add_executable(test_find_replace tests/test_find_replace.cpp)
  target_link_libraries(test_find_replace PRIVATE top100 Boost::unit_test_framework)
  # Per-case tests for FindReplaceSuite
  add_test(NAME fr_find_by_imdb_and_title_year COMMAND test_find_replace --run_test=FindReplaceSuite/find_by_imdb_and_title_year)
  add_test(NAME fr_replace_movie COMMAND test_find_replace --run_test=FindReplaceSuite/replace_movie)

  # Ensure tests are built by default when building 'all'
  add_custom_target(tests_build ALL
    DEPENDS test_core test_sorting test_movie_json test_find_replace)

  # Ranking tests
  add_executable(test_ranking tests/test_ranking.cpp)
  target_link_libraries(test_ranking PRIVATE top100 Boost::unit_test_framework nlohmann_json::nlohmann_json)
  add_test(NAME ranking_json_fields COMMAND test_ranking --run_test=RankingSuite/json_rank_fields_round_trip)
  add_test(NAME ranking_recompute_and_sort COMMAND test_ranking --run_test=RankingSuite/recompute_ranks_and_sorting)
  add_test(NAME ranking_elo_update COMMAND test_ranking --run_test=RankingSuite/elo_update_changes_scores_and_order)

  # Config tests
  add_executable(test_config tests/test_config.cpp)
  target_link_libraries(test_config PRIVATE top100_config Boost::unit_test_framework nlohmann_json::nlohmann_json)
  add_test(NAME config_defaults COMMAND test_config --run_test=ConfigSuite/create_default_when_missing)
  add_test(NAME config_round_trip COMMAND test_config --run_test=ConfigSuite/load_save_round_trip)

  # Menu tests
  # Include the displaymenu implementation so the test can link/execute it
  add_executable(test_menu tests/test_menu.cpp cli/displaymenu.cpp)
  target_include_directories(test_menu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(test_menu PRIVATE Boost::unit_test_framework)
  add_test(NAME menu_disabled COMMAND test_menu --run_test=show_config_when_omdb_disabled)
  add_test(NAME menu_enabled COMMAND test_menu --run_test=show_omdb_when_enabled)
  add_test(NAME menu_bsky_disabled COMMAND test_menu --run_test=show_bsky_when_disabled)
  add_test(NAME menu_bsky_enabled COMMAND test_menu --run_test=show_bsky_when_enabled)

  add_executable(test_config_utils tests/test_config_utils.cpp)
  target_link_libraries(test_config_utils PRIVATE top100_config top100_config_utils Boost::unit_test_framework)
  add_test(NAME cfg_utils_reject COMMAND test_config_utils --run_test=ConfigUtilsSuite/fails_when_verifier_rejects)
  add_test(NAME cfg_utils_accept COMMAND test_config_utils --run_test=ConfigUtilsSuite/succeeds_and_persists_when_verifier_accepts)

  # UI strings test (header-only constants)
  add_executable(test_ui_strings tests/test_ui_strings.cpp)
  target_link_libraries(test_ui_strings PRIVATE Boost::unit_test_framework)
  add_test(NAME ui_strings_constants COMMAND test_ui_strings)

  # Expand aggregate tests_build dependencies to include all tests
  add_dependencies(tests_build test_ranking test_config test_config_utils test_menu test_ui_strings)
endif()

# --- Documentation (Doxygen) ---
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
  if(EXISTS ${DOXYFILE_IN})
    add_custom_target(docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_IN}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen (output in docs/html)"
      VERBATIM)
  else()
    message(STATUS "Doxygen found, but no Doxyfile present. Skipping docs target.")
  endif()
else()
  message(STATUS "Doxygen not found. 'docs' target will not be available.")
endif()

# --- UI Frontends -----------------------------------------------------------
if(TOP100_UI_QT OR TOP100_UI_KDE)
  # Enable Qt automoc/uic/rcc only if any UI is requested
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

if(TOP100_UI_QT)
  # Prefer Qt6, fall back to Qt5
  find_package(Qt6 QUIET COMPONENTS Widgets Concurrent Network)
  if(Qt6_FOUND)
  add_executable(top100_qt
    ui/qt/app.cpp
    ui/qt/window.cpp
    ui/qt/handlers.cpp
    ui/qt/poster.cpp
    ui/qt/menu.cpp
    ui/qt/toolbar.cpp
    ui/qt/adddialog.cpp
    ui/common/Top100ListModel.h ui/common/Top100ListModel.cpp)
  target_link_libraries(top100_qt PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::Network top100 top100_config top100_config_utils top100_services)
    target_compile_features(top100_qt PRIVATE cxx_std_17)
    set_target_properties(top100_qt PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE FALSE)
  else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent Network)
  add_executable(top100_qt
    ui/qt/app.cpp
    ui/qt/window.cpp
    ui/qt/handlers.cpp
    ui/qt/poster.cpp
    ui/qt/menu.cpp
    ui/qt/toolbar.cpp
    ui/qt/adddialog.cpp
    ui/common/Top100ListModel.h ui/common/Top100ListModel.cpp)
  target_link_libraries(top100_qt PRIVATE Qt5::Widgets Qt5::Concurrent Qt5::Network top100 top100_config top100_config_utils top100_services)
    target_compile_features(top100_qt PRIVATE cxx_std_17)
    set_target_properties(top100_qt PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE FALSE)
  endif()
endif()

if(TOP100_UI_KDE)
  # KDE Kirigami UI (QML)
  # Prefer Qt6 Quick/Qml first; only fall back to Qt5 if Qt6 isn't available.
  set(_kirigami_target "")
  set(_qt_quick_targets "")
  set(_qt_major "")

  find_package(Qt6 QUIET COMPONENTS Quick Qml Concurrent)
  if(Qt6_FOUND)
    set(_qt_major 6)
    set(_qt_quick_targets Qt6::Quick Qt6::Qml Qt6::Concurrent)
    # Link to Kirigami2 target if available for KF6; otherwise proceed with runtime QML module
    find_package(KF6Kirigami2 QUIET)
    if(KF6Kirigami2_FOUND)
      set(_kirigami_target KF6::Kirigami2)
    else()
      message(WARNING "KF6 Kirigami2 CMake package not found. Building with Qt6 Quick; ensure qml6-module-org-kde-kirigami is installed at runtime.")
    endif()
  else()
    # Qt6 not found; require Qt5 Quick/Qml and optionally link KF5 Kirigami
  find_package(Qt5 REQUIRED COMPONENTS Quick Qml Concurrent)
    set(_qt_major 5)
  set(_qt_quick_targets Qt5::Quick Qt5::Qml Qt5::Concurrent)
    find_package(KF5Kirigami2 QUIET)
    if(KF5Kirigami2_FOUND)
      set(_kirigami_target KF5::Kirigami2)
    else()
      message(WARNING "Kirigami2 CMake package not found. Building with Qt5 Quick; ensure qml-module-org-kde-kirigami2 is installed at runtime.")
    endif()
  endif()

  # Resources: QML packaged via qrc using matching Qt major
  if(_qt_major EQUAL 6)
    qt_add_resources(kde_qml_qrc ui/kde/resources.qrc)
  else()
    qt5_add_resources(kde_qml_qrc ui/kde/resources.qrc)
  endif()

  add_executable(top100_kde ui/kde/main.cpp ui/common/Top100ListModel.h ui/common/Top100ListModel.cpp ${kde_qml_qrc})
  target_link_libraries(top100_kde PRIVATE ${_qt_quick_targets} ${_kirigami_target} top100 top100_config top100_config_utils top100_services)
  target_compile_features(top100_kde PRIVATE cxx_std_17)
  set_target_properties(top100_kde PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE FALSE)
endif()

# GTK UI (gtkmm 3)
if(TOP100_UI_GTK)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
  # GTK frontend does not use the Qt-based Top100ListModel; use core libs directly
  add_executable(top100_gtk
    ui/gtk/app.cpp
    ui/gtk/window.cpp
    ui/gtk/menu.cpp
    ui/gtk/poster.cpp
    ui/gtk/handlers.cpp
    ui/gtk/toolbar.cpp
    ui/gtk/adddialog.cpp
  )
  target_include_directories(top100_gtk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui/common ${CMAKE_CURRENT_SOURCE_DIR}/lib)
  target_link_libraries(top100_gtk PRIVATE PkgConfig::GTKMM top100 top100_config top100_config_utils top100_services)
  target_compile_features(top100_gtk PRIVATE cxx_std_17)
  set_target_properties(top100_gtk PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE FALSE)
endif()

# Haiku UI (BeAPI)
if(TOP100_UI_HAIKU)
  if(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    add_executable(top100_haiku
      ui/haiku/app.cpp
      ui/haiku/window.cpp
      ui/haiku/add_dialog.cpp
    )
    target_include_directories(top100_haiku PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui/haiku ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    target_link_libraries(top100_haiku PRIVATE top100 top100_config top100_config_utils top100_services be tracker)
    target_compile_features(top100_haiku PRIVATE cxx_std_17)
    set_target_properties(top100_haiku PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE FALSE)
  else()
    message(WARNING "TOP100_UI_HAIKU is ON but host OS is not Haiku; skipping Haiku UI build.")
  endif()
endif()