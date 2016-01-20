#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/epoll.h>

#define MAX_PATH PATH_MAX
#define O_BINARY 0
#define O_TEXT 0
#define stricmp strcasecmp
#define strnicmp strncasecmp

#define WM_APP      0x8000
#define WM_QUIT     0x0012
#define WAIT_OBJECT_0   0
#define QS_ALLINPUT 0x04FF
#define PM_REMOVE   0x0001

#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT 2
#define PROCESSOR_INTEL_386 386
#define PROCESSOR_INTEL_486 486
#define PROCESSOR_INTEL_PENTIUM 586

// common windows data types
typedef unsigned char BYTE;
typedef unsigned short WORD;
#define MAKEWORD(low, high) ((WORD)((((WORD)(high)) << 8) | ((BYTE)(low))))
typedef unsigned long DWORD;
typedef long long INT64;
typedef void* PVOID;
typedef void* LPVOID;
typedef PVOID HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HWND;
typedef unsigned long long UINT64;
typedef unsigned int UINT;
typedef bool BOOL;
#define TRUE 1
#define FALSE 0
typedef unsigned long LPARAM;
typedef unsigned int WPARAM;
typedef const char* LPCSTR;
typedef LPCSTR LPCTSTR;

// inet
#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

#define closesocket(s) close(s)
#define GetLastError() errno
#define WSAEWOULDBLOCK EWOULDBLOCK
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKADDR_IN6 sockaddr_in6
#define SOCKADDR sockaddr
#define IN6_ADDR in6_addr
#define FD_READ     1
#define FD_WRITE    2
#define FD_OOB      4
#define FD_ACCEPT   8
#define FD_CONNECT  16
#define FD_CLOSE    32


typedef int SOCKET;

typedef struct WSAData {
  WORD           wVersion;
  WORD           wHighVersion;
  char           szDescription[WSADESCRIPTION_LEN+1];
  char           szSystemStatus[WSASYS_STATUS_LEN+1];
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char           *lpVendorInfo;
} WSADATA, *LPWSADATA;

#define MAXGETHOSTSTRUCT 64

#include "linux-critical_section.h"
#include "mutex_linux.h"
#include "thdmsgqueue_linux.h"
#include "main_linux.h"
