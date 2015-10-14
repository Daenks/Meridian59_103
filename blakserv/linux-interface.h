#ifndef _INTERFAC_LINUX_H
#define _INTERFAC_LINUX_H

#define FatalError(a) FatalErrorShow(__FILE__,__LINE__,a)

//extern HWND hwndMain;

void InitInterface(void);
void StoreInstanceData(HINSTANCE hInstance,int how_show);

int GetUsedSessions(void);

void StartupPrintf(const char *fmt,...);
void StartupComplete(void);

void InterfaceUpdate(void);
void InterfaceLogon(session_node *s);
void InterfaceLogoff(session_node *s);
void InterfaceUpdateSession(session_node *s);
void InterfaceUpdateChannel(void);
void InterfaceSignalConsole(void);
void InterfaceCheckPortal(void);

void InterfaceSendBufferList(buffer_node *blist);
void InterfaceSendBytes(char *buf,int len_buf);

void StartAsyncSocketAccept(SOCKET sock,int connection_type);
HANDLE StartAsyncNameLookup(char *peer_addr,char *buf);
void StartAsyncSession(session_node *s);
void StartAsyncPortalSocket(SOCKET sock);

void FatalErrorShow(const char *filename,int line,const char *str);

#endif

