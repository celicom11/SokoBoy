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
	m_pClosedStgs = new SQVecEx(*this);//test new container
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
	if (m_aField[nRow][nCol] != 0) //could be 4-"imposed wall"
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
	//outFile << m_TPC.ToString() << endl;
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
		m_Reporter.PC("Initializing ...").PEol();
		if (!Initialize_(wsPPath.c_str())) {
			continue; //skip bad entry
		}
		if(m_stInit.nWeight!= SOKOINF)
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

void Sokoban::UpdateStageWeight(IN OUT Stage& stage) const {
	stage.nWeight = m_RSM.GetMinDist(stage);
	if (stage.nWeight >= SOKOINF - m_nBoxes)
		stage.nWeight = SOKOINF - m_DLM.NearestFGLSize(stage);
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
	return pParent ? pParent->nWeight : SOKOINF;
}
bool Sokoban::AreFixedGoals(const vector<Point>& vStgPts) const {
	Stage stage,temp; stage.llBoxPos = CellsPos(vStgPts);
	vector<Point> vRCells;
	for (Point ptG : vStgPts) {
		//check if G push is valid/not a DL
		bool bCanPush = false;
		//UP/DN
		if (!NotSpace(stage, ptG.nRow - 1, ptG.nCol) && !NotSpace(stage, ptG.nRow + 1, ptG.nCol)) {
			//UP
			if (!IsDeadPos(ptG.nRow - 1, ptG.nCol)) {//~CanPushUp
				stage.ptR = ptG;stage.ptR.nRow++; 
				temp = PushUp(stage);
				if (!m_DLM.IsStaticDeadLock(temp, SB_UP) && (IsStorage(ptG.nRow - 1, ptG.nCol) || CanPushAnyBox_(temp))) {
					//check the corral size
					GetReachableCells(stage, vRCells);
					if (vRCells.size() >= MIN_RC0_SPACE) //R could be there?
						bCanPush = true;//box is not fixed
				}
			}
			//DN
			if (!bCanPush && !IsDeadPos(ptG.nRow + 1, ptG.nCol)) {//~CanPushDown
				stage.ptR = ptG; stage.ptR.nRow--; 
				temp = PushDown(stage);
				if (!m_DLM.IsStaticDeadLock(temp, SB_DN) && (IsStorage(ptG.nRow+1, ptG.nCol) || CanPushAnyBox_(temp))) {
					//last check: corral' size
					GetReachableCells(stage, vRCells);
					if (vRCells.size() >= MIN_RC0_SPACE) //R could be there?
						bCanPush = true;//box is not fixed
				}
			}
		}
		//LT/RT
		if (!bCanPush && !NotSpace(stage, ptG.nRow, ptG.nCol - 1) && !NotSpace(stage, ptG.nRow, ptG.nCol + 1)) {
			//LT
			if (!IsDeadPos(ptG.nRow, ptG.nCol - 1)) {//~CanPushLeft
				stage.ptR = ptG; stage.ptR.nCol++;
				temp = PushLeft(stage);
				if (!m_DLM.IsStaticDeadLock(temp, SB_LT) && (IsStorage(ptG.nRow, ptG.nCol-1) || CanPushAnyBox_(temp))) {
					//check the corral size
					GetReachableCells(stage, vRCells);
					if (vRCells.size() >= MIN_RC0_SPACE) //R could be there?
						bCanPush = true;//box is not fixed
				}
			}
			//RT
			if (!bCanPush && !IsDeadPos(ptG.nRow, ptG.nCol + 1)) {//~CanPushRight
				stage.ptR = ptG; stage.ptR.nCol--;
				temp = PushRight(stage);
				if (!m_DLM.IsStaticDeadLock(temp, SB_RT) && (IsStorage(ptG.nRow, ptG.nCol+1) || CanPushAnyBox_(temp))) {
					//check the corral size
					GetReachableCells(stage, vRCells);
					if (vRCells.size() >= MIN_RC0_SPACE) //R could be there?
						bCanPush = true;//box is not fixed
				}
			}
		}
		if (bCanPush) {
			//check if box can be pulled; if not its a Pull DL!
			if (!IsWall(ptG.nRow - 1, ptG.nCol) && !NotSpace(stage, ptG.nRow - 2, ptG.nCol))
				return false;
			if (!IsWall(ptG.nRow + 1, ptG.nCol) && !NotSpace(stage, ptG.nRow + 2, ptG.nCol))
				return false;
			if (!IsWall(ptG.nRow, ptG.nCol - 1) && !NotSpace(stage, ptG.nRow, ptG.nCol - 2))
				return false;
			if (!IsWall(ptG.nRow, ptG.nCol + 1) && !NotSpace(stage, ptG.nRow, ptG.nCol + 2))
				return false;
			//else - G cannot be pulled=> fixed!
		}
	}
	return true;
}
bool Sokoban::CanPushAnyBox_(const Stage& stage) const {
	Stage temp;
	vector<Point> vRCells;
	GetReachableCells(stage, vRCells);
	for (Point ptCell : vRCells) {
		//UP
		temp = stage; temp.ptR = ptCell;
		if (CanPushUp(temp)) {
			temp = PushUp(temp);
			if (!m_DLM.IsStaticDeadLock(temp, SB_UP))
				return true;
		}
		//DN
		temp = stage; temp.ptR = ptCell;
		if (CanPushDown(temp)) {
			temp = PushDown(temp);
			if (!m_DLM.IsStaticDeadLock(temp, SB_DN))
				return true;
		}
		//LT
		temp = stage; temp.ptR = ptCell;
		if (CanPushLeft(temp)) {
			temp = PushLeft(temp);
			if (!m_DLM.IsStaticDeadLock(temp, SB_LT))
				return true;
		}
		//RT
		temp = stage; temp.ptR = ptCell;
		if (CanPushRight(temp)) {
			temp = PushRight(temp);
			if (!m_DLM.IsStaticDeadLock(temp, SB_RT))
				return true;
		}
	}
	return false;
}
void Sokoban::GetReachableCells(const Stage& stage, OUT vector<Point>& vCells) const {
	vCells.clear();
	Stage temp, current = stage;
	queue<Stage> queueStages;//BFS
	//prolog
	int64_t llAttended = 1ll << CellPos(stage.ptR), llNewPt = 0;
	vCells.push_back(stage.ptR);
	while (1) {
		//UP
		if (CanWalkUp(current)) {
			temp = WalkUp(current);
			llNewPt = 1ll << CellPos(temp.ptR);
			if (0==(llAttended & llNewPt)) {
				llAttended |= llNewPt;		//new robot pos
				queueStages.push(temp);
				vCells.push_back(temp.ptR);
			}
		}
		//LT
		if (CanWalkLeft(current)) {
			temp = WalkLeft(current);
			llNewPt = 1ll << CellPos(temp.ptR);
			if (0 == (llAttended & llNewPt)) {
				llAttended |= llNewPt;		//new robot pos
				queueStages.push(temp);
				vCells.push_back(temp.ptR);
			}
		}
		//DN
		if (CanWalkDown(current)) {
			temp = WalkDown(current);
			llNewPt = 1ll << CellPos(temp.ptR);
			if (0 == (llAttended & llNewPt)) {
				llAttended |= llNewPt;		//new robot pos
				queueStages.push(temp);
				vCells.push_back(temp.ptR);
			}
		}
		//RT
		if (CanWalkRight(current)) {
			temp = WalkRight(current);
			llNewPt = 1ll << CellPos(temp.ptR);
			if (0 == (llAttended & llNewPt)) {
				llAttended |= llNewPt;		//new robot pos
				queueStages.push(temp);
				vCells.push_back(temp.ptR);
			}
		}

		if (queueStages.empty())
			break; //done
		current = queueStages.front();
		queueStages.pop();
	}//while
}




