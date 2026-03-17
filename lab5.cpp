#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h> 
#include <GL/glut.h> 

using namespace std;

// --- СТРУКТУРЫ ДАННЫХ ---
struct Vector3 { float x, y, z; };
struct Face { 
    int v[3];  // Индексы вершин
    int vn[3]; // Индексы нормалей
};

// Глобальные хранилища данных модели
vector<Vector3> vertices;
vector<Vector3> normals;
vector<Face> faces;

// Переменные вращения
float angleY = 0.0f;

// --- 1. ЗАГРУЗЧИК OBJ ФАЙЛОВ ---
bool loadOBJ(const string& path) {
    ifstream file(path);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string prefix;
        ss >> prefix;

        if (prefix == "v") { // Вершина (Vertex)
            Vector3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (prefix == "vn") { // Нормаль (Normal) - важна для света!
            Vector3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (prefix == "f") { // Грань (Face)
            Face f;
            char slash;
            int trash; // Для пропуска текстурных координат (vt)

            // OBJ формат бывает разным: v/vt/vn или v//vn
            // Мы читаем 3 вершины треугольника
            for (int i = 0; i < 3; i++) {
                // Считываем индекс Вершины
                ss >> f.v[i]; 
                
                // Проверяем следующий символ
                if (ss.peek() == '/') {
                    ss.ignore(); // Пропускаем первый слэш
                    if (ss.peek() == '/') {
                        // Формат v//vn
                        ss.ignore(); // Пропускаем второй слэш
                        ss >> f.vn[i];
                    } else {
                        // Формат v/vt/vn
                        ss >> trash; // Читаем текстуру (игнорируем)
                        ss >> slash; // Читаем слэш
                        ss >> f.vn[i]; // Читаем нормаль
                    }
                }
                
                // OBJ индексы начинаются с 1, а в C++ с 0. Вычитаем 1.
                f.v[i]--;
                f.vn[i]--;
            }
            faces.push_back(f);
        }
    }
    file.close();
    return true;
}

// --- 2. НАСТРОЙКА СВЕТА (PHONG MODEL) ---
void initLighting() {
    glEnable(GL_DEPTH_TEST); // Z-буфер
    glEnable(GL_LIGHTING);   // Включаем расчет освещения
    glEnable(GL_LIGHT0);     // Включаем Лампу №0

    // Позиция света (справа сверху спереди)
    GLfloat lightPos[] = { 2.0f, 4.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Цвет света (Ambient, Diffuse, Specular)
    GLfloat amb[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Фоновый свет (чтобы в тени не было черноты)
    GLfloat diff[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Основной свет
    GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Цвет блика (белый)

    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    // --- МАТЕРИАЛ (Золотой пластик) ---
    glEnable(GL_COLOR_MATERIAL); // Разрешаем менять цвет материала через glColor
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat matSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Яркий белый блик
    GLfloat matShine[] = { 50.0f };                 // Размер пятна блика (Фонг)
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShine);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Камера
    gluLookAt(0, 2, 4,  0, 0, 0,  0, 1, 0);

    // Вращение модели
    glRotatef(angleY, 0.0f, 1.0f, 0.0f);

    glColor3f(1.0f, 0.8f, 0.2f); // Золотистый цвет

    // --- ОТРИСОВКА МОДЕЛИ ---
    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        // Рисуем треугольник
        for (int i = 0; i < 3; i++) {
            // 1. Сначала ставим НОРМАЛЬ (важно для света!)
            // Если нормали нет, берем (0,1,0), иначе свет сломается
            if (face.vn[i] >= 0 && face.vn[i] < normals.size()) {
                Vector3 n = normals[face.vn[i]];
                glNormal3f(n.x, n.y, n.z);
            }

            // 2. Потом ставим ВЕРШИНУ
            Vector3 v = vertices[face.v[i]];
            glVertex3f(v.x, v.y, v.z);
        }
    }
    glEnd();

    glutSwapBuffers();
}

void timer(int value) {
    angleY += 1.0f; // Крутим
    if (angleY > 360) angleY -= 360;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
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
    // Попытка загрузить модель
    // Сначала ищем в папке с exe (для портативности)
    if (!loadOBJ("model.obj")) {
        // Если не нашли, пробуем искать в папке исходников (для режима отладки)
        if (!loadOBJ("../model.obj")) {
            cout << "CRITICAL ERROR: 'model.obj' not found!" << endl;
            cout << "Put model.obj next to the .exe file." << endl;
            system("pause");
            return -1;
        }
    }
    
    cout << "Loaded Model:" << endl;
    cout << " Vertices: " << vertices.size() << endl;
    cout << " Faces:    " << faces.size() << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab 5: Phong Lighting & Model Loader");

    initLighting();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}