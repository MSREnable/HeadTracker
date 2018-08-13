//
// CalibrationPage.xaml.cpp
// Implementation of the CalibrationPage class
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "CalibrationPage.xaml.h"
#include "CalibrationPointViewer.xaml.h"

using namespace HeadViewer;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

#pragma optimize("", off)

CalibrationPage::CalibrationPage()
{
	InitializeComponent();
    m_curRow = 0;
    m_curCol = 0;

    m_calibrationTimer = ref new DispatcherTimer();
    TimeSpan ts;
    ts.Duration = 2 * 1000 * 1000 * 10;
    m_calibrationTimer->Interval = ts;
    m_calibrationTimer->Tick += ref new EventHandler<Platform::Object ^>(this, &CalibrationPage::OnCalibrationTimerTick);

    m_frameReader = ref new FrameReader();
    m_calibrationProcessor = ref new CalibrationProcessor();
}


void CalibrationPage::OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_calibrationTimer->Start();
}

void CalibrationPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    m_pageParams = dynamic_cast<CalibrationPageParams^>(e->Parameter);
    StartStreamingAsync(m_pageParams);
}

void CalibrationPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
    StopStreamingAsync();
}

task<void> CalibrationPage::StartStreamingAsync(CalibrationPageParams^ params)
{
    //m_faceDetector = co_await FaceDetector::CreateAsync();

    auto frameHandler = ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(this, &CalibrationPage::OnFrameArrived);
    co_await m_frameReader->StartStreamingAsync(params->SourceGroup, params->SourceInfo, params->FrameFormat, frameHandler);
}

task<void> CalibrationPage::StopStreamingAsync()
{
    m_faceDetector = nullptr;
    co_await m_frameReader->StopStreamingAsync();
}

void CalibrationPage::OnCalibrationTimerTick(Object ^sender, Object^ args)
{
    if ((m_curCol == 0) && (m_curRow == 0))
    {
        CalibrationDot->Visibility = Windows::UI::Xaml::Visibility::Visible;
        m_startCollecting = true;
    }

    CalibrationGrid->SetRow(CalibrationDot, m_curRow);
    CalibrationGrid->SetColumn(CalibrationDot, m_curCol);
    
    auto entry = ref new CalibrationEntry();

    if (m_curRow >= 5)
    {
        OnShowAllImages(nullptr, nullptr);
        return;
    }

    m_calibrationProcessor->CalibrationData->Append(ref new CalibrationEntry());

    m_curCol += 2;

    if (m_curCol >= 5)
    {
        m_curCol = 0;
        m_curRow += 2;
    }
}

void CalibrationPage::OnFrameArrived(MediaFrameReader^ reader, MediaFrameArrivedEventArgs^ args)
{
    if (!m_startCollecting)
    {
        return;
    }

    auto frame = reader->TryAcquireLatestFrame();
    if (frame == nullptr)
    {
        return;
    }

    auto bitmap = m_frameReader->ConvertToDisplayableImage(frame->VideoMediaFrame);

    auto curEntry = m_calibrationProcessor->CalibrationData->GetAt(m_calibrationProcessor->CalibrationData->Size - 1);
    if ((curEntry->X == 0) && (curEntry->Y == 0))
    {
        Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, curEntry]()
        {
            auto ttv = CalibrationDot->TransformToVisual(Window::Current->Content);
            auto point = ttv->TransformPoint(Point(0, 0));
            curEntry->X = point.X + (CalibrationDot->ActualWidth / 2);
            curEntry->Y = point.Y + (CalibrationDot->ActualHeight / 2);
        }));
    }
    curEntry->AppendBitmap(ref new SoftwareBitmapWrapper(bitmap));
}

void CalibrationPage::OnCloseCalibration(Object^ sender, RoutedEventArgs^ e)
{

}

void CalibrationPage::OnRepeatCalibration(Object^ sender, RoutedEventArgs^ e)
{
    CalibrationResults->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    m_calibrationTimer->Start();
    StartStreamingAsync(m_pageParams);
}

void CalibrationPage::OnShowAllImages(Object^ sender, RoutedEventArgs^ e)
{
    if (m_frameReader == nullptr)
    {
        return;
    }

    m_frameReader->StopStreamingAsync();
    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]()
    {
        m_calibrationTimer->Stop();
        CalibrationGrid->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        CalibrationResults->Visibility = Windows::UI::Xaml::Visibility::Visible;

        m_calibrationProcessor->ProcessCalibrationEntries();

        for (auto child : CalibrationImages->Children->GetView())
        {
            auto calibPointViewer = dynamic_cast<CalibrationPointViewer^>(child);
            int row = CalibrationImages->GetRow(calibPointViewer);
            int col = CalibrationImages->GetColumn(calibPointViewer);
            int index = (row * 3) + col;
            if (index >= m_calibrationProcessor->CalibrationData->Size)
            {
                continue;
            }
            calibPointViewer->FrameRate = m_pageParams->FrameFormat->FrameRate;
            calibPointViewer->CalibEntry = m_calibrationProcessor->CalibrationData->GetAt(index);
        }
    }));
}

void CalibrationPage::OnShowBestImage(Object^ sender, RoutedEventArgs^ e)
{
    for (auto child : CalibrationImages->Children->GetView())
    {
        auto calibPointViewer = dynamic_cast<CalibrationPointViewer^>(child);
        calibPointViewer->ShowBestImage();
    }
}

void CalibrationPage::OnShowFace(Object^ sender, RoutedEventArgs^ e)
{
    for (auto child : CalibrationImages->Children->GetView())
    {
        auto calibPointViewer = dynamic_cast<CalibrationPointViewer^>(child);
        calibPointViewer->ShowFace();
    }
}

void CalibrationPage::OnCapturedImageClick(Object^ sender, RoutedEventArgs^ e)
{
    auto source = dynamic_cast<Button^>(sender);
    auto tag = _wtoi(source->Tag->ToString()->Data());

}


#pragma optimize("", on)
