#include "StdAfx.h"
#include "Reporter.h"

//default output to console
CReporter::CReporter() {
//Unicode stdout
	setlocale(LC_ALL, "");
	_setmode(_fileno(stdout), _O_U16TEXT);
}
bool CReporter::SetFile(PCWSTR wszPath) {
	if(m_File.is_open())
		m_File.close();
	m_File.open(wszPath, ofstream::out | ofstream::trunc);//rewrite
	return m_File.is_open();
}
//priniting
CReporter& CReporter::PC(const char* szTxt) {
	wcout << szTxt; return *this;
}
CReporter& CReporter::PC(const wchar_t* wszTxt) {
	wcout << wszTxt; return *this;
}
CReporter& CReporter::PC(char cTxt) {
	wcout << cTxt; return *this;
}
CReporter& CReporter::PC(wchar_t wcTxt) {
	wcout << wcTxt; return *this;
}
CReporter& CReporter::P(uint8_t nVal) {
	wcout << (unsigned int)nVal; return *this;
}
CReporter& CReporter::P(uint16_t nVal) {
	wcout << (unsigned int)nVal; return *this;
}
CReporter& CReporter::P(int32_t nVal) {
	wcout << nVal; return *this;
}
CReporter& CReporter::P(uint32_t nVal, bool bHex) {
	if (bHex)
		wcout << std::hex;
	wcout << nVal; return *this;
	if (bHex)
		wcout << std::dec;//restore!
}
CReporter& CReporter::P(int64_t nVal, bool bBits) {
	if (bBits) {
		std::bitset<64> btsVal(nVal);
		wcout << btsVal;
	} else
		wcout << nVal; 
	return *this;
}
CReporter& CReporter::P(double dblVal) {
	wcout << dblVal;
	return *this;
}
CReporter& CReporter::PC(wstring wsTxt) {
	wcout << wsTxt;
	return *this;
}

//Sokoban specific
//If bUI, print nice walls/boxes; otherwise: XSB format
CReporter& CReporter::PCell(char cCode, bool bUI) {
	wchar_t wc = cCode;
	if (bUI) {//convert to ○☺□☻■▓
		switch (cCode) {
			case '+': wc = L'☺'; break;
			case '@': wc = L'☻'; break;
			case '*': wc = L'□'; break;
			case '$': wc = L'■'; break;
			case '.': wc = L'◦'; break;
			case '#': wc = L'▓'; break;
		}
	};
	wcout << wc;
	return *this;
}
CReporter& CReporter::P(Point pt) {
	wcout << L"{" << (uint32_t)pt.nRow << L"," << (uint32_t)pt.nCol << L"}";
	return *this;
}
