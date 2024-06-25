#include <windows.h>
#include <vector>
#include <CommCtrl.h>
#include <commdlg.h>  
#include "resource.h"


enum DrawingMode { NONE, DRAW_POINT, DRAW_LINE, DRAW_POLYLINE, DRAW_POLYGON, DRAW_RECTANGLE };
DrawingMode currentMode = NONE;

int lineWidth = 1;
int pointSize = 5;
COLORREF bgColor = RGB(255, 255, 255);
COLORREF drawColor = RGB(0, 0, 0); 


std::vector<POINT> points;
std::vector<std::vector<POINT>> lines;
std::vector<std::vector<POINT>> polylines;
std::vector<std::vector<POINT>> polygons;
std::vector<std::vector<POINT>> rectangles;

bool isDrawing = false;
std::vector<POINT> currentPoints;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddMenus(HWND);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    WNDCLASS wc = { 0 };
    wc.lpszClassName = TEXT("SketchPad");
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(bgColor);
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(0, IDC_ARROW);

    RegisterClass(&wc);
    CreateWindow(wc.lpszClassName, TEXT("SketchPad"),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 800, 600, 0, 0, hInstance, 0);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

void AddMenus(HWND hwnd) {
    HMENU hMenubar = CreateMenu();
    HMENU hMenu = CreateMenu();

    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, TEXT("Drawing"));
    AppendMenu(hMenu, MF_STRING, IDM_DRAW_POINT, TEXT("Point"));
    AppendMenu(hMenu, MF_STRING, IDM_DRAW_LINE, TEXT("Line"));
    AppendMenu(hMenu, MF_STRING, IDM_DRAW_POLYLINE, TEXT("Polyline"));
    AppendMenu(hMenu, MF_STRING, IDM_DRAW_POLYGON, TEXT("Polygon"));
    AppendMenu(hMenu, MF_STRING, IDM_DRAW_RECTANGLE, TEXT("Rectangle"));

    hMenu = CreateMenu();
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, TEXT("Line Width"));
    AppendMenu(hMenu, MF_STRING, IDM_INCREASE_WIDTH, TEXT("Increase"));
    AppendMenu(hMenu, MF_STRING, IDM_DECREASE_WIDTH, TEXT("Decrease"));

    hMenu = CreateMenu();
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, TEXT("Point Size"));
    AppendMenu(hMenu, MF_STRING, IDM_INCREASE_SIZE, TEXT("Increase"));
    AppendMenu(hMenu, MF_STRING, IDM_DECREASE_SIZE, TEXT("Decrease"));

    hMenu = CreateMenu();
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, TEXT("Colors"));
    AppendMenu(hMenu, MF_STRING, IDM_SET_BG_COLOR, TEXT("Set Background Color"));
    AppendMenu(hMenu, MF_STRING, IDM_SET_DRAW_COLOR, TEXT("Set Drawing Color"));

    hMenu = CreateMenu();
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, TEXT("Erase"));
    AppendMenu(hMenu, MF_STRING, IDM_ERASE_LAST, TEXT("Erase Last"));
    AppendMenu(hMenu, MF_STRING, IDM_CLEAR_ALL, TEXT("Clear All"));

    SetMenu(hwnd, hMenubar);
}

void setDrawColor(HWND hwnd) {
    CHOOSECOLOR cc;
    static COLORREF acrCustClr[16];
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = drawColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc) == TRUE) {
        drawColor = cc.rgbResult;
    }
}

void setBgColor(HWND hwnd) {
    CHOOSECOLOR cc;
    static COLORREF acrCustClr[16];
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = bgColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc) == TRUE) {
        bgColor = cc.rgbResult;
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void drawPoints(HDC hdc) {
    HBRUSH brush = CreateSolidBrush(drawColor);
    SelectObject(hdc, brush);

    for (const auto& point : points) {
        Ellipse(hdc, point.x - pointSize, point.y - pointSize, point.x + pointSize, point.y + pointSize);
    }

    DeleteObject(brush);
}

void drawLines(HDC hdc) {
    HPEN pen = CreatePen(PS_SOLID, lineWidth, drawColor);
    SelectObject(hdc, pen);

    for (const auto& line : lines) {
        MoveToEx(hdc, line[0].x, line[0].y, NULL);
        LineTo(hdc, line[1].x, line[1].y);
    }

    DeleteObject(pen);
}

void drawPolylines(HDC hdc) {
    HPEN pen = CreatePen(PS_SOLID, lineWidth, drawColor);
    SelectObject(hdc, pen);

    for (const auto& polyline : polylines) {
        if (polyline.size() < 2) continue;
        MoveToEx(hdc, polyline[0].x, polyline[0].y, NULL);
        for (size_t i = 1; i < polyline.size(); ++i) {
            LineTo(hdc, polyline[i].x, polyline[i].y);
        }
    }

    DeleteObject(pen);
}

void drawPolygons(HDC hdc) {
    HBRUSH brush = CreateSolidBrush(drawColor);
    SelectObject(hdc, brush);

    for (const auto& polygon : polygons) {
        if (polygon.size() < 3) continue;
        POINT* pointsArray = new POINT[polygon.size()];
        for (size_t i = 0; i < polygon.size(); ++i) {
            pointsArray[i] = polygon[i];
        }
        Polygon(hdc, pointsArray, polygon.size());
        delete[] pointsArray;
    }

    DeleteObject(brush);
}

void drawRectangles(HDC hdc) {
    HPEN pen = CreatePen(PS_SOLID, lineWidth, drawColor);
    HBRUSH brush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, pen);
    SelectObject(hdc, brush);

    for (const auto& rectangle : rectangles) {
        Rectangle(hdc, rectangle[0].x, rectangle[0].y, rectangle[1].x, rectangle[1].y);
    }

    DeleteObject(pen);
}

