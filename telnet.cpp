#include <assert.h>
#include <map>
#include <queue>
#include <ctype.h>
#include <deque>
#include <queue>
#include <vector>

/*!
**
** Copyright (c) 2009 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliffscarab@gmail.com
** Personal website: http://jratcliffscarab.blogspot.com
** Coding Website:   http://codesuppository.blogspot.com
** FundRaising Blog: http://amillionpixels.blogspot.com
** Fundraising site: http://www.amillionpixels.us
** New Temple Site:  http://newtemple.blogspot.com
**
**
** The MIT license:
**
** Permission is hereby granted, freeof charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

//***********************************************************************************
//*** This code snippet allows you to embed a telnet client or server easily into any
//*** of your applications.  Currently this code only builds for Windows and the XBOX
//*** It is very desirable to me to have this code build for APPLE, Linux, and Iphone
//*** If you wish to contribute to this cause, please let me know and I will add you
//*** as a developer to the google code page.  http://code.google.com/p/telnet
//***
//*** To test the program simply run 'telnet.exe'.
//***
//*** The first time you run it, it will be set up as a telnet server.  You can
//*** connect to it by runing a telnet client and typing 'open localhost 23'
//*** If you run the program a second time, it will detect that a server is already
//*** using Port 23 and will instead start up as a client.  You can now send messages
//*** between the two instances of the application.
//*** You can keep launching as many of them as you wish.
//***********************************************************************************


#if defined(_XBOX)
	#include "NxXBOX.h"
	#include <winsockx.h>
#endif

#if defined(WIN32)
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x400
	#endif
	#include <windows.h>
#endif

#if defined(_XBOX)
	#include "NxXBOX.h"
#endif

#if defined(__APPLE__) || defined(LINUX)
	#include <pthread.h>
#endif

#include "telnet.h"

// Common Telnet Functionality.
#ifdef WIN32
#pragma warning(disable:4786 4996)
#pragma comment(lib,"wsock32.lib")
#endif

TELNET::Telnet *gTelnet=0; // optional global variable representing the TELNET singleton for the application.

namespace TELNET
{

//******************************************************
//*** Mutex layer
//******************************************************


class OdfMutex
{
	public:
		OdfMutex(void);
		~OdfMutex(void);

	public:
		// Blocking Lock.
		void Lock(void);

		// Non-blocking Lock. Return's false if already locked.
		bool TryLock(void);

		// Unlock.
		void Unlock(void);

	private:
		#if defined(WIN32) || defined(_XBOX)
		CRITICAL_SECTION m_Mutex;
		#elif defined(__APPLE__) || defined(LINUX)
		pthread_mutex_t  m_Mutex;
		#endif
};

OdfMutex::OdfMutex(void)
{
#if defined(WIN32) || defined(_XBOX)
	InitializeCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
	pthread_mutex_init(&m_Mutex, 0);
#endif
}

OdfMutex::~OdfMutex(void)
{
#if defined(WIN32) || defined(_XBOX)
	DeleteCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
	pthread_mutex_destroy(&m_Mutex);
#endif
}

// Blocking Lock.
void OdfMutex::Lock(void)
{
#if defined(WIN32) || defined(_XBOX)
	EnterCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
	pthread_mutex_lock(&m_Mutex);
#endif
}

// Non-blocking Lock. Return's false if already locked.
bool OdfMutex::TryLock(void)
{
	bool bRet = false;
#if defined(WIN32) || defined(_XBOX)
	//assert(("TryEnterCriticalSection seems to not work on XP???", 0));
	bRet = TryEnterCriticalSection(&m_Mutex) ? true : false;
#elif defined(__APPLE__) || defined(LINUX)
	pthread_mutex_trylock(&m_Mutex)
#endif
	return bRet;
}

// Unlock.
void OdfMutex::Unlock(void)
{
#if defined(WIN32) || defined(_XBOX)
	LeaveCriticalSection(&m_Mutex);
#elif defined(__APPLE__) || defined(LINUX)
	pthread_mutex_unlock(&m_Mutex)
#endif
}

//******************************************************
//*** End Of Mutex layer
//******************************************************

//******************************************************
//*** Threading layer
//******************************************************

class OdfThread
{
	protected:
		// Called when the thread is started. This method runs on the Thread.
		virtual void OnThreadExecute(void)=0;

	public:
		OdfThread(void);
	 ~OdfThread(void);

	public:
		// Start execution of the thread.
		void ThreadExecute(void);

		// Wait for the Thread to terminate.
		void ThreadWait(void);

		// Stop the thread's execution (not safe).
		void ThreadKill(void);

	protected:
		// Safely Quit the thread.
		void ThreadQuit(void);

	private:
			HANDLE m_hThread;

			friend static DWORD WINAPI _ODFThreadFunc(LPVOID arg);
};


static DWORD WINAPI _ODFThreadFunc(LPVOID arg)
{
	((OdfThread *)arg)->OnThreadExecute();
	return 0;
}



OdfThread::OdfThread(void)
{
		m_hThread = 0;
}

OdfThread::~OdfThread(void)
{
		assert(!m_hThread);
}

// Start execution of the thread.
void OdfThread::ThreadExecute(void)
{
		if(!m_hThread)
		{
			m_hThread = CreateThread(0, 0, _ODFThreadFunc, this, 0, 0);
		}
}

// Wait for the Thread to terminate.
void OdfThread::ThreadWait(void)
{
		if(m_hThread)
		{
			WaitForSingleObject(m_hThread, INFINITE);
		}
}

// Safely Quit the thread.
void OdfThread::ThreadQuit(void)
{
		if(m_hThread)
		{
			m_hThread = 0;
			ExitThread(0);
		}
}

// Stop the thread's execution (not safe).
void OdfThread::ThreadKill(void)
{
		if(m_hThread)
		{
			#if defined(WIN32)
			TerminateThread(m_hThread, 0);
			#endif
			#if defined(_XBOX)
			//-- TODO: Please figure out the equivalent of TerminateThread
			assert(false);
			#endif
			CloseHandle(m_hThread);
			m_hThread = 0;
		}
}



//******************************************************
//*** End of Threading layer
//******************************************************

//******************************************************
//** The Telnet header file
//******************************************************


class TelnetLineNode;
typedef std::queue<TelnetLineNode*> TelnetLineNodeQueue;

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif


class TelnetBlock
{
public:
  TelnetBlock(void)
  {
    m_uiClient = 0;
    m_pcName = 0;
    m_pData  = 0;
    m_uiDataSize = 0;
  }

  ~TelnetBlock(void)
  {
    Free();
  }

  void setBlock(unsigned int client,const char *name,const void *mem,unsigned int len)
  {
    Free();
    m_uiClient = client;
    if ( name )
    {
      size_t len = strlen(name);
      m_pcName = new char[len+1];
      strcpy(m_pcName,name);
    }
    if ( mem && len )
    {
      m_pData = new char[len];
      memcpy(m_pData,mem,len);
      m_uiDataSize = len;
    }
  }

  const void *GetData(void) { return m_pData; }
	unsigned int GetSize(void) { return m_uiDataSize; }

	void Free(void)
	{
		if(m_pcName)
		{
			delete [] m_pcName;
			m_pcName = 0;
		}
		if(m_pData)
		{
			delete [] m_pData;
			m_pData = 0;
		}
	}
		unsigned int  m_uiClient;
		char   *m_pcName;
		void   *m_pData;
		unsigned int  m_uiDataSize;
};

typedef std::queue<TelnetBlock*> TelnetBlockQueue;

// Common Telnet Functionality.
class TelnetInterface
{
	public:
		// Called when a connection has been established.
		virtual void OnConnect(unsigned int uiClient) {}

	public:
		// Sends text across the telnet connection.
		// returns false on failure.
		virtual bool SendText(unsigned int uiClient, const char *pcLine, ...)=0;

		// Sends a binary block of data across the telnet session.
		// returns false on failure.
		virtual bool SendBlock(const TelnetBlock &block)=0;

	public:
		// Waits until there is a block ready to be read.
		bool WaitForBlock(void);

		// Pops the last line off the local queue.
		// returns 0 if no lines available.
		const char *GetLine(unsigned int &uiClient);

		// Pops the last binary block off the local queue.
		// returns 0 if no lines available.
		bool GetBlock(TelnetBlock &block);

		void ReleaseBlock(TelnetBlock &block);

	protected:
		// Add a Line to the Local Queue.
		void PushLine(const char *pcLine, int len, unsigned int uiClient);

		// Add a block to the local queue.
		void PushBlock(TelnetBlock *pBlock);

	protected:
		TelnetInterface(void);
		TelnetInterface(const TelnetInterface&){}
	 ~TelnetInterface(void);

	private:
		OdfMutex              m_LineMutex;
		TelnetLineNodeQueue   m_Lines;
		char                 *m_pcLineBuffer;

		OdfMutex              m_BlockMutex;
		OdfMutex              m_HaveBlock;
		TelnetBlockQueue      m_Blocks;
};

#ifdef WIN32
#pragma warning(pop)
#endif


//******************************************************
//** The Telnet header file
//******************************************************


class TelnetLineNode
{
	public:
		char         *pcLine;
		unsigned int  uiClient;
};

// Waits until there is a block ready to be read.
bool TelnetInterface::WaitForBlock(void)
{
	bool ret = false;
	if(!m_Blocks.empty()) return true;
	m_HaveBlock.Lock();
	if(!m_Blocks.empty()) ret = true;
	m_HaveBlock.Unlock();
	return ret;
}

// Pops the last line off the local queue.
// returns 0 if no lines available.
const char *TelnetInterface::GetLine(unsigned int &uiClient)
{
	const char *pRet = 0;
	
	m_LineMutex.Lock();
	if(!m_Lines.empty())
	{
		TelnetLineNode *node = m_Lines.front();
		m_Lines.pop();
		
		if(m_pcLineBuffer)
		{
			::free(m_pcLineBuffer);
			m_pcLineBuffer = 0;
		}
		
		m_pcLineBuffer  = node->pcLine;
		uiClient        = node->uiClient;

		delete node;
		
		pRet            = m_pcLineBuffer;
	}
	m_LineMutex.Unlock();
	
	return pRet;
}

// Pops the last binary block off the local queue.
// returns 0 if no lines available.
bool  TelnetInterface::GetBlock(TelnetBlock &block)
{
	bool ret = false;
	
	m_BlockMutex.Lock();
	if(!m_Blocks.empty())
	{
		TelnetBlock *pBlock = m_Blocks.front();
		m_Blocks.pop();
		memcpy(&block, pBlock, sizeof(TelnetBlock));
		delete pBlock;
		ret = true;
	}
	m_BlockMutex.Unlock();

	if(m_Blocks.empty())
	{
		m_HaveBlock.TryLock();
	}

	return ret;
}

void TelnetInterface::ReleaseBlock(TelnetBlock &block)
{
	block.Free();
}


// Add a Line to the Local Queue.
void TelnetInterface::PushLine(const char *pcLine, int len, unsigned int uiClient)
{
	if (len <= 0 )
	{
		return;
	}

	m_LineMutex.Lock();
	TelnetLineNode *node = new TelnetLineNode;
	//node->pcLine = (char*)::malloc(sizeof(char)*(strlen(pcLine)+1));
	node->pcLine = (char*)::malloc(sizeof(char)*(len +1));
	memcpy(node->pcLine, pcLine, len);
	node->pcLine[len] = '\0';
	node->uiClient = uiClient;
	m_Lines.push(node);
	m_LineMutex.Unlock();
}

// Add a block to the local queue.
void TelnetInterface::PushBlock(TelnetBlock *pBlock)
{
	assert(pBlock);
	m_BlockMutex.Lock();
	m_Blocks.push(pBlock);
	m_BlockMutex.Unlock();

	m_HaveBlock.TryLock();
	m_HaveBlock.Unlock();
}

TelnetInterface::TelnetInterface(void)
{
	m_pcLineBuffer = 0;
	m_HaveBlock.Lock();
}

TelnetInterface::~TelnetInterface(void)
{
	if(m_pcLineBuffer)
	{
		::free(m_pcLineBuffer);
	}

	while(!m_Lines.empty())
	{
		TelnetLineNode *pLine = m_Lines.front();
		::free(pLine->pcLine);
		delete pLine;
		m_Lines.pop();
	}

	m_HaveBlock.TryLock();
	m_HaveBlock.Unlock();
}

//************************************************************************************
//** The Telnet Parser header file.
//************************************************************************************


typedef std::queue<char*> StringQueue;

class _Block
{
	public:
		void         *m_pData;
		unsigned int  m_uiDataSize;
};

typedef std::queue<_Block> BlockQueue;


class TelnetParser
{
	public:
		TelnetParser(void)
		{
			m_uiBufferSize  = 1024;
			m_uiBufferUsed  = 0;
      m_pBuffer       = (char *)::malloc(sizeof(char)*m_uiBufferSize);
			m_pcLastLine    = 0;

			m_uiCurrBlockSize = 1024;
			m_uiCurrBlockUsed = 0;
      m_pCurrBlock      = (char *)::malloc(sizeof(char)*m_uiCurrBlockSize);
			
			m_bBlockMode      = false;
		}
		
	 ~TelnetParser(void)
		{
			if(m_pBuffer)
			{
        ::free(m_pBuffer);
			}
			
			if(m_pcLastLine)
			{
				::free(m_pcLastLine);
			}

			if(m_pCurrBlock)
			{
				::free(m_pCurrBlock);
			}
			
			while(!m_Lines.empty())
			{
				char *pTemp = m_Lines.front();
				m_Lines.pop();
				if(pTemp)
				{
					::free(pTemp);
				}
			}
			
			while(!m_Blocks.empty())
			{
				_Block &block = m_Blocks.front();
				if(block.m_pData)
				{
					::free(block.m_pData);
				}
				m_Blocks.pop();
			}
		}
	 
	public:
		void AddBuffer(const char *pcBuffer, unsigned int uiLen)
		{
			unsigned int uiNewSize = uiLen + m_uiBufferUsed;
			if(uiNewSize >= m_uiBufferSize) Resize(uiNewSize);
			
			unsigned int index = 0;
			if (pcBuffer[index] == -1)
			{
                //Extended ASCII Codes: 255,
				while ((pcBuffer[index] == -1) && (index <= uiLen - 3))
				{
					index += 3;
				}
			}

			if (index >= uiLen)
			{
                //Hard code, for command message transfered with the FreeSSHd telnet server.
                //TODO: need to optimize the design of dealing about control message before login.
				
                /*
                    0xff: IAC
                    0xfb: WILL

                    message(0xff, 0xfb, 0x18) : <IAC, WILL, 24>
                    message(0xff, 0xfd, 0x03) : <IAC, WILL, 3>
                    message(0xff, 0xfd, 0x03) : <IAC, WILL, 3>
                    message(0xff, 0xfd, 0x01) : <IAC, WILL, 1>
                    message(0xff, 0xfd, 0x1f) : <IAC, WILL, 31>
                */
                const char buff[15] = {0xff, 0xfb, 0x18, 0xff, 0xfd, 0x03, 0xff, 0xfb, 0x03, 0xff, 0xfd, 0x01, 0xff, 0xfb, 0x1f};
				gTelnet->sendMessage(0, buff, 15);
				return;
			}

			for(unsigned int i=index; i<uiLen; i++)
			{
				if(m_bBlockMode)
					BlockChar(pcBuffer[i]);
				else
					ParseChar(pcBuffer[i]);
			}
			
			ParseChar('\r');
			ParseChar('\n');

		}
		
		const char *GetLine(void)
		{
			const char *pRet = 0;
			
			if(!m_Lines.empty())
			{
				if(m_pcLastLine)
				{
					::free(m_pcLastLine);
				}
				m_pcLastLine = m_Lines.front();
				m_Lines.pop();
				pRet = m_pcLastLine;
			}
			
			return pRet;
		}
		
		const void *GetBlock(unsigned int &uiSize)
		{
			void *pRet = 0;
			if(!m_Blocks.empty())
			{
				_Block &block = m_Blocks.front();
				pRet = block.m_pData;
				uiSize = block.m_uiDataSize;
				m_Blocks.pop();
			}
			return pRet;
		}
	
	private:
		void ParseChar(char c)
		{
			if(c >= 32 && c < 128)
			{
				// simply add the character.
				m_pBuffer[m_uiBufferUsed++] = c;
			}
			else if(c == '\n' && m_uiBufferUsed)
			{
				// Add a line to the queue.
				char *pcLine = (char*)::malloc(sizeof(char)*(m_uiBufferUsed+1));
				memcpy(pcLine, m_pBuffer, m_uiBufferUsed);
				pcLine[m_uiBufferUsed] = 0;
				m_Lines.push(pcLine);
				m_uiBufferUsed = 0;
			}
			else if(c == 8 && m_uiBufferUsed)
			{
				// if backspace then remove the last character.
				m_uiBufferUsed--;
			}
			else
			{
				// m_pBuffer[m_uiBufferUsed++] = c;
			}
			
			m_pBuffer[m_uiBufferUsed] = 0;
			
			char *pBlockStart = strstr(m_pBuffer, "<NxBlock");
			if(pBlockStart)
			{
				char *equals  = strchr(pBlockStart, '=');
				char *end     = strchr(pBlockStart, '>');
				if(end)
				{
					if(equals && equals < end)
					{
						char *name = equals+1;
						unsigned int len = (unsigned int)(end - name) + 1;
					}
					m_bBlockMode = true;
				}
			}
			
		}
		
		void BlockChar(char c)
		{
			if(m_uiCurrBlockUsed+1 >=  m_uiCurrBlockSize)
			{
				unsigned int uiNewSize = m_uiCurrBlockSize * 2;
				char *pBlock = (char*)::malloc(sizeof(char)*uiNewSize);
				memcpy(pBlock, m_pCurrBlock, m_uiCurrBlockUsed);
				if(m_pCurrBlock)
				{
					::free(m_pCurrBlock);
				}
				m_pCurrBlock = pBlock;
				m_uiCurrBlockSize = uiNewSize;
			}
			
			m_pCurrBlock[m_uiCurrBlockUsed++] = c;
			m_pCurrBlock[m_uiCurrBlockUsed]   = 0;
			
			char *pcEnd = strstr(m_pCurrBlock, "</NxBlock>");
			if(pcEnd)
			{
				m_bBlockMode = false;
				unsigned int uiSize = (unsigned int)(pcEnd - m_pCurrBlock);
				
				_Block block;
				block.m_uiDataSize  = uiSize;
				block.m_pData       = (char*)::malloc(sizeof(char)*uiSize);
				memcpy(block.m_pData, m_pCurrBlock, uiSize);
				m_Blocks.push(block);
				
				for(unsigned int i=uiSize+9; i<m_uiCurrBlockUsed; i++)
				{
					ParseChar(m_pCurrBlock[i]);
				}
				
				m_uiCurrBlockUsed = 0;
			}
		}
		
		void Resize(unsigned int uiNewSize)
		{
			char *pNewBuffer = (char*)::malloc(sizeof(char)*uiNewSize);
			if(m_uiBufferUsed)
			{
				memcpy(pNewBuffer, m_pBuffer, m_uiBufferUsed);
			}
			if(m_pBuffer)
			{
				::free(m_pBuffer);
			}
			m_pBuffer = pNewBuffer;
			m_uiBufferSize = uiNewSize;
		}
	
	private:
		char             *m_pBuffer;
		unsigned int      m_uiBufferSize;
		unsigned int      m_uiBufferUsed;
		
		char             *m_pcLastLine;
		StringQueue       m_Lines;
		
		bool              m_bBlockMode;
		char             *m_pCurrBlock;
		unsigned int      m_uiCurrBlockSize;
		unsigned int      m_uiCurrBlockUsed;
		BlockQueue        m_Blocks;
};



