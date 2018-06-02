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
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    /// SourceGroupCollection will setup device watcher to monitor
    /// SourceGroup devices enabled or disabled from the system.
    m_groupCollection = ref new SourceGroupCollection(Dispatcher);
    GroupComboBox->ItemsSource = m_groupCollection->FrameSourceGroups;
}

void MainPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
    delete m_groupCollection;
    StopReaderAsync();
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

    delete m_mediaCapture.Get();
    m_mediaCapture = nullptr;
}

task<void> MainPage::GroupComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    co_await StopReaderAsync();
    DisposeMediaCapture();
    auto group = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    if (group != nullptr)
    {
        auto initialized = co_await TryInitializeCaptureAsync();
        SourceComboBox->ItemsSource = group->SourceInfos;
        SourceComboBox->SelectedIndex = 0;
    }
}

task<void> MainPage::SourceComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    co_await StopReaderAsync();
    if (SourceComboBox->SelectedItem != nullptr)
    {
        co_await StartReaderAsync();
        auto formats = ref new Vector<FrameFormatModel^>();
        if (m_mediaCapture != nullptr && m_source != nullptr)
        {
            for (auto format : m_source->SupportedFormats)
            {
                if (FrameRenderer::GetSubtypeForFrameReader(m_source->Info->SourceKind, format) != nullptr)
                {
                    formats->Append(ref new FrameFormatModel(format));
                }
            }
        }
        FormatComboBox->ItemsSource = formats;
    }
}

task<void> MainPage::FormatComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e)
{
    auto format = dynamic_cast<FrameFormatModel^>(FormatComboBox->SelectedItem);
    co_await ChangeMediaFormatAsync(format);
}

task<void> MainPage::StartButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    co_await StartReaderAsync();
}

task<void> MainPage::StopButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    co_await StopReaderAsync();
}


task<void> MainPage::StartReaderAsync()
{
    co_await CreateReaderAsync();
    auto result = co_await m_reader->StartAsync();
    if (result == MediaFrameReaderStartStatus::Success)
    {
        m_streaming = true;
    
    }
    return co_await UpdateButtonStateAsync();
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
    co_await UpdateButtonStateAsync();

    DisposeMediaCapture();
}

task<void> MainPage::CreateReaderAsync()
{
    auto success = co_await TryInitializeCaptureAsync();
    if (success)
    {
        UpdateFrameSource();
        if (m_source != nullptr)
        {
            // Ask the MediaFrameReader to use a subtype that we can render.
            String^ requestedSubtype = FrameRenderer::GetSubtypeForFrameReader(m_source->Info->SourceKind, m_source->CurrentFormat);
            if (requestedSubtype != nullptr)
            {
                m_reader = co_await m_mediaCapture->CreateFrameReaderAsync(m_source, requestedSubtype);
                m_frameArrivedToken = m_reader->FrameArrived +=
                        ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(
                            this, &MainPage::Reader_FrameArrived);
                    //m_logger->Log("Reader created on source: " + m_source->Info->Id);
            }
        }
    }
}

task<bool> MainPage::TryInitializeCaptureAsync()
{
    if (m_mediaCapture != nullptr)
    {
        co_await task_from_result(true);
    }

    auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    if (groupModel == nullptr)
    {
        co_await task_from_result(false);
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

        //m_logger->Log("Successfully initialized MediaCapture for " + groupModel->DisplayName);
    }
    catch (Exception^ exception)
    {
        //m_logger->Log("Failed to initialize media capture: " + exception->Message);
        DisposeMediaCapture();
        return false;
    }
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
}

task<void> MainPage::ChangeMediaFormatAsync(FrameFormatModel^ format)
{
    if (m_source == nullptr)
    {
        //m_logger->Log("Unable to set format when source is not set.");
        co_await task_from_result();
    }

    // Do nothing if no format was selected, or if the selected format is the same as the current one.
    if (format == nullptr || format->HasSameFormat(m_source->CurrentFormat))
    {
        co_await task_from_result();
    }

    co_await m_source->SetFormatAsync(format->Format);
}

void MainPage::Reader_FrameArrived(MediaFrameReader^ reader, MediaFrameArrivedEventArgs^ args)
{
    // TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
    // This can return null if there is no such frame, or if the reader is not in the
    // "Started" state. The latter can occur if a FrameArrived event was in flight
    // when the reader was stopped.
    if (MediaFrameReference^ frame = reader->TryAcquireLatestFrame())
    {
        m_frameRenderer->ProcessFrame(frame);
    }
}
