#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <d3d11.h>
#include "defines.h"

struct Renderer {
    u32 *colorBuffer;
    u32 bufferWidth;
    u32 bufferHeight;
    HWND hwnd;

    ID3D11Device *device;
    ID3D11DeviceContext *deviceContext;
    IDXGISwapChain *swapChain;
    ID3D11RenderTargetView *renderTargetView;

    ID3D11VertexShader *vertexShader;
    ID3D11PixelShader  *pixelShader;
    ID3D11InputLayout  *inputLayout;
    ID3D11Buffer *vertexBuffer;

    ID3D11Texture2D *backBuffer;
    ID3D11ShaderResourceView *colorMap;
    ID3D11SamplerState *colorMapSampler;
};

Renderer *RendererCreate(HWND hwnd, i32 bufferWidth, i32 bufferHeight);
void RendererDestroy(Renderer *renderer);
void RendererClearBuffer(Renderer *renderer, u32 cleanColor);
void RendererPresent(Renderer *renderer);
void RendererSetPixelColor(Renderer *renderer, u32 x, u32 y, u32 color);
void RendererSetSize(Renderer *renderer, u32 width, u32 height);
void RendererGetFrameBuffer(Renderer *renderer, u32 **buffer, u32 *bufferWidth, u32 *bufferHeight);

#endif
