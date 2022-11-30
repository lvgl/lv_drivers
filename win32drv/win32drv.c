/**
 * @file win32drv.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "win32drv.h"

#if USE_WIN32DRV

#include <windowsx.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

#define WINDOW_EX_STYLE WS_EX_CLIENTEDGE

#define WINDOW_STYLE \
    (WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME))

#ifndef WIN32DRV_MONITOR_ZOOM
#define WIN32DRV_MONITOR_ZOOM 1
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _window_thread_params_t {
    wchar_t                     screen_title[64];
    HANDLE                      window_mutex;
    HINSTANCE                   instance_handle;
    HICON                       icon_handle;
    lv_coord_t                  hor_res;
    lv_coord_t                  ver_res;
    int                         show_window_mode;
} window_thread_params_t;

typedef struct _lv_win32_window_info_t {
    // Disp params
    lv_disp_t*                  display;
    HWND                        window_handle;
    HDC                         buffer_dc_handle;
    UINT32*                     pixel_buffer;
    SIZE_T                      pixel_buffer_size;
    bool volatile               display_refreshing;
    lv_color_t*                 malloc_pixel_buffer;

    // LV Structs
    lv_disp_draw_buf_t          display_draw_buf;
    lv_disp_drv_t               display_driver;
    lv_indev_drv_t              pointer_driver;
    lv_indev_drv_t              keypad_driver;
    lv_indev_drv_t              encoder_driver;

    // Thread params
    window_thread_params_t*     temporary_thread_params;

    // Drivers data
    bool     volatile           mouse_pressed;
    LPARAM   volatile           mouse_value;
    bool     volatile           mousewheel_pressed;
    int16_t  volatile           mousewheel_value;
    bool     volatile           keyboard_pressed;
    WPARAM   volatile           keyboard_wparam;
    LPARAM   volatile           keyboard_lparam;
} lv_win32_window_info_t;

typedef struct _displays_info_t {
    lv_win32_window_info_t  data[WIN32DRV_MAX_DISPLAYS];
    uint8_t                 counter;
} displays_info_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * @brief Creates a B8G8R8A8 frame buffer.
 * @param WindowHandle A handle to the window for the creation of the frame
 *                     buffer. If this value is NULL, the entire screen will be
 *                     referenced.
 * @param Width The width of the frame buffer.
 * @param Height The height of the frame buffer.
 * @param PixelBuffer The raw pixel buffer of the frame buffer you created.
 * @param PixelBufferSize The size of the frame buffer you created.
 * @return If the function succeeds, the return value is a handle to the device
 *         context (DC) for the frame buffer. If the function fails, the return
 *         value is NULL, and PixelBuffer parameter is NULL.
*/
static HDC lv_win32_create_frame_buffer(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_ UINT32** PixelBuffer,
    _Out_ SIZE_T* PixelBufferSize);

/**
 * @brief Enables WM_DPICHANGED message for child window for the associated
 *        window.
 * @param WindowHandle The window you want to enable WM_DPICHANGED message for
 *                     child window.
 * @return If the function succeeds, the return value is non-zero. If the
 *         function fails, the return value is zero.
 * @remarks You need to use this function in Windows 10 Threshold 1 or Windows
 *          10 Threshold 2.
*/
static BOOL lv_win32_enable_child_window_dpi_message(_In_ HWND WindowHandle);

/**
 * @brief Registers a window as being touch-capable.
 * @param hWnd The handle of the window being registered.
 * @param ulFlags A set of bit flags that specify optional modifications.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see RegisterTouchWindow.
*/
static BOOL lv_win32_register_touch_window(HWND hWnd,ULONG ulFlags);

/**
 * @brief Retrieves detailed information about touch inputs associated with a
 *        particular touch input handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @param cInputs The number of structures in the pInputs array.
 * @param pInputs A pointer to an array of TOUCHINPUT structures to receive
 *                information about the touch points associated with the
 *                specified touch input handle.
 * @param cbSize The size, in bytes, of a single TOUCHINPUT structure.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see GetTouchInputInfo.
*/
static BOOL lv_win32_get_touch_input_info(HTOUCHINPUT hTouchInput,UINT cInputs,PTOUCHINPUT pInputs,int cbSize);

/**
 * @brief Closes a touch input handle, frees process memory associated with it,
          and invalidates the handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see CloseTouchInputHandle.
*/
static BOOL lv_win32_close_touch_input_handle(HTOUCHINPUT hTouchInput);

