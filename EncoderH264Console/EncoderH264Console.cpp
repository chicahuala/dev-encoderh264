// EncoderH264Console.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>

#include <Gdiplus.h>
#include <gdiplusheaders.h>

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>
namespace fs = std::filesystem;

#include <codecvt>
#include <locale> 

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

ICodecAPI* pCodecAPI = nullptr;

//HRESULT InitCodec()
//	{
//	// ICodecAPI* pCodecAPI;
//
//	if (SUCCEEDED(m_pMpeg2Muxer->QueryInterface(IID_ICodecAPI, (void**)&pCodecAPI)))
//		{
//		_variant_t _value;
//		if (m_lVideoBitrate > 0)
//			{
//			_value = m_lVideoBitrate;
//			if (FAILED(pCodecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &_value)))
//				{
//				m_lVideoBitrate = 0;
//				}
//			}
//		if (m_lVideoBitrate <= 0)
//			{
//			if (SUCCEEDED(pCodecAPI->GetDefaultValue(&CODECAPI_AVEncCommonMeanBitRate, &_value)))
//				{
//				m_lVideoBitrate = _value;
//				pCodecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &_value);
//				}
//			}
//		pCodecAPI->Release();
//		}
//
//	return NOERROR;
//	}

/** Convenience function to create a locally implemented COM instance without the overhead of CoCreateInstance.
The COM class does not need to be registred for construction to succeed. However, lack of registration can
cause problems if transporting the class out-of-process. */
template <class T>
static CComPtr<T> CreateLocalInstance()
	{
	// create an object (with ref. count zero)
	CComObject<T>* tmp = nullptr;
	if (FAILED(CComObject<T>::CreateInstance(&tmp)))
		throw std::runtime_error("CreateInstance failed");

	// move into smart-ptr (will incr. ref. count to one)
	return CComPtr<T>(static_cast<T*>(tmp));
	}

/******************************************************************************
 * Checks to see if a directory exists. Note: This method only checks the
 * existence of the full path AND if path leaf is a dir.
 *
 * @return  >0 if dir exists AND is a dir,
 *           0 if dir does not exist OR exists but not a dir,
 *          <0 if an error occurred (errno is also set)
 *****************************************************************************/
int dirExists(const char* const path)
	{
	struct stat info;

	int statRC = stat(path, &info);
	if (statRC != 0)
		{
		if (errno == ENOENT) { return 0; } // something along the path does not exist
		if (errno == ENOTDIR) { return 0; } // something in path prefix is not a dir
		return -1;
		}

	return (info.st_mode & S_IFDIR) ? 1 : 0;
	}

bool DirExist(const char* const path)
	{
	return fs::exists(path);
	}

//std::wstring stringToWstring(const char* utf8Bytes)
//	{
//	//setup converter
//	using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
//	std::wstring_convert<convert_type, typename std::wstring::value_type> converter;
//
//	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
//	return converter.from_bytes(utf8Bytes);
//	}

int StringToWString(std::wstring& ws, const std::string& s)
	{
	std::wstring wsTmp(s.begin(), s.end());

	ws = wsTmp;

	return 0;
	}

void UsagePrint(const char* const mensajeExtra)
	{
	if (mensajeExtra)
		{
		if (strlen(mensajeExtra) > 0)
			{
			std::cout << "error: " << mensajeExtra << std::endl << std::endl;
			}
		}

	std::cout << "Usage  : EncoderH264Console.exe directorioDeFiles outFileNameSinExtension" << std::endl;
	std::cout << "Example: EncoderH264Console.exe d:\\imagenes d:\\videos\\fileH264" << std::endl << std::flush;
	}

int main(int argc, char* argv[])
	{
	std::cout << "EncoderH264Console: test app de h264 encoder." << std::endl;
	if (argc < 3)
		{
		UsagePrint("Faltan parametros");
		return -1;
		}

	if (!fs::exists(argv[1]))
		{
		UsagePrint("No existe el directorio de imagenes.");
		return -2;
		}

	if (!fs::exists(argv[2]))
		{
		UsagePrint("No existe el directorio de destino.");
		return -3;
		}

	std::string path = argv[1];
	if (path.back() != '\\')
		{
		path += '\\';
		}

	path += "*.jpg";

	std::wstring w_path;
	StringToWString(w_path, path);

	//MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, argv[1], -1, );
	//WideCharToMultiByte();

	std::vector<std::wstring> vs;
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	hFind = FindFirstFile(w_path.c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
		{
		do {
			vs.push_back(FindFileData.cFileName);
			} while (FindNextFile(hFind, &FindFileData));
			FindClose(hFind);
		}

	std::cout << "count:" << vs.size() << "\n";
	for (auto file : vs)
		{
		std::wcout << file << std::endl;

		Image* pImg = nullptr;
		// pImg = FromFile(IN const WCHAR * filename, IN BOOL        useEmbeddedColorManagement);
		}

#if (false)
	// create H.264/MPEG4 encoder
	std::cout << "Starting web server to stream window " << std::hex << win_handle << ". Please connect with a web browser on port " << port << " to receive the stream" << std::endl;
	// auto ds = CreateLocalInstance<DataStream>();
	auto ws = CreateLocalInstance<WebStream>();
	ws->SetPortAndWindowHandle(port, win_handle); // blocking call
	std::cout << "Connecting to client..." << std::endl;

	const unsigned int FPS = 25;
#ifdef ENABLE_FFMPEG
	VideoEncoderFF encoder(dims, FPS, ws);
#else
	VideoEncoderMF encoder(dims, FPS, ws);
	// VideoEncoderMF encoder(dims, FPS, ds);
#endif

	// encode & transmit frames
	HRESULT hr = S_OK;
	while (SUCCEEDED(hr))
		{
		hr = EncodeFrame(encoder, wnd_dc);

		// synchronize framerate
		Sleep(1000 / FPS);
		}
#endif
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
