#pragma once

class CCfgReader {
//DATA
  map<wstring, wstring>  m_mapDict;
public:
//CTOR/DTOR/Init
  CCfgReader() = default;
  ~CCfgReader() = default;
  bool Init(PCWSTR wszPath);
//METHODS
  bool GetSVal(PCWSTR wszKey, IN OUT wstring& wsVal);
  bool GetNVal(PCWSTR wszKey, IN OUT int32_t& nVal);
  bool GetNVal(PCWSTR wszKey, IN OUT int16_t& nVal);
  bool GetNVal(PCWSTR wszKey, IN OUT uint32_t& nVal);
  bool GetNVal(PCWSTR wszKey, IN OUT uint16_t& nVal);
  bool GetBVal(PCWSTR wszKey, IN OUT bool& bVal);
};