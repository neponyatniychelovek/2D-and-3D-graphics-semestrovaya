// Определяем, что мы используем статическую или динамическую версию,
// но для простоты просто подключаем заголовок.
#include <iostream>
// Сначала подключаем Windows, потом GL
#include <windows.h> 
#include <GL/glut.h> 

// Переменные для вращения
float angleX = 0.0f;
float angleY = 0.0f;
float angleZ = 0.0f;

// --- НАСТРОЙКА СЦЕНЫ ---
void init() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Темно-серый фон
    glEnable(GL_DEPTH_TEST); // Включаем Z-буфер (чтобы грани не просвечивали)

    // Свет
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_pos[] = { 4.0f, 4.0f, 4.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    // Материалы
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 60.0f };
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

// --- ИЗМЕНЕНИЕ РАЗМЕРА ОКНА ---
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);
    
    glMatrixMode(GL_MODELVIEW);
}

// --- РИСОВАНИЕ ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Камера: стоим в (0,0,6), смотрим в (0,0,0)
    gluLookAt(0.0f, 0.0f, 6.0f, 
              0.0f, 0.0f, 0.0f, 
              0.0f, 1.0f, 0.0f);

    // Вращение
    glRotatef(angleX, 1.0f, 0.0f, 0.0f);
    glRotatef(angleY, 0.0f, 1.0f, 0.0f);
    glRotatef(angleZ, 0.0f, 0.0f, 1.0f);

    glColor3f(0.0f, 0.5f, 1.0f); // Синий куб

    glBegin(GL_QUADS);
    // Передняя грань
    glNormal3f(0.0f, 0.0f, 1.0f); 
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    // Задняя
    glNormal3f(0.0f, 0.0f, -1.0f); 
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    // Верхняя
    glNormal3f(0.0f, 1.0f, 0.0f); 
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    // Нижняя
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    // Правая
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f( 1.0f, -1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f, -1.0f);
    glVertex3f( 1.0f,  1.0f,  1.0f);
    glVertex3f( 1.0f, -1.0f,  1.0f);
    // Левая
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glEnd();

    glutSwapBuffers();
}

// --- ТАЙМЕР ---
void timer(int value) {
    angleX += 1.0f; 
    angleY += 1.5f;
    angleZ += 0.5f;

    if (angleX > 360) angleX -= 360;
    if (angleY > 360) angleY -= 360;
    if (angleZ > 360) angleZ -= 360;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab 4: 3D Cube");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}