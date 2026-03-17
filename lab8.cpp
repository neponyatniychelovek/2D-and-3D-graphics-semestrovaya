#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp> // Для чтения картинки
#include <windows.h>
#include <GL/glut.h>          // Для рисования 3D

using namespace std;
using namespace cv;

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
Mat heightMap;        // Сюда загрузим картинку
float angleY = 0.0f;  // Вращение камеры
float cameraH = 200.0f; // Высота камеры

// --- ЗАГРУЗКА КАРТЫ ВЫСОТ ---
void loadHeightMap(const string& filename) {
    // Загружаем в оттенках серого (0..255)
    heightMap = imread(filename, IMREAD_GRAYSCALE);

    if (heightMap.empty()) {
        cout << "Error: Could not load " << filename << endl;
        cout << "Generating random terrain..." << endl;
        // Если файла нет, создаем случайный шум (чтобы программа не упала)
        heightMap = Mat(128, 128, CV_8UC1);
        randu(heightMap, Scalar(0), Scalar(255));
    } else {
        // Если картинка слишком большая, уменьшим её для быстродействия
        // OpenGL в старом режиме медленно рисует миллионы треугольников
        if (heightMap.rows > 256 || heightMap.cols > 256) {
            resize(heightMap, heightMap, Size(256, 256));
            cout << "Image resized to 256x256 for performance." << endl;
        }
    }
}

// Получить высоту в точке (x, z)
// Возвращает float от 0.0 до 50.0 (масштаб высоты)
float getHeight(int x, int z) {
    // Защита от вылета за границы массива
    if (x < 0 || x >= heightMap.cols || z < 0 || z >= heightMap.rows) {
        return 0.0f;
    }
    // uchar - это байт (0..255).
    unsigned char pixel = heightMap.at<uchar>(z, x);
    return (float)pixel / 255.0f * 50.0f; // 50.0f - максимальная высота горы
}

// Установить цвет в зависимости от высоты
void setColorByHeight(float h) {
    float relativeH = h / 50.0f; // 0..1

    if (relativeH < 0.2f) {
        glColor3f(0.0f, 0.0f, 0.8f); // Синяя вода (низины)
    } 
    else if (relativeH < 0.7f) {
        glColor3f(0.1f, 0.6f, 0.1f); // Зеленая трава
    } 
    else {
        glColor3f(1.0f, 1.0f, 1.0f); // Белый снег (вершины)
    }
}

// --- ОТРИСОВКА ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Камера летает вокруг центра
    // x = sin, z = cos для кругового движения
    float rad = angleY * M_PI / 180.0f;
    float camX = sin(rad) * 200.0f;
    float camZ = cos(rad) * 200.0f;

    // Смотрим в центр карты
    gluLookAt(camX, cameraH, camZ, 
              0.0f, 0.0f, 0.0f, 
              0.0f, 1.0f, 0.0f);

    // Сдвигаем карту так, чтобы её центр был в (0,0,0)
    // Иначе она будет рисоваться сбоку
    float offsetX = -heightMap.cols / 2.0f;
    float offsetZ = -heightMap.rows / 2.0f;
    glTranslatef(offsetX, 0.0f, offsetZ);

    // --- ГЛАВНЫЙ ЦИКЛ ГЕНЕРАЦИИ МЕША ---
    // Мы рисуем полосками (Triangle Strips).
    // Берем ряд Z и ряд Z+1 и соединяем их треугольниками.
    
    for (int z = 0; z < heightMap.rows - 1; z++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = 0; x < heightMap.cols; x++) {
            
            // Точка 1 (Текущий ряд)
            float h1 = getHeight(x, z);
            setColorByHeight(h1);
            glVertex3f(x, h1, z);

            // Точка 2 (Следующий ряд)
            float h2 = getHeight(x, z + 1);
            setColorByHeight(h2);
            glVertex3f(x, h2, z + 1);
        }
        glEnd();
    }

    // Рисуем рамку воды (просто синий квадрат внизу)
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.3f, 0.8f);
    glVertex3f(0, 5, 0);
    glVertex3f(heightMap.cols, 5, 0);
    glVertex3f(heightMap.cols, 5, heightMap.rows);
    glVertex3f(0, 5, heightMap.rows);
    glEnd();

    glutSwapBuffers();
}

// Управление
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // ESC
    if (key == 'w') cameraH += 5.0f; // Вверх
    if (key == 's') cameraH -= 5.0f; // Вниз
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) angleY -= 5.0f;
    if (key == GLUT_KEY_RIGHT) angleY += 5.0f;
    glutPostRedisplay();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / h, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    // 1. Сначала загружаем карту (чтобы знать размеры окна или ошибки)
    loadHeightMap("heightmap.jpg");

    // 2. Инициализация GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab 8: Terrain Generator");

    // Включаем тест глубины, чтобы горы перекрывали друг друга
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // Голубое небо

    // Режим "Сетки" (раскомментируй, чтобы видеть полигоны)
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    cout << "Controls:" << endl;
    cout << "  [Left/Right] Rotate Camera" << endl;
    cout << "  [W/S] Camera Height" << endl;
    cout << "  [ESC] Exit" << endl;

    glutMainLoop();
    return 0;
}