#include "pch.h"
#include "HeadTracker.h"

#include <MemoryBuffer.h>

using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;

using namespace cv;
using namespace dlib;
using namespace HeadViewer;

// offsets into the array of face landmarks identified
const int NOSE_TIP = 33;
const int CHIN = 8;
const int LEFT_EYE_CORNER = 36;
const int RIGHT_EYE_CORNER = 45;
const int MOUTH_LEFT_CORNER = 48;
const int MOUTH_RIGHT_CORNER = 54;

static double GetApproxFocalLength(int fovDegrees, int width, int height)
{
    // https://www.learnopencv.com/approximate-focal-length-for-webcams-and-cell-phone-cameras/

    // compute horizontal and vertical fov separately
    const double PI = 3.14159265;
    double diagFov = fovDegrees * PI / 180.0;
    double diag = sqrt((width * width) + (height * height));
    double focalLength = diag / (2 * (tan(diagFov / 2)));
    return focalLength;
}


HeadTracker::HeadTracker()
{
    deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();
    InitializeModelPoints();
}

void HeadTracker::InitializeModelPoints()
{
    m_modelPoints.push_back(cv::Point3d(0.0, 0.0, 0.0));         // nose tip 
    m_modelPoints.push_back(cv::Point3d(0.0, -330.0, -65.0));    // chin
    m_modelPoints.push_back(cv::Point3d(-225.0, 170.0, -135.0));    // left corner of eye
    m_modelPoints.push_back(cv::Point3d(225.0, 170.0, -135.0));    // right corner of eye
    m_modelPoints.push_back(cv::Point3d(-150.0, -150.0, -125.0));    // left corner of mouth
    m_modelPoints.push_back(cv::Point3d(150.0, -150.0, -125.0));     // right corner of mouth
}

std::vector<cv::Point2d> HeadTracker::GetImagePoints(full_object_detection &d)
{
    std::vector<cv::Point2d> image_points;
    image_points.push_back(cv::Point2d(d.part(NOSE_TIP).x(), d.part(NOSE_TIP).y()));    // Nose tip
    image_points.push_back(cv::Point2d(d.part(CHIN).x(), d.part(CHIN).y()));      // Chin
    image_points.push_back(cv::Point2d(d.part(LEFT_EYE_CORNER).x(), d.part(LEFT_EYE_CORNER).y()));    // Left eye left corner
    image_points.push_back(cv::Point2d(d.part(RIGHT_EYE_CORNER).x(), d.part(RIGHT_EYE_CORNER).y()));    // Right eye right corner
    image_points.push_back(cv::Point2d(d.part(MOUTH_LEFT_CORNER).x(), d.part(MOUTH_LEFT_CORNER).y()));    // Left Mouth corner
    image_points.push_back(cv::Point2d(d.part(MOUTH_RIGHT_CORNER).x(), d.part(MOUTH_RIGHT_CORNER).y()));    // Right mouth corner
    return image_points;
}

cv::Mat HeadTracker::GetCameraMatrix(float focal_length, cv::Point2d center)
{
    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0, center.x, 0, focal_length, center.y, 0, 0, 1);
    return camera_matrix;
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


    cv::Mat imgBGRA(rows, cols, CV_8UC4, bmpData, 0);
    cv::Mat imgBGR(rows, cols, CV_8UC3);
    cv::cvtColor(imgBGRA, imgBGR, CV_BGRA2BGR, 3);

    dlib::cv_image<bgr_pixel> img(imgBGR);

    std::vector<dlib::rectangle> faces = m_faceDetector(img);
    if (faces.size() <= 0)
    {
        return nullptr;
    }

    full_object_detection shape = m_shapePredictor(img, faces[0]);

    cv::rectangle(imgBGR, cv::Point(faces[0].left(), faces[0].top()), cv::Point(faces[0].bottom(), faces[0].right()), cv::Scalar(0, 255, 0));
    for (unsigned long i = 0; i < shape.num_parts(); i++)
    {
        auto dpt = shape.part(i);
        cv::circle(imgBGR, cv::Point(dpt.x(), dpt.y()), 2, cv::Scalar(0, 0, 255));
    }

    // calculate head pose
    auto imagePoints = GetImagePoints(shape);
    auto cameraMatrix = GetCameraMatrix(cols, cv::Point2d(cols / 2, rows / 2));
    cv::Mat rotationVector;
    cv::Mat rotationMatrix;
    cv::Mat translationVector;

    cv::Mat distCoeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type);
    cv::solvePnP(m_modelPoints, imagePoints, cameraMatrix, distCoeffs, rotationVector, translationVector);

    std::vector<cv::Point3d> noseEndPoint3D;
    std::vector<cv::Point2d> noseEndPoint2D;
    noseEndPoint3D.push_back(cv::Point3d(0, 0, 1000.0));

    cv::projectPoints(noseEndPoint3D, rotationVector, translationVector, cameraMatrix, distCoeffs, noseEndPoint2D);
    // cv::Point2d projected_point = find_projected_point(rotation_matrix, translation_vector, camera_matrix, cv::Point3d(0,0,1000.0));
    cv::line(imgBGR, imagePoints[0], noseEndPoint2D[0], cv::Scalar(255, 0, 0), 2);
    //  cv::line(im,image_points[0], projected_point, cv::Scalar(0,0,255), 2);

    cv::cvtColor(imgBGR, imgBGRA, CV_BGR2BGRA, 4);

    auto result = ref new HeadTrackerResult();
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