/**
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
static UINT lv_win32_get_dpi_for_window(_In_ HWND WindowHandle);
static void lv_win32_display_driver_flush_callback(lv_disp_drv_t* disp_drv,const lv_area_t* area,lv_color_t* color_p);
static void lv_win32_display_refresh_handler(lv_timer_t* param);
static void lv_win32_pointer_driver_read_callback(lv_indev_drv_t* indev_drv,lv_indev_data_t* data);
static void lv_win32_keypad_driver_read_callback(lv_indev_drv_t* indev_drv,lv_indev_data_t* data);
static void lv_win32_encoder_driver_read_callback(lv_indev_drv_t* indev_drv,lv_indev_data_t* data);

static LRESULT CALLBACK lv_win32_window_message_callback(HWND   hWnd,UINT   uMsg,WPARAM wParam,LPARAM lParam);
static unsigned int __stdcall lv_win32_window_thread_entrypoint(void* raw_parameter);

/**********************
 *  GLOBAL VARIABLES
 **********************/

EXTERN_C bool lv_win32_quit_signal                  = false;

/**********************
 *  STATIC VARIABLES
 **********************/
static displays_info_t  s_disp_info = { NULL, 0 };
static int volatile     s_dpi_value = USER_DEFAULT_SCREEN_DPI;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_disp_t* lv_win32_create_disp(lv_coord_t hor_res, lv_coord_t ver_res, const wchar_t* screen_title, HICON icon_handle)
{
    // Check if there still space for another display
    if ( s_disp_info.counter >= WIN32DRV_MAX_DISPLAYS ) {
        puts("Failed to create display, please increase `WIN32DRV_MAX_DISPLAYS`!");
        return NULL;
    }
    lv_win32_window_info_t* lv_win32_window_info = &s_disp_info.data[s_disp_info.counter];
    memset(lv_win32_window_info, 0, sizeof(lv_win32_window_info_t));

    // Thread params config
    window_thread_params_t thread_params = { 0 };
    wcscpy(thread_params.screen_title, screen_title ? screen_title : L"LVGL Simulator for Windows Desktop");
    thread_params.window_mutex      = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    thread_params.instance_handle   = GetModuleHandleW(NULL);
    thread_params.icon_handle       = icon_handle;
    thread_params.hor_res           = hor_res;
    thread_params.ver_res           = ver_res;
    thread_params.show_window_mode  = SW_SHOW;

    // Wait window creation, timeout after one minute
    if (!thread_params.window_mutex) {return NULL;}
    lv_win32_window_info->temporary_thread_params = &thread_params;
    _beginthreadex(NULL, 0, lv_win32_window_thread_entrypoint, lv_win32_window_info, 0, NULL);
    DWORD waitStatus = WaitForSingleObjectEx(thread_params.window_mutex, 60000U, FALSE);
    switch (waitStatus) {
        case WAIT_OBJECT_0:
            break;
        case WAIT_IO_COMPLETION:
        case WAIT_ABANDONED:
        case WAIT_TIMEOUT:
        case WAIT_FAILED:
        default:
            printf("Failed to create display %d!\r\n", s_disp_info.counter + 1);
            return NULL;
    }

    // Setup display buffer
#if (LV_COLOR_DEPTH == 32) || \
    (LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0) || \
    (LV_COLOR_DEPTH == 8) || \
    (LV_COLOR_DEPTH == 1)
    lv_disp_draw_buf_init(
        &lv_win32_window_info->display_draw_buf,
        (lv_color_t*)lv_win32_window_info->pixel_buffer,
        NULL,
        hor_res * ver_res);
#else
    lv_win32_window_info->malloc_pixel_buffer = malloc(hor_res * ver_res * sizeof(lv_color_t));
    lv_disp_draw_buf_init(
        &lv_win32_window_info->display_draw_buf,
        lv_win32_window_info->malloc_pixel_buffer,
        NULL,
        hor_res * ver_res);
#endif

    // Setup display driver
    lv_disp_drv_init(&lv_win32_window_info->display_driver);
    lv_win32_window_info->display_driver.hor_res      = hor_res;
    lv_win32_window_info->display_driver.ver_res      = ver_res;
    lv_win32_window_info->display_driver.flush_cb     = lv_win32_display_driver_flush_callback;
    lv_win32_window_info->display_driver.draw_buf     = &lv_win32_window_info->display_draw_buf;
    lv_win32_window_info->display_driver.direct_mode  = 1;
    lv_win32_window_info->display                     = lv_disp_drv_register(&lv_win32_window_info->display_driver);

    // Single timer refreshes all displays with vsync
    lv_timer_del(lv_win32_window_info->display->refr_timer);
    lv_win32_window_info->display->refr_timer = NULL;
    if (s_disp_info.counter == 0) {
        // TODO LV_DISP_DEF_REFR_PERIOD must be respected, but vsync requirement forces to skip a call
        lv_timer_create(lv_win32_display_refresh_handler, 8, NULL);
    }

    // Initialize display input drivers
    // All displays share the same hw-driver (keyboard, mouse, mousewheel)
    {
        // Mouse move and left-click
        lv_indev_drv_t* pointer_driver = &lv_win32_window_info->pointer_driver;
        lv_indev_drv_init(pointer_driver);
        pointer_driver->type        = LV_INDEV_TYPE_POINTER;
        pointer_driver->read_cb     = lv_win32_pointer_driver_read_callback;
        pointer_driver->disp        = lv_win32_window_info->display;
        lv_indev_t* pointer_device  = lv_indev_drv_register(pointer_driver);

        // Keyboard
        lv_indev_drv_t* keypad_driver = &lv_win32_window_info->keypad_driver;
        lv_indev_drv_init(keypad_driver);
        keypad_driver->type         = LV_INDEV_TYPE_KEYPAD;
        keypad_driver->read_cb      = lv_win32_keypad_driver_read_callback;
        keypad_driver->disp         = lv_win32_window_info->display;
        lv_indev_t* keypad_device = lv_indev_drv_register(keypad_driver);

        // Mouse-wheel and click
        lv_indev_drv_t* encoder_driver = &lv_win32_window_info->encoder_driver;
        lv_indev_drv_init(encoder_driver);
        encoder_driver->type        = LV_INDEV_TYPE_ENCODER;
        encoder_driver->read_cb     = lv_win32_encoder_driver_read_callback;
        encoder_driver->disp        = lv_win32_window_info->display;
        lv_indev_t* encoder_device = lv_indev_drv_register(encoder_driver);

        // At display input devices to the default group
        lv_group_t* group = lv_group_get_default();
        if (!group) {
            group = lv_group_create();
            lv_group_set_default(group); // Currently in LVGL all displays share the same default group
        }
        lv_indev_set_group(pointer_device, group);
        lv_indev_set_group(keypad_device, group);
        lv_indev_set_group(encoder_device, group);
    }

    s_disp_info.counter++;                  // Finnaly increase the display counter
    return lv_win32_window_info->display;   // Return lv_disp_t*
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static HDC lv_win32_create_frame_buffer(
    HWND WindowHandle,
    LONG Width,
    LONG Height,
    UINT32** PixelBuffer,
    SIZE_T* PixelBufferSize)
{
    HDC hFrameBufferDC = NULL;

    HDC hWindowDC = GetDC(WindowHandle);
    if (hWindowDC)
    {
        hFrameBufferDC = CreateCompatibleDC(hWindowDC);
        ReleaseDC(WindowHandle, hWindowDC);
    }

    if (hFrameBufferDC)
    {
        #if LV_COLOR_DEPTH == 32
            BITMAPINFO BitmapInfo = { 0 };
        #elif LV_COLOR_DEPTH == 16
            typedef struct _BITMAPINFO_16BPP {
                BITMAPINFOHEADER bmiHeader;
                DWORD bmiColorMask[3];
            } BITMAPINFO_16BPP, *PBITMAPINFO_16BPP;

            BITMAPINFO_16BPP BitmapInfo = { 0 };
        #elif LV_COLOR_DEPTH == 8
            typedef struct _BITMAPINFO_8BPP {
                BITMAPINFOHEADER bmiHeader;
                RGBQUAD bmiColors[256];
            } BITMAPINFO_8BPP, *PBITMAPINFO_8BPP;

            BITMAPINFO_8BPP BitmapInfo = { 0 };
        #elif LV_COLOR_DEPTH == 1
            typedef struct _BITMAPINFO_1BPP {
                BITMAPINFOHEADER bmiHeader;
                RGBQUAD bmiColors[2];
            } BITMAPINFO_1BPP, *PBITMAPINFO_1BPP;

            BITMAPINFO_1BPP BitmapInfo = { 0 };
        #else
            BITMAPINFO BitmapInfo = { 0 };
        #endif

        BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth = Width;
        BitmapInfo.bmiHeader.biHeight = -Height;
        BitmapInfo.bmiHeader.biPlanes = 1;
        #if LV_COLOR_DEPTH == 32
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
        #elif LV_COLOR_DEPTH == 16
            BitmapInfo.bmiHeader.biBitCount = 16;
            BitmapInfo.bmiHeader.biCompression = BI_BITFIELDS;
            BitmapInfo.bmiColorMask[0] = 0xF800;
            BitmapInfo.bmiColorMask[1] = 0x07E0;
            BitmapInfo.bmiColorMask[2] = 0x001F;
        #elif LV_COLOR_DEPTH == 8
            BitmapInfo.bmiHeader.biBitCount = 8;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
            for (size_t i = 0; i < 256; ++i)
            {
                lv_color8_t color;
                color.full = i;

                BitmapInfo.bmiColors[i].rgbRed = LV_COLOR_GET_R(color) * 36;
                BitmapInfo.bmiColors[i].rgbGreen = LV_COLOR_GET_G(color) * 36;
                BitmapInfo.bmiColors[i].rgbBlue = LV_COLOR_GET_B(color) * 85;
                BitmapInfo.bmiColors[i].rgbReserved = 0xFF;
            }
        #elif LV_COLOR_DEPTH == 1
            BitmapInfo.bmiHeader.biBitCount = 8;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
            BitmapInfo.bmiHeader.biClrUsed = 2;
            BitmapInfo.bmiHeader.biClrImportant = 2;
            BitmapInfo.bmiColors[0].rgbRed = 0x00;
            BitmapInfo.bmiColors[0].rgbGreen = 0x00;
            BitmapInfo.bmiColors[0].rgbBlue = 0x00;
            BitmapInfo.bmiColors[0].rgbReserved = 0xFF;
            BitmapInfo.bmiColors[1].rgbRed = 0xFF;
            BitmapInfo.bmiColors[1].rgbGreen = 0xFF;
            BitmapInfo.bmiColors[1].rgbBlue = 0xFF;
            BitmapInfo.bmiColors[1].rgbReserved = 0xFF;
        #else
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
        #endif

        HBITMAP hBitmap = CreateDIBSection(
            hFrameBufferDC,
            (PBITMAPINFO)(&BitmapInfo),
            DIB_RGB_COLORS,
            (void**)PixelBuffer,
            NULL,
            0);
        if (hBitmap)
        {
            #if LV_COLOR_DEPTH == 32
                *PixelBufferSize = Width * Height * sizeof(UINT32);
            #elif LV_COLOR_DEPTH == 16
                *PixelBufferSize = Width * Height * sizeof(UINT16);
            #elif LV_COLOR_DEPTH == 8
                *PixelBufferSize = Width * Height * sizeof(UINT8);
            #elif LV_COLOR_DEPTH == 1
                *PixelBufferSize = Width * Height * sizeof(UINT8);
            #else
                *PixelBufferSize = Width * Height * sizeof(UINT32);
            #endif

            DeleteObject(SelectObject(hFrameBufferDC, hBitmap));
            DeleteObject(hBitmap);
        }
        else
        {
            DeleteDC(hFrameBufferDC);
            hFrameBufferDC = NULL;
            *PixelBuffer   = NULL;
        }
    }

    return hFrameBufferDC;
}

static BOOL lv_win32_enable_child_window_dpi_message(
    HWND WindowHandle)
{
    // This hack is only for Windows 10 TH1/TH2 only.
    // We don't need this hack if the Per Monitor Aware V2 is existed.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwMajorVersion = 10;
    OSVersionInfoEx.dwMinorVersion = 0;
    OSVersionInfoEx.dwBuildNumber = 14393;
    if (!VerifyVersionInfoW(
        &OSVersionInfoEx,
        VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
        VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(
                    0,
                    VER_MAJORVERSION,
                    VER_GREATER_EQUAL),
                VER_MINORVERSION,
                VER_GREATER_EQUAL),
            VER_BUILDNUMBER,
            VER_LESS)))
    {
        return FALSE;
    }

    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, BOOL);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "EnableChildWindowDpiMessage"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(WindowHandle, TRUE);
}

