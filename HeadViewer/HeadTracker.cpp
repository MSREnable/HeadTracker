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

// average of mean male and female IPD computed from https://en.wikipedia.org/wiki/Pupillary_distance
const float AVERAGE_IPD = 62.85;

const float INITIAL_FOCAL_LENGTH_MM = 1.5;

////#pragma optimize("", off)

static float GetApproxFocalLength(int fovDegrees, int width, int height)
{
    // https://www.learnopencv.com/approximate-focal-length-for-webcams-and-cell-phone-cameras/

    // compute horizontal and vertical fov separately
    const float PI = 3.14159265;
    float diagFov = fovDegrees * PI / 180.0;
    float diag = sqrtf((width * width) + (height * height));
    float focalLength = diag / (2 * (tan(diagFov / 2)));
    return (float)focalLength;
}


HeadTracker::HeadTracker()
{
    deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    m_faceDetector = dlib::get_frontal_face_detector();
    InitializeModelPoints();
    InitializeCalibrationPoints();
}

void HeadTracker::InitializeModelPoints()
{
    m_modelPoints.push_back(cv::Point3f(0.0, 0.0, 0.0));         // nose tip 
    m_modelPoints.push_back(cv::Point3f(0.0, -330.0, -65.0));    // chin
    m_modelPoints.push_back(cv::Point3f(-225.0, 170.0, -135.0));    // left corner of eye
    m_modelPoints.push_back(cv::Point3f(225.0, 170.0, -135.0));    // right corner of eye
    m_modelPoints.push_back(cv::Point3f(-150.0, -150.0, -125.0));    // left corner of mouth
    m_modelPoints.push_back(cv::Point3f(150.0, -150.0, -125.0));     // right corner of mouth
}

std::vector<cv::Point2f> HeadTracker::GetImagePoints(full_object_detection &d)
{
    std::vector<cv::Point2f> image_points;
    image_points.push_back(cv::Point2f(d.part(NOSE_TIP).x(), d.part(NOSE_TIP).y()));    // Nose tip
    image_points.push_back(cv::Point2f(d.part(CHIN).x(), d.part(CHIN).y()));      // Chin
    image_points.push_back(cv::Point2f(d.part(LEFT_EYE_CORNER).x(), d.part(LEFT_EYE_CORNER).y()));    // Left eye left corner
    image_points.push_back(cv::Point2f(d.part(RIGHT_EYE_CORNER).x(), d.part(RIGHT_EYE_CORNER).y()));    // Right eye right corner
    image_points.push_back(cv::Point2f(d.part(MOUTH_LEFT_CORNER).x(), d.part(MOUTH_LEFT_CORNER).y()));    // Left Mouth corner
    image_points.push_back(cv::Point2f(d.part(MOUTH_RIGHT_CORNER).x(), d.part(MOUTH_RIGHT_CORNER).y()));    // Right mouth corner
    return image_points;
}

void HeadTracker::InitializeCalibrationPoints()
{
    _validCalibration = false;

    cv::Point3f calibPoints[] = {
    { 0, 0, 0 },{ 0, 1, 0 },{ 0, 2, 0 },{ 0, 3, 0 },{ 0, 4, 0 },{ 0, 5, 0 },{ 0, 6, 0 },
    { 1, 0, 0 },{ 1, 1, 0 },{ 1, 2, 0 },{ 1, 3, 0 },{ 1, 4, 0 },{ 1, 5, 0 },{ 1, 6, 0 },
    { 2, 0, 0 },{ 2, 1, 0 },{ 2, 2, 0 },{ 2, 3, 0 },{ 2, 4, 0 },{ 2, 5, 0 },{ 2, 6, 0 },
    { 3, 0, 0 },{ 3, 1, 0 },{ 3, 2, 0 },{ 3, 3, 0 },{ 3, 4, 0 },{ 3, 5, 0 },{ 3, 6, 0 },
    { 4, 0, 0 },{ 4, 1, 0 },{ 4, 2, 0 },{ 4, 3, 0 },{ 4, 4, 0 },{ 4, 5, 0 },{ 4, 6, 0 },
    { 5, 0, 0 },{ 5, 1, 0 },{ 5, 2, 0 },{ 5, 3, 0 },{ 5, 4, 0 },{ 5, 5, 0 },{ 5, 6, 0 },
    { 6, 0, 0 },{ 6, 1, 0 },{ 6, 2, 0 },{ 6, 3, 0 },{ 6, 4, 0 },{ 6, 5, 0 },{ 6, 6, 0 },
    };

    for (int i = 0; i < sizeof(calibPoints) / sizeof(cv::Point3f); i++)
    {
        auto pt = calibPoints[i] * 25.4;
        m_chessboardPoints.push_back(calibPoints[i]);
    }
}

