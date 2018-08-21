#include "pch.h"
#include "CalibrationProcessor.h"
#include <robuffer.h>

////#pragma optimize("", off)

using namespace HeadViewer;
using namespace Windows::UI::Xaml::Media::Imaging;

CalibrationProcessor::CalibrationProcessor()
{
    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();
    CalibrationData = ref new Vector<CalibrationEntry^>();
}

task<void> CalibrationProcessor::ProcessCalibrationEntries()
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
    Debug::WriteLine(L"Using Normalized face rect of size (w,h)=[%d, %d]", m_faceWidth, m_faceHeight);

    for (auto entry : CalibrationData)
    {
        entry->NormalizedFaceRect = rc; // entry->MainFaceRect;
        entry->NormalizedFace = GetNormalizedFaceBitmap(entry);
    }


    // compute correlation matrix
    cv::Mat correlationMatrix(9, 9, CV_64F);
    cv::Mat Y(2, 9, CV_64F);
    for (unsigned int i = 0; i < CalibrationData->Size; i++)
    {
        auto calib1 = CalibrationData->GetAt(i);
        auto mat1 = calib1->NormalizedFace->ImageGray;
        Y.at<double>(0, i) = calib1->X;
        Y.at<double>(1, i) = calib1->Y;

        for (unsigned int j = 0; j < CalibrationData->Size; j++)
        {
            auto calib2 = CalibrationData->GetAt(j);

            auto mat2 = calib2->NormalizedFace->ImageGray;
            auto diffMat = mat1 - mat2;

            auto sqrMat = diffMat.mul(diffMat);
            auto sumMat = cv::sum(sqrMat);
            correlationMatrix.at<double>(i, j) = sqrt(sumMat[0]);
        }
    }

    auto correlationInverse = correlationMatrix.inv();

    auto calibrationMatrix = Y * correlationInverse;

    CalibrationMatrix = calibrationMatrix;

    IsCalibrationValid = true;

    co_return;
}

void CalibrationProcessor::Reset()
{
    CalibrationData->Clear();
    IsCalibrationValid = false;
}

Point CalibrationProcessor::ComputeHeadGazeCoordinates(SoftwareBitmapWrapper^ bitmap)
{
    auto rc = GetMainFaceRect(bitmap);
    if (rc.IsEmpty)
    {
        return Point(-1, -1);
    }

    cv::Mat faceRoi = bitmap->ImageGray(cv::Rect(rc.Left, rc.Top, rc.Width, rc.Height));
    
    cv::Mat normalFace;
    cv::resize(faceRoi, normalFace, cv::Size(m_faceWidth, m_faceHeight));

    cv::Mat testMat(9, 1, CV_64F);
    for (unsigned int i = 0; i < CalibrationData->Size; i++)
    {
        auto calib = CalibrationData->GetAt(i);
        auto calibMat = calib->NormalizedFace->ImageGray;
        auto diffMat = normalFace - calibMat;
        auto sqrMat = diffMat.mul(diffMat);
        auto sumMat = cv::sum(sqrMat);
        testMat.at<double>(i, 0) = sqrt(sumMat[0]);
    }

    cv::Mat result = CalibrationMatrix * testMat;
    auto point = Point(result.at<double>(0, 0), result.at<double>(1, 0));
    return point;
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

SoftwareBitmapWrapper^ CalibrationProcessor::GetNormalizedFaceBitmap(CalibrationEntry ^entry)
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

    return ref new SoftwareBitmapWrapper(bmp);
}

#pragma optimize("", on)
