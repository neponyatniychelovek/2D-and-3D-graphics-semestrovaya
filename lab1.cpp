#include <iostream>
#include <opencv2/opencv.hpp>
#include <windows.h> // Для системных пауз

using namespace std;
using namespace cv;

// Глобальные переменные
int sliderVal = 0;       
const int MAX_VAL = 100;

// Режимы работы
enum Mode {
    ORIGINAL = 1,
    GRAYSCALE = 2,
    SEPIA = 3,
    BLUR = 4
};

Mode currentMode = ORIGINAL;

int main() {
    // Включаем UTF-8 для консоли (чтобы в черном окне писать по-русски можно было)
    SetConsoleOutputCP(65001);

    cout << "=== PHOTO EDITOR 2025 ===" << endl;
    cout << "Enter filename (e.g. mad.jpeg): ";
    
    string imagePath;
    getline(cin, imagePath); 
    if (imagePath.empty()) imagePath = "mad.jpeg"; 

    Mat img = imread(imagePath);

    if (img.empty()) {
        cout << "Error: Image not found!" << endl;
        cout << "Make sure the file '" << imagePath << "' is in the build folder." << endl;
        system("pause");
        return -1;
    }

    // Создаем окно с понятным названием на английском
    namedWindow("Photo Editor", WINDOW_NORMAL | WINDOW_GUI_NORMAL);
    resizeWindow("Photo Editor", 1024, 720);
    
    // Ползунок тоже на английском
    createTrackbar("Effect Power", "Photo Editor", &sliderVal, MAX_VAL);

    Mat resultImg = img.clone();
    Mat tempImg;
    
    // Матрица Сепии
    Mat sepiaKernel = (Mat_<float>(3, 3) <<
        0.272, 0.534, 0.131,
        0.349, 0.686, 0.168,
        0.393, 0.769, 0.189);

    cout << "Launched! Controls:" << endl;
    cout << " [1] Original  [2] B&W  [3] Sepia  [4] Blur" << endl;
    cout << " [S] Save Image  [ESC] Exit" << endl;

    // Переменные для оптимизации (чтобы не лагало)
    int oldSliderVal = -1; 
    Mode oldMode = (Mode)0; 
    string saveMsg = ""; // Сообщение о сохранении
    int msgTimer = 0;    // Таймер, чтобы сообщение исчезало

    while (true) {
        // Проверяем, изменилось ли что-то
        bool changed = (sliderVal != oldSliderVal) || (currentMode != oldMode);

        // Если есть изменения ИЛИ висит сообщение о сохранении (надо перерисовать)
        if (changed || msgTimer > 0) {
            float alpha = sliderVal / 100.0f; 

            // Применяем эффекты
            switch (currentMode) {
            case ORIGINAL:
                resultImg = img.clone();
                break;

            case GRAYSCALE:
                cvtColor(img, tempImg, COLOR_BGR2GRAY);
                cvtColor(tempImg, tempImg, COLOR_GRAY2BGR); 
                addWeighted(tempImg, alpha, img, 1.0 - alpha, 0, resultImg);
                break;

            case SEPIA:
                transform(img, tempImg, sepiaKernel);
                addWeighted(tempImg, alpha, img, 1.0 - alpha, 0, resultImg);
                break;

            case BLUR:
                // Формула для плавного размытия:
                // 1 -> 3 -> 5 -> 7 ...
                int kSize = (int)(sliderVal * 0.5f) * 2 + 1; 
                if (kSize < 1) kSize = 1;
                
                GaussianBlur(img, resultImg, Size(kSize, kSize), 0);
                break;
            }

            // --- ОТРИСОВКА ИНТЕРФЕЙСА ---
            
            // 1. Черная полоска сверху
            rectangle(resultImg, Point(0, 0), Point(resultImg.cols, 40), Scalar(20, 20, 20), -1);

            // 2. Текст режима
            string modeText = "";
            switch (currentMode) {
                case ORIGINAL:  modeText = "Original"; break;
                case GRAYSCALE: modeText = "Black & White"; break;
                case SEPIA:     modeText = "Sepia Filter"; break;
                case BLUR:      modeText = "Gaussian Blur"; break;
            }

            string uiText = "Mode: " + modeText + " | Press [S] to Save";
            
            // Цвет текста (Белый)
            putText(resultImg, uiText, Point(15, 28), FONT_HERSHEY_DUPLEX, 0.7, Scalar(255, 255, 255), 1, LINE_AA);

            // 3. Если было сохранение, показываем зеленую надпись
            if (msgTimer > 0) {
                putText(resultImg, saveMsg, Point(resultImg.cols - 350, 28), FONT_HERSHEY_DUPLEX, 0.7, Scalar(100, 255, 100), 1, LINE_AA);
                msgTimer--; // Уменьшаем таймер
            }

            // Показываем итог
            imshow("Photo Editor", resultImg);

            // Запоминаем состояние
            oldSliderVal = sliderVal;
            oldMode = currentMode;
        }

        // Ждем нажатия (10мс)
        char key = (char)waitKey(10);

        if (key == 27) break; // ESC
        if (key == '1') currentMode = ORIGINAL;
        if (key == '2') currentMode = GRAYSCALE;
        if (key == '3') currentMode = SEPIA;
        if (key == '4') currentMode = BLUR;

        // Сохранение
        if (key == 's' || key == 'S') {
            string outName = "result_" + to_string(rand() % 10000) + ".jpg";
            imwrite(outName, resultImg);
            
            // Показываем сообщение на экране
            saveMsg = "SAVED: " + outName;
            msgTimer = 100; // Показывать надпись примерно 1-2 секунды
            oldSliderVal = -1; // Форсируем перерисовку кадра
            
            cout << "File saved: " << outName << endl;
        }
    }

    destroyAllWindows();
    return 0;
}