float HeadTracker::ComputeIPDInPixels(dlib::full_object_detection &d)
{
    auto rightPupil1 = cv::Point2f((d.part(37).x() + d.part(40).x()) / 2, (d.part(37).y() + d.part(40).y()) / 2);
    auto rightPupil2 = cv::Point2f((d.part(38).x() + d.part(41).x()) / 2, (d.part(38).y() + d.part(41).y()) / 2);
    auto rightPupil = cv::Point2f((rightPupil1.x + rightPupil2.x) / 2, (rightPupil1.y + rightPupil2.y) / 2);

    auto leftPupil1 = cv::Point2f((d.part(43).x() + d.part(46).x()) / 2, (d.part(43).y() + d.part(46).y()) / 2);
    auto leftPupil2 = cv::Point2f((d.part(44).x() + d.part(47).x()) / 2, (d.part(44).y() + d.part(47).y()) / 2);
    auto leftPupil = cv::Point2f((leftPupil1.x + leftPupil2.x) / 2, (leftPupil1.y + leftPupil2.y) / 2);

    float ipd = sqrtf(((leftPupil.x - rightPupil.x)*(leftPupil.x - rightPupil.x)) + ((leftPupil.y - rightPupil.y) * (leftPupil.y - rightPupil.y)));

    return ipd;
}

cv::Mat HeadTracker::GetCameraMatrix(float focal_length, cv::Point2f center)
{
    cv::Mat camera_matrix = (cv::Mat_<float>(3, 3) << focal_length, 0, center.x, 0, focal_length, center.y, 0, 0, 1);
    return camera_matrix;
}

void HeadTracker::CalibrateCamera(cv::Mat &imgBGR)
{
    cv::Mat imgGray;
    cv::Mat corners;
    cv::cvtColor(imgBGR, imgGray, CV_BGR2GRAY);
    if (!cv::findChessboardCorners(imgGray, cv::Size(7, 7), corners))
    {
        return;
    }

    cv::TermCriteria criteria;
    cv::cornerSubPix(imgGray, corners, cv::Size(11, 11), cv::Size(-1, -1), criteria);
    cv::drawChessboardCorners(imgBGR, cv::Size(7, 7), corners, true);

    cv::Mat rotationVector, translationVector;
    std::vector<std::vector<cv::Point3f>> objectPoints;
    objectPoints.push_back(m_chessboardPoints);

    std::vector<std::vector<cv::Point2f>> imagePoints;
    imagePoints.push_back(corners);

    double error = cv::calibrateCamera(objectPoints, imagePoints, cv::Size(imgGray.cols, imgGray.rows), m_cameraMatrix, m_distortionCoefficients, rotationVector, translationVector);
    DebugPrintMatrix(L"Camera Matrix:", m_cameraMatrix);
    DebugPrintMatrix(L"Dist Coeffs", m_distortionCoefficients);
    Debug::WriteLine(L"Calibration error = %g\n", error);

    //float pixelsPerMm = m_cameraMatrix.at(0, 0) / INITIAL_FOCAL_LENGTH_MM;
    _validCalibration = true;
}