//****************************************************************************************
//** Telnet Client header file
//****************************************************************************************


// Simple Telnet Client.
class TelnetClient : public TelnetInterface
{
	public:
		TelnetClient(void);
	 ~TelnetClient(void);

	public:
		// Connect to a remote Telnet server.
		// returns false on failure.
		bool Connect(const char *pcAddress, unsigned short uiPort);

		// Closes the current connection.
		void Close(void);

	public:
		// Sends text across the telnet connection.
		// returns false on failure.
		virtual bool SendText(unsigned int uiClient, const char *pcLine, ...);

		// Sends a binary block of data across the telnet session.
		// returns false on failure.
		virtual bool SendBlock(const TelnetBlock &block);

		virtual bool SendRawCommand(const char *vcBuffer, unsigned int len);

	private:
		void ThreadFunc(void);
		friend DWORD WINAPI _TelnetClientFunc(LPVOID arg);

	private:
		TelnetClient(const TelnetClient &){}

	private:
		OdfMutex  m_Mutex;
		SOCKET    m_Socket;
		HANDLE    m_Thread;
		bool      m_ControlCmdSent; //Flag for checking: has already send the command message before the login message.  
};

//*************************************************************************************
//** The Telnet Client source code
//*************************************************************************************

	static DWORD WINAPI _TelnetClientFunc(LPVOID arg)
	{
		((TelnetClient *)arg)->ThreadFunc();
		ExitThread(0);
		return 0;
	}


