#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <MFidl.h>
#include <Mfreadwrite.h>

#define FILE_NAME ("d:\\video\\try_file.mp4")

class MemIMFByteStream
	:
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<MemIMFByteStream>,
	public IMFByteStream
	{
	public:
		unsigned __int64 m_cur_pos = 0;

		MemIMFByteStream() : IMFByteStream()
			{
			errno_t err = fopen_s(&file, FILE_NAME, "w+");
			if (err != 0)
				{
				GetLastError();
				}

			// init our end position
			end_position = -1;

			// init referance counter
			reference_counter = 1;
			};

		~MemIMFByteStream()
			{
			};

		HRESULT STDMETHODCALLTYPE Close()
			{
			fclose(file);
			return 0;
			};

		HRESULT STDMETHODCALLTYPE GetCapabilities(
			/* [out] */ __RPC__out DWORD* pdwCapabilities) override
			{
			*pdwCapabilities = MFBYTESTREAM_IS_WRITABLE | MFBYTESTREAM_IS_REMOTE;
			return S_OK;
			}

		HRESULT STDMETHODCALLTYPE GetLength(
			/* [out] */ __RPC__out QWORD* pqwLength) override
			{
			return E_NOTIMPL;
			}

		HRESULT STDMETHODCALLTYPE SetLength(
			/* [in] */ QWORD qwLength) override
			{
			return E_NOTIMPL;
			}

		HRESULT STDMETHODCALLTYPE GetCurrentPosition(
			/* [out] */ __RPC__out QWORD* pqwPosition) override
			{
			*pqwPosition = m_cur_pos;
			return S_OK;
			}

		HRESULT STDMETHODCALLTYPE SetCurrentPosition(
			/* [in] */ QWORD qwPosition) override
			{
			return E_NOTIMPL;
			}

		HRESULT STDMETHODCALLTYPE IsEndOfStream(
			/* [out] */ __RPC__out BOOL* pfEndOfStream) override
			{
			return E_NOTIMPL;
			}

		HRESULT STDMETHODCALLTYPE Read(
			/* [size_is][out] */ __RPC__out_ecount_full(cb) BYTE* pb,
			/* [in] */ ULONG cb,
			/* [out] */ __RPC__out ULONG* pcbRead) override
			{
			return E_NOTIMPL;
			}

		/* [local] */ HRESULT STDMETHODCALLTYPE BeginRead(
			/* [annotation][out] */
			_Out_writes_bytes_(cb)  BYTE* pb,
			/* [in] */ ULONG cb,
			/* [in] */ IMFAsyncCallback* pCallback,
			/* [in] */ IUnknown* punkState) override
			{
			return E_NOTIMPL;
			}

		/* [local] */ HRESULT STDMETHODCALLTYPE EndRead(
			/* [in] */ IMFAsyncResult* pResult,
			/* [annotation][out] */
			_Out_  ULONG* pcbRead) override
			{
			return E_NOTIMPL;
			}

		HRESULT STDMETHODCALLTYPE Write(
			/* [size_is][in] */ __RPC__in_ecount_full(cb) const BYTE* pb,
			/* [in] */ ULONG cb,
			/* [out] */ __RPC__out ULONG* pcbWritten) override
			{
			BYTE* n_pb = new BYTE[cb];
			for (ULONG i = 0; i < cb; i++)
				{
				n_pb[i] = pb[i] ^ 0x55;
				}

			*pcbWritten = fwrite(n_pb, 1, cb, file);
			return S_OK;
			}

		HRESULT STDMETHODCALLTYPE Flush()
			{
			return fflush(file);
			}

		/* [local] */ HRESULT STDMETHODCALLTYPE BeginWrite(
			/* [annotation][in] */
			_In_reads_bytes_(cb)  const BYTE* pb,
			/* [in] */ ULONG cb,
			/* [in] */ IMFAsyncCallback* pCallback,
			/* [in] */ IUnknown* punkState) override
			{
	   // must be implemented with a thread 
			std::thread([=] { inner_async_write(pb, cb, pCallback, punkState); });
			return S_OK;
				
		// m_mutex.lock();

			//HRESULT hr = WriteImpl(pb, cb);
			//if (FAILED(hr)) {
			//	m_mutex.unlock();
			//	return E_FAIL;
			//	}

			// m_tmp_bytes_written = cb;

			//CComPtr<IMFAsyncResult> async_res;
			//if (FAILED(MFCreateAsyncResult(nullptr, callback, unkState, &async_res)))
			//	throw std::runtime_error("MFCreateAsyncResult failed");

			//hr = callback->Invoke(async_res); // will trigger EndWrite
			// return hr;
			return S_OK;
			}

		/* [local] */ HRESULT STDMETHODCALLTYPE EndWrite(
			/* [in] */ IMFAsyncResult* pResult,
			/* [annotation][out] */
			_Out_  ULONG* pcbWritten) override
			{
			*pcbWritten = actual_read;
			pResult->SetStatus(S_OK);

			return S_OK;
			}

		HRESULT STDMETHODCALLTYPE Seek(
			/* [in] */ MFBYTESTREAM_SEEK_ORIGIN SeekOrigin,
			/* [in] */ LONGLONG llSeekOffset,
			/* [in] */ DWORD dwSeekFlags,
			/* [out] */ __RPC__out QWORD* pqwCurrentPosition) override
			{
			return E_NOTIMPL;
			}

		//BEGIN_COM_MAP(MemIMFByteStream)
		//	COM_INTERFACE_ENTRY(IMFByteStream)
		//END_COM_MAP()

		ULONG STDMETHODCALLTYPE Release()
			{
			if (reference_counter == 0)
				{
				delete this;
				return 0;
				}

			reference_counter--;
			return reference_counter;
			}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
			{
				// no idea what this code douse ...
			static const QITAB qit[] =
				{
					QITABENT(MemIMFByteStream, IMFByteStream),
					{ 0 }
				};

			return QISearch(this, qit, riid, ppv);
			}

		ULONG STDMETHODCALLTYPE AddRef()
			{
			reference_counter++;
			return reference_counter;
			}

	private:
		void inner_async_write(const BYTE* pb, ULONG     cb, IMFAsyncCallback* pCallback, IUnknown* punkState)
			{
				// use lock 
			mtx.lock();

			// make the actual write
			actual_write = 0;
			Write(pb, cb, &actual_write);

			// set the results
			ULONG pcbUnclear = 0;
			if (punkState != NULL)
				{
				EndWrite((IMFAsyncResult*)punkState, &pcbUnclear);
				}


				// free lock
			mtx.unlock();

			// call callback "we done"
			pCallback->Invoke((IMFAsyncResult*)punkState);
			}

		FILE* file = nullptr;
		std::mutex mtx;           // mutex for critical section

		long int end_position = -1;
		ULONG actual_read = 0;
		ULONG actual_write = 0;
		ULONG reference_counter = 0;

	};