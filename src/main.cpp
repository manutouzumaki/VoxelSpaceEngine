#include <stdio.h>
#include <windows.h>
#include <math.h>
#include "renderer.h"
#include "bmp.h"


#define PI 3.14159265359
#define RAD(value) (value * (PI/180.0))
#define DEG(value) (value * (180.0/PI))

struct Camera {
    f32 x;
    f32 y;
    f32 height;
    f32 angle;
    f32 horizon;
    f32 zfar;
};

global_variable Renderer *gRenderer;
global_variable Bmp heightmap;
global_variable Bmp colormap;
global_variable Camera camera = {512, 512, 150, RAD(270.0), 100, 800};
global_variable f32 fogDistance = 0.25;

global_variable DWORD keys[256];

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(msg) {
        case WM_CLOSE: {
            PostQuitMessage(0);                   
        }break;
        case WM_SIZE: {
            if(gRenderer) {
                u32 width = LOWORD(lParam);              
                u32 height = HIWORD(lParam);
                RendererSetSize(gRenderer, width, height);  
            }          
        } break;
        case WM_KEYDOWN: {
            DWORD key = (DWORD)wParam;
            keys[key] = true;
        } break;
        case WM_KEYUP: {
            DWORD key = (DWORD)wParam;
            keys[key] = false;
        } break;
        default: {
            result = DefWindowProcA(hwnd, msg, wParam, lParam); 
        }
    }
    return result;
}

f32 lerp(f32 a, f32 b, f32 t) {
    return (1.0f - t) * a + b * t;
}

f32 invlerp(f32 a, f32 b, f32 v) {
    return (v - a) / (b - a);
}