static BOOL lv_win32_register_touch_window(
    HWND hWnd,
    ULONG ulFlags)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, ULONG);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "RegisterTouchWindow"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hWnd, ulFlags);
}

static BOOL lv_win32_get_touch_input_info(
    HTOUCHINPUT hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "GetTouchInputInfo"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput, cInputs, pInputs, cbSize);
}

static BOOL lv_win32_close_touch_input_handle(
    HTOUCHINPUT hTouchInput)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HTOUCHINPUT);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "CloseTouchInputHandle"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput);
}

static UINT lv_win32_get_dpi_for_window(
    _In_ HWND WindowHandle)
{
    UINT Result = (UINT)(-1);

    HMODULE ModuleHandle = LoadLibraryW(L"SHCore.dll");
    if (ModuleHandle)
    {
        typedef enum MONITOR_DPI_TYPE_PRIVATE {
            MDT_EFFECTIVE_DPI = 0,
            MDT_ANGULAR_DPI = 1,
            MDT_RAW_DPI = 2,
            MDT_DEFAULT = MDT_EFFECTIVE_DPI
        } MONITOR_DPI_TYPE_PRIVATE;

        typedef HRESULT(WINAPI* FunctionType)(
            HMONITOR, MONITOR_DPI_TYPE_PRIVATE, UINT*, UINT*);

        FunctionType pFunction = (FunctionType)(
            GetProcAddress(ModuleHandle, "GetDpiForMonitor"));
        if (pFunction)
        {
            HMONITOR MonitorHandle = MonitorFromWindow(
                WindowHandle,
                MONITOR_DEFAULTTONEAREST);

            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(pFunction(
                MonitorHandle,
                MDT_EFFECTIVE_DPI,
                &dpiX,
                &dpiY)))
            {
                Result = dpiX;
            }
        }

        FreeLibrary(ModuleHandle);
    }

    if (Result == (UINT)(-1))
    {
        HDC hWindowDC = GetDC(WindowHandle);
        if (hWindowDC)
        {
            Result = GetDeviceCaps(hWindowDC, LOGPIXELSX);
            ReleaseDC(WindowHandle, hWindowDC);
        }
    }

    if (Result == (UINT)(-1))
    {
        Result = USER_DEFAULT_SCREEN_DPI;
    }

    return Result;
}

