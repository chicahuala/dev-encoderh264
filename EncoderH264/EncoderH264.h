#pragma once

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the ENCODERH264_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// ENCODERH264_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ENCODERH264_EXPORTS
#define ENCODERH264_API __declspec(dllexport)
#else
#define ENCODERH264_API __declspec(dllimport)
#endif

// This class is exported from the dll
class ENCODERH264_API CEncoderH264 {
public:
	CEncoderH264(void);
	// TODO: add your methods here.
	};

extern ENCODERH264_API int nEncoderH264;

ENCODERH264_API int fnEncoderH264(void);

#ifndef byte
#define	byte	BYTE
#endif

// https://github.com/forderud/AppWebStream/blob/master/VideoEncoder.hpp
#include <stdexcept>
#include <iostream>
#include <vector>
#include <array>
#include <cassert>
#include <Windows.h>
#include <mfapi.h>
#include <atlbase.h>

/** 32bit color value. */
struct R8G8B8A8 {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
	};

/** 24bit color value. */
struct R8G8B8 {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	};

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


_COM_SMARTPTR_TYPEDEF(IMFSinkWriter, __uuidof(IMFSinkWriter));
_COM_SMARTPTR_TYPEDEF(IMFMediaBuffer, __uuidof(IMFMediaBuffer));
_COM_SMARTPTR_TYPEDEF(IMFSample, __uuidof(IMFSample));
_COM_SMARTPTR_TYPEDEF(IMFMediaType, __uuidof(IMFMediaType));

/** Converts unicode string to ASCII */
static inline std::string ToAscii(const std::wstring& w_str) {
#pragma warning(push)
#pragma warning(disable: 4996) // function or variable may be unsafe
	size_t N = w_str.size();
	std::string s_str;
	s_str.resize(N);
	wcstombs(const_cast<char*>(s_str.data()), w_str.c_str(), N);

	return s_str;
#pragma warning(pop)
	}

// virtual encoder
class VideoEncoder
	{
	public:
		/** Grow size to become a multiple of the MEPG macroblock size (typ. 8). */
		static unsigned int Align(unsigned int size, unsigned int block_size = 8)
			{
			if ((size % block_size) == 0)
				return size;
			else
				return size + block_size - (size % block_size);
			}

		VideoEncoder(std::array<unsigned short, 2> dimensions, bool use24bits) : m_width(dimensions[0]), m_height(dimensions[1]), m_use24bits(use24bits), m_error(false), m_strErrorDescriptor("none")	/* , m_frame_size(4 * Align(m_width) * Align(m_height)) */
			{
			if (m_use24bits)
				{
				m_frame_size = 3 * Align(m_width) * Align(m_height);
				}
			else
				{
				m_frame_size = 4 * Align(m_width) * Align(m_height);
				}
			}

		virtual ~VideoEncoder() = default;

		std::array<unsigned short, 2> Dims() const
			{
			return { m_width, m_height };
			}

		virtual R8G8B8A8* WriteFrameBegin() = 0;
		virtual HRESULT   WriteFrameEnd() = 0;

		HRESULT WriteFrame(R8G8B8A8* src_data, bool swap_rb)
			{
			R8G8B8A8* buffer_ptr = WriteFrameBegin();

			for (unsigned int j = 0; j < m_height; j++) {
				R8G8B8A8* src_row = &src_data[(m_height - 1 - j) * m_width]; // flip upside down
				R8G8B8A8* dst_row = &buffer_ptr[j * Align(m_width)];

				if (swap_rb)
					{
					for (unsigned int i = 0; i < m_width; i++)
						dst_row[i] = SwapRGBAtoBGRA(src_row[i]);
					}
				else
					{
					// copy scanline as-is
					memcpy(dst_row, src_row, 4 * m_width);
					}

				// clear padding at end of scanline
				size_t hor_padding = Align(m_width) - m_width;
				if (hor_padding)
					std::memset(&dst_row[m_width], 0, 4 * hor_padding);
				}

			// clear padding after last scanline
			size_t vert_padding = Align(m_height) - m_height;
			if (vert_padding)
				std::memset(&buffer_ptr[m_height * Align(m_width)], 0, 4 * Align(m_width) * vert_padding);

			return WriteFrameEnd();
			}

		static R8G8B8A8 SwapRGBAtoBGRA(R8G8B8A8 in)
			{
			return{ in.b, in.g, in.r, in.a };
			}

		DWORD m_frame_size;
		bool m_use24bits;
		bool m_error;
		std::string m_strErrorDescriptor;

	protected:
		const unsigned short m_width;  ///< horizontal img. resolution (excluding padding)
		const unsigned short m_height; ///< vertical img. resolution (excluding padding)
	};


