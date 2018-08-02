#include "pch.h"
#include "RBFTracker.h"


using namespace cv;
using namespace dlib;
using namespace HeadViewer;
using namespace Microsoft::WRL;
using namespace Windows::Graphics::Imaging;

RBFTracker::RBFTracker()
{
    deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();

}

void RBFTracker::ProcessFrame(SoftwareBitmap^ frameBitmap)
{
    BYTE *bmpData;
    UINT32 capacity;
    ComPtr<IMemoryBufferByteAccess> retBuffer = GetBitmapBuffer(frameBitmap, &bmpData, &capacity);

    if (retBuffer == nullptr)
    {
        Debug::WriteLine(L"ERROR: Failed to get bitmap buffer!");
        return;
    }

    int rows = frameBitmap->PixelHeight;
    int cols = frameBitmap->PixelWidth;

    cv::Mat imgBGRA(rows, cols, CV_8UC4, bmpData, 0);
    cv::Mat imgBGR(rows, cols, CV_8UC3);
    cv::cvtColor(imgBGRA, imgBGR, CV_BGRA2BGR, 3);

    dlib::cv_image<bgr_pixel> img(imgBGR);

    std::vector<dlib::rectangle> faces = m_faceDetector(img);
    if (faces.size() <= 0)
    {
        Debug::WriteLine(L"ERROR: No face found!");
        return;
    }

    int faceIndex = 0;  // TODO: Use the index of the largest face found in the frame



}

ComPtr<IMemoryBufferByteAccess> RBFTracker::GetBitmapBuffer(SoftwareBitmap^ frameBitmap, BYTE **bmpData, UINT32 *capacity)
{
    int rows = frameBitmap->PixelHeight;
    int cols = frameBitmap->PixelWidth;
    BitmapBuffer^ bitmapBuffer = frameBitmap->LockBuffer(BitmapBufferAccessMode::Read);
    auto reference = bitmapBuffer->CreateReference();
    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> bufferByteAccess;

    HRESULT hr = reinterpret_cast<IInspectable*>(reference)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
    if (FAILED(hr))
    {
        return bufferByteAccess;
    }

    hr = bufferByteAccess->GetBuffer(bmpData, capacity);

    return bufferByteAccess;
}
