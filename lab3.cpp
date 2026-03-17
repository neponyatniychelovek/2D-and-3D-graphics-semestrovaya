#include <iostream>
#include <opencv2/opencv.hpp>
#include <algorithm> // Нужно для функций min() и max()
#include <vector>

using namespace std;
using namespace cv;

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
Mat canvas;             // Наш холст
vector<Point> clicks;   // Список кликов
int currentMode = 1;    // 1 = Треугольник, 2 = Круг

// --- ФУНКЦИЯ ОТРИСОВКИ ИНТЕРФЕЙСА (НОВОЕ) ---
void drawInterface() {
    // 1. Рисуем темно-серую плашку сверху (фон для текста)
    // Point(0, 0) - левый верх, Point(canvas.cols, 40) - правый край, высота 40
    rectangle(canvas, Point(0, 0), Point(canvas.cols, 40), Scalar(50, 50, 50), -1);

    // 2. Формируем текст (На английском, чтобы не было ????)
    string text = "[1] Triangle  [2] Circle  [S] Save  [C] Clear  [ESC] Exit";

    // 3. Пишем текст белым цветом
    putText(canvas, text, Point(10, 28), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1, LINE_AA);

    // 4. Показываем текущий режим (справа)
    string modeText = (currentMode == 1) ? "Mode: TRIANGLE" : "Mode: CIRCLE";
    putText(canvas, modeText, Point(canvas.cols - 200, 28), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 1, LINE_AA);
}

// --- 1. АЛГОРИТМ РИСОВАНИЯ КРУГА ---
void myRasterizeCircle(Mat& img, Point center, int radius, Vec3b color) {
    int r2 = radius * radius; 
    int minX = max(0, center.x - radius);
    int maxX = min(img.cols - 1, center.x + radius);
    int minY = max(0, center.y - radius);
    int maxY = min(img.rows - 1, center.y + radius);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int dx = x - center.x;
            int dy = y - center.y;
            if (dx * dx + dy * dy <= r2) {
                // ВАЖНО: Не рисуем поверх меню (верхние 40 пикселей)
                if (y > 40) img.at<Vec3b>(y, x) = color;
            }
        }
    }
}

// --- 2. ПОМОЩНИК ДЛЯ ТРЕУГОЛЬНИКА ---
int edgeFunction(Point a, Point b, Point p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

// --- 3. АЛГОРИТМ РИСОВАНИЯ ТРЕУГОЛЬНИКА ---
void myRasterizeTriangle(Mat& img, Point p0, Point p1, Point p2, Vec3b color) {
    int minX = min({ p0.x, p1.x, p2.x });
    int maxX = max({ p0.x, p1.x, p2.x });
    int minY = min({ p0.y, p1.y, p2.y });
    int maxY = max({ p0.y, p1.y, p2.y });

    minX = max(minX, 0);
    minY = max(minY, 0);
    maxX = min(maxX, img.cols - 1);
    maxY = min(maxY, img.rows - 1);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Point p(x, y); 
            int w0 = edgeFunction(p1, p2, p);
            int w1 = edgeFunction(p2, p0, p);
            int w2 = edgeFunction(p0, p1, p);
            bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
            bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);

            if (!(hasNeg && hasPos)) {
                // ВАЖНО: Не рисуем поверх меню
                if (y > 40) img.at<Vec3b>(y, x) = color;
            }
        }
    }
}

// --- ОБРАБОТКА МЫШИ ---
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        // Не даем кликать по меню сверху
        if (y < 40) return;

        clicks.push_back(Point(x, y)); 

        circle(canvas, Point(x, y), 2, Scalar(255, 255, 255), -1);
        imshow("Rasterizer", canvas); 

        // ТРЕУГОЛЬНИК (3 клика)
        if (currentMode == 1 && clicks.size() == 3) {
            myRasterizeTriangle(canvas, clicks[0], clicks[1], clicks[2], Vec3b(0, 255, 0)); 
            clicks.clear(); 
            imshow("Rasterizer", canvas);
        }

        // КРУГ (2 клика)
        if (currentMode == 2 && clicks.size() == 2) {
            Point center = clicks[0]; 
            Point edge = clicks[1];   
            int radius = (int)sqrt(pow(center.x - edge.x, 2) + pow(center.y - edge.y, 2));
            
            myRasterizeCircle(canvas, center, radius, Vec3b(0, 0, 255)); 
            clicks.clear();
            imshow("Rasterizer", canvas);
        }
    }
}

int main() {
    // Холст черный
    canvas = Mat(600, 800, CV_8UC3, Scalar(0, 0, 0));

    // Сразу рисуем интерфейс при запуске
    drawInterface();

    namedWindow("Rasterizer", WINDOW_AUTOSIZE);
    setMouseCallback("Rasterizer", mouseCallback, NULL);

    cout << "Started! Look at the window header for controls." << endl;
    imshow("Rasterizer", canvas);

    while (true) {
        char key = (char)waitKey(0);

        if (key == 27) break; // ESC

        // Смена режимов
        if (key == '1') {
            currentMode = 1;
            clicks.clear();
            // Перерисовываем интерфейс, чтобы обновилась надпись Mode
            drawInterface(); 
            imshow("Rasterizer", canvas);
        }
        if (key == '2') {
            currentMode = 2;
            clicks.clear();
            drawInterface();
            imshow("Rasterizer", canvas);
        }

        // Очистка (Clear)
        if (key == 'c' || key == 'C') {
            canvas = Scalar(0, 0, 0); // Заливаем черным
            clicks.clear();
            drawInterface(); // ВАЖНО: Рисуем меню заново после очистки
            imshow("Rasterizer", canvas);
        }

        // Сохранение
        if (key == 's' || key == 'S') {
            string filename = "drawing_" + to_string(rand() % 1000) + ".jpg";
            imwrite(filename, canvas);
            
            // Мигаем зеленой рамкой при сохранении
            rectangle(canvas, Point(0,0), Point(canvas.cols, canvas.rows), Scalar(0,255,0), 5);
            imshow("Rasterizer", canvas);
            waitKey(100);
            
            // Возвращаем как было
            rectangle(canvas, Point(0,0), Point(canvas.cols, canvas.rows), Scalar(0,0,0), 5);
            drawInterface(); // Восстанавливаем меню, если задели рамкой
            imshow("Rasterizer", canvas);
            
            cout << "Saved: " << filename << endl;
        }
    }

    return 0;
}