/** Media-Foundation-based H.264 video encoder. */
class VideoEncoderMF : public VideoEncoder
	{
	public:
		// ** File-based video encoding. **
		VideoEncoderMF(std::array<unsigned short, 2> dimensions, unsigned int fps, const wchar_t* filename, bool use24bits)
			: VideoEncoderMF(dimensions, fps, use24bits)
			{
			const unsigned int bit_rate = static_cast<unsigned int>(0.78f * fps * Align(m_width) * Align(m_height)); // yields 40Mb/s for 1920x1080@25fps (max blu-ray quality)

			CComPtr<IMFAttributes> attribs;
			COM_CHECK(MFCreateAttributes(&attribs, 0));
			COM_CHECK(attribs->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4));
			COM_CHECK(attribs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));

			// create sink writer with specified output format
			COM_CHECK(MFCreateSinkWriterFromURL(filename, nullptr, attribs, &m_sink_writer));
			IMFMediaTypePtr mediaTypeOut = MediaTypeOutput(fps, bit_rate);
			COM_CHECK(m_sink_writer->AddStream(mediaTypeOut, &m_stream_index));

			// connect input to output
			IMFMediaTypePtr mediaTypeIn = MediaTypeInput(fps);
			COM_CHECK(m_sink_writer->SetInputMediaType(m_stream_index, mediaTypeIn, nullptr));
			COM_CHECK(m_sink_writer->BeginWriting());
			}

		// ** Stream-based video encoding. The underlying MFCreateFMPEG4MediaSink system call require Windows 8 or newer. **
		VideoEncoderMF(std::array<unsigned short, 2> dimensions, unsigned int fps, IMFByteStream* stream, bool use24bits)
			: VideoEncoderMF(dimensions, fps, use24bits)
			{
			const unsigned int bit_rate = static_cast<unsigned int>(0.78f * fps * m_width * m_height); // yields 40Mb/s for 1920x1080@25fps

			CComPtr<IMFAttributes> attribs;
			COM_CHECK(MFCreateAttributes(&attribs, 0));
			COM_CHECK(attribs->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_FMPEG4));
			COM_CHECK(attribs->SetUINT32(MF_LOW_LATENCY, TRUE));
			COM_CHECK(attribs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));

			// create sink writer with specified output format
			IMFMediaTypePtr mediaTypeOut = MediaTypeOutput(fps, bit_rate);
			COM_CHECK(MFCreateFMPEG4MediaSink(stream, mediaTypeOut, nullptr, &m_media_sink)); // "fragmented" MPEG4 does not require seekable byte-stream
			COM_CHECK(MFCreateSinkWriterFromMediaSink(m_media_sink, attribs, &m_sink_writer));

			// connect input to output
			IMFMediaTypePtr mediaTypeIn = MediaTypeInput(fps);
			COM_CHECK(m_sink_writer->SetInputMediaType(m_stream_index, mediaTypeIn, nullptr));

			{
				// access H.264 encoder directly (https://msdn.microsoft.com/en-us/library/windows/desktop/dd797816.aspx)
			CComPtr<ICodecAPI> codec;
			COM_CHECK(m_sink_writer->GetServiceForStream(m_stream_index, GUID_NULL, IID_ICodecAPI, (void**)&codec));
			CComVariant quality;
			codec->GetValue(&CODECAPI_AVEncCommonQuality, &quality); // not supported by Intel encoder (mfx_mft_h264ve_64.dll)
			CComVariant low_latency;
			COM_CHECK(codec->GetValue(&CODECAPI_AVLowLatencyMode, &low_latency));
			//assert(low_latency.boolVal != FALSE);
			// CODECAPI_AVEncAdaptiveMode not implemented

			// query group-of-pictures (GoP) size
			CComVariant gop_size;
			COM_CHECK(codec->GetValue(&CODECAPI_AVEncMPVGOPSize, &gop_size));
			//gop_size = (unsigned int)1; // VT_UI4 type
			//COM_CHECK(codec->SetValue(&CODECAPI_AVEncMPVGOPSize, &gop_size));
			}

			COM_CHECK(m_sink_writer->BeginWriting());
			}

		// common constructor
		VideoEncoderMF(std::array<unsigned short, 2> dimensions, unsigned int fps, bool use24bits) : VideoEncoder(dimensions, use24bits), m_media_sink(NULL), m_sink_writer(NULL), m_buffer(NULL)
			{
			//COM_CHECK(MFStartup(MF_VERSION));
			COM_CHECK(MFFrameRateToAverageTimePerFrame(fps, 1, const_cast<unsigned long long*>(&m_frame_duration)));
			}

		~VideoEncoderMF() noexcept
			{
			//HRESULT hr = m_sink_writer->Finalize(); // fails on prior I/O errors
			//hr; // discard error

			if (m_sink_writer) m_sink_writer->Finalize(); // fails on prior I/O errors

			// delete objects before shutdown-call
			if (m_buffer) m_buffer->Release();

			if (m_sink_writer) m_sink_writer->Release();

			if (m_media_sink)
				{
				COM_CHECK(m_media_sink->Shutdown());
				m_media_sink.Release();
				}

			// COM_CHECK(MFShutdown());
			}

		R8G8B8A8* WriteFrameBegin() override
			{
			// const DWORD frame_size = 4 * Align(m_width) * Align(m_height);

			// Create a new memory buffer.
			if (!m_buffer)
				COM_CHECK(MFCreateMemoryBuffer(m_frame_size, &m_buffer));

			if (m_use24bits)
				{
				}
			else
				{
				}

			// Lock buffer to get data pointer
			R8G8B8A8* buffer_ptr = nullptr;
			COM_CHECK(m_buffer->Lock(reinterpret_cast<BYTE**>(&buffer_ptr), NULL, NULL));
			return buffer_ptr;
			}

		HRESULT WriteFrameEnd() override
			{
			// const DWORD frame_size = 4 * Align(m_width) * Align(m_height);

			COM_CHECK(m_buffer->Unlock());

			// Set the data length of the buffer.
			COM_CHECK(m_buffer->SetCurrentLength(m_frame_size));

			// Create a media sample and add the buffer to the sample.
			// IMFSamplePtr sample;
			IMFSample* sample;
			COM_CHECK(MFCreateSample(&sample));
			COM_CHECK(sample->AddBuffer(m_buffer));

			//IMFSample *sample;
			//MFCreateSample(&sample);
			// sample->AddBuffer(m_buffer);

			//// Set the time stamp and the duration.
			COM_CHECK(sample->SetSampleTime(m_time_stamp));
			COM_CHECK(sample->SetSampleDuration(m_frame_duration));

			//COM_CHECK(m_sample->SetSampleTime(m_time_stamp));
			//COM_CHECK(m_sample->SetSampleDuration(m_frame_duration));

			DWORD slength = 0;
			if (m_sink_writer)
				{
				HRESULT hr = sample->GetTotalLength(&slength);

				// send sample to Sink Writer.
				hr = m_sink_writer->WriteSample(m_stream_index, sample); // fails on I/O error
				hr = sample->GetTotalLength(&slength);
				if (FAILED(hr))
					{
					// sample->Release();
					return hr;
					}
				}
			else
				{
				IMFMediaBuffer* pIMFMediaBuffer = nullptr;
				sample->ConvertToContiguousBuffer(&pIMFMediaBuffer);

				BYTE* pBYTE = nullptr;
				DWORD stlength = 0;
				HRESULT hr = sample->GetTotalLength(&stlength);
				DWORD smlength = 0;
				pIMFMediaBuffer->GetMaxLength(&smlength);
				DWORD sclength = 0;
				pIMFMediaBuffer->GetCurrentLength(&sclength);
				pIMFMediaBuffer->Lock(&pBYTE, &smlength, &sclength);
				pIMFMediaBuffer->Unlock();

				pIMFMediaBuffer->Release();
				}

			HRESULT hr = sample->GetTotalLength(&slength);

			sample->Release();

			// increment time
			m_time_stamp += m_frame_duration;
			return S_OK;
			}

	private:
		void InitMediaType(IMFMediaType* M, const GUID& Format, UINT BitRate, UINT Width, UINT Height, UINT FrameRate)
			{
			M->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			M->SetGUID(MF_MT_SUBTYPE, Format);
			M->SetUINT32(MF_MT_AVG_BITRATE, BitRate);
			MFSetAttributeSize(M, MF_MT_FRAME_SIZE, Width, Height);
			MFSetAttributeRatio(M, MF_MT_FRAME_RATE, FrameRate, 1);
			MFSetAttributeRatio(M, MF_MT_FRAME_RATE_RANGE_MAX, FrameRate, 1);
			MFSetAttributeRatio(M, MF_MT_FRAME_RATE_RANGE_MIN, FrameRate / 2, 1);
			MFSetAttributeRatio(M, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
			M->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
			M->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
			M->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, 1);
			M->SetUINT32(MF_MT_SAMPLE_SIZE, Width * Height * 4);
			M->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Main); //eAVEncH264VProfile_Base

			//M->SetUINT32(MF_MT_DEFAULT_STRIDE, -960);
			//M->SetGUID(MF_MT_AM_FORMAT_TYPE, Webcam);
			}

		IMFMediaTypePtr MediaTypeInput(unsigned int fps)
			{
			// configure input format. Frame size is aligned to avoid crash
			IMFMediaTypePtr mediaTypeIn;
			COM_CHECK(MFCreateMediaType(&mediaTypeIn));
			COM_CHECK(mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
			COM_CHECK(mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)); // X8R8G8B8 format
			COM_CHECK(mediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
			COM_CHECK(mediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));

			// Frame size is aligned to avoid crash
			COM_CHECK(MFSetAttributeSize(mediaTypeIn, MF_MT_FRAME_SIZE, Align(m_width), Align(m_height)));
			COM_CHECK(MFSetAttributeRatio(mediaTypeIn, MF_MT_FRAME_RATE, fps, 1));
			COM_CHECK(MFSetAttributeRatio(mediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
			return mediaTypeIn;
			}

		IMFMediaTypePtr MediaTypeInput24(unsigned int fps)
			{
			// configure input format. Frame size is aligned to avoid crash
			IMFMediaTypePtr mediaTypeIn;
			COM_CHECK(MFCreateMediaType(&mediaTypeIn));
			COM_CHECK(mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
			COM_CHECK(mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24));					// D3DFMT_R8G8B8
			COM_CHECK(mediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
			COM_CHECK(mediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));

			// Frame size is aligned to avoid crash
			COM_CHECK(MFSetAttributeSize(mediaTypeIn, MF_MT_FRAME_SIZE, Align(m_width), Align(m_height)));
			COM_CHECK(MFSetAttributeRatio(mediaTypeIn, MF_MT_FRAME_RATE, fps, 1));
			COM_CHECK(MFSetAttributeRatio(mediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
			return mediaTypeIn;
			}

		IMFMediaTypePtr MediaTypeOutput(unsigned int fps, unsigned int bit_rate)
			{
			IMFMediaTypePtr mediaTypeOut;
			COM_CHECK(MFCreateMediaType(&mediaTypeOut));
			COM_CHECK(mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
			COM_CHECK(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264)); // H.264 format
			COM_CHECK(mediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, bit_rate));
			COM_CHECK(mediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

			// Frame size is aligned to avoid crash
			COM_CHECK(MFSetAttributeSize(mediaTypeOut, MF_MT_FRAME_SIZE, Align(m_width), Align(m_height)));
			COM_CHECK(MFSetAttributeRatio(mediaTypeOut, MF_MT_FRAME_RATE, fps, 1));
			COM_CHECK(MFSetAttributeRatio(mediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
			return mediaTypeOut;
			}

		IMFMediaTypePtr MediaTypeOutputFps0(unsigned int fps, unsigned int bit_rate)
			{
			IMFMediaTypePtr mediaTypeOut;
			COM_CHECK(MFCreateMediaType(&mediaTypeOut));
			COM_CHECK(mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
			COM_CHECK(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264)); // H.264 format
			COM_CHECK(mediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, bit_rate));
			COM_CHECK(mediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

			// ver en codecapi.h, eAVEncVideoChromaResolution
			// AVEncVideoInputChromaResolution
			// AVEncVideoOutputChromaResolution
			// hr = pAttributes->SetUINT32(CODECAPI_AVEncMPVGOPSize, 1);			// puede que sea lo mejor
			COM_CHECK(mediaTypeOut->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, 0));

			// Frame size is aligned to avoid crash
			COM_CHECK(MFSetAttributeSize(mediaTypeOut, MF_MT_FRAME_SIZE, Align(m_width), Align(m_height)));
			COM_CHECK(MFSetAttributeRatio(mediaTypeOut, MF_MT_FRAME_RATE, fps, 1));
			COM_CHECK(MFSetAttributeRatio(mediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
			return mediaTypeOut;
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

		const unsigned long long m_frame_duration = 0;
		long long                m_time_stamp = 0;

		CComPtr<IMFMediaSink>    m_media_sink;
		IMFSinkWriterPtr         m_sink_writer;
		IMFMediaBufferPtr        m_buffer;

		unsigned long            m_stream_index = 0;
	};