static void lv_win32_display_driver_flush_callback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p)
{
    // Find data
    lv_win32_window_info_t* lv_win32_window_info = NULL;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        if (disp_drv && disp_drv == &s_disp_info.data[i].display_driver) {
            lv_win32_window_info = &s_disp_info.data[i];
            break;
        }
    }
    if (!lv_win32_window_info) {
        return;
    }
    

    if (lv_disp_flush_is_last(disp_drv))
    {
#if (LV_COLOR_DEPTH == 32) || \
    (LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0) || \
    (LV_COLOR_DEPTH == 8) || \
    (LV_COLOR_DEPTH == 1)
        UNREFERENCED_PARAMETER(color_p);
#elif (LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0)
        SIZE_T count        = lv_win32_window_info->pixel_buffer_size / sizeof(UINT16);
        PUINT16 source      = (PUINT16)color_p;
        PUINT16 destination = (PUINT16)lv_win32_window_info->pixel_buffer;
        for (SIZE_T i = 0; i < count; ++i)
        {
            UINT16 current = *source;
            *destination = (LOBYTE(current) << 8) | HIBYTE(current);

            ++source;
            ++destination;
        }
#else
        for (int y = area->y1; y <= area->y2; ++y)
        {
            for (int x = area->x1; x <= area->x2; ++x)
            {
                lv_win32_window_info->pixel_buffer[y * disp_drv->hor_res + x] = lv_color_to32(*color_p);
                color_p++;
            }
        }
#endif
        InvalidateRect(lv_win32_window_info->window_handle, NULL, FALSE);
    }

    lv_disp_flush_ready(disp_drv);
}