void TelnetClient::ThreadFunc(void)
{
	bool bDone = false;

	m_Mutex.Lock();
	SOCKET  clientSocket  = m_Socket;
	bDone = clientSocket == INVALID_SOCKET ? true : false;
	m_Mutex.Unlock();
	
	#define BUFFER_SIZE 8192
	char vcBuffer[BUFFER_SIZE+1];
	TelnetParser parser;
	
	
	while(!bDone)
	{
		int iBytesRead = 0;
		iBytesRead = recv(clientSocket, vcBuffer, BUFFER_SIZE, 0);

		if(iBytesRead <= 0)
		{
			bDone = true;
			m_Mutex.Lock();
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
			m_Socket = clientSocket;
			m_Mutex.Unlock();
		}
		else
		{
			//parser.AddBuffer(vcBuffer, iBytesRead);

			int index = 0;
			if (vcBuffer[index] == -1)
			{
				if (!m_ControlCmdSent)
				{
					// Define the echo message before Login message.
                    // There will be three message between client and server according telnet protocal.
                    // In other word, client and server must transfer some command. 
					const char buff[16] = {0xff, 0xfb, 0x18, 0xff, 0xfd, 0x03, 0xff, 0xfb, 0x03, 0xff, 0xfd, 0x01, 0xff, 0xfb, 0x1f, 0};
					gTelnet->sendMessage(0, buff, 15);
					m_ControlCmdSent = true; //Command message has been send to server. 
				}
			}

            bool hasReadableChar= false;
			int flag = 0;
			int lineStart = 0;
			while (index < iBytesRead)
			{
				unsigned char c = (unsigned char) vcBuffer[index];
				switch(flag)
				{
				case 0:
					if (c == 255)
					{
						// jump 3 (one commend is marked by 3 bytes.),
                        // Command message is started by IAC(255), followed by one byte for action verb, last with the other message bye.
                        // BYTE + BYTE + BYTE
                        // We just want to collect the readable char, so we jump the command positions.
						index += 3;
						continue;
					}
					else if(c >= 32 && c < 128)
					{
                        hasReadableChar = true;
						// If current char is readable, so mark the position.
						lineStart = index; //mark the start position of readable message.
						flag = 1; // Set the flag for find the end of this readable line.
						index ++;
						continue;
					}
					else
					{
						// we do not consider the other condition, just add the index, and find the next.
						index++;
						continue;
					}
					break;

				case 1:
					if (c == '\n')
					{
						// Save the readable message(signed char) in current line.
						PushLine(vcBuffer + lineStart, (index - lineStart + 1), 0);
						lineStart = index;
						flag = 0; // Current line is end, start to deal with the new line.
					}
					index++;
					break;
				}
			}
			
            // For the exception conditon: If this message has no '\n' to end.
			if ((index > lineStart) && hasReadableChar)
			{
				// If there has some readable char in current message, push filtered message in buffer(,and display in clinet windows).
				PushLine(vcBuffer + lineStart, (index - lineStart), 0);
			}

		}

		m_Mutex.Lock();
		clientSocket  = m_Socket;
		bDone = clientSocket == INVALID_SOCKET ? true : false;
		m_Mutex.Unlock();
	}
	
}

