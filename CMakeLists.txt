cmake_minimum_required(VERSION 3.0)

project(allolib_playground)

# This cmake file is for single cpp file projects that does not have
# CMakeLists.txt file in the project folder.

# user needs to pass definition `AL_APP_FILE` to this script

# example:
# at/proj/folder > mkdir build
# at/proj/folder > cd build
# at/proj/folder/build > cmake -DAL_APP_FILE=../my_app.cpp path/to/allolib/cmake/single_file
# at/proj/folder/build > make
# at/proj/folder/build > cd ../bin
# at/proj/folder/build > ./my_app

# allolib_playground's run.sh script will do above process too

option(AL_VERBOSE_OUTPUT "" OFF)
option(AL_APP_RUN "" ON)

if (NOT AL_APP_FILE)
  message("[!] app file not provided, building all\n")
  set(BUILD_EXAMPLES 1)
  set(BUILD_ALL 1)
endif ()

if (IS_DIRECTORY AL_APP_FILE)
  message(CRITICAL_ERROR "[!] pass file to this cmake script\n")
endif ()


if (IS_ABSOLUTE ${AL_APP_FILE})
  get_filename_component(file_full_path ${AL_APP_FILE} ABSOLUTE)
else ()
  get_filename_component(file_full_path ${CMAKE_CURRENT_BINARY_DIR}/${AL_APP_FILE} ABSOLUTE)
endif ()


