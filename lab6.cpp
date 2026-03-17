#define _USE_MATH_DEFINES

#include <iostream>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// --- НАСТРОЙКИ СЦЕНЫ ---
const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_DEPTH = 3; // Сколько раз луч может отразиться (глубина рекурсии)

// --- СТРУКТУРЫ ---

// Шар: Центр, Радиус, Цвет, Зеркальность
struct Sphere {
    Vec3f center;
    float radius;
    Vec3f color;
    float reflection; // 0.0 - матовый, 1.0 - зеркало
};

// Свет: Позиция, Яркость
struct Light {
    Vec3f position;
    float intensity;
};

// --- СЦЕНА (ОБЪЕКТЫ) ---
// Определяем объекты "аналитически" (формулами, а не полигонами)
vector<Sphere> spheres = {
    // Центральный красный шар
    { Vec3f(0, 0, -5), 1.0f, Vec3f(0, 0, 1), 0.2f }, 
    // Левый зеленый зеркальный шар
    { Vec3f(-2, 0, -6), 1.0f, Vec3f(0, 1, 0), 0.6f }, 
    // Правый синий матовый шар
    { Vec3f(2, 0, -4), 1.0f, Vec3f(1, 0, 0), 0.0f },
    // Большой желтый шар внизу (имитирует "пол")
    { Vec3f(0, -1001, -5), 1000.0f, Vec3f(0.8, 0.8, 0.8), 0.1f } 
};

// Источник света
Light light = { Vec3f(5, 5, 0), 1.2f };

// --- МАТЕМАТИКА ПЕРЕСЕЧЕНИЙ ---