/*****************
** TelnetClient **
*****************/

TelnetClient::TelnetClient(void)
{
	m_Socket  = INVALID_SOCKET;
	m_Thread  = 0;
	m_ControlCmdSent = false;

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
}

TelnetClient::~TelnetClient(void)
{
	 Close();
}

// Connect to a remote Telnet server.
// returns false on failure.
bool TelnetClient::Connect(const char *pcAddress, unsigned short uiPort)
{
	bool bRet = false;
	Close();

	m_Thread = 0;

#if defined(WIN32)
	
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_Socket != INVALID_SOCKET)
	{
		sockaddr_in addr = {0};
		addr.sin_family       = AF_INET;
		addr.sin_port         = htons(uiPort);
		addr.sin_addr.s_addr	= inet_addr(pcAddress);
		
		if(addr.sin_addr.s_addr == INADDR_NONE)
		{
			hostent *pHost = gethostbyname(pcAddress);
			memcpy(&addr.sin_addr, pHost->h_addr, pHost->h_length);
			addr.sin_family = pHost->h_addrtype;
		}
		
		if(connect(m_Socket, (sockaddr *)&addr, sizeof(addr)) == 0)
		{
			OnConnect(0);
			m_Thread = CreateThread(0, 0, _TelnetClientFunc, this, 0, 0);
			bRet = true;
		}
		else
		{
			//assert(0);
            return false;
		}
		
		if(!m_Thread)
		{
			int error = WSAGetLastError();
			closesocket(m_Socket);
			m_Socket = INVALID_SOCKET;
		}
	}
	else
	{
		//assert(0);
        return false;
	}
	
