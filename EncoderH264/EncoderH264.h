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

