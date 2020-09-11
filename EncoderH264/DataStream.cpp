#define WIN32_LEAN_AND_MEAN

#include "pch.h"

#include <atomic>
#include <Mfapi.h>
#include "DataStream.h"
// #include "WebSocket.hpp"
#include "MP4FragmentEditor.h"

#ifdef __socket__
#endif // __socket__

#ifdef __socket__
struct DataStream::impl : public StreamSockSetter
	{
	impl(const char* port_str, HWND wnd) : m_server(port_str), m_block_ctor(true), m_wnd(wnd)
		{
		// start server thread
		m_thread = std::thread(&DataStream::impl::WaitForClients, this);

		// wait for video request or socket failure
		std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);
		while (m_block_ctor)
			m_cond_var.wait(lock);

		// start streaming video
		}

	void WaitForClients()
		{
		for (;;) {
			auto current = m_server.WaitForClient();
			if (!current)
				break;

			// create a thread to the handle client connection
			current->Start(m_wnd, this);
			m_clients.push_back(std::move(current));
			}

		Unblock();
		}

	void SetStreamSocket(ClientSock& s) override
		{
		for (size_t i = 0; i < m_clients.size(); ++i)
			{
			if (m_clients[i].get() == &s)
				{
				m_stream_client = std::move(m_clients[i]);
				return;
				}
			}
		}

	void Unblock() override
		{
		m_block_ctor = false;
		m_cond_var.notify_all();
		}

	~impl() override
		{
		// close open sockets (forces blocking calls to complete)
		m_server = ServerSock();
		m_thread.join();

		m_stream_client.reset();

		// wait for client threads to complete
		for (auto& c : m_clients)
			{
			if (c)
				c.reset();
			}
		}

	ServerSock              m_server;  ///< listens for new connections
	std::unique_ptr<ClientSock> m_stream_client;  ///< video streaming socket
	std::atomic<bool>       m_block_ctor;
	std::condition_variable m_cond_var;
	std::thread             m_thread;
	std::vector<std::unique_ptr<ClientSock>> m_clients; // heap allocated objects to ensure that they never change addreess
	HWND                    m_wnd = nullptr;
	unsigned __int64        m_cur_pos = 0;
	MP4FragmentEditor       m_stream_editor;
	};
#else
/** Get handle to current DLL/EXE .*/
static HMODULE CurrentModule() {
	HMODULE module = nullptr;
	LPCTSTR module_ptr = (LPCTSTR)CurrentModule; // pointer to current function
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module_ptr, &module);
	return module;
	}

enum class Connect {
	END,
	CONTINUE,
	STREAM,
	SOCKET_EMPTY,
	SOCKET_FAILURE,
	};

class ClientSock; // forward decl

class StreamSockSetter
	{
	public:
		virtual ~StreamSockSetter() = default;
		virtual void SetStreamSocket(ClientSock& s) = 0;
		virtual void Unblock() = 0;
	};

class ClientSock final {
public:
	explicit ClientSock(SOCKET cs) : m_sock(cs) {
		}

		// non-assignable
	ClientSock(const ClientSock&) = delete;
	ClientSock& operator = (const ClientSock&) = delete;
	// non-movable (not safe due to threading)
	ClientSock(ClientSock&& other) = delete;
	ClientSock& operator = (ClientSock&& other) = delete;

