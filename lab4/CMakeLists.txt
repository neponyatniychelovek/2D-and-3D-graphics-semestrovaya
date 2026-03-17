cmake_minimum_required(VERSION 3.10)

project(Cube3D)

# 1. Находим стандартный OpenGL (встроен в Windows)
find_package(OpenGL REQUIRED)

# 2. Указываем путь к нашему FreeGLUT
set(FREEGLUT_DIR "${CMAKE_SOURCE_DIR}/freeglut")

# Подключаем папку include (чтобы работало #include <GL/glut.h>)
include_directories("${FREEGLUT_DIR}/include")

# 3. Создаем программу
add_executable(Cube3D main.cpp)

# 4. Привязываем библиотеки
# opengl32 и glu32 - стандартные системные
# freeglut.lib - берем из нашей папки
target_link_libraries(Cube3D 
    opengl32 
    glu32 
    "${FREEGLUT_DIR}/lib/x64/freeglut.lib"
)

# --- АВТО-КОПИРОВАНИЕ DLL (ПОРТАТИВНОСТЬ) ---
# Копируем freeglut.dll к exe-файлу, иначе программа не запустится
add_custom_command(TARGET Cube3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${FREEGLUT_DIR}/bin/x64/freeglut.dll"
    $<TARGET_FILE_DIR:Cube3D>
)