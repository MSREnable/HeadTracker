//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "FrameSourceViewModels.h"

using namespace HeadViewer;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
    m_frameRenderer = ref new FrameRenderer(PreviewImage);
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    /// SourceGroupCollection will setup device watcher to monitor
    /// SourceGroup devices enabled or disabled from the system.
    m_groupCollection = ref new SourceGroupCollection(Dispatcher);
    GroupComboBox->ItemsSource = m_groupCollection->FrameSourceGroups;
    DeserializeFaceLandmarkDataAsync();
}

void MainPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
    HandleNavigatedFrom();
}

task<void> MainPage::HandleNavigatedFrom()
{
    delete m_groupCollection;
    co_await StopReaderAsync();
    DisposeMediaCapture();
}

task<void> MainPage::UpdateButtonStateAsync()
{
    return create_task(Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]()
    {
        StartButton->IsEnabled = m_source != nullptr && !m_streaming;
        StopButton->IsEnabled = m_source != nullptr && m_streaming;
    })));
}

void MainPage::DisposeMediaCapture()
{
    FormatComboBox->ItemsSource = nullptr;
    SourceComboBox->ItemsSource = nullptr;

    m_source = nullptr;

    if (m_mediaCapture.Get())
    {
        delete m_mediaCapture.Get();
        m_mediaCapture = nullptr;
    }
}

void MainPage::GroupComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    OnGroupSelectionChanged();
}

void MainPage::SourceComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    OnSourceSelectionChanged();
}

void MainPage::FormatComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    OnFormatSelectionChanged();
}

task<void> MainPage::OnGroupSelectionChanged()
{
    co_await StopReaderAsync();
    DisposeMediaCapture();
    auto group = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    if (group == nullptr)
    {
        return;
    }

    auto initialized = co_await TryInitializeCaptureAsync();
    if (initialized)
    {
        SourceComboBox->ItemsSource = group->SourceInfos;
        SourceComboBox->SelectedIndex = 0;
    }
}

task<void> MainPage::OnSourceSelectionChanged()
{
    co_await StopReaderAsync();
    if (SourceComboBox->SelectedItem == nullptr)
    {
        return;
    }
    co_await StartReaderAsync();
    auto formats = ref new Vector<FrameFormatModel^>();
    if (m_mediaCapture == nullptr || m_source == nullptr)
    {
        return;
    }

    for (auto format : m_source->SupportedFormats)
    {
        if (FrameRenderer::GetSubtypeForFrameReader(m_source->Info->SourceKind, format) != nullptr)
        {
            formats->Append(ref new FrameFormatModel(format));
        }
    }

    FormatComboBox->ItemsSource = formats;
}

task<void> MainPage::OnFormatSelectionChanged()
{
    auto format = dynamic_cast<FrameFormatModel^>(FormatComboBox->SelectedItem);
    co_await ChangeMediaFormatAsync(format);
}

void MainPage::StartButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    StartReaderAsync();
}

void MainPage::StopButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    StopReaderAsync();
}

task<void> MainPage::StartReaderAsync()
{
    co_await CreateReaderAsync();
    if (m_reader == nullptr || m_streaming)
    {
        return;
    }

    auto result = co_await m_reader->StartAsync();

    if (result == MediaFrameReaderStartStatus::Success)
    {
        m_streaming = true;
        UpdateButtonStateAsync();
    }
}

task<void> MainPage::StopReaderAsync()
{
    m_streaming = false;
    if (m_reader != nullptr)
    {
        co_await m_reader->StopAsync();
        m_reader->FrameArrived -= m_frameArrivedToken;
        m_reader = nullptr;
        //m_logger->Log("Reader stopped");
    }
    UpdateButtonStateAsync();
}

task<void> MainPage::CreateReaderAsync()
{
    auto initialized = co_await TryInitializeCaptureAsync();
    if (!initialized)
    {
        return;
    }

    UpdateFrameSource();

    if (m_source == nullptr)
    {
        return;
    }
    // Ask the MediaFrameReader to use a subtype that we can render.
    String^ requestedSubtype = FrameRenderer::GetSubtypeForFrameReader(m_source->Info->SourceKind, m_source->CurrentFormat);
    if (requestedSubtype == nullptr)
    {
        return;
    }

    m_reader = co_await m_mediaCapture->CreateFrameReaderAsync(m_source, requestedSubtype);
    m_frameArrivedToken = m_reader->FrameArrived +=
            ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(
                this, &MainPage::Reader_FrameArrived);
    //m_logger->Log("Reader created on source: " + m_source->Info->Id);
}