	Connect Handshake(HWND wnd) {
		// receive request
		std::string receive_buf;
		receive_buf.resize(1024);
		int res = recv(m_sock, const_cast<char*>(receive_buf.data()), static_cast<int>(receive_buf.size()), 0);
		if (res == SOCKET_ERROR)
			return Connect::SOCKET_FAILURE;
		if (res == 0)
			return Connect::SOCKET_EMPTY;
		receive_buf.resize(res);

		const std::string NO_CONTENT_RESPONSE = "HTTP/1.1 204 No Content\r\n\r\n";

		if ((receive_buf.find("Range: bytes=0-") != std::string::npos) || (receive_buf.find("GET /movie.mp4") != std::string::npos)) {
			// streaming video request

			// send HTTP header
			std::string header = "HTTP/1.1 200 OK\r\n";
			header += "Content-Type: video/mp4\r\n";
			header += "Accept-Ranges: none\r\n"; // no support for partial requests
			header += "Access-Control-Allow-Origin: *\r\n";
			header += "Cache-Control: no-store, must-revalidate\r\n";
			header += "\r\n";
			return SendResponse(header, Connect::STREAM);
			}
		else if (receive_buf.find("GET / ") != std::string::npos) {
		 // index.html request 

		 // load HTML page from resource embedded into DLL/EXE
			//HRSRC   html_info = FindResource(CurrentModule(), MAKEINTRESOURCE(IDR_WebStreamHtml), RT_RCDATA);
			//HGLOBAL html_handle = LoadResource(CurrentModule(), html_info);
			//unsigned int html_len = SizeofResource(CurrentModule(), html_info);

			//// send HTTP header
			//std::string header = "HTTP/1.1 200 OK\r\n";
			//header += "Content-Type: text/html; charset=utf-8\r\n";
			//header += "Accept-Ranges: none\r\n"; // no support for partial requests
			//header += "Cache-Control: no-store, must-revalidate\r\n";
			//header += "Content-Length: " + std::to_string(html_len) + "\r\n";
			//header += "\r\n";
			//res = send(m_sock, header.data(), static_cast<int>(header.size()), 0);
			//if (res == SOCKET_ERROR)
			//	return Connect::SOCKET_FAILURE;

			//res = send(m_sock, (char*)LockResource(html_handle), html_len, 0);
			//if (res == SOCKET_ERROR)
			//	return Connect::SOCKET_FAILURE;

			return Connect::CONTINUE;
			}
		else {
		 // unknown request
			std::string header = "HTTP/1.1 404 Not found\r\n";
			header += "Content-Length: 0\r\n";
			header += "\r\n";
			return SendResponse(header, Connect::CONTINUE);
			}
		}

	Connect SendResponse(const std::string& message, Connect success_code) {
		int res = send(m_sock, message.data(), static_cast<int>(message.size()), 0);
		if (res == SOCKET_ERROR)
			return Connect::SOCKET_FAILURE;

		return success_code;
		}

	~ClientSock() {
		if (m_sock == INVALID_SOCKET)
			return; // already destroyed

		int res = shutdown(m_sock, SD_SEND);
		// deliberately discard errors

		res = closesocket(m_sock);
		// deliberately discard errors

		m_sock = INVALID_SOCKET;

		if (m_thread.joinable())
			m_thread.join();
		}

	void Start(HWND wnd, StreamSockSetter* parent) {
		m_thread = std::thread(&ClientSock::ConnectionThread, this, wnd, parent);
		}

	SOCKET Socket() {
		return m_sock;
		}

private:
	void ConnectionThread(HWND wnd, StreamSockSetter* parent) {
		Connect type;
		do {
			type = Handshake(wnd);
			} while (type == Connect::CONTINUE);

			if (type == Connect::SOCKET_FAILURE) {
				parent->Unblock();
				}
			else if (type == Connect::STREAM) {
				parent->SetStreamSocket(*this);
				parent->Unblock();
				}
		}

	SOCKET      m_sock = INVALID_SOCKET;
	std::thread m_thread;
	};

class ServerSock final {
public:
	ServerSock() {
		}
	ServerSock(const char* port_str) {
		WSADATA wsaData = {};
		int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (res)
			throw std::runtime_error("WSAStartup failure");

		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		addrinfo* result = nullptr;
		// res = getaddrinfo(NULL, port_str, &hints, &result);
		if (res)
			throw std::runtime_error("getaddrinfo failure");

		m_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (m_sock == INVALID_SOCKET)
			throw std::runtime_error("socket failure");

		res = bind(m_sock, result->ai_addr, static_cast<int>(result->ai_addrlen));
		if (res == SOCKET_ERROR)
			throw std::runtime_error("bind failure");

		// freeaddrinfo(result);
		result = nullptr;

		res = listen(m_sock, SOMAXCONN);
		if (res == SOCKET_ERROR)
			throw std::runtime_error("listen failure");
		}