#endif
	
	return bRet;
}

// Closes the current connection.
void TelnetClient::Close(void)
{
	if(m_Socket != INVALID_SOCKET)
	{
		// Close the Socket.
		m_Mutex.Lock();
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		m_Mutex.Unlock();
		
		// Wait for the Listen Thread to stop.
		WaitForSingleObject(m_Thread, INFINITE);
		m_Thread = 0;
	}
}

bool TelnetClient::SendRawCommand(const char *vcBuffer, unsigned int len)
{
	send(m_Socket, vcBuffer, len, 0);
	return true;
}

// Sends text across the telnet connection.
// returns false on failure.
bool TelnetClient::SendText(unsigned int uiClient, const char *pcLine, ...)
{
	char vcBuffer[8192];
	_vsnprintf(vcBuffer,8191, pcLine, (va_list)(&pcLine+1));
	unsigned int uiLen = (unsigned int)strlen(vcBuffer);
	
	send(m_Socket, vcBuffer, uiLen, 0);
	
	return true;
}

// Sends a binary block of data across the telnet session.
// returns false on failure.
bool TelnetClient::SendBlock(const TelnetBlock &block)
{
	bool          bRet        = false;
	char          vcFooter[]  = "</NxBlock>\r\n";
	char          vcHeader[256];
	
	sprintf(vcHeader, "\r\n<NxBlock=%s>", block.m_pcName);
	
	send(m_Socket, vcHeader, (int)strlen(vcHeader),0);

	// Send in small chunks...
	const char *pData = (const char*)block.m_pData;
	unsigned int uiDataSent = 0;
	while(uiDataSent < block.m_uiDataSize)
	{
		unsigned int uiSendSize = block.m_uiDataSize - uiDataSent;
		uiSendSize = uiSendSize > 1024 ? 1024 : uiSendSize;
		int iSent = send(m_Socket, &pData[uiDataSent], uiSendSize, 0);
		uiDataSent += (unsigned int)iSent;
	} 
	
	send(m_Socket, vcFooter,(int)strlen(vcFooter),   0);
	bRet = true;
	
	return bRet;
}

//****************************************************************************
//** Telnet server header file
//***************************************************************************


class TelnetServer_Client;
typedef std::map<unsigned int, TelnetServer_Client*> TelnetServer_ClientMap;

// Simple Telnet Server.
class TelnetServer : public TelnetInterface
{
	friend class TelnetServer_Client;

	public:
		TelnetServer(void);
	 ~TelnetServer(void);

	public:
		// Starts the Telnet server listening on
		// the specified port.
		// returns false if failure.
		bool Listen(unsigned short uiPort);

		// Closes all connections.
		void Close(void);

	public:
		// Sends text across the telnet connection.
		// returns false on failure.
		virtual bool SendText(unsigned int uiClient, const char *pcLine, ...);

		// Sends a binary block of data across the telnet session.
		// returns false on failure.
		virtual bool SendBlock(const TelnetBlock &block);

	private:
		void ThreadFunc(void);
		friend DWORD WINAPI _TelnetServerFunc(LPVOID arg);

	private:
		TelnetServer(const TelnetServer &){}

	private:
		unsigned int            m_uiLastClient;

		OdfMutex                m_ListenMutex;
		SOCKET                  m_ListenSocket;
		HANDLE                  m_ListenThread;

		TelnetServer_ClientMap  m_Clients;
};

//****************************************************************************
//** Telnet server source
//****************************************************************************

class TelnetServer_Client
{
	public:
		TelnetServer *m_pParent;
		unsigned int  m_uiClient;
		OdfMutex      m_Mutex;
		SOCKET        m_Socket;
		HANDLE        m_Thread;
	
	public:
	 ~TelnetServer_Client(void)
		{
			TelnetServer_ClientMap::iterator iter;
			iter = m_pParent->m_Clients.find(m_uiClient);
			
			if(iter != m_pParent->m_Clients.end())
			{
				m_pParent->m_Clients[m_uiClient] = 0;
				//m_pParent->m_Clients.erase(iter);
			}
		}
	
	private:
		void ThreadFunc(void);
		friend DWORD WINAPI _TelnetServerClientFunc(LPVOID arg);
};


/**********************
** Threading Support **
**********************/

	static DWORD WINAPI _TelnetServerClientFunc(LPVOID arg)
	{
		TelnetServer_Client *pClient = (TelnetServer_Client *)arg;
		pClient->ThreadFunc();
		delete pClient;
		ExitThread(0);
		return 0;
	}

	static DWORD WINAPI _TelnetServerFunc(LPVOID arg)
	{
		((TelnetServer *)arg)->ThreadFunc();
		ExitThread(0);
		return 0;
	}