void MainPage::UpdateFrameSource()
{
    auto info = dynamic_cast<FrameSourceInfoModel^>(SourceComboBox->SelectedItem);
    if (m_mediaCapture != nullptr && info != nullptr && info->SourceGroup != nullptr)
    {
        auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
        if (groupModel == nullptr || groupModel->Id != info->SourceGroup->Id)
        {
            SourceComboBox->SelectedItem = nullptr;
            return;
        }
        if (m_source == nullptr || m_source->Info->Id != info->SourceInfo->Id)
        {
            if (m_mediaCapture->FrameSources->HasKey(info->SourceInfo->Id))
            {
                m_source = m_mediaCapture->FrameSources->Lookup(info->SourceInfo->Id);
            }
            else
            {
                m_source = nullptr;
            }
        }
    }
    else
    {
        m_source = nullptr;
    }

    bool enableIrOptions = (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared);
    UsePseudoColor->IsEnabled = enableIrOptions;
    OddFrames->IsEnabled = enableIrOptions;
    EvenFrames->IsEnabled = enableIrOptions;
}

task<bool> MainPage::TryInitializeCaptureAsync()
{
    if (m_mediaCapture != nullptr)
    {
        co_return true;
    }

    auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    if (groupModel == nullptr)
    {
        co_return false;
    }

    // Create a new media capture object.
    m_mediaCapture = ref new MediaCapture();

    auto settings = ref new MediaCaptureInitializationSettings();

    // Select the source we will be reading from.
    settings->SourceGroup = groupModel->SourceGroup;

    // This media capture has exclusive control of the source.
    settings->SharingMode = MediaCaptureSharingMode::ExclusiveControl;

    // Set to CPU to ensure frames always contain CPU SoftwareBitmap images,
    // instead of preferring GPU D3DSurface images.
    settings->MemoryPreference = MediaCaptureMemoryPreference::Cpu;

    // Capture only video. Audio device will not be initialized.
    settings->StreamingCaptureMode = StreamingCaptureMode::Video;

    // Initialize MediaCapture with the specified group.
    // This must occur on the UI thread because some device families
    // (such as Xbox) will prompt the user to grant consent for the
    // app to access cameras.
    // This can raise an exception if the source no longer exists,
    // or if the source could not be initialized.
    try
    {
        co_await m_mediaCapture->InitializeAsync(settings);

        Debug::WriteLine(L"Successfully initialized MediaCapture for %s", groupModel->DisplayName->Data());
        co_return true;
    }
    catch (Exception^ exception)
    {
        Debug::WriteLine(L"Failed to initialize media capture: %s", exception->Message->Data());
        DisposeMediaCapture();
        co_return false;
    }
}

task<void> MainPage::ChangeMediaFormatAsync(FrameFormatModel^ format)
{
    if (m_source == nullptr)
    {
        //m_logger->Log("Unable to set format when source is not set.");
        co_return;
    }

    // Do nothing if no format was selected, or if the selected format is the same as the current one.
    if (format == nullptr || format->HasSameFormat(m_source->CurrentFormat))
    {
        co_return;
    }

    m_source->SetFormatAsync(format->Format);
}

void MainPage::Reader_FrameArrived(MediaFrameReader^ reader, MediaFrameArrivedEventArgs^ args)
{
    m_frameNum++;

    bool processFrame = (((m_processEvenFrames) && (m_frameNum % 2 == 0)) || ((m_processOddFrames) && (m_frameNum % 2 == 1)));
    bool usingIr = (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared);

    if (usingIr && !processFrame)
    {
        return;
    }

    // TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
    // This can return null if there is no such frame, or if the reader is not in the
    // "Started" state. The latter can occur if a FrameArrived event was in flight
    // when the reader was stopped.

    if (MediaFrameReference^ frame = reader->TryAcquireLatestFrame())
    {
        m_frameRenderer->ProcessFrame(frame);
    }


}

void HeadViewer::MainPage::UsePseudoColor_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    {
        m_frameRenderer->UsePseudoColorForInfrared = UsePseudoColor->IsEnabled && UsePseudoColor->IsOn;
    }
}


void HeadViewer::MainPage::OddFrames_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    {
        m_processOddFrames = OddFrames->IsEnabled && OddFrames->IsOn;
    }
}

void HeadViewer::MainPage::EvenFrames_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    {
        m_processEvenFrames = EvenFrames->IsEnabled && EvenFrames->IsOn;
    }
}

task<void> MainPage::DeserializeFaceLandmarkDataAsync()
{
    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> m_shapePredictor;
    co_return;
}
