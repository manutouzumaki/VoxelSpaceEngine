
#include <windows.h>
#include <d3dcompiler.h>

#include "renderer.h"

struct vec3 {
    f32 x, y, z;
};

struct vec2 {
    f32 x, y;
};

struct VertexD3D11 {
    vec3 position;
    vec2 uvs;
};

// Vertex Shader
global_variable char *vertexShaderSource =
"struct VS_Input\n"
"{\n"
"   float4 pos : POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"};\n"
"struct PS_Input\n"
"{\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"};\n"
"PS_Input VS_Main( VS_Input vertex )\n"
"{\n"
"   PS_Input vsOut = ( PS_Input )0;\n"
"   vsOut.pos = vertex.pos;\n"
"   vsOut.tex0 = vertex.tex0;\n"
"   return vsOut;\n"
"}\0";

// Pixel Shader
global_variable char *pixelShaderSource  =
"Texture2D colorMap : register( t0 );\n"
"SamplerState colorSampler : register( s0 );\n"
"struct PS_Input\n"
"{\n"
"   float4 pos : SV_POSITION;\n"
"   float2 tex0 : TEXCOORD0;\n"
"};\n"
"float4 PS_Main( PS_Input frag ) : SV_TARGET\n"
"{\n"
"   float4 color = colorMap.Sample(colorSampler, frag.tex0.xy);\n"
"   return float4(color.rgb, 1);\n"
"}\0";


internal i32 StringLength(char * String) {
    i32 Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return Count;
}

Renderer *RendererCreate(HWND hwnd, i32 bufferWidth, i32 bufferHeight) {

    Renderer *renderer = (Renderer *)malloc(sizeof(Renderer));

    // - 1: Define the device types and feature level we want to check for.
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_SOFTWARE
    };
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    i32 driverTypesCount = ARRAY_LENGTH(driverTypes);
    i32 featureLevelsCount = ARRAY_LENGTH(featureLevels);


    // - 2: create the d3d11 device, rendering context, and swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = bufferWidth;
    swapChainDesc.BufferDesc.Height = bufferHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = (i32)FPS;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    D3D_FEATURE_LEVEL featureLevel;
    D3D_DRIVER_TYPE driverType;
    HRESULT result;
    for(u32 driver = 0; driver < driverTypesCount; ++driver) {
        result = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driver], NULL, 0, featureLevels, featureLevelsCount, D3D11_SDK_VERSION, &swapChainDesc,
                                               &renderer->swapChain, &renderer->device, &featureLevel, &renderer->deviceContext);
        if(SUCCEEDED(result)) {
            driverType = driverTypes[driver];
            break;
        }
    }

    // - 3: Create render target view.
    ID3D11Texture2D *backBufferTexture = NULL;
    result = renderer->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBufferTexture);
    result = renderer->device->CreateRenderTargetView(backBufferTexture, 0, &renderer->renderTargetView);
    if(backBufferTexture) {
        backBufferTexture->Release();
    }

    OutputDebugString("D3D11 Initialized\n");

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width =  bufferWidth;
    viewport.Height = bufferHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    renderer->deviceContext->RSSetViewports(1, &viewport);

    renderer->bufferWidth = bufferWidth;
    renderer->bufferHeight = bufferHeight;
    renderer->colorBuffer = (u32 *)malloc(renderer->bufferWidth * renderer->bufferHeight * sizeof(u32));
    
    // - 5: Create Vertex, Pixel shader and Input Layout
    ID3DBlob *vertexShaderCompiled = 0;
    ID3DBlob *errorVertexShader    = 0;
    result = D3DCompile((void *)vertexShaderSource,
                        (SIZE_T)StringLength(vertexShaderSource),
                        0, 0, 0, "VS_Main", "vs_4_0",
                        D3DCOMPILE_ENABLE_STRICTNESS, 0,
                        &vertexShaderCompiled, &errorVertexShader);
    if(errorVertexShader != 0)
    {
        errorVertexShader->Release();
    }

    ID3DBlob *pixelShaderCompiled = 0;
    ID3DBlob *errorPixelShader    = 0;
    result = D3DCompile((void *)pixelShaderSource,
                        (SIZE_T)StringLength(pixelShaderSource),
                        0, 0, 0, "PS_Main", "ps_4_0",
                        D3DCOMPILE_ENABLE_STRICTNESS, 0,
                        &pixelShaderCompiled, &errorPixelShader);
    if(errorPixelShader != 0)
    {
        errorPixelShader->Release();
    }

    // Create the Vertex Shader.
    result = renderer->device->CreateVertexShader(vertexShaderCompiled->GetBufferPointer(),
                                                  vertexShaderCompiled->GetBufferSize(), 0,
                                                  &renderer->vertexShader);
    // Create the Input layout.
    D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
        0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    u32 totalLayoutElements = ARRAY_LENGTH(inputLayoutDesc);
    result = renderer->device->CreateInputLayout(inputLayoutDesc,
                                                 totalLayoutElements,
                                                 vertexShaderCompiled->GetBufferPointer(),
                                                 vertexShaderCompiled->GetBufferSize(),
                                                 &renderer->inputLayout);
    // Create Pixel Shader.
    result = renderer->device->CreatePixelShader(pixelShaderCompiled->GetBufferPointer(),
                                                 pixelShaderCompiled->GetBufferSize(), 0,
                                                 &renderer->pixelShader); 
    vertexShaderCompiled->Release();
    pixelShaderCompiled->Release();

    // Create Vertex Buffer
    VertexD3D11 vertices[] = {
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f
    };

    // buffer description
    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(VertexD3D11) * 6;
    // pass the buffer data (Vertices).
    D3D11_SUBRESOURCE_DATA resourceData = {};
    resourceData.pSysMem = vertices;
    // Create the VertexBuffer
    result = renderer->device->CreateBuffer(&vertexDesc, &resourceData, &renderer->vertexBuffer);

    D3D11_TEXTURE2D_DESC textureDesc = {}; 
    textureDesc.Width = renderer->bufferWidth;
    textureDesc.Height = renderer->bufferHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags = 0;
    // Create out Texture 
    result = renderer->device->CreateTexture2D(&textureDesc, NULL, &renderer->backBuffer);
    if(SUCCEEDED(result))
    {
        OutputDebugString("SUCCEEDED Creating Texture\n");
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc = {};
    shaderResourceDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    shaderResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceDesc.Texture2D.MipLevels = 1;
    result = renderer->device->CreateShaderResourceView(renderer->backBuffer, &shaderResourceDesc, &renderer->colorMap);
    if(SUCCEEDED(result))
    {
        OutputDebugString("SUCCEEDED Creating Shader resource view\n");
    }

    D3D11_SAMPLER_DESC colorMapDesc = {};
    colorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    colorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; //D3D11_FILTER_MIN_MAG_MIP_LINEAR | D3D11_FILTER_MIN_MAG_MIP_POINT
    colorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = renderer->device->CreateSamplerState(&colorMapDesc, &renderer->colorMapSampler);
    if(SUCCEEDED(result))
    {
        OutputDebugString("SUCCEEDED Creating sampler state\n");
    }


    renderer->deviceContext->OMSetRenderTargets(1, &renderer->renderTargetView, 0);
    renderer->deviceContext->PSSetShaderResources(0, 1, &renderer->colorMap);
    renderer->deviceContext->PSSetSamplers(0, 1, &renderer->colorMapSampler);
    u32 stride = sizeof(VertexD3D11);
    u32 offset = 0;
    renderer->deviceContext->IASetInputLayout(renderer->inputLayout);
    renderer->deviceContext->IASetVertexBuffers(0, 1, &renderer->vertexBuffer, &stride, &offset);
    renderer->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer->deviceContext->VSSetShader(renderer->vertexShader, 0, 0);
    renderer->deviceContext->PSSetShader(renderer->pixelShader,  0, 0);

    return renderer;
}

