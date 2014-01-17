// 2002/07/05
// awzzz

// SFMMem.h: interface for the CSFMServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SFMSERVER_H__2D76A439_6388_4B07_AE7A_C82F458642ED__INCLUDED_)
#define AFX_SFMSERVER_H__2D76A439_6388_4B07_AE7A_C82F458642ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <Windows.h>
#define	DEFAULT_FILENAME	NULL
#define	DEFAULT_MAPNAME		L"_SFM_OBJ_"
#define	DEFAULT_MAPSIZE		(0xFFFF + 1)

// Shared FileMap Server
// ʹ��˵��
// 1.
// ����CSFMServer����
// CSFMServer(char *szFileName, char *szMapName, DWORD dwSize);
// Create(char *szFileName, char *szMapName, DWORD dwSize);
// ����1:NULL��ָ�����ļ�(��������򿪲���д/�鷳)
// ����2:Ҫ�����Ĺ����ڴ������
// ����3:Ҫ�����Ĺ����ڴ�����С
// 2.
// ����ʹ���ڴ�
// LPVOID GetBuffer()
// ���ع����ڴ��ַ
//
// �Զ�����
class CSFMServer  
{
public:
	CSFMServer();
	virtual ~CSFMServer();
	CSFMServer(const TCHAR *szFileName, const TCHAR *szMapName, DWORD dwSize);

protected:
	HANDLE	m_hFile;
	HANDLE	m_hFileMap;
	LPVOID	m_lpFileMapBuffer;

	TCHAR	*m_pFileName;
	TCHAR	*m_pMapName;
	DWORD	m_dwSize;

	int		m_iCreateFlag;

private:
	void _Init();
	void _Destory();

public:
	void Create(const TCHAR *szFileName, const TCHAR *szMapName, DWORD dwSize, DWORD dwSizeHigh=0);
	LPVOID GetBuffer();
	DWORD GetSize();
};

// Shared FileMap Client
// ʹ��˵��
// 1.
// ����CSFMClient����
// CSFMClient(DWORD dwAccess, char *szMapName);
// Open(DWORD dwAccess, char *szMapName);
// ����1:�����ڴ������ʷ�ʽ(FILE_MAP_READ|FILE_MAP_WRITE)
// ����2:�����ڴ������
// 2.
// ����ʹ���ڴ�
// LPVOID GetBuffer()
// ���ع����ڴ��ַ
//
// �Զ�����
class CSFMClient  
{
public:
	CSFMClient();
	virtual ~CSFMClient();
	CSFMClient(DWORD dwAccess, const TCHAR *szMapName);

protected:
	HANDLE	m_hFileMap;
	LPVOID	m_lpFileMapBuffer;

	TCHAR	*m_pMapName;

	int		m_iOpenFlag;

private:
	void _Init();
	void _Destory();

public:
	void Open(DWORD dwAccess, const TCHAR *szMapName);
	LPVOID GetBuffer();
	DWORD GetSize();
};

#endif // !defined(AFX_SFMSERVER_H__2D76A439_6388_4B07_AE7A_C82F458642ED__INCLUDED_)