		// non-assignable class (only movable)
	ServerSock(const ServerSock&) = delete;
	ServerSock& operator = (const ServerSock&) = delete;

	/** Explicit move operators, since the default generated incorrectly assumes value semantics. */
	ServerSock(ServerSock&& other) {
		std::swap(m_sock, other.m_sock);
		}
	ServerSock& operator = (ServerSock&& other) {
		std::swap(m_sock, other.m_sock);
		return *this;
		}

	~ServerSock() {
		if (m_sock == INVALID_SOCKET)
			return;

		int res = closesocket(m_sock);
		if (res)
			std::terminate();
		m_sock = INVALID_SOCKET;

		res = WSACleanup();
		if (res == SOCKET_ERROR)
			std::terminate();
		}

	std::unique_ptr<ClientSock> WaitForClient() {
		SOCKET cs = accept(m_sock, NULL, NULL);
		if (cs == INVALID_SOCKET)
			return std::unique_ptr<ClientSock>(); // aborted

												  // client is now connected
		return std::make_unique<ClientSock>(cs);
		}

private:
	SOCKET m_sock = INVALID_SOCKET;
	};

class MySetter
	{
	public:
		~MySetter()
			{
			}

		void SetStreamSocket(ClientSock& s)
			{
			};

		void Unblock()
			{
			};
	};



struct DataStream::impl : public StreamSockSetter
	{
	impl(const char* fileName) : m_block_ctor(true), m_fileName(fileName)
		{
		// m_fileName = fileName;

		// start server thread
		// m_thread = std::thread(&DataStream::impl::WaitForClients, this);

		// wait for video request or socket failure
		std::mutex mutex;
		std::unique_lock<std::mutex> lock(mutex);
		//while (m_block_ctor)
		//	m_cond_var.wait(lock);

		// start streaming video
		}

	//void WaitForClients()
	//	{
	//	for (;;) {
	//		auto current = m_server.WaitForClient();
	//		if (!current)
	//			break;

	//		// create a thread to the handle client connection
	//		current->Start(m_wnd, this);
	//		m_clients.push_back(std::move(current));
	//		}

	//	Unblock();
	//	}

	void SetStreamSocket(ClientSock& s) override
		{
		//for (size_t i = 0; i < m_clients.size(); ++i)
		//	{
		//	if (m_clients[i].get() == &s)
		//		{
		//		m_stream_client = std::move(m_clients[i]);
		//		return;
		//		}
		//	}
		}

	void Unblock() override
	// void Unblock()
		{
		m_block_ctor = false;
		m_cond_var.notify_all();
		}

	~impl() override
	// ~impl()
		{
		// close open sockets (forces blocking calls to complete)
		// m_server = ServerSock();
		// m_thread.join();

		// m_stream_client.reset();

		// wait for client threads to complete
		//for (auto& c : m_clients)
		//	{
		//	if (c)
		//		c.reset();
		//	}
		}

	// ServerSock              m_server;  ///< listens for new connections
	// std::unique_ptr<ClientSock> m_stream_client;  ///< video streaming socket
	std::atomic<bool>       m_block_ctor;
	std::condition_variable m_cond_var;
	std::thread             m_thread;
	// std::vector<std::unique_ptr<ClientSock>> m_clients; // heap allocated objects to ensure that they never change addreess
	// HWND                    m_wnd = nullptr;
	unsigned __int64        m_cur_pos = 0;
	MP4FragmentEditor       m_stream_editor;

	std::string m_fileName;
	};
#endif // __socket__

DataStream::DataStream()
	{
	}

DataStream::~DataStream()
	{
	}

#ifdef __socket__
void DataStream::SetPortAndWindowHandle(const char* port_str, HWND wnd)
	{
	m_impl = std::make_unique<impl>(port_str, wnd);
	}
