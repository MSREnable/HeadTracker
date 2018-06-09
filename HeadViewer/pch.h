﻿//
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