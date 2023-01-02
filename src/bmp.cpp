#include <windows.h>
#include <memory.h>
#include "bmp.h"

struct ReadFileResult {
    void *data;
    size_t size;
};

#pragma pack(push, 1)
struct BitmapHeader {
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
	u32 size;             
	i32 width;            
    i32 height;           
	u16 planes;           
	u16 bitsPerPixel;    
	u32 compression;      
	u32 sizeOfBitmap;     
	i32 horzResolution;  
	i32 vertResolution;  
	u32 colorsUsed;       
	u32 colorsImportant;  
	u32 redMask;          
	u32 greenMask;        
	u32 blueMask;         
	u32 alphaMask;        
};
#pragma pack(pop)

ReadFileResult ReadFile(char *path) {
    ReadFileResult result = {};
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        GetFileSizeEx(file, &fileSize);
        result.data = malloc(fileSize.QuadPart);
        if(ReadFile(file, result.data, (DWORD)fileSize.QuadPart, 0, 0)) {
            result.size = fileSize.QuadPart; 
        }
        else {
            ASSERT(!"ERROR Reading the file");
        }
        CloseHandle(file);
    }
    return result;
}

internal
u32 BitScanForward(u32 mask) {
    unsigned long shift = 0;
    _BitScanForward(&shift, mask);
    return (u32)shift;
}

Bmp BmpLoad(char *path) {
    ReadFileResult fileResult = ReadFile(path);
    BitmapHeader *header = (BitmapHeader *)fileResult.data;
    Bmp bitmap = {};
    bitmap.width = header->width;
    bitmap.height = header->height;
    bitmap.data = malloc(sizeof(u32) * header->width * header->height);
    void *data = (void *)((u8 *)fileResult.data + header->bitmapOffset);
    memcpy(bitmap.data, data, sizeof(u32) * header->width * header->height);
    u32 redShift = BitScanForward(header->redMask);
    u32 greenShift = BitScanForward(header->greenMask);
    u32 blueShift = BitScanForward(header->blueMask);
    u32 alphaShift = BitScanForward(header->alphaMask);
    u32 *colorData = (u32 *)bitmap.data;
    for(u32 i = 0; i < bitmap.width*bitmap.height; ++i)
    {
        u32 red = (colorData[i] & header->redMask) >> redShift;       
        u32 green = (colorData[i] & header->greenMask) >> greenShift;       
        u32 blue = (colorData[i] & header->blueMask) >> blueShift;       
        u32 alpha = (colorData[i] & header->alphaMask) >> alphaShift;       
        colorData[i] = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
    }
    free(fileResult.data);
    return bitmap;
}

void BmpDestroy(Bmp *bitmap) {
    free(bitmap->data);
}
