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

    BYTE* data;
    UINT32 capacity;
    hr = bufferByteAccess->GetBuffer(&data, &capacity);
    if (FAILED(hr))
    {
        return nullptr;
    }

    auto curData = data;
    array2d<rgb_pixel> img;
    img.set_size(rows, cols);
    for (auto curPixel = img.begin(); curPixel != img.end(); img.move_next())
    {
        curPixel->red = curData[2];
        curPixel->green = curData[1];
        curPixel->blue = curData[0];
        //curPixel->alpha = curData[3];
        curPixel++;
        curData += 4;
    }

    frontal_face_detector detector = get_frontal_face_detector();
    std::vector<rectangle> faces = detector(img);
    if (faces.size() <= 0)
    {
        return nullptr;
    }

    full_object_detection shape = m_shapePredictor(img, faces[0]);

    auto result = ref new HeadTrackerResult();
    result->FaceRect = Rect(faces[0].left(), faces[0].top(), faces[0].width(), faces[0].height());
    
    auto points = ref new Vector<Point>();
    for (unsigned long i = 0; i < shape.num_parts(); i++)
    {
        point dpt = shape.part(i);
        auto pt = Point(dpt.x(), dpt.y());
        points->Append(pt);
    }

    result->FacePoints = points->GetView();

    return result;
}