static void lv_win32_display_refresh_handler(lv_timer_t* timer) {
    lv_disp_t* act_default = lv_disp_get_default();
    if (act_default) {
        for (uint8_t i = 0; i < s_disp_info.counter; i++) {
            // check if this display buffer is ready
            if (s_disp_info.data[i].display_refreshing) { continue; }
            lv_disp_set_default(s_disp_info.data[i].display);
            _lv_disp_refr_timer(NULL);
        }
        lv_disp_set_default(act_default);
    }
}

static void lv_win32_pointer_driver_read_callback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    // Find lv_win32_window_info
    lv_win32_window_info_t* lv_win32_window_info = NULL;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        if (indev_drv == &s_disp_info.data[i].pointer_driver) {
            lv_win32_window_info = &s_disp_info.data[i];
            break;
        }
    }
    if (!lv_win32_window_info) {
        return;
    }
    
    data->state   = (lv_indev_state_t)(lv_win32_window_info->mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->point.x = MulDiv(GET_X_LPARAM(lv_win32_window_info->mouse_value), USER_DEFAULT_SCREEN_DPI, WIN32DRV_MONITOR_ZOOM * s_dpi_value);
    data->point.y = MulDiv(GET_Y_LPARAM(lv_win32_window_info->mouse_value), USER_DEFAULT_SCREEN_DPI, WIN32DRV_MONITOR_ZOOM * s_dpi_value);

    if (data->point.x < 0) { data->point.x = 0; }
    if (data->point.y < 0) { data->point.y = 0; }

    lv_disp_t* disp = indev_drv->disp;
    if (data->point.x > disp->driver->hor_res - 1) { data->point.x = disp->driver->hor_res - 1; }
    if (data->point.y > disp->driver->ver_res - 1) { data->point.y = disp->driver->ver_res - 1; }
}