# include allolib target and Gamma target
set(al_path ${CMAKE_CURRENT_SOURCE_DIR}/allolib)
if (DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
  add_subdirectory("${al_path}" "${al_path}/build")
else()
  if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_subdirectory("${al_path}" "${al_path}/build/Debug")
  else()
    add_subdirectory("${al_path}" "${al_path}/build/Release")
  endif()
endif ()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/al_ext")
  add_subdirectory(al_ext "${al_path}/build/al_ext")
  get_target_property(AL_EXT_LIBRARIES al_ext AL_EXT_LIBRARIES)
  message("al_ext linking to ${AL_EXT_LIBRARIES}")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # On macOS if jack is present, RtMidi will link to jack but not provide the
    # /usr/local/lib location as link directory. This statement only serves the
    # purpose of working around that bug. Perhaps can be removed on future
    # updates of RtMidi
    link_directories("/usr/local/lib")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

get_filename_component(app_name ${file_full_path} NAME_WE)
get_filename_component(app_path ${file_full_path} DIRECTORY)
set(app_files_list ${file_full_path})

macro(BUILD_FILE  this_app_name this_app_path this_app_files_list)
    if(EXISTS "${this_app_path}/flags.cmake")
      if (AL_VERBOSE_OUTPUT)
        message("found flags.cmake")
      endif()
      include(${this_app_path}/flags.cmake)
    endif()

    if(EXISTS "${this_app_path}/app_config.cmake")
      if (AL_VERBOSE_OUTPUT)
        message("found app_config.cmake")
      endif()
      include(${this_app_path}/app_config.cmake)
    endif()

    if (AL_VERBOSE_OUTPUT)
      message("binary dir: ${CMAKE_CURRENT_BINARY_DIR}")
      message("file full path: ${file_full_path}")
      message("app path: ${this_app_path}")
      message("app name: ${this_app_name}")
      message("al path: ${al_path}")
      message("include dirs: ${app_include_dirs}")
      message("link libs: ${app_link_libs}")
      message("definitions: ${app_definitions}")
      message("compile flags: ${app_compile_flags}")
      message("linker flags: ${app_linker_flags}")
    endif()

    add_executable(${this_app_name} ${this_app_files_list})

    set_target_properties(${this_app_name} PROPERTIES
      DEBUG_POSTFIX d
      RUNTIME_OUTPUT_DIRECTORY ${this_app_path}/bin
      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${this_app_path}/bin
      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${this_app_path}/bin
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED ON
    )

    # flags
    if (AL_WINDOWS)
      target_compile_options(${this_app_name} PRIVATE "")
    else ()
      target_compile_options(${this_app_name} PRIVATE "-Wall")
    endif (AL_WINDOWS)

    target_link_libraries(${this_app_name} PUBLIC al ${al_libs})

    foreach(include_dir IN LISTS app_include_dirs)
        if (IS_ABSOLUTE ${include_dir})
            target_include_directories(${this_app_name} PRIVATE ${include_dir})
        else()
            target_include_directories(${this_app_name} PRIVATE ${this_app_path}/${include_dir})
        endif()
    endforeach(include_dir IN app_include_dirs)

    target_include_directories(${this_app_name} PRIVATE ${al_includes})

    target_link_libraries(${this_app_name} PRIVATE ${app_link_libs} ${AL_EXT_LIBRARIES})
    target_compile_definitions(${this_app_name} PRIVATE ${app_definitions})
    target_compile_options(${this_app_name} PRIVATE ${app_compile_flags})
    # Item names starting with -, but not -l or -framework, are treated as linker flags.
    target_link_libraries(${this_app_name} PRIVATE ${app_linker_flags})
endmacro()


if (AL_APP_FILE)
  BUILD_FILE("${app_name}" "${app_path}" "${app_files_list}")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      set (DEBUGGER "gdb" "-ex" "run")
  elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set (DEBUGGER "lldb" "-o run")
  endif()

  add_custom_target(${app_name}_run
      ./${app_name}
      DEPENDS ${app_name}
      WORKING_DIRECTORY ${app_path}/bin
      USES_TERMINAL
      )
  add_custom_target(${app_name}_run_debug
      ${DEBUGGER} ./${app_name}d
      DEPENDS ${app_name}
      WORKING_DIRECTORY ${app_path}/bin
      USES_TERMINAL
      )


  if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # when run from Visual Studio, working directory is where the solution is by default
    # set it to app output directory
    set_target_properties(${app_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${app_path}/bin)
    # startup project is `ALL_BUILD` by default so we change it to app project
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${app_name})

  if (USE_PORTAUDIO)
    list(APPEND post_build_command
      robocopy ${al_path}/dependencies/portaudio/ ${app_path}/bin portaudio_x64.dll &
    )
  endif (USE_PORTAUDIO)

  if (USE_APR)
    list(APPEND post_build_command
      robocopy ${al_path}/dependencies/apr/ ${app_path}/bin libapr-1.dll &
    )
  endif (USE_APR)

  if (FREEIMAGE_FOUND)
    list(APPEND post_build_command
      robocopy ${al_path}/dependencies/FreeImage/Dist/x64 ${app_path}/bin FreeImage.dll &
    )
  endif (FREEIMAGE_FOUND)

  list(APPEND post_build_command
    IF %ERRORLEVEL% LEQ 1 exit 0
  )

  add_custom_command(
    TARGET ${app_name}
    POST_BUILD
    COMMAND ${post_build_command}
  )

  add_custom_command(
    TARGET ${app_name}_run
    POST_BUILD
    COMMAND ${post_build_command}
    )
  add_custom_command(
    TARGET ${app_name}_run_debug
    POST_BUILD
    COMMAND ${post_build_command}
    )

endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

else()
    # Build everything
    list(APPEND paths "tutorials/primer/*.cpp")
    list(APPEND paths "tutorials/interaction-sequencing/*.cpp")
    list(APPEND paths "tutorials/gui/*.cpp")
    list(APPEND paths "tutorials/synthesis/*.cpp")
    list(APPEND paths "tutorials/allosphere/*.cpp")
    list(APPEND paths "cookbook/av/*.cpp")
    list(APPEND paths "cookbook/ui/*.cpp")
    list(APPEND paths "cookbook/shaders/*.cpp")
    list(APPEND paths "cookbook/simulation/*.cpp")
    list(APPEND paths "cookbook/sonification/*.cpp")
    list(APPEND paths "cookbook/distributed/*.cpp")
    list(APPEND paths "tools/audio/*.cpp")
    foreach(path IN LISTS paths)
        message("Building path ${path}")
        FILE(GLOB sources "${path}")
        foreach(source IN LISTS sources)
            get_filename_component(app_name ${source} NAME_WE)
            get_filename_component(app_path ${source} DIRECTORY)
            string(REGEX MATCHALL "[a-zA-Z]+$" parent_dir "${app_path}")
            message("Building ${app_name} from ${parent_dir}")
            BUILD_FILE("${parent_dir}_${app_name}" "${app_path}" "${source}")
        endforeach()
    endforeach()

endif(AL_APP_FILE)
