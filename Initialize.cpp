#include "StdAfx.h"
#include "Sokoban.h"
#include "CfgReader.h"

#include <Windows.h>
//static helpers
namespace {
	void _ReadAllFiles(PCWSTR wszPath, OUT vector<wstring>& vPaths) {
		vPaths.clear();
		if (!wszPath) {
			assert(0);
			return;
		}
		wstring wsPath(wszPath);
		if (wsPath.size() < 3 || wsPath.size() > _MAX_PATH ) {
			assert(0);
			return;
		}
		uint32_t nStarPos = (uint32_t)wsPath.find(L'*');
		if (nStarPos == wstring::npos)
			vPaths.push_back(wsPath);
		else { //find all files in the dir
			//sorry: Windows impl only - feel free to use std::filesystem OR port it to *nix directly!
			WIN32_FIND_DATA ffd;
			HANDLE hFind = ::FindFirstFileW(wszPath, &ffd);
			if(!hFind || hFind == INVALID_HANDLE_VALUE)
				return;//error or no files
			//else
			wstring wsCurDir,wsSrchDir;
			uint32_t nEODirPos = (uint32_t)wsPath.find_first_of(L"\\/");
			if (nEODirPos != wstring::npos)
				wsSrchDir = wsPath.substr(0, nEODirPos+1);
			if (wszPath[1] != L':') {//relative path?
				wchar_t wszCurDir[MAX_PATH]{ 0 };
				::GetCurrentDirectoryW(_countof(wszCurDir), wszCurDir);
				wsCurDir = wszCurDir;
			}
			do {
				if (0 == (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					wstring wsPuzzlePath = (!wsCurDir.empty() ? (wsCurDir + L'\\') : wstring()) + wsSrchDir + ffd.cFileName;
					vPaths.push_back(wsPuzzlePath);
				}
			} while (::FindNextFileW(hFind, &ffd) != 0);
			::FindClose(hFind);
		}
	}
	bool _ReadSokoRows(PCWSTR wszPuzzlePath, OUT vector<string>& vRows) {
		ifstream inFile;
		inFile.open(wszPuzzlePath, ifstream::in);
		if (!inFile) {
			//m_Reporter.PC("Could not open").PC(wszPuzzlePath).PC("file!").PEol();
			return false;
		}

		vRows.clear();
		string temp;
		//prolog: skip trailing empty and/or comment lines
		while (std::getline(inFile, temp)) {
			if (temp.empty() || temp.front() == ';' || temp.find('#') == string::npos)
				continue;
			break;
		}
		do {
			if (temp.front() == ';' || temp.find('#') == string::npos)
				break;
			vRows.push_back(temp);
		} while (std::getline(inFile, temp));

		return true;
	}
	bool _ValidXSB(char cCode) {
		return cCode == ' ' || cCode == '_' || cCode == '.' || cCode == '$' || cCode == 'B' || cCode == '*' || cCode == 'R' || cCode == '+' || 
			cCode == '@' || cCode == '#';
	}
}
void Sokoban::AddDeadCornerPos_() {
	for (uint8_t i = 1; i < m_dim.nRows; i++) {
		for (uint8_t j = 1; j < m_dim.nCols; j++) {
			if (IsSpace(i, j) && !IsStorage(i, j) && 
				(IsWall(i, j - 1) || IsWall(i, j+1)) &&
				(IsWall(i - 1, j) || IsWall(i+1, j))
				) {
				m_btsDeadPos.set(m_aBitPos[i][j]-1);
			}
		}
	}
}
void Sokoban::InitTunnelPos_() {
	m_btsTnlPosUP.reset(); m_btsTnlPosDN.reset();
	m_btsTnlPosLT.reset(); m_btsTnlPosRT.reset();
	for (uint8_t i = 1; i < m_dim.nRows; i++) {
		for (uint8_t j = 1; j < m_dim.nCols; j++) {
			if (!IsSpace(i, j))
				continue;
			//UP
			if (IsWall(i, j - 1) && IsWall(i, j + 1) && IsSpace(i - 1, j) && !IsStorage(i-1, j)) {
				if(IsWall(i-1, j - 1) || IsWall(i-1, j + 1) || 
					(IsWall(i-1, j - 2) && IsWall(i-1, j + 2))
					)
					m_btsTnlPosUP.set(m_aBitPos[i][j] - 1);
			}
			//DN
			if (IsWall(i, j - 1) && IsWall(i, j + 1) && IsSpace(i + 1, j) && !IsStorage(i+1, j)) {
				if (IsWall(i + 1, j - 1) || IsWall(i + 1, j + 1) ||
					(IsWall(i + 1, j - 2) && IsWall(i + 1, j + 2))
					)
					m_btsTnlPosDN.set(m_aBitPos[i][j] - 1);
			}
			//LT
			if (IsWall(i-1, j) && IsWall(i+1, j) && IsSpace(i, j-1) && !IsStorage(i, j-1)) {
				if (IsWall(i - 1, j - 1) || IsWall(i + 1, j - 1) ||
					(IsWall(i - 2, j - 1) && IsWall(i + 2, j -1 ))
					)
					m_btsTnlPosLT.set(m_aBitPos[i][j] - 1);
			}
			//RT
			if (IsWall(i - 1, j) && IsWall(i + 1, j) && IsSpace(i, j + 1) && !IsStorage(i, j+1)) {
				if (IsWall(i - 1, j + 1) || IsWall(i + 1, j + 1) ||
					(IsWall(i - 2, j + 1) && IsWall(i + 2, j + 1))
					)
					m_btsTnlPosRT.set(m_aBitPos[i][j] - 1);
			}
		}
	}
}
void Sokoban::InitDeadHWalls_() {
	memset(m_aDeadHWalls, 0, sizeof(m_aDeadHWalls));
	for (uint8_t i = 0; i < m_dim.nRows; i++) {
		for (uint8_t j = 0; j < m_dim.nCols; j++) {
			if (IsSpace(i, j) && IsWall(i, j - 1)) {
				//check a "dead" wall candidate
				for (uint8_t nCol = j; nCol < m_dim.nCols; ++nCol) {
					if (IsSpace(i-1, nCol) && IsSpace(i+1, nCol))
						break;//not a dw
					if (IsWall(i, nCol + 1)) {//it is!
						uint16_t nLR = (j << 8) | (nCol + 1);
						bool bHasG = false;
						for (uint8_t nCol1 = j; nCol1 < nCol + 1; ++nCol1) {
							if (IsStorage(i, nCol1))
								bHasG = true;
							m_aDeadHWalls[i][nCol1] = nLR;
						}
						if (!bHasG) {//add to dead bits
							for (uint8_t nCol1 = j; nCol1 < nCol + 1; ++nCol1) {
								m_btsDeadPos.set(m_aBitPos[i][nCol1]-1);
							}
						}
						break;
					}
				}
			}//if dw candidate
		}
	}
}
void Sokoban::InitDeadVWalls_() {
	memset(m_aDeadVWalls, 0, sizeof(m_aDeadVWalls));
	for (uint8_t i = 0; i < m_dim.nRows; i++) {
		for (uint8_t j = 0; j < m_dim.nCols; j++) {
			if (IsSpace(i, j) && IsWall(i-1, j)) {
				//check a "dead" wall candidate
				for (uint8_t nRow = i; nRow < m_dim.nRows; ++nRow) {
					if (IsSpace(nRow, j-1) && IsSpace(nRow, j+1))
						break;//not a dw
					if (IsWall(nRow+1, j)) {//it is!
						uint16_t nUD = (i << 8) | (nRow + 1);
						bool bHasG = false;
						for (uint8_t nRow1 = i; nRow1 < nRow + 1; ++nRow1) {
							if (IsStorage(nRow1, j))
								bHasG = true;
							m_aDeadVWalls[nRow1][j] = nUD;
						}
						if (!bHasG) {//add to dead bits
							for (uint8_t nRow1 = i; nRow1 < nRow + 1; ++nRow1) {
								m_btsDeadPos.set(m_aBitPos[nRow1][j] - 1);
							}
						}
						break;
					}
				}
			}//if dw candidate
		}
	}
}
//void Sokoban::InitStgDist_() {
//	for (Storage& stg : m_vStg) {
//		memset(stg.aMDist, 0xFFFF, sizeof(stg.aMDist));
//		for (uint8_t i = 0; i < m_dim.nRows; i++) {
//			for (uint8_t j = 0; j < m_dim.nCols; j++) {
//				if (IsSpace(i, j))
//					stg.aMDist[m_aBitPos[i][j]-1] = MinWalk_(stg.pt, { i,j });
//			}
//		}
//	}
//	//TODO: SGoal struct with calculated wwight 
//	//corners are high priority,etc.
//	//next - check if taking some goals first, blocks some boxes from being delivered to any other storages.
//}
//InitStgDist_ helper - walk through NON-dead pos!
uint16_t Sokoban::MinWalk_(Point pt1, Point pt2) const {
	if (IsDeadPos(pt1.nRow, pt1.nCol) || IsDeadPos(pt2.nRow, pt2.nCol))
		return 0xFFFF;
	Stage temp, current = m_stInit; current.ptR = pt1;
	bitset<256> bst;//16x16 bit matrix
	queue<Stage> queueStages;//BFS

	//prolog
	bst.set(current.ptR.nRow * m_dim.nCols + current.ptR.nCol);//current robot pos
	current.nWeight = 0;//reuse it for steps count
	while (!(current.ptR == pt2)) {
		//UP
		if (IsSpace(current.ptR.nRow - 1, current.ptR.nCol) && !IsDeadPos(current.ptR.nRow - 1, current.ptR.nCol)) {
			temp = WalkUp(current);
			if (!bst.test(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol)) {
				bst.set(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol);//current robot pos
				++temp.nWeight;
				queueStages.push(temp);
			}
		}
		//LEFT
		if (IsSpace(current.ptR.nRow, current.ptR.nCol - 1) && !IsDeadPos(current.ptR.nRow, current.ptR.nCol - 1)) {
			temp = WalkLeft(current);
			if (!bst.test(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol)) {
				bst.set(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol);//current robot pos
				++temp.nWeight;
				queueStages.push(temp);
			}
		}
		//DOWN
		if (IsSpace(current.ptR.nRow + 1, current.ptR.nCol) && !IsDeadPos(current.ptR.nRow + 1, current.ptR.nCol)) {
			temp = WalkDown(current);
			if (!bst.test(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol)) {
				bst.set(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol);//current robot pos
				++temp.nWeight;
				queueStages.push(temp);
			}
		}
		//RIGHT
		if (IsSpace(current.ptR.nRow, current.ptR.nCol + 1) && !IsDeadPos(current.ptR.nRow, current.ptR.nCol + 1)) {
			temp = WalkRight(current);
			if (!bst.test(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol)) {
				bst.set(temp.ptR.nRow * m_dim.nCols + temp.ptR.nCol);//current robot pos
				++temp.nWeight;
				queueStages.push(temp);
			}
		}
		if (queueStages.empty())
			return 0xFFFF;//cannot push anything from pt1 to pt2
		current = queueStages.front();
		queueStages.pop();
	}//while
	return current.nWeight;
}

bool Sokoban::InitCfg_() {
	CCfgReader crdr;
	if (!crdr.Init(L"Sokoboy.cfg")) {
		m_Reporter.PC("Failed to read Sokoboy.cfg file!").PEol();
		assert(0);
		return false;
	}
	crdr.GetBVal(L"Rpt_PIC_Merge", m_Cfg.bRpt_PIC_Merge);
	crdr.GetBVal(L"Rpt_Sol", m_Cfg.bRpt_Sol);
	crdr.GetBVal(L"Rpt_UIMode", m_Cfg.bRpt_UIMode);
	crdr.GetNVal(L"RSM_Depth", m_Cfg.nRSM_Depth);
	crdr.GetNVal(L"RSM_GBRelax", m_Cfg.nRSM_GBRelax);
	crdr.GetNVal(L"DFS_MaxDepth", m_Cfg.nDFS_MaxDepth);
	crdr.GetNVal(L"Rpt_SQInc", m_Cfg.nRpt_SQInc);
	crdr.GetSVal(L"Search", m_Cfg.wsSearch);
	crdr.GetSVal(L"Rpt_Path", m_Cfg.wsRpt_Path);
	//list of Soko files
	wstring wsFiles;
	crdr.GetSVal(L"PuzzlePath", wsFiles);
	if (wsFiles.empty()) {
		m_Reporter.PC("PuzzlePath is missed in the config!").PEol();
		assert(0);
		return false;
	}
	_ReadAllFiles(wsFiles.c_str(), m_Cfg.vPuzzles);
	if (m_Cfg.vPuzzles.empty()) {
		m_Reporter.PC("No files found in ").PC(wsFiles).PC(" !").PEol();
		assert(0);
		return false;
	}

	return true;
}
bool Sokoban::Initialize_(PCWSTR wszPuzzlePath) {
	//CLEANUP
	m_nBoxes = m_nSpaces = 0;
	m_dim.nCols = m_dim.nRows = 0;
	m_llStgPos = 0;
	m_vStg.clear();
	m_stInit.nWeight = 0xFFFF; m_stInit.nPIdx = 0; m_stInit.llBoxPos = 0; m_stInit.ptR = { 0,0 };
	memset(m_aField, 0, sizeof(m_aField));
	memset(m_aBitPos, 0, sizeof(m_aBitPos));
	m_btsDeadPos.reset();

	//Read Puzzle file
	vector<string> vRows;
	if (!_ReadSokoRows(wszPuzzlePath, vRows) || vRows.size() < 4) {
		m_Reporter.PC("Failed to read puzzle from ").PC(wszPuzzlePath).PC(" file!").PEol();
		return false;
	}
	//calc columns
	m_dim.nCols = 0;
	uint32_t nCol0 = 0xFFFF; //starting pos
	uint32_t nCol1 = 0; //last pos
	for (const string& sRow : vRows) {
		uint32_t nPos0 = (uint32_t)sRow.find(L'#');
		if (nPos0 < nCol0)
			nCol0 = nPos0;
		uint32_t nPos1 = (uint32_t)sRow.rfind(L'#');
		if (nPos1 > nCol1)
			nCol1 = nPos1;
	}
	if (nCol0 || nCol1 < 2) {
		m_Reporter.PC("Puzzle must have row starting/ending with '#'! Fix ").PC(wszPuzzlePath).PC(" file!").PEol();
		return false;
	}
	m_dim.nCols = nCol1+1;
	m_dim.nRows = (uint16_t)vRows.size();
	if (m_dim.nRows > MAX_DIM || m_dim.nCols > MAX_DIM) {
		m_Reporter.PC("Could get puzzle dimensions or number of rows/columns is too big").PEol();
		return false;
	}
	//fill trailing gaps
	for (string& sRow : vRows) {
		if (sRow.size() < m_dim.nCols)
			sRow.resize(m_dim.nCols, ' ');
		uint32_t nPos1 = (uint32_t)sRow.rfind('#');
		while (++nPos1 < m_dim.nCols) {
			sRow[nPos1] = '#';
		}
		for (char& cCode : sRow) {
			if (cCode == '#')
				break;
			if (cCode == ' ' || cCode == '_')
				cCode = '#'; //replace trailing space
			else {
				m_Reporter.PC("Puzzle ").PC(wszPuzzlePath).PC(" has row that starts with '").PC(cCode).PC("' !").PEol();
				return false;
			}
		}
	}
	Point ptCell{ 0,0 };//current cell, 0 based
	for (string& sRow : vRows) {
		if (sRow.size() != m_dim.nCols) {
			m_Reporter.PC("Puzzle ").PC(wszPuzzlePath).PC(" has row with ").P((uint32_t)sRow.size()).PC(" chars instead of ").P(m_dim.nCols).PC(" !").PEol();
			return false;
		}
		bool bInside = false;
		ptCell.nCol = 0;
		for (char& cCode : sRow) {
			//fill gaps/sanity check
			if (!_ValidXSB(cCode)) {
				m_Reporter.PC("Row ").P(ptCell.nRow).PC(" has invalid code '").PC(cCode).PC("' !").PEol();
				return false;
			}
			//parse
			if (cCode == '.' || cCode == '*' || cCode == '+') {//storage(goal) OR //Box in goal/Man on goal
				m_aField[ptCell.nRow][ptCell.nCol] = 2;
				m_vStg.emplace_back();
				Storage& stg = m_vStg.back();
				stg.pt = ptCell;
				m_llStgPos |= 1ll << m_nSpaces;
				if (cCode == '*') {
					m_stInit.llBoxPos |= 1ll << m_nSpaces;
					++m_nBoxes;
				}
				else if (cCode == '+') 
					m_stInit.ptR = ptCell;
				m_aSpaces[m_nSpaces] = ptCell;
				++m_nSpaces;
				m_aBitPos[ptCell.nRow][ptCell.nCol] = (int8_t)m_nSpaces;
			}
			else if (cCode == ' ' || cCode == '_') {//space
				m_aField[ptCell.nRow][ptCell.nCol] = 1;
				m_aSpaces[m_nSpaces] = ptCell;
				++m_nSpaces;
				m_aBitPos[ptCell.nRow][ptCell.nCol] = (int8_t)m_nSpaces;
			}
			else if (cCode == 'B' || cCode == '$') { //Box in space
				m_aField[ptCell.nRow][ptCell.nCol] = 1;
				m_stInit.llBoxPos |= 1ll << m_nSpaces;
				m_aSpaces[m_nSpaces] = ptCell;
				++m_nSpaces;
				m_aBitPos[ptCell.nRow][ptCell.nCol] = (int8_t)m_nSpaces;
				++m_nBoxes;
			}
			else if (cCode == 'R' || cCode == '@') { //Man in space
				m_aField[ptCell.nRow][ptCell.nCol] = 1;
				m_aSpaces[m_nSpaces] = ptCell;
				++m_nSpaces;
				m_aBitPos[ptCell.nRow][ptCell.nCol] = (int8_t)m_nSpaces;

				m_stInit.ptR = ptCell;
			}
			else
				assert(cCode == '#');//default , ntd
			if (m_nSpaces > MAX_SPACES) {
				m_Reporter.PC("Number of empty cells in puzzle greater than ").P(MAX_SPACES).PEol();
				return false;
			}
			++ptCell.nCol;
		}//for columns
		++ptCell.nRow;
	}//for rows
	if (m_nBoxes == 0 || m_nBoxes > MAX_BOXES) {
		m_Reporter.PC("Number of boxes is 0 or greater than ").P(MAX_BOXES).PEol();
		return false;
	}
	if (m_nBoxes != m_vStg.size()) {
		m_Reporter.PC("Number of boxes ").P(m_nBoxes).PC(" is not equal to number of storages ").P((uint32_t)m_vStg.size()).PC(" !\n");
		return false;
	}
	if(!m_DLM.Init()) {
		m_Reporter.PC("Failed to init DeadLock manager!").PEol();
		return false;
	}
	//Precalculations/analysis
	//InitDeadBoxBits_();
	InitDeadHWalls_();
	InitDeadVWalls_();
	AddDeadCornerPos_();
	InitTunnelPos_();
	//InitStgDist_();
	if (m_Cfg.wsSearch != L"BFS") {
		uint16_t nRSMDepth = m_Cfg.nRSM_Depth ? m_Cfg.nRSM_Depth : (uint16_t)log2(m_nSpaces) + m_nBoxes*2/3;// *5 / 4;
		m_RSM.Init(nRSMDepth);
		m_Reporter.PC("RSphere of radius ").P(nRSMDepth).PC(" has ").P(m_RSM.size()).PC(" nodes.").PEol();
		UpdateStageWeight(m_stInit);
	}
	return true;
}