static void lv_win32_keypad_driver_read_callback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    // Find lv_win32_window_info
    lv_win32_window_info_t* lv_win32_window_info = NULL;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        if (indev_drv == &s_disp_info.data[i].keypad_driver) {
            lv_win32_window_info = &s_disp_info.data[i];
            break;
        }
    }
    if (!lv_win32_window_info) {
        return;
    }

    data->state = (lv_indev_state_t)(lv_win32_window_info->keyboard_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    WPARAM KeyboardValue = lv_win32_window_info->keyboard_wparam;

    switch (KeyboardValue)
    {
        case VK_UP:     data->key = LV_KEY_UP; break;
        case VK_DOWN:   data->key = LV_KEY_DOWN; break;
        case VK_LEFT:   data->key = LV_KEY_LEFT; break;
        case VK_RIGHT:  data->key = LV_KEY_RIGHT; break;
        case VK_ESCAPE: data->key = LV_KEY_ESC; break;
        case VK_DELETE: data->key = LV_KEY_DEL; break;
        case VK_BACK:   data->key = LV_KEY_BACKSPACE; break;
        case VK_RETURN: data->key = LV_KEY_ENTER; break;
        case VK_NEXT:   data->key = LV_KEY_NEXT; break;
        case VK_PRIOR:  data->key = LV_KEY_PREV; break;
        case VK_HOME:   data->key = LV_KEY_HOME; break;
        case VK_END:    data->key = LV_KEY_END; break;
        default:
        {
            // Get current full keyboard state
            BYTE keyState[256];
            if (!GetKeyboardState(keyState)) { return; }
            // Modifier keys state need to be get individually
            keyState[VK_CONTROL]    = (BYTE)GetKeyState(VK_CONTROL);
            keyState[VK_SHIFT]      = (BYTE)GetKeyState(VK_SHIFT);
            keyState[VK_MENU]       = (BYTE)GetKeyState(VK_MENU);
            keyState[VK_LCONTROL]   = (BYTE)GetKeyState(VK_LCONTROL);
            keyState[VK_RCONTROL]   = (BYTE)GetKeyState(VK_RCONTROL);
            keyState[VK_LSHIFT]     = (BYTE)GetKeyState(VK_LSHIFT);
            keyState[VK_RSHIFT]     = (BYTE)GetKeyState(VK_RSHIFT);
            keyState[VK_LMENU]      = (BYTE)GetKeyState(VK_LMENU);
            keyState[VK_RMENU]      = (BYTE)GetKeyState(VK_RMENU);
            UINT scanCode = (lv_win32_window_info->keyboard_lparam >> 16) & 0xFF;

            // Translate current keyboard state to a char
            uint32_t translatedKeyValue = 0;
            void* bufTranslated = &translatedKeyValue;
            #if 1
            ToUnicode(KeyboardValue, scanCode, keyState, (LPWSTR)bufTranslated, 2, 0);
            #else
            ToAscii(KeyboardValue, scanCode, keyState, (LPWORD)bufTranslated, 0);
            #endif
            data->key = translatedKeyValue;
            break;
        }
    }
}

static void lv_win32_encoder_driver_read_callback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    // Find lv_win32_window_info
    lv_win32_window_info_t* lv_win32_window_info = NULL;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        if (indev_drv == &s_disp_info.data[i].encoder_driver) {
            lv_win32_window_info = &s_disp_info.data[i];
            break;
        }
    }
    if (!lv_win32_window_info) {
        return;
    }

    data->state     = (lv_indev_state_t)(lv_win32_window_info->mousewheel_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->enc_diff  = lv_win32_window_info->mousewheel_value;
    lv_win32_window_info->mousewheel_value = 0;
}