void TelnetServer_Client::ThreadFunc(void)
{
	bool bDone = false;
	OdfMutex &mutex = m_Mutex;
	
	#define BUFFER_SIZE 8192
	char vcBuffer[BUFFER_SIZE];
	
	TelnetParser parser;
	
	mutex.Lock();
	SOCKET  clientSocket  = m_Socket;
	bDone = clientSocket == INVALID_SOCKET ? true : false;
	mutex.Unlock();
	
	while(!bDone)
	{
	int iBytesRead=0;

	#if !defined(_XBOX)
	iBytesRead = recv(clientSocket, vcBuffer, BUFFER_SIZE, 0);
	#endif
		
		if(iBytesRead > 0)
		{
			parser.AddBuffer(vcBuffer, iBytesRead);
		}

		const char *pcLine = 0;
		
		while((pcLine = parser.GetLine()))
		{
			m_pParent->PushLine(pcLine, strlen(pcLine), m_uiClient);
		}
		
		mutex.Lock();
		clientSocket  = m_Socket;
		bDone = clientSocket == INVALID_SOCKET ? true : false;
		mutex.Unlock();
	}


}

void TelnetServer::ThreadFunc(void)
{
	bool bDone = false;
	
	m_ListenMutex.Lock();
	SOCKET listenSocket = m_ListenSocket;
	bDone = listenSocket == INVALID_SOCKET ? true : false;
	m_ListenMutex.Unlock();
	
	while(!bDone)
	{
		SOCKET clientSocket= INVALID_SOCKET;
		#if !defined(_XBOX)
		clientSocket = accept(listenSocket, 0, 0);
		#endif
		if(clientSocket != INVALID_SOCKET)
		{
			TelnetServer_Client *pClient = new TelnetServer_Client;
			pClient->m_pParent  = this;
			pClient->m_uiClient = ++m_uiLastClient;
			pClient->m_Socket   = clientSocket;
			m_Clients[pClient->m_uiClient] = pClient;
			pClient->m_Thread   = CreateThread(0, 0, _TelnetServerClientFunc, pClient, 0, 0);
			
			OnConnect(pClient->m_uiClient);
		}
		
		m_ListenMutex.Lock();
		listenSocket = m_ListenSocket;
		bDone = listenSocket == INVALID_SOCKET ? true : false;
		m_ListenMutex.Unlock();
	}
	
	TelnetServer_ClientMap::iterator iter;
	for(iter=m_Clients.begin(); iter!=m_Clients.end(); iter++)
	{
		TelnetServer_Client *pClient = (*iter).second;
		if(pClient)
		{
			OdfMutex &mutex = pClient->m_Mutex;
			
			// Signal the socket.
			mutex.Lock();
			#if !defined(_XBOX)
			closesocket(pClient->m_Socket);			
			#endif
			pClient->m_Socket = INVALID_SOCKET;
			mutex.Unlock();
			
			// Wait for the thread to stop.
			WaitForSingleObject(pClient->m_Thread, INFINITE);
		}
	}
	m_Clients.clear();
	
}

/*****************
** TelnetServer **
*****************/

TelnetServer::TelnetServer(void)
{
	m_uiLastClient = 1;

	m_ListenSocket  = INVALID_SOCKET;
	m_ListenThread  = 0;

	#if !defined(_XBOX)
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	#endif
}

TelnetServer::~TelnetServer(void)
{
	Close();
}

// Starts the Telnet server listening on
// the specified port.
// returns false if failure.
bool TelnetServer::Listen(unsigned short uiPort)
{
	bool bRet = false;

	Close();
	
	m_ListenThread  = 0;

	#if !defined(_XBOX)
	m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(m_ListenSocket != INVALID_SOCKET)
	{
		sockaddr_in addr      = {0};
		addr.sin_family       = AF_INET;
		addr.sin_port         = htons(uiPort);
		addr.sin_addr.s_addr  = htonl(INADDR_ANY);
		
		if(bind(m_ListenSocket, (sockaddr*)&addr, sizeof(addr)) == 0)
		{
			if(listen(m_ListenSocket, SOMAXCONN) == 0)
			{
				m_ListenThread = CreateThread(0, 0, _TelnetServerFunc, this, 0, 0);
				bRet = true;
			}
			else
			{
				int error = WSAGetLastError();
				assert(0);
			}
		}
		else // if(bind(m_ListenSocket, (sockaddr*)&addr, sizeof(addr)) == 0)
		{
			int error = WSAGetLastError();
//			assert(0);
		}
		
		if(!m_ListenThread)
		{
			int error = WSAGetLastError();
			closesocket(m_ListenSocket);
			m_ListenSocket = INVALID_SOCKET;
		}
	}
	else // if(m_ListenSocket != INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		assert(0);
	}
	

	
//	assert(m_ListenSocket != INVALID_SOCKET);

	#endif

	
	return bRet;
}

// Closes all connections.
void TelnetServer::Close(void)
{
	if(m_ListenSocket != INVALID_SOCKET)
	{
		// Close the Socket.
		m_ListenMutex.Lock();
		#if !defined(_XBOX)		
		closesocket(m_ListenSocket);
		#endif
		m_ListenSocket = INVALID_SOCKET;
		m_ListenMutex.Unlock();
		
		// Wait for the Listen Thread to stop.
		WaitForSingleObject(m_ListenThread, INFINITE);
		m_ListenThread = 0;
	}
}

// Sends text across the telnet connection.
// returns false on failure.
bool TelnetServer::SendText(unsigned int uiClient, const char *pcLine, ...)
{
	bool bRet = false;
	
	TelnetServer_ClientMap::iterator iter;
	
	char vcBuffer[8192];
	_vsnprintf(vcBuffer, 8191, pcLine, (va_list)(&pcLine+1));
	unsigned int uiLen = (unsigned int)strlen(vcBuffer);
	
	if(!uiClient)
	{
		for(iter=m_Clients.begin(); iter!=m_Clients.end(); iter++)
		{
			TelnetServer_Client *pClient  = (*iter).second;
			pClient->m_Mutex.Lock();
			#if !defined(_XBOX)
			send(pClient->m_Socket, vcBuffer, uiLen, 0);
			#endif
			pClient->m_Mutex.Unlock();
			bRet = true;
		}
	}
	else
	{
		iter = m_Clients.find(uiClient);
		if(iter != m_Clients.end())
		{
			TelnetServer_Client *pClient  = (*iter).second;
			pClient->m_Mutex.Lock();
			#if !defined(_XBOX)			
			send(pClient->m_Socket, vcBuffer, uiLen, 0);
			#endif
			pClient->m_Mutex.Unlock();
			bRet = true;
		}
	}
	
	return bRet;
}

