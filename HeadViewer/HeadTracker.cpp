#include "pch.h"
#include "HeadTracker.h"

#include <MemoryBuffer.h>

using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;

using namespace dlib;
using namespace HeadViewer;

HeadTracker::HeadTracker()
{
    deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();
}

HeadTrackerResult^ HeadTracker::ProcessBitmap(Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap)
{
    int rows = softwareBitmap->PixelHeight;
    int cols = softwareBitmap->PixelWidth;
    BitmapBuffer^ bitmapBuffer = softwareBitmap->LockBuffer(BitmapBufferAccessMode::Read);
    auto reference = bitmapBuffer->CreateReference();
    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> bufferByteAccess;

    HRESULT hr = reinterpret_cast<IInspectable*>(reference)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
    if (FAILED(hr))
    {
        return nullptr;
    }

    BYTE* bmpData;
    UINT32 capacity;
    hr = bufferByteAccess->GetBuffer(&bmpData, &capacity);
    if (FAILED(hr))
    {
        return nullptr;
    }

    auto curData = bmpData;
    array2d<bgr_pixel> img;
    img.set_size(rows, cols);
    for (auto curPixel = img.begin(); curPixel != img.end(); img.move_next())
    {
        curPixel->blue = curData[0];
        curPixel->green = curData[1];
        curPixel->red = curData[2];
        //curPixel->alpha = curData[3];
        curPixel++;
        curData += 4;
    }

    std::vector<rectangle> faces = m_faceDetector(img);
    if (faces.size() <= 0)
    {
        return nullptr;
    }

    full_object_detection shape = m_shapePredictor(img, faces[0]);


    auto result = ref new HeadTrackerResult();
    auto rc = Rect(faces[0].left(), faces[0].top(), faces[0].width(), faces[0].height());
    result->FaceRect = rc;
    Debug::WriteLine(L"face=[%d, %d, %d, %d]", (int)rc.X, (int)rc.Y, (int)rc.Width, (int)rc.Height);
    DrawRectangle(bmpData, cols, rows, (int)rc.X, (int)rc.Y, (int)(rc.X + rc.Width), (int)(rc.Y + rc.Height), RGB(255, 0, 0));


    auto points = ref new Vector<Point>();
    for (unsigned long i = 0; i < shape.num_parts(); i++)
    {
        point dpt = shape.part(i);
        auto pt = Point(dpt.x(), dpt.y());
        points->Append(pt);
        DrawRectangle(bmpData, cols, rows, (int)(pt.X - 1), (int)(pt.Y - 1), (int)(pt.X + 1), (int)(pt.Y + 1), RGB(0, 255, 0));
    }

    result->FacePoints = points->GetView();

    //bitmapBuffer->Close();

    return result;
}



void HeadTracker::DrawRectangle(BYTE *buffer, int width, int height, int left, int top, int right, int bottom, COLORREF color)
{
    DrawHorizontalLine(buffer, width, height, left, right, top, color);
    DrawHorizontalLine(buffer, width, height, left, right, bottom, color);
    DrawVerticalLine(buffer, width, height, top, bottom, left, color);
    DrawVerticalLine(buffer, width, height, top, bottom, right, color);
}

void HeadTracker::DrawPlus(BYTE *buffer, int width, int height, int x, int y, int size, COLORREF color)
{
    DrawHorizontalLine(buffer, width, height, x - (size / 2), x + (size / 2), y, color);
    DrawVerticalLine(buffer, width, height, y - (size / 2), y + (size / 2), x, color);
}

void HeadTracker::DrawHorizontalLine(BYTE *buffer, int width, int height, int x1, int x2, int y, COLORREF color)
{
    x1 = (x1 < 0) ? 0 : x1;
    x2 = (x2 < 0) ? 0 : x2;
    y =  (y <  0) ? 0 : y;

    x1 = (x1 > width) ? width : x1;
    x2 = (x2 > width) ? width : x2;
    y = (y > height) ? height : y;
    
    BYTE *pixels = buffer + 4 * ((y * width) + x1);
    int count = x2 - x1 + 1;
    while (count--)
    {
        *pixels++ = GetBValue(color);
        *pixels++ = GetGValue(color);
        *pixels++ = GetRValue(color);
        *pixels++ = 255;
    }
}

void HeadTracker::DrawVerticalLine(BYTE *buffer, int width, int height, int y1, int y2, int x, COLORREF color)
{
    y1 = (y1 < 0) ? 0 : y1;
    y2 = (y2 < 0) ? 0 : y2;
    x = (x <  0) ? 0 : x;

    y1 = (y1 > height) ? height : y1;
    y2 = (y2 > height) ? height : y2;
    x = (x > height) ? width : x;

    byte *pixels;
    int count = y2 - y1 + 1;

    pixels = buffer + 4 * ((y1 * width) + x);
    count = y2 - y1 + 1;
    while (count--)
    {
        byte *pixel = pixels;
        *pixel++ = GetBValue(color);
        *pixel++ = GetGValue(color);
        *pixel++ = GetRValue(color);
        *pixel++ = 255;
        pixels += (4 * width);
    }
}