void RendererDestroy(Renderer *renderer) {
    renderer->colorMapSampler->Release();
    renderer->colorMap->Release();
    renderer->backBuffer->Release();
    renderer->vertexBuffer->Release();
    renderer->inputLayout->Release();
    renderer->pixelShader->Release();
    renderer->vertexShader->Release();
    renderer->renderTargetView->Release();
    renderer->swapChain->Release();
    renderer->deviceContext->Release();
    renderer->device->Release();
    free(renderer->colorBuffer);
    free(renderer);
}

void RendererSetSize(Renderer *renderer, u32 width, u32 height) {
    renderer->deviceContext->OMSetRenderTargets(0, 0, 0);
    // Release all outstading references to swapchain buffers
    renderer->renderTargetView->Release();
    HRESULT hr;
    // Preserve the existing buffer count and format
    // Automatically choose the width and height to match the client rect for HWND
    hr = renderer->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    // get buffer and create render target view
    ID3D11Texture2D *buffer = 0;
    hr = renderer->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&buffer);
    hr = renderer->device->CreateRenderTargetView(buffer, NULL, &renderer->renderTargetView);
    buffer->Release();

    renderer->deviceContext->OMSetRenderTargets(1, &renderer->renderTargetView, NULL);

#if 1
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width =  width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    renderer->deviceContext->RSSetViewports(1, &viewport);
#endif
}

void RendererClearBuffer(Renderer *renderer, u32 cleanColor) {
    for(i32 y = 0; y < renderer->bufferHeight; ++y) {
        for(i32 x = 0; x < renderer->bufferWidth; ++x) {
            u32 *pixelPt = renderer->colorBuffer + ((y * renderer->bufferWidth) + x);
            *pixelPt = cleanColor;
        }
    }
    float ClearColor[4] = {1, 1, 1, 1};
    renderer->deviceContext->ClearRenderTargetView(renderer->renderTargetView, ClearColor);
}

void RendererPresent(Renderer *renderer) {
    D3D11_MAPPED_SUBRESOURCE buffer;
    renderer->deviceContext->Map(renderer->backBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer);
    memcpy(buffer.pData, renderer->colorBuffer, renderer->bufferWidth*renderer->bufferHeight*sizeof(u32));
    renderer->deviceContext->Unmap(renderer->backBuffer, 0);
    renderer->deviceContext->Draw(6, 0);
    renderer->swapChain->Present(1, 0);
}

void RendererGetFrameBuffer(Renderer *renderer, u32 **buffer, u32 *bufferWidth, u32 *bufferHeight) {
    *buffer = renderer->colorBuffer;
    *bufferWidth = renderer->bufferWidth;
    *bufferHeight = renderer->bufferHeight;
}


