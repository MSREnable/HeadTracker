//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the Microsoft Public License.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "LookupTable.h"
#include "HeadTracker.h"
#include <MemoryBuffer.h>

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Capture;
using namespace Windows::Media::Capture::Frames;

namespace HeadViewer
{

    // Function type used to map a scanline of pixels to an alternate pixel format.
    typedef std::function<void(int, byte*, byte*)> TransformScanline;

    private ref class FrameReader sealed
    {
    public:
        FrameReader();

    public: // Public methods.
        IAsyncOperation<MediaCapture^>^ TryInitializeMediaCaptureAsync(MediaFrameSourceGroup^ sourceGroup);

        IAsyncOperation<IVectorView<MediaFrameFormat^>^>^ GetSupportedFormats(MediaFrameSourceGroup^ sourceGroup, MediaFrameSourceInfo^ sourceInfo);
            
        IAsyncOperation<bool>^ StartStreamingAsync(MediaFrameSourceGroup^ sourceGroup, 
                                                   MediaFrameSourceInfo^ sourceInfo, 
                                                   MediaFrameFormat^ frameFormat,
                                                   TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>^ frameArrivedEvent);
        IAsyncOperation<bool>^ StopStreamingAsync();

        /// <summary>
        /// Determines the subtype to request from the MediaFrameReader that will result in
        /// a frame that can be rendered by ConvertToDisplayableImage.
        /// </summary>
        /// <returns>Subtype string to request, or null if subtype is not renderable.</returns>
        static Platform::String^ GetSubtypeForFrameReader(
            Windows::Media::Capture::Frames::MediaFrameSourceKind kind,
            Windows::Media::Capture::Frames::MediaFrameFormat^ format);

        property bool UsePseudoColorForInfrared;
        property bool ShowFaceLandmarks;
        property bool IsStreaming;

        /// <summary>
        /// Converts the input frame to BGRA8 premultiplied alpha format and returns the result.
        /// Returns nullptr if the input frame cannot be converted BGRA8 premultiplied alpha.
        /// </summary>
        Windows::Graphics::Imaging::SoftwareBitmap^ ConvertToDisplayableImage(
            Windows::Media::Capture::Frames::VideoMediaFrame^ inputFrame);

    private: // Private instance methods.
        /// <summary>
        /// Transforms pixels of inputBitmap to an output bitmap using the supplied pixel transformation method.
        /// Returns nullptr if translation fails.
        /// </summary>
        Windows::Graphics::Imaging::SoftwareBitmap^ TransformBitmap(
            Windows::Graphics::Imaging::SoftwareBitmap^ inputBitmap,
            TransformScanline pixelTransformation);


    private: // Private instance methods.
        /// <summary>
        /// Keep presenting the m_backBuffer until there are no more.
        /// </summary>
        Concurrency::task<bool> StartStreamingInternalAsync(
            MediaFrameSourceGroup^ sourceGroup,
            MediaFrameSourceInfo^ sourceInfo,
            MediaFrameFormat^ frameFormat,
            TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>^ frameArrivedEvent);
        Concurrency::task<bool> StopStreamingInternalAsync();

    private: // Private data.
        Windows::UI::Xaml::Controls::Image^ m_imageElement;
        Windows::Graphics::Imaging::SoftwareBitmap^ m_backBuffer;
        bool m_taskRunning = false;
        HeadTracker^ m_headTracker;

        Agile<Windows::Media::Capture::MediaCapture^> m_mediaCapture;

        Windows::Media::Capture::Frames::MediaFrameSource^ m_source;
        Windows::Media::Capture::Frames::MediaFrameReader^ m_reader;
        
        Windows::Foundation::EventRegistrationToken m_frameArrivedToken;
    };
} // HeadViewer