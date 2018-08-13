#pragma once

#include <MemoryBuffer.h>
#include <opencv2/opencv.hpp>

using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;

namespace HeadViewer
{
    private ref class SoftwareBitmapWrapper
    {
    internal:
        SoftwareBitmapWrapper(SoftwareBitmap^ bitmap);

        property SoftwareBitmap^ Bitmap
        {
            SoftwareBitmap^ get()
            {
                return m_bitmap;
            }
        }

    internal:
        cv::Mat ImageBGR;
        cv::Mat ImageGray;

    private:
        SoftwareBitmap^                                 m_bitmap;
        BitmapBuffer^                                   m_bitmapBuffer;
        IMemoryBufferReference^                         m_memoryBufferReference;
        Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> m_bufferByteAccess;
    };
}