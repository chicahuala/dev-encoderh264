using System;
using System.Collections.Generic;
using System.Text;

namespace EncoderH264_test.Extras
	{
	public class interop
		{
#if x64_debug
		public const string dllTarget_encoderH264 = "encoderH264D64.dll";
		public static string modelo = $"x64_debug, 64 bits debug";
		public static string version = $"EncoderH264 (c#) v: 2020.8.12 1200hs, 64 bits debug";
#endif

#if x64_release
		public const string dllTarget_encoderH264 = "encoderH264R64.dll";
		public static string modelo = $"x64_release, 64 bits release";
		public static string version = $"EncoderH264 (c#) v: 2020.8.12 1200hs, 64 bits release";
#endif

		[System.Runtime.InteropServices.DllImport(dllTarget_encoderH264, CharSet = System.Runtime.InteropServices.CharSet.Auto, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
		public static extern int encode_h264_init(int iWidth, int iHeight, int modo, [System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.FunctionPtr)] OperationResultInfoCallbackDelegate resultReport);

		[System.Runtime.InteropServices.DllImport(dllTarget_encoderH264, CharSet = System.Runtime.InteropServices.CharSet.Auto, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
		public static extern int encode_frame_from_rgb32_to_h264(int iContext, int frameNum, [System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.FunctionPtr)] OperationResultInfoCallbackDelegate resultReport, int stride, IntPtr pImagen, int iImagenDataLen);

		[System.Runtime.InteropServices.DllImport(dllTarget_encoderH264, CharSet = System.Runtime.InteropServices.CharSet.Auto, CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
		public static extern int encode_h264_close(int iContext, [System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.FunctionPtr)] OperationResultInfoCallbackDelegate resultReport);

		// ------------------------------------------------------------------------------------------------------------------------------------------------------
		// delegates
		[System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.StdCall)]
		public delegate void CompresionH264ResultCallbackDelegate(int iContext, bool KeyFrame, IntPtr pFrameH264, int iFrameH264Len);

		[System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.StdCall)]
		public delegate void ScaleResultCallbackDelegate(int iContext, IntPtr pFrameScaleY, IntPtr pFrameScaleU, IntPtr pFrameScaleV, int iWidth, int iHeight, int iStrideY, int iStrideUV);

		[System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.StdCall)]
		public delegate void OperationResultInfoCallbackDelegate(int iContext, int iStringLen, IntPtr pMensaje);
		}
	}
