#include "StdAfx.h"
#include "Sokoban.h"
#include "StageQueue.h"

namespace {
	wstring _PathNoExt(const wstring& wsPPath) {
		size_t nDot = wsPPath.find_last_of(L'.');
		if (nDot == wstring::npos)
			return wsPPath;//snbh!
		return wsPPath.substr(0, nDot);
	}
	wstring _PathFileName(const wstring& wsPPath) {
		size_t nDir = wsPPath.find_last_of(L"\\/");
		if (nDir == wstring::npos)
			return wsPPath;
		return wsPPath.substr(nDir+1);

	}
}
Sokoban::Sokoban() : m_DLM(*this), m_RSM(*this) {
	m_pClosedStgs = new SQVec;
}
Sokoban::~Sokoban() {
	delete m_pClosedStgs;
}
//XSB/Soko format
char Sokoban::GetCode(const Stage& stage, int nRow, int nCol) const {
	if (stage.ptR.nRow == nRow && stage.ptR.nCol == nCol)
		return IsStorage(nRow, nCol)?'+': '@';
	if (HasBox(stage, nRow, nCol))
		return (IsStorage(nRow, nCol)?'*':'$');
	if (IsStorage(nRow, nCol))
		return '.';
	if (IsSpace(nRow, nCol))
		return ' ';
	return '#'; //wall
}
bool Sokoban::ReportSolution(const Stage& last, PCWSTR wszPath, double dblTime) {
	CReporter rptF;
	ofstream outFile;
	outFile.open(wszPath, ofstream::out);

	if (!outFile) {
		m_Reporter.PC("Could not create/open ").PC(wszPath).PC("!!\n");
		return false;
	}
	m_Reporter.PC("Saving solution to ").PC(wszPath).PEol();

	const Stage* pStage = &last;
	while(pStage)  {
		for (int8_t i = 0; i < m_dim.nRows; ++i)
		{
			for (int8_t j = 0; j < m_dim.nCols; ++j)
			{
				outFile << GetCode(*pStage, i,j);
			}
			outFile << endl;
		}
		outFile << endl;

		//get parent stage
		pStage = m_pClosedStgs->Parent(*pStage);
	}

	outFile << "This search took: " << dblTime << " seconds" << endl;

	return true;
}

void Sokoban::Display(const Stage& stage) const {
	for (int8_t i = 0; i < m_dim.nRows; ++i) {
		for (int8_t j = 0; j < m_dim.nCols; ++j) {
			m_Reporter.PCell(GetCode(stage,i,j), m_Cfg.bRpt_UIMode);
		}
		m_Reporter.PEol();
	}
}

void Sokoban::DisplayCorral(Point ptR, const Corral& crl) {
	Stage stage; stage.ptR = ptR; stage.llBoxPos = crl.llBoxes;
	for (uint8_t i = 0; i < m_dim.nRows; ++i) {
		for (uint8_t j = 0; j < m_dim.nCols; ++j) {
			char cCode = GetCode(stage, i, j);
			if ((cCode == L' ' || cCode == L'.') && (1ll << CellPos({ i,j })) & crl.llCells) //replace space with space
				m_Reporter.PC(L'◊');
			else
				m_Reporter.PCell(cCode, true);
			
		}
		m_Reporter.PEol();
	}
}

bool Sokoban::Run() {
	if (!InitCfg_())
		return false;
	for (const wstring& wsPPath : m_Cfg.vPuzzles) {
		time_t tBegin{ 0 }, tEnd{ 0 };
		m_Reporter.PC("\nSolving ").PC(_PathFileName(wsPPath)).PC(" with ").PC(m_Cfg.wsSearch).PEol();
		time(&tBegin);
		m_Reporter.PC("Initializing ... \n");
		if (!Initialize_(wsPPath.c_str())) {
			continue; //skip bad entry
		}
		Display(m_stInit);
		if(m_stInit.nWeight!=0xFFFF)
			m_Reporter.PC("Init Stage Distance: ").P(m_stInit.nWeight).PEol();
		bool bRet = false;
		Stage current = m_stInit;
		IStageQueue* pSQ = nullptr;
		if (m_Cfg.wsSearch == L"BFS")
			pSQ = new SQDeque;
		else if (m_Cfg.wsSearch == L"DFS")
			pSQ = new SQStackW(*this);
		else if (m_Cfg.wsSearch == L"AStar")
			pSQ = new SQPQueue(*this);
		else {
			m_Reporter.PC("ERROR: Unexpected Search ").PC(m_Cfg.wsSearch);
			abort();
		}
		bRet = Search(pSQ, current);
		time(&tEnd);
		delete pSQ;
		if (!bRet)
			m_Reporter.PC(m_Cfg.wsSearch).PC(" could not find a solution!!!").PEol();
		else {
			double dblTime = difftime(tEnd, tBegin);
			m_Reporter.PC(m_Cfg.wsSearch).PC(" found solution in ").P(dblTime).PC("  seconds").PEol();
			if (m_Cfg.bRpt_Sol) {
				wstring wsSolPath = _PathNoExt(wsPPath) + L"_" + m_Cfg.wsSearch + L".txt";
				ReportSolution(current, wsSolPath.c_str(), dblTime);
			}
		}
	}//for
	return true;
}

