#pragma once

int WinGetSystemErrorMessageEx(ULONG ErrorCode,PWSTR *ppMessage,ULONG dwLanguageId);
int WinGetErrorMessage(ULONG ErrorCode,PWSTR *ppMessage);
int WinGetSystemErrorMessage(ULONG ErrorCode,PWSTR *ppMessage);
void WinFreeErrorMessage(PWSTR pMessage);
VOID WINAPI _GetDateTimeStringEx2(ULONG64 DateTime,LPWSTR pszText,int cchTextMax,LPWSTR DateFormat,LPWSTR TimeFormat,BOOL bDisplayAsUTC,BOOL bMilliseconds);