#else
void DataStream::SetOutputFile(const char* fileName)
	{
	m_impl = std::make_unique<impl>(fileName);
	}
#endif // __socket__

HRESULT STDMETHODCALLTYPE DataStream::GetCapabilities(/*out*/DWORD* capabilities)
	{
	*capabilities = MFBYTESTREAM_IS_WRITABLE | MFBYTESTREAM_IS_REMOTE;
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::GetLength(/*out*/QWORD* /*length*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::SetLength(/*in*/QWORD /*length*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::GetCurrentPosition(/*out*/QWORD* position)
	{
	*position = m_impl->m_cur_pos;
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::SetCurrentPosition(/*in*/QWORD /*position*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::IsEndOfStream(/*out*/BOOL* /*endOfStream*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::Read(/*out*/BYTE* /*pb*/, /*in*/ULONG /*cb*/, /*out*/ULONG* /*bRead*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::BeginRead(/*out*/BYTE* /*pb*/, /*in*/ULONG /*cb*/, /*in*/IMFAsyncCallback* /*callback*/, /*in*/IUnknown* /*unkState*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::EndRead(/*in*/IMFAsyncResult* /*result*/, /*out*/ULONG* /*cbRead*/)
	{
	return E_NOTIMPL;
	}

HRESULT DataStream::WriteImpl(/*in*/const BYTE* pb, /*in*/ULONG cb)
	{
#ifndef ENABLE_FFMPEG
	std::tie(pb, cb) = m_impl->m_stream_editor.ModifyMovieFragment(pb, cb);
#endif

	//int byte_count = send(m_impl->m_stream_client->Socket(), reinterpret_cast<const char*>(pb), cb, 0);
	//if (byte_count == SOCKET_ERROR)
	//	{
	//	//_com_error error(WSAGetLastError());
	//	//const TCHAR* msg = error.ErrorMessage();

	//	// destroy failing client socket (typ. caused by client-side closing)
	//	m_impl->m_stream_client.reset();
	//	return E_FAIL;
	//	}

	// m_impl->m_cur_pos += byte_count;
	m_impl->m_cur_pos += cb;
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::Write(/*in*/const BYTE* pb, /*in*/ULONG cb, /*out*/ULONG* cbWritten)
	{
	std::lock_guard<std::mutex> lock(m_mutex);

	HRESULT hr = WriteImpl(pb, cb);
	if (FAILED(hr))
		return E_FAIL;

	*cbWritten = cb;
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::BeginWrite(/*in*/const BYTE* pb, /*in*/ULONG cb, /*in*/IMFAsyncCallback* callback, /*in*/IUnknown* unkState)
	{
	m_mutex.lock();

	HRESULT hr = WriteImpl(pb, cb);
	if (FAILED(hr)) {
		m_mutex.unlock();
		return E_FAIL;
		}

	m_tmp_bytes_written = cb;

	CComPtr<IMFAsyncResult> async_res;
#ifndef ENABLE_FFMPEG
	if (FAILED(MFCreateAsyncResult(nullptr, callback, unkState, &async_res)))
		throw std::runtime_error("MFCreateAsyncResult failed");
#endif

	hr = callback->Invoke(async_res); // will trigger EndWrite
	return hr;
	}

HRESULT STDMETHODCALLTYPE DataStream::EndWrite(/*in*/IMFAsyncResult* /*result*/, /*out*/ULONG* cbWritten)
	{
	*cbWritten = m_tmp_bytes_written;
	m_tmp_bytes_written = 0;

	m_mutex.unlock();
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::Seek(/*in*/MFBYTESTREAM_SEEK_ORIGIN /*SeekOrigin*/, /*in*/LONGLONG /*SeekOffset*/,/*in*/DWORD /*SeekFlags*/, /*out*/QWORD* /*CurrentPosition*/)
	{
	return E_NOTIMPL;
	}

HRESULT STDMETHODCALLTYPE DataStream::Flush()
	{
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE DataStream::Close()
	{
	std::lock_guard<std::mutex> lock(m_mutex);

	// m_impl.reset();
	return S_OK;
	}