//OOOOOOOOOOOOOO
//O  B   S  B  O
bool Sokoban::IsDeadHWall(const Stage& temp, int nRow, int nCol) const {
	uint16_t nLR = m_aDeadHWalls[nRow][nCol];
	if (!nLR)
		return false;//nothing to check
	int nBoxes = 0, nStg = 0;
	const int nL = nLR >> 8, nR = nLR & 0xFF;
	for (int nX = nL; nX < nR; ++nX) {
		if (IsStorage(nRow, nX))
			++nStg;
		if (HasBox(temp, nRow, nX))
			++nBoxes;
	}
	return nStg < nBoxes;
}
//Vert wall check
bool Sokoban::IsDeadVWall(const Stage& temp, int nRow, int nCol) const {
	uint16_t nUD = m_aDeadVWalls[nRow][nCol];
	if (!nUD)
		return false;//nothing to check
	int nBoxes = 0, nStg = 0;
	const int nU = nUD >> 8, nD = nUD & 0xFF;
	for (int nY = nU; nY < nD; ++nY) {
		if (IsStorage(nY, nCol))
			++nStg;
		if (HasBox(temp, nY, nCol))
			++nBoxes;
	}
	return nStg < nBoxes;
}
void Sokoban::UpdateStageWeight(IN OUT Stage& stage) const {
	stage.nWeight = m_RSM.GetMinDist(stage);
	if (stage.nWeight >= 0xFFFF - m_nBoxes)
		stage.nWeight = 0xFFFF - m_DLM.GetFGLBits(stage);
}
uint32_t Sokoban::Depth(const Stage& stage) const{
	uint32_t nRet = 0;
	const Stage* pStage = &stage;
	while (pStage && pStage->nPIdx) {
		pStage = m_pClosedStgs->Parent(*pStage);
		++nRet;
	}
	return nRet;
}
uint16_t Sokoban::ParentWeight(const Stage& stage) const {
	const Stage* pParent = m_pClosedStgs->Parent(stage);
	return pParent ? pParent->nWeight : 0xFFFF;
}
bool Sokoban::AreFixedGoals(const vector<int>& vStgIdx, OUT vector<Point>& vStgPts) const {
	vStgPts.clear();
	Stage stage;
	for (int nIdx : vStgIdx) {
		vStgPts.push_back(m_vStg[nIdx].pt);
		stage.llBoxPos |= 1ll << CellPos(vStgPts.back());
	}
	for (Point ptG : vStgPts) {
		if (
			(NotSpace(stage, ptG.nRow - 1, ptG.nCol) || NotSpace(stage, ptG.nRow + 1, ptG.nCol) || 
				(IsDeadPos(ptG.nRow - 1, ptG.nCol) && IsDeadPos(ptG.nRow + 1, ptG.nCol)) ) &&
			(NotSpace(stage, ptG.nRow, ptG.nCol - 1) || NotSpace(stage, ptG.nRow, ptG.nCol + 1) || 
				(IsDeadPos(ptG.nRow, ptG.nCol- 1 ) && IsDeadPos(ptG.nRow, ptG.nCol + 1)) )
			)
			continue;//~static DL
		//else
		if (!NotSpace(stage, ptG.nRow - 1, ptG.nCol) && !NotSpace(stage, ptG.nRow - 2, ptG.nCol))
				return false;
		if (!NotSpace(stage, ptG.nRow + 1, ptG.nCol) && !NotSpace(stage, ptG.nRow + 2, ptG.nCol))
			return false;
		if (!NotSpace(stage, ptG.nRow, ptG.nCol - 1) && !NotSpace(stage, ptG.nRow, ptG.nCol - 2))
			return false;
		if (!NotSpace(stage, ptG.nRow, ptG.nCol + 1) && !NotSpace(stage, ptG.nRow, ptG.nCol + 2))
			return false;
	}
	return true;
}



