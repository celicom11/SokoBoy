#pragma once

class CReporter {
  wofstream m_File;
public:
//CTOR/DTOR
  CReporter();
  ~CReporter() {
    if (m_File.is_open())
      m_File.close();
  }
//METHODS
  bool SetFile(PCWSTR wszPath); //null for Console
  //priniting
  void PEol() { PC(L'\n'); }
  CReporter& PC(const char* szTxt);
  CReporter& PC(const wchar_t* wszTxt);
  CReporter& PC(char cTxt);
  CReporter& PC(wchar_t wcTxt);
  CReporter& PC(wstring wsTxt);
  CReporter& P(uint8_t nVal);
  CReporter& P(uint16_t nVal);
  CReporter& P(int32_t nVal);
  CReporter& P(uint32_t nVal, bool bHex = false);
  CReporter& P(int64_t nVal, bool bBits = false);
  CReporter& P(double dblVal);
  //Sokoban specific
  CReporter& PCell(char cCode, bool bUI); //bUI=true to show nice walls/boxes; otherwise: XSB format
  CReporter& P(Point pt);
  //TODO: add colors support for console?
};