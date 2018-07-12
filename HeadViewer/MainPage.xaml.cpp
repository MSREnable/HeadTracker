//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "CalibrationPage.xaml.h"
#include "FrameSourceViewModels.h"

using namespace HeadViewer;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

#pragma optimize("", off)

MainPage::MainPage()
{
	InitializeComponent();
    m_frameReader = ref new FrameReader(PreviewImage);
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
    HandleNavigatedFrom();
}

task<void> MainPage::HandleNavigatedFrom()
{
    delete m_groupCollection;
    m_frameReader->StopStreamingAsync();
}

task<void> MainPage::UpdateButtonStateAsync()
{
    return create_task(Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]()
    {
        StartButton->IsEnabled = !m_streaming;
        StopButton->IsEnabled = m_streaming;
    })));
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
    auto group = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    if (group != nullptr)
    {
        SourceComboBox->ItemsSource = group->SourceInfos;
        SourceComboBox->SelectedIndex = 0;
    }
    co_return;
}

task<void> MainPage::OnSourceSelectionChanged()
{
    auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    auto sourceModel = dynamic_cast<FrameSourceInfoModel^>(SourceComboBox->SelectedItem);

    if ((groupModel == nullptr) || (sourceModel == nullptr))
    {
        return;
    }
    
    auto supportedFormats = co_await m_frameReader->GetSupportedFormats(groupModel->SourceGroup, sourceModel->SourceInfo);
    auto formats = ref new Vector<FrameFormatModel^>();

    for (auto format : supportedFormats)
    {
        if (FrameReader::GetSubtypeForFrameReader(sourceModel->SourceInfo->SourceKind, format) != nullptr)
        {
            formats->Append(ref new FrameFormatModel(format));
        }
    }

    FormatComboBox->ItemsSource = formats;

    co_return;
}

task<void> MainPage::OnFormatSelectionChanged()
{
    auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    auto sourceModel = dynamic_cast<FrameSourceInfoModel^>(SourceComboBox->SelectedItem);
    auto formatModel = dynamic_cast<FrameFormatModel^>(FormatComboBox->SelectedItem);

    if ((groupModel == nullptr) || (sourceModel == nullptr) || (formatModel == nullptr))
    {
        co_return;
    }
    UpdateButtonStateAsync();
    co_return;
}

void MainPage::StartButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    StartReaderAsync();
}

void MainPage::StopButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    StopReaderAsync();
}

void MainPage::CalibrateButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(CalibrationPage::typeid));
}

task<void> MainPage::StartReaderAsync()
{
    auto groupModel = dynamic_cast<FrameSourceGroupModel^>(GroupComboBox->SelectedItem);
    auto sourceModel = dynamic_cast<FrameSourceInfoModel^>(SourceComboBox->SelectedItem);
    auto formatModel = dynamic_cast<FrameFormatModel^>(FormatComboBox->SelectedItem);

    if ((groupModel == nullptr) || (sourceModel == nullptr) || (formatModel == nullptr))
    {
        co_return;
    }
    auto group = groupModel->SourceGroup;
    auto source = sourceModel->SourceInfo;
    auto format = formatModel->Format;
    auto frameHandler = ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(this, &MainPage::Reader_FrameArrived);

    auto result = co_await m_frameReader->StartStreamingAsync(group, source, format, frameHandler);

    if (result)
    {
        m_streaming = true;
        UpdateButtonStateAsync();
    }
}

task<void> MainPage::StopReaderAsync()
{
    m_streaming = false;
    co_await m_frameReader->StopStreamingAsync();
    UpdateButtonStateAsync();
}

void MainPage::Reader_FrameArrived(MediaFrameReader^ reader, MediaFrameArrivedEventArgs^ args)
{
    // TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
    // This can return null if there is no such frame, or if the reader is not in the
    // "Started" state. The latter can occur if a FrameArrived event was in flight
    // when the reader was stopped.
    MediaFrameReference^ frame = reader->TryAcquireLatestFrame();
    if (frame == nullptr)
    {
        return;
    }

    m_frameNum++;

    //bool processFrame = (((m_processEvenFrames) && (m_frameNum % 2 == 0)) || ((m_processOddFrames) && (m_frameNum % 2 == 1)));
    //bool usingIr = (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared);

    //if (usingIr && !processFrame)
    //{
    //    return;
    //}

    auto bitmap = m_frameReader->ConvertToDisplayableImage(frame->VideoMediaFrame);

    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, bitmap]()
    {
        SoftwareBitmapSource^ imageSource = dynamic_cast<SoftwareBitmapSource^>(PreviewImage->Source);
        imageSource->SetBitmapAsync(bitmap);
        //RotationX->Text = result->RotationX.ToString();
        //RotationY->Text = result->RotationY.ToString();
        //RotationZ->Text = result->RotationZ.ToString();
        //TranslationX->Text = result->TranslationX.ToString();
        //TranslationY->Text = result->TranslationY.ToString();
        //TranslationZ->Text = result->TranslationZ.ToString();
    }));
}

void HeadViewer::MainPage::ShowFaceLandmarks_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_frameReader->ShowFaceLandmarks = ShowFaceLandmarks->IsEnabled && ShowFaceLandmarks->IsOn;
}

void HeadViewer::MainPage::UsePseudoColor_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    //if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    //{
    //    m_frameReader->UsePseudoColorForInfrared = UsePseudoColor->IsEnabled && UsePseudoColor->IsOn;
    //}
}


void HeadViewer::MainPage::OddFrames_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    //if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    //{
    //    m_processOddFrames = OddFrames->IsEnabled && OddFrames->IsOn;
    //}
}

void HeadViewer::MainPage::EvenFrames_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    //if ((m_source != nullptr) && (m_source->Info->SourceKind == MediaFrameSourceKind::Infrared))
    //{
    //    m_processEvenFrames = EvenFrames->IsEnabled && EvenFrames->IsOn;
    //}
}
#pragma optimize("", on)
