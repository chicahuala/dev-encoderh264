// EncoderH264.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"

#include <sstream>
#include <iostream>
#include <string>

#include <cstddef>

using namespace std;

#include "framework.h"
#include "EncoderH264.h"

#include "WebStream.hpp"

#include "DataStream.h"

#include "MemStream.h"

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

//// This is an example of an exported variable
//ENCODERH264_API int nEncoderH264 = 0;
//
//// This is an example of an exported function.
//ENCODERH264_API int fnEncoderH264(void)
//	{
//	return 0;
//	}
//
//// This is the constructor of a class that has been exported.
//CEncoderH264::CEncoderH264()
//	{
//	return;
//	}

extern "C"
	{
	typedef void(__stdcall* CompresionH264ResultCallback)(int iContext, bool KeyFrame, const byte* pFrameH264, int iFrameH264Len);
	typedef void(__stdcall* ScaleResultCallback)(int iContext, const byte* pFrameScaleY, const byte* pFrameScaleU, const byte* pFrameScaleV, int iWidth, int iHeight, int iStrideY, int iStrideUV);

	typedef void(__stdcall* OperationResultInfoCallback)(int iContext, int messageLen, const char* pMessage);
	}

bool bInicializacionGeneral = false;
bool bInicializando = false;
bool bOnShutdown = false;

int iContextsReserved = 0;
int iContextsActived = 0;

int bad_counter = 0;

typedef struct _tag_h264EncodeContext
	{
	bool contexto_en_uso;
	bool contexto_con_error;
	bool ok_critical_section;

	CRITICAL_SECTION context_local_cs;

	int coded_width, coded_height;

	VideoEncoderMF* pEncoder;

	DataStream* pDataStream;

	MemIMFByteStream* pMemIMFByteStream;

	WebStream* pWebStream;

	int iGenericBufferLength;
	TCHAR* pGenericBuffer;
	} h264EncodeContext;

h264EncodeContext* pH264EncodeContext = NULL;
CRITICAL_SECTION encodeCriticalSection;

#define max_number_of_contexts	16

// aqui se llega con la critical section tomada
int encoders_h264_close_forced(int iContext)
	{
	if (iContext >= max_number_of_contexts) return -1001;
	if (iContext < 0) return -1002;
	// if (bOnShutdown) return -1004;

	if (pH264EncodeContext[iContext].ok_critical_section)
		{
		EnterCriticalSection(&pH264EncodeContext[iContext].context_local_cs);		// solo nosotros podemos usar el contexto
		}
	else
		{
		bad_counter++;
		}

	if (pH264EncodeContext[iContext].ok_critical_section)
		{
		LeaveCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
		DeleteCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
		}

	// todo a zero, ya se puede usar el contexto
	ZeroMemory(&pH264EncodeContext[iContext], sizeof(h264EncodeContext));
	return 0;
	}


static void UnloadAndClear()
	{
	// primero borramos esto ya que puede estar usando recursos del contexto h264
	bOnShutdown = true;					// signaling this is the end
	EnterCriticalSection(&encodeCriticalSection);	// no more codec's

	for (int x = 0; x < max_number_of_contexts; x++)
		{
		encoders_h264_close_forced(x);
		}

	// delete pContextsH264;
	free(pH264EncodeContext);
	pH264EncodeContext = NULL;

	MFShutdown();

	LeaveCriticalSection(&encodeCriticalSection);
	DeleteCriticalSection(&encodeCriticalSection);
	}

static void COM_CHECK(HRESULT hr)
	{
	if (FAILED(hr)) {
		_com_error err(hr);
#ifdef _UNICODE
		const wchar_t* msg = err.ErrorMessage(); // weak ptr.
		throw std::runtime_error(ToAscii(msg));
#else
		const char* msg = err.ErrorMessage(); // weak ptr.
		throw std::runtime_error(msg);
#endif
		}
	}

void InicializarContextosH264()
	{
	bInicializando = true;
	if (!bInicializacionGeneral)
		{
		InitializeCriticalSection(&encodeCriticalSection);
		EnterCriticalSection(&encodeCriticalSection);

		pH264EncodeContext = (h264EncodeContext*)malloc(max_number_of_contexts * sizeof(h264EncodeContext));

		ZeroMemory(pH264EncodeContext, sizeof(h264EncodeContext) * max_number_of_contexts);
		iContextsReserved = max_number_of_contexts;

		// preparamos la salida
		atexit(UnloadAndClear);
		bInicializacionGeneral = true;

		COM_CHECK(MFStartup(MF_VERSION));
		// COM_CHECK(MFFrameRateToAverageTimePerFrame(fps, 1, const_cast<unsigned long long*>(&m_frame_duration)));
		
		LeaveCriticalSection(&encodeCriticalSection);
		}

	bInicializando = false;
	}