void eraseLast() {
    switch (currentMode) {
    case DRAW_POINT:
        if (!points.empty()) points.pop_back();
        break;
    case DRAW_LINE:
        if (!lines.empty()) lines.pop_back();
        break;
    case DRAW_POLYLINE:
        if (!polylines.empty()) polylines.pop_back();
        break;
    case DRAW_POLYGON:
        if (!polygons.empty()) polygons.pop_back();
        break;
    case DRAW_RECTANGLE:
        if (!rectangles.empty()) rectangles.pop_back();
        break;
    default:
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        AddMenus(hwnd);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_DRAW_POINT:
            currentMode = DRAW_POINT;
            break;
        case IDM_DRAW_LINE:
            currentMode = DRAW_LINE;
            break;
        case IDM_DRAW_POLYLINE:
            currentMode = DRAW_POLYLINE;
            break;
        case IDM_DRAW_POLYGON:
            currentMode = DRAW_POLYGON;
            break;
        case IDM_DRAW_RECTANGLE:
            currentMode = DRAW_RECTANGLE;
            break;
        case IDM_INCREASE_WIDTH:
            lineWidth++;
            break;
        case IDM_DECREASE_WIDTH:
            if (lineWidth > 1) lineWidth--;
            break;
        case IDM_INCREASE_SIZE:
            pointSize++;
            break;
        case IDM_DECREASE_SIZE:
            if (pointSize > 1) pointSize--;
            break;
        case IDM_SET_BG_COLOR:
            setBgColor(hwnd);
            break;
        case IDM_SET_DRAW_COLOR:
            setDrawColor(hwnd);
            break;
        case IDM_ERASE_LAST:
            eraseLast();
            break;
        case IDM_CLEAR_ALL:
            points.clear();
            lines.clear();
            polylines.clear();
            polygons.clear();
            rectangles.clear();
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    case WM_LBUTTONDOWN:
        isDrawing = true;
        currentPoints.push_back({ LOWORD(lParam), HIWORD(lParam) });
        if (currentMode == DRAW_POINT) {
            points.push_back({ LOWORD(lParam), HIWORD(lParam) });
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_MOUSEMOVE:
        if (isDrawing) {
            if (currentMode == DRAW_LINE || currentMode == DRAW_RECTANGLE) {
                if (currentPoints.size() == 1) {
                    currentPoints.push_back({ LOWORD(lParam), HIWORD(lParam) });
                }
                else {
                    currentPoints[1] = { LOWORD(lParam), HIWORD(lParam) };
                }
            }
            else if (currentMode == DRAW_POLYLINE || currentMode == DRAW_POLYGON) {
                currentPoints.push_back({ LOWORD(lParam), HIWORD(lParam) });
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONUP:
        isDrawing = false;
        switch (currentMode) {
        case DRAW_LINE:
            if (currentPoints.size() == 2) {
                lines.push_back(currentPoints);
            }
            break;
        case DRAW_POLYLINE:
            if (currentPoints.size() > 1) {
                polylines.push_back(currentPoints);
            }
            break;
        case DRAW_POLYGON:
            if (currentPoints.size() > 2) {
                polygons.push_back(currentPoints);
            }
            break;
        case DRAW_RECTANGLE:
            if (currentPoints.size() == 2) {
                rectangles.push_back(currentPoints);
            }
            break;
        default:
            break;
        }
        currentPoints.clear();
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Create a memory device context for double buffering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
        SelectObject(memDC, memBitmap);

        // Fill the background
        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        FillRect(memDC, &ps.rcPaint, bgBrush);
        DeleteObject(bgBrush);

        // Draw existing shapes
        drawPoints(memDC);
        drawLines(memDC);
        drawPolylines(memDC);
        drawPolygons(memDC);
        drawRectangles(memDC);

        // Draw current shape
        if (isDrawing && !currentPoints.empty()) {
            HPEN pen = CreatePen(PS_SOLID, lineWidth, RGB(255, 0, 0));
            SelectObject(memDC, pen);

            switch (currentMode) {
            case DRAW_LINE:
                if (currentPoints.size() == 2) {
                    MoveToEx(memDC, currentPoints[0].x, currentPoints[0].y, NULL);
                    LineTo(memDC, currentPoints[1].x, currentPoints[1].y);
                }
                break;
            case DRAW_POLYLINE:
                if (currentPoints.size() > 1) {
                    MoveToEx(memDC, currentPoints[0].x, currentPoints[0].y, NULL);
                    for (size_t i = 1; i < currentPoints.size(); ++i) {
                        LineTo(memDC, currentPoints[i].x, currentPoints[i].y);
                    }
                }
                break;
            case DRAW_POLYGON:
                if (currentPoints.size() > 2) {
                    POINT* pointsArray = new POINT[currentPoints.size()];
                    for (size_t i = 0; i < currentPoints.size(); ++i) {
                        pointsArray[i] = currentPoints[i];
                    }
                    Polygon(memDC, pointsArray, currentPoints.size());
                    delete[] pointsArray;
                }
                break;
            case DRAW_RECTANGLE:
                if (currentPoints.size() == 2) {
                    Rectangle(memDC, currentPoints[0].x, currentPoints[0].y, currentPoints[1].x, currentPoints[1].y);
                }
                break;
            default:
                break;
            }

            DeleteObject(pen);
        }

       
        BitBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, memDC, 0, 0, SRCCOPY);

        
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
    }
    break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
