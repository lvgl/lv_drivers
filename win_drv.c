/**
 * @file win_drv.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "win_drv.h"
#if 1 // FIXME USE_WINDOWS

#include <windows.h>
#include <windowsx.h>
#include "lvgl/lv_core/lv_vdb.h"
#include "lvgl/lvgl.h"
#include "lvgl/lv_core/lv_refr.h"

#if LV_COLOR_DEPTH < 16
#error Windows driver only supports true RGB colors at this time
#endif

/**********************
 *       DEFINES
 **********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void win_drv_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);
static void win_drv_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
static void win_drv_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p);
static bool win_drv_read(lv_indev_data_t * data);

static COLORREF lv_color_to_colorref(const lv_color_t color);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**********************
 *  STATIC VARIABLES
 **********************/
static HWND hwnd;
static uint32_t *fbp = NULL; /* Raw framebuffer memory */
static bool mouse_pressed;
static int mouse_x, mouse_y;


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
const char g_szClassName[] = "LittlevGL";

void windrv_init(void)
{
    WNDCLASSEX wc;
    RECT winrect;

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    winrect.left = 0;
    winrect.right = WINDOW_HOR_RES - 1;
    winrect.top = 0;
    winrect.bottom = WINDOW_VER_RES - 1;
    AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX), FALSE, WS_EX_CLIENTEDGE);
    OffsetRect(&winrect, -winrect.left, -winrect.top);
    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "The title of my window",
        WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT, winrect.right, winrect.bottom,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.disp_flush = win_drv_flush;
    disp_drv.disp_fill = win_drv_fill;
    disp_drv.disp_map = win_drv_map;
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

 static bool win_drv_read(lv_indev_data_t * data)
{
    data->state = mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    return false;
}

 static void on_paint(void)
 {
    HBITMAP bmp = CreateBitmap(WINDOW_HOR_RES, WINDOW_VER_RES, 1, 32, fbp);
    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmOld = SelectObject(hdcMem, bmp);

    BitBlt(hdc, 0, 0, WINDOW_HOR_RES, WINDOW_VER_RES, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);

    EndPaint(hwnd, &ps);
    DeleteObject(bmp);

}
/**
 * Flush a buffer to the marked area
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 * @param color_p an array of colors
 */
static void win_drv_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    win_drv_map(x1, y1, x2, y2, color_p);
    lv_flush_ready();
}

/**
 * Fill out the marked area with a color
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 * @param color fill color
 */
static void win_drv_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
    #if 0
    HBRUSH brush = CreateSolidBrush(lv_color_to_colorref(color));
    HPEN pen = SelectObject(hwnd_dc, NULL_PEN);
    RECT rect;
    rect.top = y1;
    rect.bottom = y2;
    rect.left = x1;
    rect.right = x2;
    FillRect(hwnd_dc, &rect, brush);
    SelectObject(hwnd_dc, pen);
    DeleteObject(brush);
    #endif
}

/**
 * Put a color map to the marked area
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 * @param color_p an array of colors
 */
static void win_drv_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p)
{
    for(int y = y1; y <= y2; y++)
    {
        for(int x = x1; x <= x2; x++)
        {
            fbp[y*WINDOW_HOR_RES+x] = lv_color_to32(*color_p);
            color_p++;
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch(msg) {
    case WM_CREATE:
        fbp = malloc(4*WINDOW_HOR_RES*WINDOW_VER_RES);
        if(fbp == NULL)
            return 1;
        SetTimer(hwnd, 0, 10, (TIMERPROC)lv_task_handler);
        SetTimer(hwnd, 1, 25, NULL);
        lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
        disp_drv.disp_flush = win_drv_flush;
        disp_drv.disp_fill = win_drv_fill;
        disp_drv.disp_map = win_drv_map;
        lv_disp_drv_register(&disp_drv);
        lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read = win_drv_read;
        lv_indev_drv_register(&indev_drv);
        return 0;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        mouse_x = GET_X_LPARAM(lParam);
        mouse_y = GET_Y_LPARAM(lParam);
        if(msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
            mouse_pressed = (msg == WM_LBUTTONDOWN);
        }
        return 0;
    case WM_CLOSE:
        free(fbp);
        fbp = NULL;
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT:
        on_paint();
        return 0;
    case WM_TIMER:
        lv_tick_inc(25);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
static COLORREF lv_color_to_colorref(const lv_color_t color)
{
    uint32_t raw_color = lv_color_to32(color);
    lv_color32_t tmp;
    tmp.full = raw_color;
    uint32_t colorref = RGB(tmp.red, tmp.green, tmp.blue);
    return colorref;
}
#endif