f32 remap(f32 imin, f32 imax, f32 omin, f32 omax, f32 v) {
    f32 t = invlerp(imin, imax, v);
    return lerp(omin, omax, t);
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {

    static TCHAR szAppName[] = TEXT ("VoxelSpaceEngine");
    HWND hwnd;
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;
    if (!RegisterClass (&wndclass)) {
        MessageBox (NULL, TEXT ("Program requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0 ;
    }
    RECT windowRect = {};
    windowRect.left = 0;
    windowRect.right = WINDOW_WIDTH;
    windowRect.top = 0;
    windowRect.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
    u32 windowWidth = windowRect.right - windowRect.left;
    u32 windowHeight = windowRect.bottom - windowRect.top; 
    hwnd = CreateWindow (szAppName, TEXT ("Voxel Space Engine"),
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         windowWidth, windowHeight,
                         NULL, NULL, hInstance, NULL);
    ShowWindow (hwnd, iCmdShow);
    UpdateWindow (hwnd);
    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    u32 clientWidth = clientRect.right - clientRect.left;
    u32 clientHeight = clientRect.bottom - clientRect.top;
 
    Renderer *renderer = RendererCreate(hwnd, clientWidth, clientHeight);
    gRenderer = renderer;

    u32 *frameBuffer = 0;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
    RendererGetFrameBuffer(renderer, &frameBuffer, &frameBufferWidth, &frameBufferHeight);

    colormap = BmpLoad("../assets/mapcolor.bmp");
    heightmap = BmpLoad("../assets/mapheight.bmp");

    bool running = true;
    while(running) {
        MSG msg;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch(msg.message)
            {
                case WM_QUIT: {
                    running = false;      
                } break;
                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }break;
            } 
        }

        if(keys['R']) {
            camera.height++;
        }
        if(keys['F']) {
            camera.height--;
        }
        if(keys['W']) {
            camera.x += cosf(-camera.angle);
            camera.y += sinf(-camera.angle);
        }
        if(keys['S']) {
            camera.x -= cosf(-camera.angle);
            camera.y -= sinf(-camera.angle);
        }
        if(keys['D']) {
            camera.x += cosf(-camera.angle - RAD(90.0));
            camera.y += sinf(-camera.angle - RAD(90.0));        
        }
        if(keys['A']) {
            camera.x -= cosf(-camera.angle - RAD(90.0));
            camera.y -= sinf(-camera.angle - RAD(90.0));         
        }
        if(keys[VK_LEFT]) {
            camera.angle -= 0.02;
        }
        if(keys[VK_RIGHT]) {
            camera.angle += 0.02;
        }
        if(keys[VK_UP]) {
            camera.horizon += 6;
        }
        if(keys[VK_DOWN]) {
            camera.horizon -= 6;
        }
        if(keys[VK_SHIFT]) {
            fogDistance += 0.01;
            if(fogDistance > 1.0f) {
                fogDistance = 1.0;
            }
        }
        if(keys[VK_CONTROL]) {
            fogDistance -= 0.01;
        }
        u32 skyColor = 0xFF8CB2F5;
        RendererClearBuffer(renderer, skyColor);

        f32 plx = cosf(camera.angle) * camera.zfar + sinf(camera.angle) * camera.zfar;
        f32 ply = sinf(camera.angle) * camera.zfar - cosf(camera.angle) * camera.zfar;
        f32 prx = cosf(camera.angle) * camera.zfar - sinf(camera.angle) * camera.zfar;
        f32 pry = sinf(camera.angle) * camera.zfar + cosf(camera.angle) * camera.zfar;

        // loop 320 rays from left to right
        f32 incx = (prx - plx) / WINDOW_WIDTH;
        f32 incy = (pry - ply) / WINDOW_WIDTH;
        for(i32 i = 0; i < WINDOW_WIDTH; ++i) {
            f32 deltax = (plx + (incx * i))/camera.zfar;
            f32 deltay = (ply + (incy * i))/camera.zfar;
            f32 rx = camera.x;
            f32 ry = camera.y;

            f32 maxheight = WINDOW_HEIGHT;

            for(i32 z = 1; z < camera.zfar; ++z) {
                rx += deltax;
                ry -= deltay;

                u32 *heightmapBuffer = (u32 *)heightmap.data;
                u32 *colormapBuffer = (u32 *)colormap.data;
                i32 mapoffset = ((heightmap.width * ((i32)ry & (colormap.width - 1))) + ((i32)rx & (colormap.width - 1))); 

                i32 height = (i32)(heightmapBuffer[mapoffset] & 0xFF);
                i32 heightonscreen = (i32)((camera.height - height) / z * SCALE_FACTOR + camera.horizon);

                if(heightonscreen < 0) {
                    heightonscreen = 0;
                }
                if(heightonscreen > WINDOW_HEIGHT) {
                    heightonscreen = WINDOW_HEIGHT - 1;
                }
                // only render the tarrain if the new projected height is taller than the previous maxheight
                if(heightonscreen < maxheight) {
                    for(i32 y = heightonscreen; y < maxheight; ++y) {

                        u32 color = colormapBuffer[mapoffset];

                        // fog implementation
                        f32 alpha = ((f32)z / camera.zfar);
                        if(alpha > fogDistance) {
                            alpha = remap(fogDistance, 1.0, 0.0, 1.0, alpha);
                            if(alpha > 1.0) alpha = 1.0;

                            f32 dstr = (f32)((color >> 16) & 0xFF);
                            f32 dstg = (f32)((color >> 8) & 0xFF);
                            f32 dstb = (f32)((color >> 0) & 0xFF);
                            f32 srcr = (f32)((skyColor >> 16) & 0xFF);
                            f32 srcg = (f32)((skyColor >> 8) & 0xFF);
                            f32 srcb = (f32)((skyColor >> 0) & 0xFF);

                            f32 r = dstr + (srcr - dstr) * alpha;
                            f32 g = dstg + (srcg - dstg) * alpha;
                            f32 b = dstb + (srcb - dstb) * alpha;

                            color = 0xFF000000 | ((u32)r << 16) | ((u32)g << 8) | ((u32)b << 0); 
                        };
                        
                        frameBuffer[(WINDOW_WIDTH * y) + i] = color;
                    }

                    maxheight = heightonscreen;
                }
            }
        }

        RendererPresent(renderer);
    }

    RendererDestroy(renderer);

    return 0;
}
