cmake_minimum_required(VERSION 3.16)
project(CheckersC C)
set(CMAKE_C_STANDARD 99)
include_directories(include)

if (WIN32)
  add_executable(CheckersC WIN32
      src/board.c
      src/rules.c
      src/ai.c
      src/gui_win.c
  )
  target_link_libraries(CheckersC PRIVATE user32 gdi32)
else()
  add_executable(CheckersC
      src/board.c
      src/rules.c
      src/ai.c
      src/gui_win.c
  )
endif()
