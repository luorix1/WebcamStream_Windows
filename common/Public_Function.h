#pragma once
#ifndef PUBLIC_FUNCTION_H
#define PUBLIC_FUNCTION_H

#include <string>
#include <map>
#include <vector>
#include <stdio.h>
#include <io.h>
#include <atlstr.h>
#include "iconv.h"
using namespace std;

/************************************************************************
Function��    AnsiToUTF8
Description�� ��AnsiString��ʽ�ַ���ת��UTF8��ʽ	
Input��       strIn ��Ҫת�����ַ���     
Output��      strOut ת���󴫳��ַ���	     
Return��     
Others��           
************************************************************************/
string AnsiToUTF8( const string& strIn, string& strOut );

/************************************************************************
Function��    AnsiToUtf8
Description�� ��AnsiString��ʽ�ַ���ת��UTF8��ʽ 
Input��       strIn ��Ҫת�����ַ��� 
Output��      
Return��      ת�����UTF8��ʽ���ַ��� 
Others��           
************************************************************************/
string AnsiToUtf8( const string& strIn);
void AnsiToUTF8( const char* _pstrIn,CString &_cstrOut);
/************************************************************************
Function��    UTF8ToAnsi
Description�� ��UTF8��ʽ���ַ���ת��AnsiString��ʽ
Input��       strIn ��Ҫת���ַ��� 
Output��      
Return��      ת�����AnsiString��ʽ���ַ���
Others��           
************************************************************************/
string UTF8ToAnsi( const string& strIn );
void UTF8ToAnsi( const char* _pstrIn,CString &_strOut);

unsigned Ucs2BeToUcs2Le(unsigned short *ucs2bige, unsigned int size);

unsigned int Ucs2ToUtf8(unsigned short *ucs2, unsigned int ucs2_size,
	unsigned char *utf8, unsigned int utf8_size);
/************************************************************************
Function��    ConvertStringToURLCoding
Description�� ���ַ�ת��ΪURL��ʽ
Input��      
Output��      
Return��     
Others��           
************************************************************************/
BOOL ConvertStringToURLCoding(CString &strDest, const char* strUTF8, int iLength);

/************************************************************************
Function��    StdStrTrim
Description�� ȥ��std string �ַ������ҿո�
Input��       
Output��      
Return��     
Others��           
************************************************************************/
string StdStrTrim(const string _strSource);

/************************************************************************
Function��    IntToStr
Description�� ������ת��Ϊ�ַ��� 
Input��       _iValue����Ҫת��������
Output��      
Return��      ת������ַ���  
Others��           
************************************************************************/
string IntToStr( int _iValue );

/************************************************************************
Function��     GetExName
Description��  ���Ӧ�ó�������
Input��       
Output��      
Return��       Ӧ�ó�������  
Others��           
************************************************************************/
string GetExName();

/************************************************************************
Function��    ExtractFilePath
Description�� ��ȡȫ·��
Input��       
Output��      
Return��     
Others��           
************************************************************************/
string ExtractFilePath(string _strFullName);
/************************************************************************
Function��    GetText
Description�� ͨ����ԴID����ȡ
Input��      
Output��      
Return��      ������Դ����     
Others��           
************************************************************************/
CString GetTextEx(UINT _uiID);

/************************************************************************
Function��    GetText
Description�� ͨ��json�ļ���ȡ
Input��      
Output��      
Return��      ������Դ����     
Others��           
************************************************************************/
CString GetText(string _strID);

/************************************************************************
Function��    Execute
Description�� ˫��ʹ��windows�Դ��鿴����
Input��      _cstrFilePath �ļ�·��
Output��      
Return��      ������Դ����     
Others��           
************************************************************************/
HINSTANCE Execute(CString _cstrFilePath);

/************************************************************************
Function��    CreateMulityDir
Description�� �����ļ���
Input��       _strDir:�ļ�·��
Output��      
Return��           
Others��           
************************************************************************/
BOOL CreateMulityDir(string _strDir);

/************************************************************************
Function��    TheContainSubStr
Description�� �ж��Ƿ����ĳ���ַ�
Input��       _wstrSource��Ҫ�жϵ��ַ���
			  _iChnSin:�Ƿ��ַ�
			  _iSize:�Ƿ��ַ�����
Output��      
Return��      -1:�����Ƿ��ַ� ,���ࣺ���зǷ��ַ�
Others��      
************************************************************************/
int __stdcall ContainSubStr(const wstring& _wstrSource, const int* _iChnSin, const int& _iSize);

/************************************************************************
Function��    IsIllCharInStrForCutShort
Description�� �Ƿ�Ϊ�Ƿ��ַ��������棩
Input��       _strSource��Ҫ�жϵ��ַ���
Output��      
Return��      true:�Ƿ���false���Ϸ�
Others��      
************************************************************************/
bool IsIllCharInStrForCutShort(const std::string& _strSource);

/************************************************************************����
Function��    IsIllCharInStr
Description�� �Ƿ�Ϊ�Ƿ��ַ�  ��ȥ��\ �ո� �� ! ()�ַ�
Input��       _strSource��Ҫ�жϵ��ַ���
Output��      
Return��      true:�Ƿ���false���Ϸ�
Others��      
************************************************************************/
bool IsIllCharInStrForPath(const std::string& _strSource);

/************************************************************************
Function��    GetLocalFileSize
Description�� �õ��ļ���С
Input��       _strPath �ļ�·��
Output��      
Return��      �ļ���С 
Others��      
************************************************************************/
__int64 GetLocalFileSize(const std::string& _strPath);
/************************************************************************
Function��    NewGUID
Description�� ����һ��UUID
Input��       
Output��      
Return��      UUID
Others��      
************************************************************************/
string NewGUID();

/************************************************************************
Function��    SaveFile
Description�� �жϱ����ļ��Ƿ�ɹ�
Input��       _strSavePath �ļ�����·��
Output��      
Return��      �ɹ�;ʧ��
Others��      
************************************************************************/
BOOL SaveFile(const std::string _strSavePath, bool _blFullPath = false);

int covert(char *, char *, char *, size_t, char *, size_t);

wchar_t* string_to_wstring(char* c);

#endif;