// Sends a binary block of data across the telnet session.
// returns false on failure.
bool TelnetServer::SendBlock(const TelnetBlock &block)
{
	unsigned int  uiClient    = block.m_uiClient;
	bool          bRet        = false;
	char          vcFooter[]  = "</NxBlock>\r\n";
	char          vcHeader[256];
	
	sprintf(vcHeader, "\r\n<NxBlock=%s>", block.m_pcName);

	TelnetServer_ClientMap::iterator iter;
	
	if(!uiClient)
	{
		for(iter=m_Clients.begin(); iter!=m_Clients.end(); iter++)
		{
			TelnetServer_Client *pClient  = (*iter).second;
			pClient->m_Mutex.Lock();
			#if !defined(_XBOX)			
			send(pClient->m_Socket, vcHeader,               (int)strlen(vcHeader),   0);
			send(pClient->m_Socket, (char*)block.m_pData,   block.m_uiDataSize, 0);
			send(pClient->m_Socket, vcFooter,               (int)strlen(vcFooter),   0);
			#endif
			pClient->m_Mutex.Unlock();
			bRet = true;
		}
	}
	else
	{
		iter = m_Clients.find(uiClient);
		if(iter != m_Clients.end())
		{
			TelnetServer_Client *pClient  = (*iter).second;
			pClient->m_Mutex.Lock();
			
			#if !defined(_XBOX)
			send(pClient->m_Socket, vcHeader,               (int)strlen(vcHeader),   0);
			
			// Send in small chunks...
			const char *pData = (const char*)block.m_pData;
			unsigned int uiDataSent = 0;
			while(uiDataSent < block.m_uiDataSize)
			{
				unsigned int uiSendSize = block.m_uiDataSize - uiDataSent;
				uiSendSize = uiSendSize > 1024 ? 1024 : uiSendSize;
				int iSent = send(pClient->m_Socket, &pData[uiDataSent], uiSendSize, 0);
				uiDataSent += (unsigned int)iSent;
			}

			send(pClient->m_Socket, vcFooter,               (int)strlen(vcFooter),   0);
			#endif


			pClient->m_Mutex.Unlock();
			bRet = true;
		}
	}

	return bRet;
}

#if defined(__APPLE__) || defined(__CELLOS_LV2__) || defined(LINUX)
#define stricmp(a, b) strcasecmp((a), (b))
#endif

/*******************************************************************/
/******************** InParser.h  ********************************/
/*******************************************************************/
class InPlaceParserInterface
{
public:
	virtual int ParseLine(int lineno,int argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
};

enum SeparatorType
{
	ST_DATA,        // is data
	ST_HARD,        // is a hard separator
	ST_SOFT,        // is a soft separator
	ST_EOS          // is a comment symbol, and everything past this character should be ignored
};

class InPlaceParser
{
public:
	InPlaceParser(void)
	{
		Init();
	}

	InPlaceParser(char *data,int len)
	{
		Init();
		SetSourceData(data,len);
	}

	InPlaceParser(const char *fname)
	{
		Init();
		SetFile(fname);
	}

	~InPlaceParser(void);

	void Init(void)
	{
		mQuoteChar = 34;
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;
		for (int i=0; i<256; i++)
		{
			mHard[i] = ST_DATA;
			mHardString[i*2] = (char)i;
			mHardString[i*2+1] = 0;
		}
		mHard[0]  = ST_EOS;
		mHard[32] = ST_SOFT;
		mHard[9]  = ST_SOFT;
		mHard[13] = ST_SOFT;
		mHard[10] = ST_SOFT;
	}

	void SetFile(const char *fname); // use this file as source data to parse.

	void SetSourceData(char *data,int len)
	{
		mData = data;
		mLen  = len;
		mMyAlloc = false;
	};

	int  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

	int ProcessLine(int lineno,char *line,InPlaceParserInterface *callback);

	const char ** GetArglist(char *source,int &count); // convert source string into an arg list, this is a destructive parse.

	void SetHardSeparator(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}

	void SetHard(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}


	void SetCommentSymbol(char c) // comment character, treated as 'end of string'
	{
		mHard[c] = ST_EOS;
	}

	void ClearHardSeparator(char c)
	{
		mHard[c] = ST_DATA;
	}


	void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

	bool EOS(char c)
	{
		if ( mHard[c] == ST_EOS )
		{
			return true;
		}
		return false;
	}

	void SetQuoteChar(char c)
	{
		mQuoteChar = c;
	}

private:


	inline char * AddHard(int &argc,const char **argv,char *foo);
	inline bool   IsHard(char c);
	inline char * SkipSpaces(char *foo);
	inline bool   IsWhiteSpace(char c);
	inline bool   IsNonSeparator(char c); // non seperator,neither hard nor soft

	bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
	char  *mData;  // ascii data to parse.
	int    mLen;   // length of data
	SeparatorType  mHard[256];
	char   mHardString[256*2];
	char           mQuoteChar;
};

/*******************************************************************/
/******************** InParser.cpp  ********************************/
/*******************************************************************/
void InPlaceParser::SetFile(const char *fname)
{
	if ( mMyAlloc )
	{
		::free(mData);
	}
	mData = 0;
	mLen  = 0;
	mMyAlloc = false;

	FILE *fph = fopen(fname,"rb");
	if ( fph )
	{
		fseek(fph,0L,SEEK_END);
		mLen = ftell(fph);
		fseek(fph,0L,SEEK_SET);
		if ( mLen )
		{
			mData = (char *) ::malloc(sizeof(char)*(mLen+1));
			int ok = (int)fread(mData, mLen, 1, fph);
			if ( !ok )
			{
				::free(mData);
				mData = 0;
			}
			else
			{
				mData[mLen] = 0; // zero byte terminate end of file marker.
				mMyAlloc = true;
			}
		}
		fclose(fph);
	}
}

InPlaceParser::~InPlaceParser(void)
{
	if ( mMyAlloc )
	{
		::free(mData);
	}
}

#define MAXARGS 512

bool InPlaceParser::IsHard(char c)
{
	return mHard[c] == ST_HARD;
}

char * InPlaceParser::AddHard(int &argc,const char **argv,char *foo)
{
	while ( IsHard(*foo) )
	{
		const char *hard = &mHardString[*foo*2];
		if ( argc < MAXARGS )
		{
			argv[argc++] = hard;
		}
		foo++;
	}
	return foo;
}

bool   InPlaceParser::IsWhiteSpace(char c)
{
	return mHard[c] == ST_SOFT;
}

char * InPlaceParser::SkipSpaces(char *foo)
{
	while ( !EOS(*foo) && IsWhiteSpace(*foo) ) foo++;
	return foo;
}

bool InPlaceParser::IsNonSeparator(char c)
{
	if ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 ) return true;
	return false;
}


