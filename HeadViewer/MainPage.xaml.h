//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "FrameSourceViewModels.h"
#include "FrameRenderer.h"

using namespace Windows::Devices::Enumeration;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
namespace HeadViewer
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

    protected:
        virtual void OnNavigatedTo(NavigationEventArgs^ e) override;
        virtual void OnNavigatedFrom(NavigationEventArgs^ e) override;

    private:
        void GroupComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e);
        void SourceComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e);
        void FormatComboBox_SelectionChanged(Object^ sender, SelectionChangedEventArgs^ e);
        void StopButton_Click(Object^ sender, RoutedEventArgs^ e);
        void StartButton_Click(Object^ sender, RoutedEventArgs^ e);

    private:
        void DisposeMediaCapture();
        void UpdateFrameSource();

        task<void> HandleNavigatedFrom();
        task<void> OnGroupSelectionChanged();
        task<void> OnSourceSelectionChanged();
        task<void> OnFormatSelectionChanged();
        task<void> StartReaderAsync();
        task<void> StopReaderAsync();
        task<void> CreateReaderAsync();
        task<bool> TryInitializeCaptureAsync();
        task<void> ChangeMediaFormatAsync(FrameFormatModel^ format);
        task<void> UpdateButtonStateAsync();

        void Reader_FrameArrived(Windows::Media::Capture::Frames::MediaFrameReader^ reader, Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs^ args);


    private:
        // Whether or not we are currently streaming.
        bool m_streaming = false;

        Agile<Windows::Media::Capture::MediaCapture^> m_mediaCapture;

        MediaFrameSource^ m_source;
        MediaFrameReader^ m_reader;
        FrameRenderer^ m_frameRenderer;

        SourceGroupCollection^ m_groupCollection;
        //SimpleLogger^ m_logger;

        Windows::Foundation::EventRegistrationToken m_frameArrivedToken;

    };
}