// Проверка: Пересекает ли луч шар?
// Возвращает расстояние t до удара. Если не пересекает, возвращает -1.
float intersectSphere(const Vec3f& rayOrigin, const Vec3f& rayDir, const Sphere& s) {
    Vec3f L = s.center - rayOrigin;
    float tca = L.dot(rayDir); // Проекция центра на луч
    float d2 = L.dot(L) - tca * tca; // Расстояние от центра до луча (в квадрате)
    
    float r2 = s.radius * s.radius;
    if (d2 > r2) return -1.0f; // Луч прошел мимо

    float thc = sqrt(r2 - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;

    if (t0 < 0.001f) t0 = t1; // Если t0 сзади, берем t1
    if (t0 < 0.001f) return -1.0f; // Оба сзади

    return t0;
}

// --- ГЛАВНАЯ ФУНКЦИЯ ТРАССИРОВКИ (RECURSIVE) ---
// rayOrigin - откуда летит луч
// rayDir - куда летит
// depth - счетчик отражений (чтобы не зациклиться)
Vec3f trace(const Vec3f& rayOrigin, const Vec3f& rayDir, int depth) {
    float closestT = 1e9; // Бесконечность
    int closestSphereIdx = -1;

    // 1. Ищем ближайший объект, в который врезался луч
    for (size_t i = 0; i < spheres.size(); i++) {
        float t = intersectSphere(rayOrigin, rayDir, spheres[i]);
        if (t > 0.0f && t < closestT) {
            closestT = t;
            closestSphereIdx = i;
        }
    }

    // Если никуда не попали — возвращаем цвет фона (голубой градиент)
    if (closestSphereIdx == -1) {
        float t = 0.5f * (rayDir[1] + 1.0f);
        return (1.0f - t) * Vec3f(1, 1, 1) + t * Vec3f(1, 0.7, 0.5); // Небо
    }

    // --- ЕСЛИ ПОПАЛИ В ОБЪЕКТ ---
    const Sphere& sphere = spheres[closestSphereIdx];
    
    // Точка удара (P) и Нормаль (N)
    Vec3f hitPoint = rayOrigin + rayDir * closestT;
    Vec3f normal = normalize(hitPoint - sphere.center);
    
    // Слегка сдвигаем точку удара наружу, чтобы избежать "самозатенения" (ошибки float)
    Vec3f bias = 1e-4f * normal; 

    Vec3f finalColor = Vec3f(0, 0, 0);

    // 2. РАСЧЕТ ОСВЕЩЕНИЯ (DIFFUSE + SPECULAR + SHADOW)
    Vec3f lightDir = normalize(light.position - hitPoint);
    
    // ТЕНИ: Пускаем луч от точки удара к лампе.
    bool inShadow = false;
    for (const auto& s : spheres) {
        float t = intersectSphere(hitPoint + bias, lightDir, s);
        // Если луч врезался во что-то, и это "что-то" ближе, чем лампа
        if (t > 0.001f) {
            inShadow = true;
            break;
        }
    }

    if (!inShadow) {
        // Диффузное освещение (угол между нормалью и светом)
        float diff = max(0.0f, normal.dot(lightDir));
        
        // Блик (Specular)
        Vec3f reflectDir = normalize(rayDir - normal * 2.0f * normal.dot(rayDir)); // Отражение
        float spec = pow(max(0.0f, reflectDir.dot(lightDir)), 50.0f) * 0.6f; // 50 - резкость блика

        // Складываем цвета (B G R)
        // sphere.color * diff * lightPower
        finalColor += sphere.color * diff * light.intensity; 
        finalColor += Vec3f(1,1,1) * spec; // Белый блик
    } else {
        // Если в тени, оставляем только слабый цвет (Ambient)
        finalColor += sphere.color * 0.1f;
    }

    // 3. ОТРАЖЕНИЕ (REFLECTION)
    // Если материал зеркальный и мы еще не достигли лимита глубины
    if (sphere.reflection > 0 && depth < MAX_DEPTH) {
        // Считаем вектор отражения: R = I - 2(N.I)N
        Vec3f r = normalize(rayDir - normal * 2.0f * normal.dot(rayDir));
        
        // Рекурсивный вызов! Пускаем луч дальше
        Vec3f reflectedColor = trace(hitPoint + bias, r, depth + 1);
        
        // Смешиваем собственный цвет и цвет отражения
        finalColor = finalColor * (1.0f - sphere.reflection) + reflectedColor * sphere.reflection;
    }

    return finalColor;
}

int main() {
    // Создаем матрицу картинки (OpenCV)
    Mat img(HEIGHT, WIDTH, CV_32FC3); // 32-bit Float, 3 канала

    cout << "=== RAY TRACER 2025 ===" << endl;
    cout << "Rendering scene... Please wait." << endl;

    // Главный цикл: идем по каждому пикселю экрана
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            
            // Превращаем координаты пикселя (x, y) в координаты экрана (-1..1)
            // Аспект (соотношение сторон) важен, чтобы шары не были овальными
            float aspect = (float)WIDTH / HEIGHT;
            float px = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * tan(M_PI / 4.0f * 0.5f) * aspect;
            float py = (1.0f - 2.0f * (y + 0.5f) / HEIGHT) * tan(M_PI / 4.0f * 0.5f);

            // Луч летит из (0,0,0) в точку экрана
            Vec3f rayOrigin(0, 0, 0);
            Vec3f rayDir = normalize(Vec3f(px, py, -1)); // -1 значит "вглубь" экрана

            // Запускаем трассировку для этого пикселя
            Vec3f pixelColor = trace(rayOrigin, rayDir, 0);

            // Ограничиваем яркость (чтобы не было > 1.0) и гамма-коррекция
            // В OpenCV цвета хранятся как (Blue, Green, Red), а не RGB!
            // Но мы считали в логике RGB. В simple sphere цвета заданы вручную, так что ок.
            img.at<Vec3f>(y, x) = pixelColor;
        }
        
        // Прогресс-бар в консоли (каждые 50 строк)
        if (y % 50 == 0) cout << "Progress: " << (y * 100 / HEIGHT) << "%" << endl;
    }

    cout << "Done! Displaying result." << endl;
    cout << "Press [S] to save image, [ESC] to exit." << endl;

    // Показываем результат
    // imshow ожидает значения 0..1 для float матриц, все ок
    namedWindow("Ray Tracing", WINDOW_AUTOSIZE);
    imshow("Ray Tracing", img);

    while (true) {
        char key = (char)waitKey(0);
        if (key == 27) break; // ESC
        if (key == 's' || key == 'S') {
            // Для сохранения переводим в 0..255
            Mat saveImg;
            img.convertTo(saveImg, CV_8UC3, 255.0);
            imwrite("render.jpg", saveImg);
            cout << "Saved: render.jpg" << endl;
        }
    }

    return 0;
}