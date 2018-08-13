#include "pch.h"
#include "CalibrationProcessor.h"
#include <robuffer.h>

#pragma optimize("", off)

using namespace HeadViewer;
using namespace Windows::UI::Xaml::Media::Imaging;

CalibrationProcessor::CalibrationProcessor()
{
    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();
    CalibrationData = ref new Vector<CalibrationEntry^>();
}

void CalibrationProcessor::ProcessCalibrationEntries()
{
    m_faceWidth = 0;
    m_faceHeight = 0;
    for (auto entry : CalibrationData)
    {
        // each calibration entry has a number of images. select the best image from that
        entry->BestImageIndex = GetBestImageIndex(entry);

        // each image can have more than one face image. Select the rect for the main face in it
        entry->MainFaceRect = GetMainFaceRect(entry->Bitmaps->GetAt(entry->BestImageIndex));

        // find the the largest of the face rects across all the calibration entries.
        // specifically, pick the largest width and largest height separately to make 
        // sure we dont exclude any specific details
        if (entry->MainFaceRect.Width > m_faceWidth)
        {
            m_faceWidth = (int)entry->MainFaceRect.Width;
        }

        if (entry->MainFaceRect.Height > m_faceHeight)
        {
            m_faceHeight = (int)entry->MainFaceRect.Height;
        }
    }

    auto rc = Rect(0, 0, m_faceWidth, m_faceHeight);
    for (auto entry : CalibrationData)
    {
        entry->NormalizedFaceRect = entry->MainFaceRect;
        entry->NormalizedFace = GetNormalizedFaceBitmap(entry);
    }
}

int CalibrationProcessor::GetBestImageIndex(CalibrationEntry^ entry)
{
    // TODO: This should identify the most stable pose and return the index of that image
    // TODO: For now just return the index of the last image
    return entry->Bitmaps->Size - 1;
}

Rect CalibrationProcessor::GetMainFaceRect(SoftwareBitmapWrapper^ bmpWrapper)
{
    dlib::cv_image<dlib::bgr_pixel> img(bmpWrapper->ImageBGR);

    std::vector<dlib::rectangle> faces = m_faceDetector(img);

    if (faces.size() == 0)
    {
        return Rect();
    }

    auto rc = std::max_element(faces.begin(), faces.end(), 
                                    [](const dlib::rectangle &a, const dlib::rectangle &b)
                                    {
                                        return a.area() > b.area();
                                    });

    return Rect(rc->left(), rc->top(), rc->width(), rc->height());
}

SoftwareBitmap^ CalibrationProcessor::GetNormalizedFaceBitmap(CalibrationEntry ^entry)
{
    auto image = entry->Bitmaps->GetAt(entry->BestImageIndex);

    auto roi = entry->NormalizedFaceRect;
    cv::Mat faceRoi = image->ImageGray(cv::Rect(roi.Left, roi.Top, roi.Width, roi.Height));
    cv::Mat face;
    cv::cvtColor(faceRoi, face, cv::COLOR_GRAY2BGRA);


    auto bitmap = ref new WriteableBitmap(entry->NormalizedFaceRect.Width, entry->NormalizedFaceRect.Height);

    Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccess;
    reinterpret_cast<IInspectable*>(bitmap->PixelBuffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));

    // Retrieve the buffer data.  
    byte* pixels = nullptr;
    bufferByteAccess->Buffer(&pixels);

    memcpy(pixels, face.data, bitmap->PixelBuffer->Capacity);


    auto bmp = ref new SoftwareBitmap(BitmapPixelFormat::Bgra8, roi.Width, roi.Height, BitmapAlphaMode::Ignore);
    bmp->CopyFromBuffer(bitmap->PixelBuffer);

    Debug::WriteLine(L"PixelFormat = %d, AlphaMode = %d", bmp->BitmapPixelFormat, bmp->BitmapAlphaMode);
    return bmp;
}

#pragma optimize("", on)
