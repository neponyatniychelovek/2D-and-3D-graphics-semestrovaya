#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <GL/glut.h>

using namespace std;

// --- НАСТРОЙКИ ---
const int MAX_PARTICLES = 500; // Количество частиц
GLuint fireTexture;            // ID текстуры

// Структура одной частицы
struct Particle {
    float x, y, z;      // Позиция
    float vx, vy, vz;   // Скорость
    float life;         // Жизнь (1.0 -> 0.0)
    float decay;        // Скорость угасания
    float size;         // Размер
};

// Массив частиц
vector<Particle> particles;

// --- 1. ГЕНЕРАЦИЯ ТЕКСТУРЫ ОГНЯ (В ПАМЯТИ) ---
// Чтобы не таскать файлы картинок, создадим пятно программно.
void createFireTexture() {
    const int W = 32;
    const int H = 32;
    GLubyte image[H][W][4]; // RGBA

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            // Расстояние от центра (0..1)
            float dx = (x - W / 2.0f) / (W / 2.0f);
            float dy = (y - H / 2.0f) / (H / 2.0f);
            float dist = sqrt(dx*dx + dy*dy);

            // Яркость падает к краям
            float alpha = 1.0f - dist;
            if (alpha < 0) alpha = 0;
            
            // Цвет белый, прозрачность зависит от расстояния
            image[y][x][0] = 255;
            image[y][x][1] = 255;
            image[y][x][2] = 255;
            image[y][x][3] = (GLubyte)(alpha * 255);
        }
    }

    // Загружаем в OpenGL
    glGenTextures(1, &fireTexture);
    glBindTexture(GL_TEXTURE_2D, fireTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

// Функция сброса частицы (рождение новой)
void resetParticle(Particle& p) {
    p.x = (rand() % 100 - 50) / 100.0f; // Разброс по X (-0.5 .. 0.5)
    p.y = 0.0f;                         // Начинаем снизу
    p.z = (rand() % 100 - 50) / 100.0f; // Разброс по Z

    p.vx = (rand() % 100 - 50) / 5000.0f; // Легкое дрожание влево-вправо
    p.vy = (rand() % 100 + 50) / 5000.0f; // Скорость вверх
    p.vz = (rand() % 100 - 50) / 5000.0f;

    p.life = 1.0f; // Полная жизнь
    p.decay = (rand() % 100) / 2000.0f + 0.005f; // Случайная скорость смерти
    p.size = 0.3f; // Начальный размер
}

// --- ИНИЦИАЛИЗАЦИЯ ---
void init() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Темный фон

    createFireTexture();

    // Создаем частицы
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle p;
        resetParticle(p);
        particles.push_back(p);
    }

    // Включаем текстуры
    glEnable(GL_TEXTURE_2D);
}

// --- ОБНОВЛЕНИЕ (ФИЗИКА) ---
void update(int value) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = particles[i];

        // Двигаем
        p.x += p.vx;
        p.y += p.vy;
        p.z += p.vz;

        // Умираем
        p.life -= p.decay;

        // Огонь сужается к верху (как конус)
        p.x *= 0.99f; 
        p.z *= 0.99f;

        // Если умерла - перерождаем внизу
        if (p.life <= 0.0f) {
            resetParticle(p);
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // 60 FPS
}

// --- ОТРИСОВКА ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Камера чуть сверху
    gluLookAt(0.0f, 2.0f, 4.0f,  
              0.0f, 1.0f, 0.0f,  
              0.0f, 1.0f, 0.0f);

    // Рисуем пол (сетка) для ориентира
    glBegin(GL_LINES);
    glColor3f(0.3f, 0.3f, 0.3f);
    for(int i=-5; i<=5; i++) {
        glVertex3f(i, 0, -5); glVertex3f(i, 0, 5);
        glVertex3f(-5, 0, i); glVertex3f(5, 0, i);
    }
    glEnd();

    // --- МАГИЯ ОГНЯ ---
    
    // 1. Включаем смешивание (Blending)
    glEnable(GL_BLEND);
    // GL_SRC_ALPHA, GL_ONE -> Аддитивное смешивание (Складывает яркость)
    // Черный фон (0) + Огонь = Огонь. Огонь + Огонь = Очень яркий Огонь.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // 2. Отключаем запись в Z-буфер (Depth Mask)
    // Мы хотим видеть частицы друг сквозь друга.
    // Если оставить true, передние частицы "закроют" задние черными квадратами.
    glDepthMask(GL_FALSE);

    glBindTexture(GL_TEXTURE_2D, fireTexture);

    // Рисуем каждую частицу
    glBegin(GL_QUADS);
    for (const auto& p : particles) {
        // Цвет зависит от жизни:
        // Жизнь 1.0 (Низ) -> Желтый/Белый
        // Жизнь 0.5 (Середина) -> Оранжевый/Красный
        // Жизнь 0.0 (Верх) -> Темный/Прозрачный
        
        float r = 1.0f;
        float g = p.life;       // Чем меньше жизни, тем краснее (меньше зеленого)
        float b = p.life * 0.5f; // Немного синего для белизны внизу

        glColor4f(r, g, b, p.life); // Alpha тоже падает

        // BILLBOARDING (Упрощенный)
        // Чтобы частицы всегда смотрели на камеру, мы просто рисуем их в плоскости XY,
        // но так как камера почти спереди, это сработает.
        // Для идеального билбординга нужно вращать матрицу, но для лабы это перебор.
        
        float s = p.size;
        // Текстурные координаты (0,0 ... 1,1) и Вершины
        glTexCoord2f(0, 0); glVertex3f(p.x - s, p.y - s, p.z);
        glTexCoord2f(1, 0); glVertex3f(p.x + s, p.y - s, p.z);
        glTexCoord2f(1, 1); glVertex3f(p.x + s, p.y + s, p.z);
        glTexCoord2f(0, 1); glVertex3f(p.x - s, p.y + s, p.z);
    }
    glEnd();

    // 3. Возвращаем обычные настройки OpenGL (чтобы не сломать отрисовку других объектов)
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab 7: Fire Particle System");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}