static LRESULT CALLBACK lv_win32_window_message_callback(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    // Find display information
    lv_win32_window_info_t* lv_win32_window_info = NULL;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        if (s_disp_info.data[i].window_handle == hWnd) {
            lv_win32_window_info = &s_disp_info.data[i];
            break;
        }
    }

    // Handle sitations where it is valid not to have found the display info
    if (!lv_win32_window_info) {
        if (uMsg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    // Current window is active
    bool windowIsActive = GetForegroundWindow() == hWnd;

    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        lv_win32_window_info->mouse_value = lParam;
        if      (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) { lv_win32_window_info->mouse_pressed      = (uMsg == WM_LBUTTONDOWN); }
        else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) { lv_win32_window_info->mousewheel_pressed = (uMsg == WM_MBUTTONDOWN); }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        lv_win32_window_info->keyboard_pressed = (uMsg == WM_KEYDOWN);
        lv_win32_window_info->keyboard_wparam  = wParam;
        lv_win32_window_info->keyboard_lparam  = lParam;
        break;
    }
    case WM_MOUSEWHEEL:
    {
        if (windowIsActive) {
            lv_win32_window_info->mousewheel_value = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        }
        break;
    }
    case WM_TOUCH:
    {
        UINT cInputs            = LOWORD(wParam);
        HTOUCHINPUT hTouchInput = (HTOUCHINPUT)(lParam);

        PTOUCHINPUT pInputs = malloc(cInputs * sizeof(TOUCHINPUT));
        if (pInputs)
        {
            if (lv_win32_get_touch_input_info(
                hTouchInput,
                cInputs,
                pInputs,
                sizeof(TOUCHINPUT)))
            {
                for (UINT i = 0; i < cInputs; ++i)
                {
                    POINT Point = { 0 };
                    Point.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
                    Point.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
                    if (!ScreenToClient(hWnd, &Point))
                    {
                        continue;
                    }

                    uint16_t x = (uint16_t)(Point.x & 0xffff);
                    uint16_t y = (uint16_t)(Point.y & 0xffff);

                    DWORD MousePressedMask = TOUCHEVENTF_MOVE | TOUCHEVENTF_DOWN;
                    lv_win32_window_info->mouse_value   = (y << 16) | x;
                    lv_win32_window_info->mouse_pressed = (pInputs[i].dwFlags & MousePressedMask);
                }
            }

            free(pInputs);
        }

        lv_win32_close_touch_input_handle(hTouchInput);
        break;
    }
    case WM_DPICHANGED:
    {
        s_dpi_value = HIWORD(wParam);
        LPRECT SuggestedRect = (LPRECT)lParam;

        SetWindowPos(
            hWnd,
            NULL,
            SuggestedRect->left,
            SuggestedRect->top,
            SuggestedRect->right,
            SuggestedRect->bottom,
            SWP_NOZORDER | SWP_NOACTIVATE);

        RECT ClientRect;
        GetClientRect(hWnd, &ClientRect);

        lv_disp_t* disp  = lv_win32_window_info->display;
        int WindowWidth  = MulDiv(disp->driver->hor_res * WIN32DRV_MONITOR_ZOOM, s_dpi_value, USER_DEFAULT_SCREEN_DPI);
        int WindowHeight = MulDiv(disp->driver->ver_res * WIN32DRV_MONITOR_ZOOM, s_dpi_value, USER_DEFAULT_SCREEN_DPI);
        SetWindowPos(
            hWnd,
            NULL,
            SuggestedRect->left,
            SuggestedRect->top,
            SuggestedRect->right + (WindowWidth - ClientRect.right),
            SuggestedRect->bottom + (WindowHeight - ClientRect.bottom),
            SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }
    case WM_PAINT:
    {
        if (lv_win32_window_info) { lv_win32_window_info->display_refreshing = true; }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (lv_win32_window_info && lv_win32_window_info->display) {
            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(
                hdc,
                ps.rcPaint.left,
                ps.rcPaint.top,
                ps.rcPaint.right - ps.rcPaint.left,
                ps.rcPaint.bottom - ps.rcPaint.top,
                lv_win32_window_info->buffer_dc_handle,
                0,
                0,
                MulDiv(ps.rcPaint.right - ps.rcPaint.left, USER_DEFAULT_SCREEN_DPI, WIN32DRV_MONITOR_ZOOM * s_dpi_value),
                MulDiv(ps.rcPaint.bottom - ps.rcPaint.top, USER_DEFAULT_SCREEN_DPI, WIN32DRV_MONITOR_ZOOM * s_dpi_value),
                SRCCOPY
            );
        }
        EndPaint(hWnd, &ps);
        if (lv_win32_window_info) { lv_win32_window_info->display_refreshing = false; }

        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

static unsigned int __stdcall lv_win32_window_thread_entrypoint(void* raw_parameter) {
    // Get thread and windows information
    lv_win32_window_info_t* lv_win32_window_info = (lv_win32_window_info_t*)raw_parameter;
    window_thread_params_t* thread_params = lv_win32_window_info->temporary_thread_params;

    // Create a custom class name for each window
    wchar_t lpszClassName[32];
    wsprintf(lpszClassName, L"lv_sim_visual_studio_%d", s_disp_info.counter);

    // Setup new window info
    WNDCLASSEXW window_class;
    window_class.cbSize             = sizeof(WNDCLASSEXW);
    window_class.style              = 0;
    window_class.lpfnWndProc        = lv_win32_window_message_callback;
    window_class.cbClsExtra         = 0;
    window_class.cbWndExtra         = 0;
    window_class.hInstance          = thread_params->instance_handle;
    window_class.hIcon              = thread_params->icon_handle;
    window_class.hCursor            = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground      = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName       = NULL;
    window_class.lpszClassName      = lpszClassName;
    window_class.hIconSm            = thread_params->icon_handle;
    if (!RegisterClassExW(&window_class)) {
        printf("Failed to register disp number %d!", s_disp_info.counter + 1);
        return 0;
    }

    // Create a new window
    HWND window_handle = CreateWindowExW(
        WINDOW_EX_STYLE,
        window_class.lpszClassName,
        thread_params->screen_title,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        NULL,
        NULL,
        thread_params->instance_handle,
        NULL
    );
    if (!window_handle) {
        printf("Window Handle for disp %d is NULL!", s_disp_info.counter + 1);
        return 0;
    }
    lv_win32_window_info->window_handle = window_handle;

    // Adjust window size
    s_dpi_value = lv_win32_get_dpi_for_window(window_handle);
    RECT window_size    = { 0 };
    window_size.left    = 0;
    window_size.top     = 0;
    window_size.right   = MulDiv(thread_params->hor_res * WIN32DRV_MONITOR_ZOOM, s_dpi_value, USER_DEFAULT_SCREEN_DPI);
    window_size.bottom  = MulDiv(thread_params->ver_res * WIN32DRV_MONITOR_ZOOM, s_dpi_value, USER_DEFAULT_SCREEN_DPI);
    AdjustWindowRectEx(&window_size, WINDOW_STYLE, FALSE, WINDOW_EX_STYLE);
    OffsetRect(&window_size, -window_size.left, -window_size.top);

    // Try positioning each screen in a way that each one is visible
    int posX = 0;
    for (uint8_t i = 0; i < s_disp_info.counter; i++) {
        WINDOWINFO winfo = { 0 };
        GetWindowInfo(s_disp_info.data[i].window_handle, &winfo);
        posX += (winfo.rcWindow.right - winfo.rcWindow.left);
    }
    if (posX < GetSystemMetrics(SM_CXSCREEN)) {
        SetWindowPos(window_handle, NULL, posX, 0, window_size.right, window_size.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else {
        SetWindowPos(window_handle, NULL, 0, 0, window_size.right, window_size.bottom, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );
    }

    // Setup window
    lv_win32_register_touch_window(window_handle, 0);
    lv_win32_enable_child_window_dpi_message(window_handle);

    // Show display window
    HDC hNewBufferDC = lv_win32_create_frame_buffer(
        window_handle,
        thread_params->hor_res,
        thread_params->ver_res,
        &lv_win32_window_info->pixel_buffer,
        &lv_win32_window_info->pixel_buffer_size
    );
    lv_win32_window_info->buffer_dc_handle = hNewBufferDC;
    ShowWindow(window_handle, thread_params->show_window_mode);
    UpdateWindow(window_handle);

    // Release mutex
    SetEvent(thread_params->window_mutex);

    // Control window until application is closed
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);

        // Forces all displays to close when a single one closes
        if (lv_win32_quit_signal) {break;}
    }
    lv_win32_quit_signal = true;

    // Free all dynamic allocated memory for this display
    if (lv_win32_window_info->malloc_pixel_buffer) {
        free(lv_win32_window_info->malloc_pixel_buffer);
        lv_win32_window_info->malloc_pixel_buffer = NULL;
    }

    return 0;
}

#endif /*USE_WIN32DRV*/
