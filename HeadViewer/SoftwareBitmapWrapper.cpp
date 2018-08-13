#include "pch.h"
#include "SoftwareBitmapWrapper.h"


using namespace HeadViewer;

SoftwareBitmapWrapper::SoftwareBitmapWrapper(SoftwareBitmap^ bitmap)
{
    m_bitmap = bitmap;

    m_bitmapBuffer = m_bitmap->LockBuffer(BitmapBufferAccessMode::Read);
    m_memoryBufferReference = m_bitmapBuffer->CreateReference();

    HRESULT hr = reinterpret_cast<IInspectable*>(m_memoryBufferReference)->QueryInterface(IID_PPV_ARGS(&m_bufferByteAccess));
    if (FAILED(hr))
    {
        return;
    }

    BYTE* bmpData;
    UINT32 capacity;
    hr = m_bufferByteAccess->GetBuffer(&bmpData, &capacity);
    if (FAILED(hr))
    {
        return;
    }

    cv::Mat imgBGRA(bitmap->PixelHeight, bitmap->PixelWidth, CV_8UC4, bmpData, 0);
    cv::cvtColor(imgBGRA, ImageBGR, CV_BGRA2BGR);
    cv::cvtColor(imgBGRA, ImageGray, CV_BGRA2GRAY);
}
