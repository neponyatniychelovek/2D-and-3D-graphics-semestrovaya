#include <iostream>
#include <opencv2/opencv.hpp>
#include <windows.h> 

using namespace std;
using namespace cv;

// --- GLOBAL VARIABLES ---
int threshVal = 180;      // Threshold (0-255)
int minAreaVal = 500;     // Min Area to filter noise
const int MAX_THRESH = 255;
const int MAX_AREA = 5000; 

int main() {
    // English interface ensures compatibility on any PC
    cout << "=== OBJECT DETECTOR 2025 ===" << endl;
    cout << "Enter filename (default: coins.jpg): "; // <--- Тут поменяй
    
    string imagePath;
    getline(cin, imagePath);
    if (imagePath.empty()) imagePath = "coins.jpg"; // <--- И тут поменяй

    Mat img = imread(imagePath);
    if (img.empty()) {
        cout << "Error: Image '" << imagePath << "' not found!" << endl;
        cout << "Make sure the file is in the build folder or next to the .exe" << endl;
        system("pause");
        return -1;
    }

    // Window setup
    namedWindow("Detector", WINDOW_NORMAL | WINDOW_GUI_NORMAL);
    resizeWindow("Detector", 1000, 700);

    // --- TRACKBARS ---
    // "Threshold" - sensitivy
    // "Min Area" - filter size
    createTrackbar("Threshold", "Detector", &threshVal, MAX_THRESH);
    createTrackbar("Min Area", "Detector", &minAreaVal, MAX_AREA);

    Mat gray, binary, displayImg;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    cout << "Launched successfully!" << endl;
    cout << "Controls:" << endl;
    cout << "  Adjust sliders to detect objects." << endl;
    cout << "  [S] - Save Image" << endl;
    cout << "  [ESC] - Exit" << endl;

    while (true) {
        // 1. Preprocessing
        cvtColor(img, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(5, 5), 0);

        // 2. Binarization
        // THRESH_BINARY_INV - Inverse because usually we look for dark objects on light background
        threshold(gray, binary, threshVal, 255, THRESH_BINARY_INV);

        // 3. Find Contours
        findContours(binary, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        displayImg = img.clone();
        int objectsFound = 0;

        // 4. Filtering and Drawing
        for (size_t i = 0; i < contours.size(); i++) {
            double area = contourArea(contours[i]);

            // Filter out small noise
            if (area < minAreaVal) {
                continue; 
            }

            // Draw green bounding box
            Rect rect = boundingRect(contours[i]);
            rectangle(displayImg, rect, Scalar(0, 255, 0), 2);
            
            objectsFound++;
        }

        // --- ON-SCREEN INFO ---
        // Black background strip
        rectangle(displayImg, Point(0, 0), Point(350, 40), Scalar(0, 0, 0), -1);
        
        string info = "Found: " + to_string(objectsFound) + " objects";
        putText(displayImg, info, Point(10, 30), FONT_HERSHEY_DUPLEX, 0.8, Scalar(0, 255, 0), 1, LINE_AA);

        // Show windows
        imshow("Detector", displayImg);
        imshow("Mask (Debug)", binary); 

        char key = (char)waitKey(30);
        if (key == 27) break; // ESC
        
        if (key == 's' || key == 'S') {
            string outName = "result_" + to_string(rand() % 1000) + ".jpg";
            imwrite(outName, displayImg);
            cout << "Saved to file: " << outName << endl;
        }
    }

    destroyAllWindows();
    return 0;
}