HeadTrackerResult^ HeadTracker::ProcessBitmap(Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap)
{
    int rows = softwareBitmap->PixelHeight;
    int cols = softwareBitmap->PixelWidth;
    BitmapBuffer^ bitmapBuffer = softwareBitmap->LockBuffer(BitmapBufferAccessMode::Read);
    auto reference = bitmapBuffer->CreateReference();
    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> bufferByteAccess;

    auto result = ref new HeadTrackerResult();

    HRESULT hr = reinterpret_cast<IInspectable*>(reference)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
    if (FAILED(hr))
    {
        return result;
    }

    BYTE* bmpData;
    UINT32 capacity;
    hr = bufferByteAccess->GetBuffer(&bmpData, &capacity);
    if (FAILED(hr))
    {
        return result;
    }


    cv::Mat imgBGRA(rows, cols, CV_8UC4, bmpData, 0);
    cv::Mat imgBGR(rows, cols, CV_8UC3);
    cv::cvtColor(imgBGRA, imgBGR, CV_BGRA2BGR, 3);

    dlib::cv_image<bgr_pixel> img(imgBGR);

    std::vector<dlib::rectangle> faces = m_faceDetector(img);
    if (faces.size() <= 0)
    {
        CalibrateCamera(imgBGR);
        cv::cvtColor(imgBGR, imgBGRA, CV_BGR2BGRA, 4);
        return result;
    }

    full_object_detection shape = m_shapePredictor(img, faces[0]);

    result->FaceRect = Windows::Foundation::Rect(faces[0].left(), faces[0].top(), faces[0].width(), faces[0].height());
    auto facePoints = ref new Vector<Windows::Foundation::Point>();
    result->FacePoints = facePoints->GetView();

    cv::rectangle(imgBGR, cv::Point(faces[0].left(), faces[0].top()), cv::Point(faces[0].right(), faces[0].bottom()), cv::Scalar(0, 255, 0));
    for (unsigned long i = 0; i < shape.num_parts(); i++)
    {
        auto dpt = shape.part(i);
        facePoints->Append(Windows::Foundation::Point(dpt.x(), dpt.y()));

        if ((i < 36) || (i > 47))
            continue;
        cv::circle(imgBGR, cv::Point(dpt.x(), dpt.y()), 2, cv::Scalar(0, 0, 255));
    }

    // calculate head pose
    cv::Mat rotationVector;
    cv::Mat rotationMatrix;
    cv::Mat translationVector;

    auto imagePoints = GetImagePoints(shape);

    if (!_validCalibration)
    {
        auto focalLength = GetApproxFocalLength(80, cols, rows);
        Debug::WriteLine(L"Focal length = %g\n", focalLength);

        m_cameraMatrix = GetCameraMatrix(focalLength, cv::Point2f(cols / 2, rows / 2));
        DebugPrintMatrix(L"Camera Matrix:", m_cameraMatrix);
        m_distortionCoefficients = cv::Mat::zeros(4, 1, cv::DataType<float>::type);

        std::vector<std::vector<cv::Point3f>> objectPoints;
        objectPoints.push_back(m_modelPoints);

        std::vector<std::vector<cv::Point2f>> calibImagePoints;
        calibImagePoints.push_back(imagePoints);

        cv::calibrateCamera(objectPoints, calibImagePoints, cv::Size(cols, rows), m_cameraMatrix, m_distortionCoefficients, rotationVector, translationVector, CALIB_USE_INTRINSIC_GUESS);
        Debug::WriteLine(L"Using Calibration");
        DebugPrintMatrix(L"Camera Matrix:", m_cameraMatrix);
        DebugPrintMatrix(L"Dist Coeffs", m_distortionCoefficients);
        DebugPrintMatrix(L"Rotation Vector", rotationVector);
        DebugPrintMatrix(L"Translation Vector", translationVector);
    }


    CvPoint3D32f positModelPoints[] = {
        { 0.0, 0.0, 0.0 },
        { 0.0, -330.0, -65.0 },
        { -225.0, 170.0, -135.0 },
        { 225.0, 170.0, -135.0 },
        { -150.0, -150.0, -125.0 },
        { 150.0, -150.0, -125.0 }
    };
    CvPoint2D32f positImagePoints[6];
    auto criteria = cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 100, 0.1f);
    for (int i = 0; i < 6; i++)
    {
        positImagePoints[i].x = imagePoints[i].x;
        positImagePoints[i].y = imagePoints[i].y;
    }

    float rot_mat[9];
    float trans_vec[3];
    auto positObject = cvCreatePOSITObject(positModelPoints, 6);
    cvPOSIT(positObject, positImagePoints, GetApproxFocalLength(80, cols, rows), criteria, rot_mat, trans_vec);
    cvReleasePOSITObject(&positObject);
    cv::Mat rotation_matrix = cv::Mat(3, 3, CV_32F, rot_mat);
    cv::Mat rotation_vector = cv::Mat(3, 1, CV_32F);
    cv::Mat translation_vector = cv::Mat(3, 1, CV_32F, trans_vec);
    Rodrigues(rotation_matrix, rotation_vector);
    Debug::WriteLine(L"Using POSIT");
    DebugPrintMatrix(L"Rotation Vector:", rotation_vector);
    Debug::WriteLine(L"Translation Vector: [%g, %g, %g]", trans_vec[0], trans_vec[1], trans_vec[2]);

    cv::solvePnP(m_modelPoints, imagePoints, m_cameraMatrix, m_distortionCoefficients, rotationVector, translationVector);
    Debug::WriteLine(L"Using solvePnP");
    DebugPrintMatrix(L"Camera Matrix:", m_cameraMatrix);
    DebugPrintMatrix(L"Dist Coeffs", m_distortionCoefficients);
    DebugPrintMatrix(L"Rotation Vector", rotationVector);
    DebugPrintMatrix(L"Translation Vector", translationVector);

    //float ipd = ComputeIPDInPixels(shape);
    //float focalLengthMM = m_cameraMatrix.at<double>(0, 0) * AVERAGE_IPD / ipd;
    //float userDistanceMM = translationVector.at<double>(2, 0) * AVERAGE_IPD / ipd;
    //Debug::WriteLine(L"focalLengthMM = %g, userDistanceMM=%g", focalLengthMM, userDistanceMM);

    std::vector<cv::Point3f> noseEndPoint3D;
    std::vector<cv::Point2f> noseEndPoint2D;
    //noseEndPoint3D.push_back(cv::Point3f(0, 0, translationVector.at<double>(2, 0) / 2));
    noseEndPoint3D.push_back(cv::Point3f(0, 0, 1000));

    cv::projectPoints(noseEndPoint3D, rotationVector, translationVector, m_cameraMatrix, m_distortionCoefficients, noseEndPoint2D);
    cv::line(imgBGR, imagePoints[0], noseEndPoint2D[0], cv::Scalar(255, 0, 0), 2);

    cv::projectPoints(noseEndPoint3D, rotation_vector, translation_vector, m_cameraMatrix, m_distortionCoefficients, noseEndPoint2D);
    cv::line(imgBGR, imagePoints[0], noseEndPoint2D[0], cv::Scalar(255, 255, 0), 2);

    cv::cvtColor(imgBGR, imgBGRA, CV_BGR2BGRA, 4);

    result->RotationX = rotationVector.at<double>(0, 0);
    result->RotationY = rotationVector.at<double>(1, 0);
    result->RotationZ = rotationVector.at<double>(2, 0);
    result->TranslationX = translationVector.at<double>(0, 0);
    result->TranslationY = translationVector.at<double>(1, 0);
    result->TranslationZ = translationVector.at<double>(2, 0);

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

#pragma optimize("", on)