// encode_h264_init inicializar el compresor h264
ENCODERH264_API int encode_h264_init(int iWidth, int iHeight, int modo, long long opResultCallback)
	{
	if (bInicializando) while (bInicializando) Sleep(10);
	if (!bInicializacionGeneral) InicializarContextosH264();

	EnterCriticalSection(&encodeCriticalSection);

	int ret_val = -1000;
	for (int x = 0; x < max_number_of_contexts; x++)
		{
		if (!pH264EncodeContext[x].contexto_en_uso)
			{
			if (bOnShutdown)
				{
				LeaveCriticalSection(&encodeCriticalSection);
				return -1004;
				}

			// eliminamos cualquier posible basura pre-existente
			ZeroMemory(&pH264EncodeContext[x], sizeof(h264EncodeContext));

			// indicamos en uso...
			pH264EncodeContext[x].contexto_en_uso = true;

			// inicializa la critical section de la instancia
			InitializeCriticalSection(&pH264EncodeContext[x].context_local_cs);
			EnterCriticalSection(&pH264EncodeContext[x].context_local_cs);
			pH264EncodeContext[x].ok_critical_section = true;

			// libera la critical section global
			LeaveCriticalSection(&encodeCriticalSection);

			// -----------------------------------
			pH264EncodeContext[x].coded_width = iWidth;
			pH264EncodeContext[x].coded_height = iHeight;

			std::array<unsigned short, 2> dims = { static_cast<unsigned short>(pH264EncodeContext[x].coded_width), static_cast<unsigned short>(pH264EncodeContext[x].coded_height) };
			std::string fileName("d:\\Cybele\\videos\\encoderh264.h264.mp4");
			// pH264EncodeContext[x].pEncoder = new VideoEncoderMF(dims, 25, fileName.c_str(), true);

			// auto ws = CreateLocalInstance<WebStream>();
			// auto ds = CreateLocalInstance<DataStream>();
			// pH264EncodeContext[x].pMemIMFByteStream = CreateLocalInstance<MemIMFByteStream>();
			// pH264EncodeContext[x].pMemIMFByteStream = new MemIMFByteStream();
			//pH264EncodeContext[x].pDataStream = CreateLocalInstance<DataStream>();
			//pH264EncodeContext[x].pDataStream->SetOutputFile(fileName.c_str());
			// pH264EncodeContext[x].pEncoder = new VideoEncoderMF(dims, 25, pH264EncodeContext[x].pDataStream, false);
			pH264EncodeContext[x].pWebStream = CreateLocalInstance<WebStream>();
			pH264EncodeContext[x].pWebStream->SetFileEndPoint(fileName.c_str());			// no bloqueante
			// pH264EncodeContext[x].pEncoder = new VideoEncoderMF(dims, 25, pH264EncodeContext[x].pMemIMFByteStream, false);
			pH264EncodeContext[x].pEncoder = new VideoEncoderMF(dims, 25, pH264EncodeContext[x].pWebStream, false);
			// pH264EncodeContext[x].pEncoder = new VideoEncoderMF(dims, 25, false);

			ret_val = x;
			LeaveCriticalSection(&pH264EncodeContext[x].context_local_cs);

			if (opResultCallback > 0)
				{
				std::stringstream  ss;
				ss << "{encode_h264_init} ok init context. id: " << ret_val;
				((OperationResultInfoCallback)opResultCallback)(0, ss.str().length(), ss.str().c_str());
				}

			return ret_val;
			}
		}

	LeaveCriticalSection(&encodeCriticalSection);
	if (opResultCallback > 0)
		{
		std::stringstream  ss;
		ss << "{encode_h264_init} Error al buscar un contexto disponible (operacion no realizada, sin contextos). errCode: " << ret_val;
		((OperationResultInfoCallback)opResultCallback)(0, ss.str().length(), ss.str().c_str());
		}
	return ret_val;

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
ENCODERH264_API int encode_frame_from_rgb32_to_h264(int iContext, int frameNum, long long opResultCallback, int stride, byte* pImagen, int iImagenDataLen)
	{
	int frame_num = 0;
	if (iContext >= 0)
		{
		if (frameNum == 5)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 8)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 10)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 15)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 20)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 25)
			{
			frame_num = frameNum;
			}
		else if (frameNum == 30)
			{
			frame_num = frameNum;
			}

		EnterCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
		R8G8B8A8* pBuffer = pH264EncodeContext[iContext].pEncoder->WriteFrameBegin();
		if (pBuffer)
			{
			MoveMemory(pBuffer, pImagen, iImagenDataLen);
			}
		else
			{
			if (opResultCallback > 0)
				{
				string msg = "{encode_frame_from_rgb32_to_h264} no obtuve el buffer desde el encoder, ERROR -100001.";
				((OperationResultInfoCallback)opResultCallback)(-1, msg.length(), msg.c_str());
				}

			pH264EncodeContext[iContext].pEncoder->WriteFrameEnd();
			LeaveCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
			return -100001;
			}

		pH264EncodeContext[iContext].pEncoder->WriteFrameEnd();

		if (opResultCallback > 0)
			{
			std::stringstream  ss;
			ss << "{encode_frame_from_rgb32_to_h264} OK compress frame: " << frameNum;
			((OperationResultInfoCallback)opResultCallback)(0, ss.str().length(), ss.str().c_str());
			}

		LeaveCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
		return 0;			// OK
		}

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
	if (iContext >= 0)
		{
		// lockea el uso del contexto
		EnterCriticalSection(&pH264EncodeContext[iContext].context_local_cs);
		encoders_h264_close_forced(iContext);

		if (opResultCallback > 0)
			{
			std::stringstream  ss;
			ss << "{encode_h264_close} ok close the encoder context. id: " << iContext;
			((OperationResultInfoCallback)opResultCallback)(0, ss.str().length(), ss.str().c_str());
			}

		return 0;
		}

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

