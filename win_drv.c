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

static COLORREF lv_color_to_colorref(const lv_color_t color);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**********************
 *  STATIC VARIABLES
 **********************/
static HWND hwnd;
static HDC hwnd_dc; /* Window itself */
static HDC target_dc; /* Temporary buffer DC */
static HBITMAP dib; /* Framebuffer bitmap */
static HBITMAP old_bitmap;
static void *fbp = NULL; /* Raw framebuffer memory */

static struct {
    int xres;
    int yres;
    int xoffset;
    int yoffset;
    int bits_per_pixel;
} vinfo;

static const BITMAPINFO dib_info = {
    .bmiHeader = {
        .biSize = sizeof(BITMAPINFOHEADER),
          .biWidth = WINDOW_HOR_RES,
          .biHeight = WINDOW_VER_RES,
          .biPlanes = 1,
          .biBitCount = LV_COLOR_DEPTH,
          .biCompression = BI_RGB,
          .biSizeImage = 0,
          .biXPelsPerMeter = 39 * LV_DPI,
          .biYPelsPerMeter = 39 * LV_DPI,
          .biClrUsed = 0,
          .biClrImportant = 0
    }
};

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
    vinfo.xres = WINDOW_HOR_RES;
    vinfo.yres = WINDOW_VER_RES;
    vinfo.bits_per_pixel = LV_COLOR_DEPTH;
    vinfo.xoffset = vinfo.yoffset = 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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
#if 0
    SetDIBitsToDevice(hwnd_dc, x1, y1, x2 - x1 + 1, y2 - y1 + 1, 0, y2 - y1, 0, y2 - y1 + 1, )
#else
    for(int y = y1; y <= y2; y++)
    {
        for(int x = x1; x <= x2; x++)
        {

            SetPixel(hwnd_dc, x, y, lv_color_to_colorref(*color_p));
            color_p++;
        }
    }
    #endif
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch(msg) {
    case WM_CREATE:
        hwnd_dc = GetDC(hwnd);
        SetTimer(hwnd, 0, 10, (TIMERPROC)lv_task_handler);
        SetTimer(hwnd, 1, 1, NULL);
        lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
        disp_drv.disp_flush = win_drv_flush;
        disp_drv.disp_fill = win_drv_fill;
        disp_drv.disp_map = win_drv_map;
        lv_disp_drv_register(&disp_drv);
        return 0;
    case WM_CLOSE:
        ReleaseDC(hwnd, hwnd_dc);
        hwnd_dc = NULL;
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        //lv_refr_now();
        EndPaint(hwnd, &ps);
        return 0;
    case WM_TIMER:
        lv_tick_inc(1);
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



