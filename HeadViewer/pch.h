//
// pch.h
//

#pragma once

#include <collection.h>
#include <ppltasks.h>

#include <experimental\resumable>
#include <pplawait.h>

#include "App.xaml.h"

using namespace Platform; 
using namespace Windows::Foundation;
using namespace concurrency;

#include <strsafe.h>
private class Debug
{
public:
    static void Write(wchar_t *format, ...)
    {
        wchar_t message[1024];
        va_list args;
        va_start(args, format);
        StringCchVPrintf(message, 1024, format, args);
        OutputDebugString(message);
    }
    static void WriteLine(wchar_t *format, ...)
    {
        wchar_t message[1024];
        va_list args;
        va_start(args, format);
        StringCchVPrintf(message, 1024, format, args);
        OutputDebugString(message);
        OutputDebugString(L"\n");
    }
};

#include <opencv2/opencv.hpp>

inline void DebugPrintMatrix(wchar_t *prefix, cv::Mat m)
{
    Debug::WriteLine(L"%s", prefix);
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols - 1; j++)
        {
            if (CV_MAT_TYPE(m.type()) == CV_32FC1)
            {
                Debug::Write(L"%g, ", m.at<float>(i, j));
            }
            else if (CV_MAT_TYPE(m.type()) == CV_64FC1)
            {
                Debug::Write(L"%g, ", m.at<double>(i, j));
            }


        }
        if (CV_MAT_TYPE(m.type()) == CV_32FC1)
        {
            Debug::Write(L"%g\n", m.at<float>(i, m.cols - 1));
        }
        else if (CV_MAT_TYPE(m.type()) == CV_64FC1)
        {
            Debug::Write(L"%g\n", m.at<double>(i, m.cols - 1));
        }
    }
}
