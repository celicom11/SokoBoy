#include "StdAfx.h"
#include "CfgReader.h"

bool CCfgReader::Init(PCWSTR wszPath) {
  m_mapDict.clear();
	wifstream inFile;
	inFile.open(wszPath, wifstream::in);
	if (!inFile)
		return false;
	wstring wsLine,wsKey,wsVal;
	while (std::getline(inFile, wsLine)) {
		if (wsLine.front() == L';' || wsLine.find('=') == wstring::npos) //skip comments/garbage lines
			continue;
		uint32_t nEqPos = (uint32_t)wsLine.find(L'=');
		wsKey = wsLine.substr(0, nEqPos);
		while (iswspace(wsKey.front())) wsKey.erase(wsKey.begin());
		while (iswspace(wsKey.back())) wsKey.pop_back();
		wsVal = wsLine.substr(nEqPos + 1);
		nEqPos = (uint32_t)wsVal.find(L';');
		if (nEqPos != wstring::npos)
			wsVal = wsVal.substr(0, nEqPos);
		while (iswspace(wsVal.front())) wsVal.erase(wsVal.begin());
		while (iswspace(wsVal.back())) wsVal.pop_back();
		if (wsKey.empty() || wsVal.empty() || m_mapDict.find(wsKey) != m_mapDict.end()) {  //duplicates are not allowed
			assert(0);
			return false;
		}
		m_mapDict[wsKey] = wsVal;
	}
	return true;
}
//METHODS
bool CCfgReader::GetSVal(PCWSTR wszKey, IN OUT wstring& wsVal) {
	if(!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	wsVal = m_mapDict[wszKey];
	return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT int32_t& nVal) {
	if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	nVal = std::stoi(m_mapDict[wszKey]);
	return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT int16_t& nVal) {
	if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	nVal = (int16_t)std::stoi(m_mapDict[wszKey]); //todo: overflow check
	return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT uint32_t& nVal) {
	if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	nVal = std::stoul(m_mapDict[wszKey]);
	return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT uint16_t& nVal) {
	if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	nVal = (uint16_t)std::stoul(m_mapDict[wszKey]); //todo: overflow check
	return true;
}
bool CCfgReader::GetBVal(PCWSTR wszKey, IN OUT bool& bVal) {
	if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
		return false;
	wstring wsVal = m_mapDict[wszKey];
	bVal = wsVal.front()==L'1' || wsVal.front() == L'y'|| wsVal.front() == L'Y'; //todo: overflow check
	return true;
}
