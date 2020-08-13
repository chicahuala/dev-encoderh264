// EncoderH264.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "EncoderH264.h"

#include <cstddef>
#include <string>
using namespace std;


// This is an example of an exported variable
ENCODERH264_API int nEncoderH264 = 0;

// This is an example of an exported function.
ENCODERH264_API int fnEncoderH264(void)
	{
	return 0;
	}

	// This is the constructor of a class that has been exported.
CEncoderH264::CEncoderH264()
	{
	return;
	}

extern "C"
	{
	typedef void(__stdcall* CompresionH264ResultCallback)(int iContext, bool KeyFrame, const byte* pFrameH264, int iFrameH264Len);
	typedef void(__stdcall* ScaleResultCallback)(int iContext, const byte* pFrameScaleY, const byte* pFrameScaleU, const byte* pFrameScaleV, int iWidth, int iHeight, int iStrideY, int iStrideUV);

	typedef void(__stdcall* OperationResultInfoCallback)(int iContext, int messageLen, const char *pMessage);
	}

// encode_h264_init inicializar el compresor h264
ENCODERH264_API int encode_h264_init(int iWidth, int iHeight, int modo, long long opResultCallback)
	{
	if (opResultCallback > 0)
		{
		//TCHAR mensaje[1024];
		//ZeroMemory(mensaje, sizeof(mensaje));
		//int len = wsprintf(mensaje, TEXT("encode_h264_init funcion no implementada."));
		string msg = "{encode_h264_init} funcion no implementada.";
		((OperationResultInfoCallback)opResultCallback)(0, msg.length(), msg.c_str());
		}

	return 0;
	}

// encode_frame_from_rgb32_to_h264 comprime frame to h264
ENCODERH264_API int encode_frame_from_rgb32_to_h264(int iContext, int frameNum, long long opResultCallback, byte* pImagen, int iImagenDataLen)
	{
	if (opResultCallback > 0)
		{
		//TCHAR mensaje[1024];
		//ZeroMemory(mensaje, sizeof(mensaje));
		//int len = wsprintf(mensaje, TEXT("encode_frame_from_rgb32_to_h264 funcion no implementada."));
		//((OperationResultInfoCallback)opResultCallback)(-1, len, &mensaje[0]);
		string msg = "{encode_frame_from_rgb32_to_h264} funcion no implementada.";
		((OperationResultInfoCallback)opResultCallback)(-1, msg.length(), msg.c_str());
		}

	return -1;
	}

// encode_h264_close close and free compresor h264
ENCODERH264_API int encode_h264_close(int iContext, long long opResultCallback)
	{
	if (opResultCallback > 0)
		{
		//TCHAR mensaje[1024];
		//ZeroMemory(mensaje, sizeof(mensaje));
		//int len = wsprintf(mensaje, TEXT("encode_h264_close funcion no implementada."));
		//((OperationResultInfoCallback)opResultCallback)(-1, len, &mensaje[0]);
		string msg = "{encode_h264_close} funcion no implementada.";
		((OperationResultInfoCallback)opResultCallback)(-1, msg.length(), msg.c_str());
		}

	return -1;
	}

