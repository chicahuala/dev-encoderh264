// EncoderH264Console.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <sstream>
#include <iostream>
#include <string>

#include <atlbase.h>
#include <atlcom.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <MFidl.h>
#include <Mfreadwrite.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <array>
#include <cassert>
#include <Windows.h>
#include <mfapi.h>
#include <atlbase.h>

#include <mfidl.h>
#include <Mfreadwrite.h>
#include <Ks.h>
#include <Codecapi.h>
#include <Dshow.h>
#include <mferror.h>
#include <comdef.h>  // COM smart-ptr with "Ptr" suffix

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Strmiids.lib")

#include "VideoEncoder.h"

ICodecAPI *pCodecAPI = nullptr;

HRESULT InitCodec()
    {
	// ICodecAPI* pCodecAPI;

	if (SUCCEEDED(m_pMpeg2Muxer->QueryInterface(IID_ICodecAPI, (void**)&pCodecAPI)))
		{
		_variant_t _value;
		if (m_lVideoBitrate > 0)
			{
			_value = m_lVideoBitrate;
			if (FAILED(pCodecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &_value)))
				{
				m_lVideoBitrate = 0;
				}
			}
		if (m_lVideoBitrate <= 0)
			{
			if (SUCCEEDED(pCodecAPI->GetDefaultValue(&CODECAPI_AVEncCommonMeanBitRate, &_value)))
				{
				m_lVideoBitrate = _value;
				pCodecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &_value);
				}
			}
		pCodecAPI->Release();
		}

	return NOERROR;
    }

int main()
{
    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