int InPlaceParser::ProcessLine(int lineno,char *line,InPlaceParserInterface *callback)
{
	int ret = 0;

	const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{

		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) ) break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			foo++;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				foo++;
			}
		}
		else
		{

			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					foo++;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar ) foo++;
					if ( *foo ) *foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						foo++;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						foo++;
						break;
					}
					foo++;
				} // end of while loop...
			}
		}
	}

	if ( argc )
	{
		ret = callback->ParseLine(lineno, argc, argv );
	}

	return ret;
}

int  InPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
{
	assert( callback );
	if ( !mData ) return 0;

	int ret = 0;

	int lineno = 0;

	char *foo   = mData;
	char *begin = foo;


	while ( *foo )
	{
		if ( *foo == 10 || *foo == 13 )
		{
			lineno++;
			*foo = 0;

			if ( *begin ) // if there is any data to parse at all...
			{
				int v = ProcessLine(lineno,begin,callback);
				if ( v ) ret = v;
			}

			foo++;
			if ( *foo == 10 ) foo++; // skip line feed, if it is in the carraige-return line-feed format...
			begin = foo;
		}
		else
		{
			foo++;
		}
	}

	lineno++; // lasst line.

	int v = ProcessLine(lineno,begin,callback);
	if ( v ) ret = v;
	return ret;
}


void InPlaceParser::DefaultSymbols(void)
{
	SetHardSeparator(',');
	SetHardSeparator('(');
	SetHardSeparator(')');
	SetHardSeparator('=');
	SetHardSeparator('[');
	SetHardSeparator(']');
	SetHardSeparator('{');
	SetHardSeparator('}');
	SetCommentSymbol('#');
}


const char ** InPlaceParser::GetArglist(char *line,int &count) // convert source string into an arg list, this is a destructive parse.
{
	const char **ret = 0;

	static const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{

		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) ) break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			foo++;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				foo++;
			}
		}
		else
		{

			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					foo++;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar ) foo++;
					if ( *foo ) *foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						foo++;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						foo++;
						break;
					}
					foo++;
				} // end of while loop...
			}
		}
	}

	count = argc;
	if ( argc )
	{
		ret = argv;
	}

	return ret;
}

#define MAXPARSEBUFFER 2048

class MyTelnet : public Telnet
{
public:
  MyTelnet(const char *address,unsigned int port, bool isServer)
  {
    mParser.DefaultSymbols();
    mClient = 0;
    mInterface = 0;

    if(isServer)
    {
        mServer = new TelnetServer;
        mIsServer = mServer->Listen(port);
        if ( mIsServer )
        {
            mHaveConnection = true;
            mInterface = static_cast< TelnetInterface *>(mServer);
            mIsCreateSuccess = true;
        }
        else
        {
            delete mServer;
            mServer = 0;
            mIsCreateSuccess = false;
            return;
        }
    }
    else
    {
        mIsServer = false;
        mClient = new TelnetClient;
        mHaveConnection = mClient->Connect(address,port);
        if ( !mHaveConnection )
        {
            delete mClient;
            mClient = 0;
            mIsCreateSuccess = false;
        }
        else
        {
            mInterface = static_cast< TelnetInterface *>(mClient);
            mIsCreateSuccess = true;
        }
    }
  }

  virtual bool isServer(void) // returns true if we are a server or a client.  First one created on a machine is a server, additional copies are clients.
  {
      return mIsServer;
  }

  virtual bool haveConnection(void)
  {
      return mHaveConnection;
  }

  bool isCreateSuccess(void)
  {
      return mIsCreateSuccess;
  }

  virtual bool sendMessage(unsigned int client,const char *fmt,...)
  {
    bool ret = false;
    if ( mInterface )
    {
    	char wbuff[MAXPARSEBUFFER];
        wbuff[MAXPARSEBUFFER-1] = 0;
    	_vsnprintf(wbuff,MAXPARSEBUFFER-1, fmt, (char *)(&fmt+1));
        ret = mInterface->SendText(client,"%s",wbuff);
    }
    return ret;
  }

  virtual const char *  receiveMessage(unsigned int &client)
  {
      const char *ret = 0;
      client = 0;

      if ( mInterface )
      {
          ret = mInterface->GetLine(client);
      }

      return ret;
  }

  virtual const char ** getArgs(const char *input,int &argc) // parse string into a series of arguments.
  {
      strncpy(mParseBuffer,input,MAXPARSEBUFFER);
      mParseBuffer[MAXPARSEBUFFER-1] = 0;
      return mParser.GetArglist(mParseBuffer,argc);
  }

  virtual ~MyTelnet(void)
  {
      if(mIsServer)
      {
          delete mServer;
      }
      else
      {
          delete mClient;
      }
  }
private:
    bool          mIsServer;
    bool          mHaveConnection;
    TelnetInterface *mInterface;
    TelnetClient *mClient;
    TelnetServer *mServer;
    bool          mIsCreateSuccess;
    char          mParseBuffer[MAXPARSEBUFFER];
    InPlaceParser mParser;
};

Telnet* createTelnetClient(const char *address,unsigned int port)
{
    MyTelnet *m = new MyTelnet(address,port, false);
    if(m->isCreateSuccess())
    {
        return static_cast< Telnet *>(m);
    }
    else
    {
        delete m;
        return NULL;
    }
}

Telnet* createTelnetServer(const char *address,unsigned int port)
{
    MyTelnet *m = new MyTelnet(address,port, true);
    if(m->isCreateSuccess())
    {
        return static_cast< Telnet *>(m);
    }
    else
    {
        delete m;
        return NULL;
    }
}

void  releaseTelnet(Telnet *t)
{
    MyTelnet *m = static_cast< MyTelnet *>(t);
    delete m;
}


}; // end of namespace
