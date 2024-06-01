#include "StdAfx.h"
#include "Corral.h"
#include "Sokoban.h"

bool CStageCorrals::NotSpace_(Point ptCell) const {
	return HasBox_(ptCell) || m_Sokoban.IsWall(ptCell.nRow, ptCell.nCol);
}
bool CStageCorrals::IsDead_(Point ptCell) const {
	return m_Sokoban.IsDeadPos(ptCell.nRow, ptCell.nCol);
}
bool CStageCorrals::IsWall_(Point ptCell) const {
	return m_Sokoban.IsWall(ptCell.nRow, ptCell.nCol);
}
bool CStageCorrals::IsG_(Point ptCell) const {
	return m_Sokoban.IsStorage(ptCell.nRow, ptCell.nCol);
}
bool CStageCorrals::IsNonCorralSpace_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const {
	return !m_Sokoban.NotSpace(*m_pStage, nRow, nCol) && m_aCells[nRow][nCol] != nCIdx;
}
bool CStageCorrals::IsOtherCorralSpace_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const {
	return IsNonCorralSpace_(nRow,nCol,nCIdx) && m_aCells[nRow][nCol] != CIDX0;
}


void CStageCorrals::Init(const Stage* pStage) {
	m_pStage = pStage;
	m_nBoxCount = m_nC0BoxCount = 0;
	memset(m_aCells, 0, sizeof(m_aCells));
	memset(m_aBoxes, 0, sizeof(m_aBoxes));
	AddCorral_(pStage->ptR, CIDX0);
	m_nLastCIdx = CIDX0;
}
bool CStageCorrals::CanPushUp(Point ptBox, uint8_t nCIdx) const {
	return ptBox.nRow > 1 && m_aCells[ptBox.nRow + 1][ptBox.nCol] == CIDX0 && 
		m_Sokoban.IsSpace(ptBox.nRow - 1, ptBox.nCol) && !m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol) &&
		(
			(nCIdx <= CIDX0 && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow - 1, ptBox.nCol)) ||
			(nCIdx > CIDX0 && m_aCells[ptBox.nRow - 1][ptBox.nCol] == nCIdx)
			);
}
bool CStageCorrals::CanPushDown(Point ptBox, uint8_t nCIdx) const {
	return m_aCells[ptBox.nRow - 1][ptBox.nCol] == CIDX0 && 
		m_Sokoban.IsSpace(ptBox.nRow + 1, ptBox.nCol) && !m_Sokoban.IsDeadPos(ptBox.nRow + 1, ptBox.nCol) &&
		(
			(nCIdx <= CIDX0 && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow + 1, ptBox.nCol)) ||
			(nCIdx > CIDX0 && m_aCells[ptBox.nRow + 1][ptBox.nCol] == nCIdx)
			);
}
bool CStageCorrals::CanPushLeft(Point ptBox, uint8_t nCIdx) const {
	return ptBox.nCol > 1 && m_aCells[ptBox.nRow][ptBox.nCol+1] == CIDX0 && 
		m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol-1) && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol-1) &&
		(
			(nCIdx <= CIDX0 && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol-1)) ||
			(nCIdx > CIDX0 && m_aCells[ptBox.nRow][ptBox.nCol-1] == nCIdx)
			);
}
bool CStageCorrals::CanPushRight(Point ptBox, uint8_t nCIdx) const {
	return m_aCells[ptBox.nRow][ptBox.nCol - 1] == CIDX0 && 
		m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol + 1) && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol + 1) &&
		(
			(nCIdx <= CIDX0 && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol + 1)) ||
			(nCIdx > CIDX0 && m_aCells[ptBox.nRow][ptBox.nCol + 1] == nCIdx)
			);
}
Corral CStageCorrals::GetCorral(uint8_t nCIdx) const {
	Corral crlRet;
	//go through ALL boxes and 
	// add all boxes that 
	// a. have a any edge with the Corral's spaces OR
	// b. are locked by one of the Corral's boxes 
	for (uint8_t nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		Point ptBox = m_aBoxes[nBox];
		if (HasCorralEdge_(ptBox, nCIdx))
			crlRet.llBoxes |= 1ll << m_Sokoban.CellPos(ptBox);
		else {
			int nCBs = 0; int nWalls = 0;
			//UP
			if (m_aCells[ptBox.nRow - 1][ptBox.nCol] == CBOX && HasCorralEdge_({ uint8_t(ptBox.nRow-1),ptBox.nCol }, nCIdx))
				++nCBs;
			else if (m_aCells[ptBox.nRow - 1][ptBox.nCol] == CWALL)
				++nWalls;
			//DN
			if (m_aCells[ptBox.nRow + 1][ptBox.nCol] == CBOX && HasCorralEdge_({ uint8_t(ptBox.nRow + 1),ptBox.nCol }, nCIdx))
				++nCBs;
			else if (m_aCells[ptBox.nRow + 1][ptBox.nCol] == CWALL)
				++nWalls;
			//LT
			if (m_aCells[ptBox.nRow][ptBox.nCol-1] == CBOX && HasCorralEdge_({ ptBox.nRow,uint8_t(ptBox.nCol-1) }, nCIdx))
				++nCBs;
			else if (m_aCells[ptBox.nRow][ptBox.nCol-1] == CWALL)
				++nWalls;
			//RT
			if (m_aCells[ptBox.nRow][ptBox.nCol + 1] == CBOX && HasCorralEdge_({ ptBox.nRow,uint8_t(ptBox.nCol + 1) }, nCIdx))
				++nCBs;
			else if (m_aCells[ptBox.nRow][ptBox.nCol + 1] == CWALL)
				++nWalls;
			if(nCBs && nCBs+nWalls > 1) //we presume only such boxes may influence Corral's deadlock - TODO: VERIFY
				crlRet.llBoxes |= 1ll << m_Sokoban.CellPos(ptBox);
		}
	}
	if (nCIdx == m_nLastPICIdx)
		crlRet.llBoxes |= m_llExternalLocked; //we MUST add external locked boxes for G-only corrals!
	for (uint8_t nRow = 0; nRow < MAX_DIM; ++nRow) {
		for (uint8_t nCol = 0; nCol < MAX_DIM; ++nCol) {
			if (m_aCells[nRow][nCol] == nCIdx)
				crlRet.llCells |= 1ll << m_Sokoban.CellPos({ nRow, nCol });
		}
	}
	return crlRet;
}
bool CStageCorrals::IsSingleCorral_() const {
	for (uint8_t nRow = 0; nRow < MAX_DIM; ++nRow) {
		for (uint8_t nCol = 0; nCol < MAX_DIM; ++nCol) {
			if (!m_Sokoban.NotSpace(*m_pStage, nRow, nCol) && m_aCells[nRow][nCol] != CIDX0)
				return false;
		}
	}
	return true;
}
void CStageCorrals::InitBoxCell_(uint8_t nRow, uint8_t nCol, bool bC0) {
	if (m_aCells[nRow][nCol] == CUNKN) {
		if (m_Sokoban.IsWall(nRow, nCol))
			m_aCells[nRow][nCol] = CWALL;
		else if (m_Sokoban.HasBox(*m_pStage, nRow, nCol)) {
			m_aCells[nRow][nCol] = CBOX;
			m_aBoxes[m_nBoxCount] = { nRow,nCol };
			++m_nBoxCount;
			if (bC0)
				++m_nC0BoxCount;
		}
	}
}
void CStageCorrals::AddCorral_(Point ptCell, uint8_t nCIdx){
	Stage temp, current = *m_pStage;
	current.ptR = ptCell;
	const bool bC0 = nCIdx == CIDX0;
	stack<Stage> stackStages;//~DFS
	//prolog
	assert(m_aCells[ptCell.nRow][ptCell.nCol] == CUNKN && !m_Sokoban.NotSpace(current, ptCell.nRow, ptCell.nCol));
	m_aCells[ptCell.nRow][ptCell.nCol] = nCIdx;
	//init boxes around, if any
	InitBoxCell_(ptCell.nRow - 1, ptCell.nCol, bC0);
	InitBoxCell_(ptCell.nRow + 1, ptCell.nCol, bC0);
	InitBoxCell_(ptCell.nRow, ptCell.nCol-1, bC0);
	InitBoxCell_(ptCell.nRow, ptCell.nCol+1, bC0);
	while (1) {
		//UP
		if (m_Sokoban.CanWalkUp(current)) {
			temp = m_Sokoban.WalkUp(current);
			if (m_aCells[temp.ptR.nRow][temp.ptR.nCol] == CUNKN) {
				m_aCells[temp.ptR.nRow][temp.ptR.nCol] = nCIdx;
				stackStages.push(temp);
				//init boxes around, if any
				InitBoxCell_(temp.ptR.nRow - 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol - 1, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol + 1, bC0);
			}
		}
		//LEFT
		if (m_Sokoban.CanWalkLeft(current)) {
			temp = m_Sokoban.WalkLeft(current);
			if (m_aCells[temp.ptR.nRow][temp.ptR.nCol] == CUNKN) {
				m_aCells[temp.ptR.nRow][temp.ptR.nCol] = nCIdx;
				stackStages.push(temp);
				//init boxes around, if any
				InitBoxCell_(temp.ptR.nRow - 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow + 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol - 1, bC0);
			}
		}
		//DOWN
		if (m_Sokoban.CanWalkDown(current)) {
			temp = m_Sokoban.WalkDown(current);
			if (m_aCells[temp.ptR.nRow][temp.ptR.nCol] == CUNKN) {
				m_aCells[temp.ptR.nRow][temp.ptR.nCol] = nCIdx;
				stackStages.push(temp);
				//init boxes around, if any
				InitBoxCell_(temp.ptR.nRow + 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol - 1, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol + 1, bC0);
			}
		}
		//RIGHT
		if (m_Sokoban.CanWalkRight(current)) {
			temp = m_Sokoban.WalkRight(current);
			if (m_aCells[temp.ptR.nRow][temp.ptR.nCol] == CUNKN) {
				m_aCells[temp.ptR.nRow][temp.ptR.nCol] = nCIdx;
				stackStages.push(temp);
				//init boxes around, if any
				InitBoxCell_(temp.ptR.nRow - 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow + 1, temp.ptR.nCol, bC0);
				InitBoxCell_(temp.ptR.nRow, temp.ptR.nCol + 1, bC0);
			}
		}
		if (stackStages.empty())
			break;
		current = stackStages.top();
		stackStages.pop();
	}//while
}

//sokobano.de/wiki/index.php?title=Sokoban_solver_%22scribbles%22_by_Brian_Damgaard_about_the_YASS_solver#PI-Corral_pruning
//returns 0 if not found, otherwise corral's idx-1 (that is >0!)
// 
//new extra rule for corral merging based on the following situation:
// ?2N
// NBO     
//  G.OON   
// RGB11N
//    NN
//
// Presumed that we have corral 2 at the top, "our" single cell PI-Corral in the middle and corral 1 at the bottom-right.
// The top G can be pushed RT wo locking top/1-cell corral, however, it does not make 3x3 DL but it MAY or NOT lock the next corral 1.
// If corral 1 is DDL, our corral becomes also DDL what is incorrect as it is not a DL Corral per se (w/o corral 1).
// For correct DDL detection, the influence of such  "neighbour corrals" can be resolved only by merging it with 
// corral 1 to a single PI-Corral! It is NOT the same for the unlocked top B, as the pushed left G CANNOT lead to the DL of the unlocked corral 2!
//NOTE: For this, instead of POST-corral analysis merging, we merge it with the detected DDL boxpos - to be verified...
int8_t CStageCorrals::GetPICorral() {
	Point ptCell;
	if (m_nC0BoxCount <= 2 || IsSingleCorral_()) //|| m_nC0BoxCount == m_Sokoban.BoxCount() was an error!
		return 0; //do not waste time
	
	//1.fill ALL spaces w Corral Idx;
	for (int8_t nBox = 0; nBox < m_nC0BoxCount; ++nBox) { //go through Corral0 boxes
		Point ptBox = m_aBoxes[nBox];
		//can push box to unkn Corral 
		//UP
		if (m_aCells[ptBox.nRow-1][ptBox.nCol] == CUNKN && m_aCells[ptBox.nRow + 1][ptBox.nCol] == CIDX0 && 
			m_Sokoban.IsSpace(ptBox.nRow - 1,ptBox.nCol) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow - 1, ptBox.nCol) ) {
			ptCell = ptBox; --ptCell.nRow;
			AddInnerCorral_(ptCell, ++m_nLastCIdx);
		}
		//DN
		else if (m_aCells[ptBox.nRow + 1][ptBox.nCol] == CUNKN && m_aCells[ptBox.nRow - 1][ptBox.nCol] == CIDX0 &&
			m_Sokoban.IsSpace(ptBox.nRow + 1, ptBox.nCol) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow + 1, ptBox.nCol)) {
			ptCell = ptBox; ++ptCell.nRow;
			AddInnerCorral_(ptCell, ++m_nLastCIdx);
		}
		//LT
		if (m_aCells[ptBox.nRow][ptBox.nCol-1] == CUNKN && m_aCells[ptBox.nRow][ptBox.nCol+1] == CIDX0 &&
			m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol-1) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol-1)) {
			ptCell = ptBox; --ptCell.nCol;
			AddInnerCorral_(ptCell, ++m_nLastCIdx);
		}
		//RT
		else if (m_aCells[ptBox.nRow][ptBox.nCol + 1] == CUNKN && m_aCells[ptBox.nRow][ptBox.nCol - 1] == CIDX0 &&
			m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol + 1) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol + 1)) {
			ptCell = ptBox; ++ptCell.nCol;
			AddInnerCorral_(ptCell, ++m_nLastCIdx);
		}
	}
	//2. add missed boxes if any
	if (m_nBoxCount < m_Sokoban.BoxCount()) {
		uint8_t nAllBoxes = m_nBoxCount;
		for (uint8_t nRow = 0; nRow < MAX_DIM; ++nRow) {
			for (uint8_t nCol = 0; nCol < MAX_DIM; ++nCol) {
				if (m_aCells[nRow][nCol] == CUNKN && m_Sokoban.HasBox(*m_pStage, nRow, nCol)) {
					m_aCells[nRow][nCol] = CBOX;
					m_aBoxes[nAllBoxes] = { nRow, nCol };
					++nAllBoxes;
				}
			}
		}
	}
	//3. Find PI Corral for Corral0 boxes
	for (uint8_t nCIdx = CIDX0 + 1; nCIdx <= m_nLastCIdx; ++nCIdx) { //go through Corral0 boxes
		m_nLastPICIdx = nCIdx;
		int64_t llExternalLocked = 0;
		if (IsPICorral_(nCIdx, llExternalLocked)) {
			m_llExternalLocked = llExternalLocked;
			return nCIdx;
		}
	}
	return 0;//not found
}
//UDLR order
void CStageCorrals::GetBoxNeighb_(Point ptBox, OUT uint8_t (&aCC)[4]) const {
	const Point aCells[4]{ {uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol},
                         {ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };

	for (int nIdx = 0; nIdx < 4; ++nIdx) {
		Point ptCell = aCells[nIdx];
		uint8_t nCC = m_aCells[ptCell.nRow][ptCell.nCol];
		if (nCC == CUNKN) {
			if (m_Sokoban.IsWall(ptCell.nRow, ptCell.nCol))
				nCC = CWALL;
			else if (m_Sokoban.HasBox(*m_pStage, ptCell.nRow, ptCell.nCol))
				nCC = CBOX;
		}
		aCC[nIdx] = nCC;
	}
}
Point CStageCorrals::OnInnerCorralAdded_() {
	Point ptCell;
	//go through all pushable boxes that have borders with last/new corral and an unkown one and merge them(recursively)!
	//NB: m_nBoxCount grows after each AddInnerCorral_
	for (uint8_t nBox = 0; nBox < m_nBoxCount; ++nBox) {
		Point ptBox = m_aBoxes[nBox];
		//cell codes for UP,DN,LT,RT squares around the box
		uint8_t aCC[4];
		GetBoxNeighb_(ptBox, aCC);
		//box on the new corral's border is pushable-inside (after it is unblockable!) from the UNKN corral(?)->Merge!
		//check TOP square
		if (aCC[SB_DN] != CBOX && aCC[SB_DN] != CWALL && !m_Sokoban.IsDeadPos(ptBox.nRow+1, ptBox.nCol) &&
			(aCC[SB_DN] == m_nLastCIdx || aCC[SB_LT] == m_nLastCIdx || aCC[SB_RT] == m_nLastCIdx) &&
			(aCC[SB_UP] == CUNKN || aCC[SB_UP] == CBOX))
		{ //got smtng to merge?
			ptCell = ptBox; --ptCell.nRow;
			if (aCC[SB_UP] == CBOX) {//check LR cells
				uint8_t aCC2[4];
				GetBoxNeighb_(ptCell, aCC2);
				if (aCC2[2] == CUNKN && (aCC2[3] == CUNKN || aCC2[3] >= CIDX0))
					--ptCell.nCol;
				else if (aCC2[3] == CUNKN && aCC2[2] >= CIDX0)
					++ptCell.nCol;
				else
					ptCell.nRow = ptCell.nCol = 0;//invalid
			}
			if (ptCell.nRow && ptCell.nCol)
				return ptCell;
		}
		// BTM
		if (aCC[SB_UP] != CBOX && aCC[SB_UP] != CWALL && !m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol) &&
			(aCC[SB_UP] == m_nLastCIdx || aCC[SB_LT] == m_nLastCIdx || aCC[SB_RT] == m_nLastCIdx) && 
			(aCC[SB_DN] == CUNKN || aCC[SB_DN] == CBOX)) 
		{
			ptCell = ptBox; ++ptCell.nRow;
			if (aCC[SB_DN] == CBOX) {//check LR cells
				uint8_t aCC2[4];
				GetBoxNeighb_(ptCell, aCC2);
				if (aCC2[2] == CUNKN && (aCC2[3] == CUNKN || aCC2[3] >= CIDX0))
					--ptCell.nCol;
				else if (aCC2[3] == CUNKN && aCC2[2] >= CIDX0)
					++ptCell.nCol;
				else
					ptCell.nRow = ptCell.nCol = 0;//invalid
			}
			if (ptCell.nRow && ptCell.nCol)
				return ptCell;
		}
		//LT
		if (aCC[SB_RT] != CBOX && aCC[SB_RT] != CWALL && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol+1) &&
			(aCC[SB_UP] == m_nLastCIdx || aCC[SB_DN] == m_nLastCIdx || aCC[SB_RT] == m_nLastCIdx) &&
			(aCC[SB_LT] == CUNKN || aCC[SB_LT] == CBOX )) 
		{
			ptCell = ptBox; --ptCell.nCol;
			if (aCC[SB_LT] == CBOX) {//check UD cells
				uint8_t aCC2[4];
				GetBoxNeighb_(ptCell, aCC2);
				if (aCC2[0] == CUNKN && (aCC2[1] == CUNKN || aCC2[1] >= CIDX0))
					--ptCell.nRow;
				else if (aCC2[1] == CUNKN && aCC2[0] >= CIDX0)
					++ptCell.nRow;
				else
					ptCell.nRow = ptCell.nCol = 0;//invalid
			}
			if (ptCell.nRow && ptCell.nCol)
				return ptCell;
		}
		//RT
		if (aCC[SB_LT] != CBOX && aCC[SB_LT] != CWALL && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol-1) &&
			(aCC[SB_UP] == m_nLastCIdx || aCC[SB_DN] == m_nLastCIdx || aCC[SB_LT] == m_nLastCIdx) &&
			(aCC[SB_RT] == CUNKN || aCC[SB_RT] == CBOX )) 
		{
			ptCell = ptBox; ++ptCell.nCol;
			if (aCC[SB_RT] == CBOX) {//check UD cells
				uint8_t aCC2[4];
				GetBoxNeighb_(ptCell, aCC2);
				if (aCC2[0] == CUNKN && (aCC2[1] == CUNKN || aCC2[1] >= CIDX0))
					--ptCell.nRow;
				else if (aCC2[1] == CUNKN && aCC2[0] >= CIDX0)
					++ptCell.nRow;
				else
					ptCell.nRow = ptCell.nCol = 0;//invalid
			}
			if (ptCell.nRow && ptCell.nCol)
				return ptCell;
		}
	}
	return { 0,0 };//nothing to merge
}

//NOTE: any already processed Corrals are NOT-PI!
// I-Corral is not NOT PI-Corral if
// A) All boxes are G AND there is no free G inside!
// B) Border box is a "gate": can be moved outside CIdx (C0 OR in other non-PI Corral)
// C) Box can be pushed inside CIdx from other (non-PI) Corral  
// D) if Box cannot be pushed into the CIdx(shown as 2), but CAN be pushed after it is unblocked in C0/other Corral (shown as 1)
//Example of un-blockable top box 'b'
//1B1 1BB BB1
//NbN	Nb1	1bN
//?2?	?2?	?2?

bool CStageCorrals::IsPICorral_(uint8_t nCIdx, OUT int64_t& llExternalLocked) const {
	bool bRet = false;//pessimistic
	llExternalLocked = 0;

	Stage temp;

	for (uint8_t nBox = 0; nBox < m_nBoxCount; ++nBox) { //go through non-Corral0 boxes
		int64_t llEL = 0;
		Point ptBox = m_aBoxes[nBox];
		uint8_t aCC[4];
		GetBoxNeighb_(ptBox, aCC);
		//get box that belongs to the corral
		if (aCC[SB_UP] != nCIdx && aCC[SB_DN] != nCIdx && aCC[SB_LT] != nCIdx && aCC[SB_RT] != nCIdx)
			continue;
		if(!m_Sokoban.IsStorage(ptBox.nRow, ptBox.nCol))
			bRet = true;//non-G box
		//B. check if box is a gate/can be moved outside the nCIdx
		if (IsNonCorralSpace_(ptBox.nRow - 1, ptBox.nCol, nCIdx) && IsNonCorralSpace_(ptBox.nRow + 1, ptBox.nCol, nCIdx)) {
			if (!m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol)) {
				//we need extra check if box would be deadlocked on push up...
				//hopefully if extrnal boxes influence it will be picked up by Push2DDL stuff....
				temp = *m_pStage;
				temp.ptR = ptBox; ++temp.ptR.nRow;//its a space!
				temp = m_Sokoban.PushUp(temp);
				if (!m_Sokoban.IsFixedDeadLock(temp, SB_UP))
					return false;
			}
			if (!m_Sokoban.IsDeadPos(ptBox.nRow + 1, ptBox.nCol)) {
				temp = *m_pStage;
				temp.ptR = ptBox; --temp.ptR.nRow;//its a space!
				temp = m_Sokoban.PushDown(temp);
				if (!m_Sokoban.IsFixedDeadLock(temp, SB_DN))
					return false;
			}
		}
		if (IsNonCorralSpace_(ptBox.nRow, ptBox.nCol - 1, nCIdx) && IsNonCorralSpace_(ptBox.nRow, ptBox.nCol + 1, nCIdx)) {
			if (!m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol - 1)) {
				temp = *m_pStage;
				temp.ptR = ptBox; ++temp.ptR.nCol;//its a space!
				temp = m_Sokoban.PushLeft(temp);
				if (!m_Sokoban.IsFixedDeadLock(temp, SB_LT))
					return false;
			}
			if (!m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol + 1)) {
				temp = *m_pStage;
				temp.ptR = ptBox; --temp.ptR.nCol;//its a space!
				temp = m_Sokoban.PushRight(temp);
				if (!m_Sokoban.IsFixedDeadLock(temp, SB_RT))
					return false;
			}
		}
		//C. If box can be moved-in from another corral
		//from TOP
		if (aCC[SB_DN] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow + 1, ptBox.nCol) && IsOtherCorralSpace_(ptBox.nRow - 1, ptBox.nCol, nCIdx))
			return false;
		//from BTM
		if (aCC[SB_UP] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol) && IsOtherCorralSpace_(ptBox.nRow + 1, ptBox.nCol, nCIdx))
			return false;
		//from LT
		if (aCC[SB_RT] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol + 1) && IsOtherCorralSpace_(ptBox.nRow, ptBox.nCol - 1, nCIdx))
			return false;
		//from RT
		if (aCC[SB_LT] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol - 1) && IsOtherCorralSpace_(ptBox.nRow, ptBox.nCol + 1, nCIdx))
			return false;
		//D.
		//TOP cell is non-dead nCIdx cell, bottom is unblockable box in non-CIdx corral
		//FIX: if it is locked - it must be added to the llExternalLocked boxes!!
		if (aCC[SB_UP] == nCIdx && aCC[SB_DN] == CBOX && !m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol)) {
			if (CanUnblockLR_(ptBox.nRow + 1, ptBox.nCol, nCIdx, llEL))
				return false;
		}
		//BTM
		if (aCC[SB_DN] == nCIdx && aCC[SB_UP] == CBOX && !m_Sokoban.IsDeadPos(ptBox.nRow + 1, ptBox.nCol)) {
			if (CanUnblockLR_(ptBox.nRow - 1, ptBox.nCol, nCIdx, llEL))
				return false;
		}
		//LT
		if (aCC[SB_LT] == nCIdx && aCC[SB_RT] == CBOX && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol - 1)) {
			if (CanUnblockUD_(ptBox.nRow, ptBox.nCol + 1, nCIdx, llEL))
				return false;
		}
		//RT
		if (aCC[SB_RT] == nCIdx && aCC[SB_LT] == CBOX && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol + 1)) {
			if (CanUnblockUD_(ptBox.nRow, ptBox.nCol - 1, nCIdx, llEL))
				return false;
		}
		llExternalLocked |= llEL;
	} //for nBox
	//add any external boxes that locks Corral border boxes
	//this is crucial for PIC DDL!!
	if (!bRet) {
		//we r here if all this is I-Corral with all G boxes on the border
		//however, if there is a free G cell inside - we must return TRUE!
		for (uint8_t nRow = 0; nRow < m_Sokoban.Dim().nRows; ++nRow) {
			for (uint8_t nCol = 0; nCol < m_Sokoban.Dim().nCols; ++nCol) {
				if (m_aCells[nRow][nCol] == nCIdx && m_Sokoban.IsStorage(nRow, nCol))
					return true;
			}
		}
		//FIX: if non-G non-border box is "locked" by Corral's G-boxes we still MUST to push inside PI-Corral to release this box!
		// #G$ - LockedBox1!
		//G..G
		// #GG$ - LockedBox2!
		//    $#
		return CalcLockedNonG_(nCIdx, llExternalLocked);
	}
	return bRet;
}
bool CStageCorrals::CalcLockedNonG_(uint8_t nCIdx, IN OUT int64_t& llExternalLocked) const {
	bool bRet = false;
	//Get all boxes connected to coral somehow
	bitset<MAX_SPACES> btsConnected;
	queue<Point> qConnected;
	//first pass - boxes around corral border
	uint8_t aBoxesPos[MAX_BOXES];
	m_pStage->GetBoxesPos(aBoxesPos);
	for (uint16_t nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
		if (HasCorralEdge_(ptBox, nCIdx)) {
			Point aPtUDLR[4] = { {uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol},
													{ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };
			for (Point ptCell : aPtUDLR) {
				if (HasBox_(ptCell) && !HasCorralEdge_(ptCell, nCIdx)) {
					if (!btsConnected.test(m_Sokoban.CellPos(ptCell))) {
						btsConnected.set(m_Sokoban.CellPos(ptCell));
						qConnected.push(ptCell);
					}
				}
			}
		}
	} //for 1
	//second pass - boxes around first pass
	while (!qConnected.empty()) {
		Point ptBox = qConnected.front(); qConnected.pop(); //Pop
		Point aPtUDLR[4] = { {uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol},
												{ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };
		for (Point ptCell : aPtUDLR) {
			if (HasBox_(ptCell) && !btsConnected.test(m_Sokoban.CellPos(ptCell)) && !HasCorralEdge_(ptCell, nCIdx)) {
				btsConnected.set(m_Sokoban.CellPos(ptCell));
				qConnected.push(ptCell); //recursion
			}
		}
	}//while/for 2
	//third pass - find ALL potentially locked among all "connected" boxes
	bitset<MAX_SPACES> btsLocked;
	for (uint16_t nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
		if (btsConnected.test(m_Sokoban.CellPos(ptBox))) {
			//check if box is >potentially< "locked" by (connected)boxes/walls OR deadPos!
			Point aPtUDLR[4] = { {uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol},
									{ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };
			if ((NotSpace_(aPtUDLR[0]) || NotSpace_(aPtUDLR[1])) &&
				(NotSpace_(aPtUDLR[2]) || NotSpace_(aPtUDLR[3]) || (IsDead_(aPtUDLR[2]) && IsDead_(aPtUDLR[3]))))
				btsLocked.set(m_Sokoban.CellPos(ptBox));
			else if ((NotSpace_(aPtUDLR[2]) || NotSpace_(aPtUDLR[3])) && (IsDead_(aPtUDLR[0]) && IsDead_(aPtUDLR[1])))
				btsLocked.set(m_Sokoban.CellPos(ptBox));
		}
	}//for 3
	//4th pass - find "loose" boxes among (potentially) connected AND locked
	for (uint16_t nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
		if (!btsLocked.test(m_Sokoban.CellPos(ptBox)))
			continue;
		Point aPtUDLR[4] = { {uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol},
								{ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };
		const int aCellPos[4]{ IsWall_(aPtUDLR[0]) ? 0 : m_Sokoban.CellPos(aPtUDLR[0]), 
													 IsWall_(aPtUDLR[1]) ? 0 : m_Sokoban.CellPos(aPtUDLR[1]),
													IsWall_(aPtUDLR[2]) ? 0 : m_Sokoban.CellPos(aPtUDLR[2]),
													IsWall_(aPtUDLR[3]) ? 0 : m_Sokoban.CellPos(aPtUDLR[3]) };
		//non-locked connected TOP box (NOTE: it can be actually still unpushable, but its TOO MUCH to check this...)
		if (HasBox_(aPtUDLR[0]) && !btsLocked.test(aCellPos[0]) && btsConnected.test(aCellPos[0])) {
			//we are still locked if at the bottom we have wall OR box which is locked OR corral 
			if (IsWall_(aPtUDLR[1]) ||
				( HasBox_(aPtUDLR[1]) && (btsLocked.test(aCellPos[1]) || !btsConnected.test(aCellPos[1])) )
				);
			else {
				btsLocked.set(m_Sokoban.CellPos(ptBox), false);//unlock the box and start over!
				nBox = (uint16_t)(-1);
				continue;
			}
		}
		//non-locked BTM box
		if (HasBox_(aPtUDLR[1]) && !btsLocked.test(aCellPos[1]) && btsConnected.test(aCellPos[1])) {
			//we are still locked if at the top we have wall OR box which is locked OR corral 
			if (IsWall_(aPtUDLR[0]) ||
				( HasBox_(aPtUDLR[0]) && (btsLocked.test(aCellPos[0]) || !btsConnected.test(aCellPos[0])) )
				);
			else {
				btsLocked.set(m_Sokoban.CellPos(ptBox), false);//unlock the box and start over!
				nBox = (uint16_t)(-1);
				continue;
			}
		}
		//non-locked LT box
		if (HasBox_(aPtUDLR[2]) && !btsLocked.test(aCellPos[2]) && btsConnected.test(aCellPos[2])) {
			//we are still locked if at the right we have wall OR box which is locked OR corral 
			if (IsWall_(aPtUDLR[3]) ||
				( HasBox_(aPtUDLR[3]) && (btsLocked.test(aCellPos[3]) || !btsConnected.test(aCellPos[3])) )
				);
			else {
				btsLocked.set(m_Sokoban.CellPos(ptBox), false);//unlock the box and start over!
				nBox = (uint16_t)(-1);
				continue;
			}
		}
		//non-locked RT box
		if (HasBox_(aPtUDLR[3]) && !btsLocked.test(aCellPos[3]) && btsConnected.test(aCellPos[3])) {
			//we are still locked if at the right we have wall OR box which is locked OR corral 
			if (IsWall_(aPtUDLR[2]) ||
				( HasBox_(aPtUDLR[2]) && (btsLocked.test(aCellPos[2]) || !btsConnected.test(aCellPos[2])) )
				);
			else {
				btsLocked.set(m_Sokoban.CellPos(ptBox), false);//unlock the box and start over!
				nBox = (uint16_t)(-1);
				continue;
			}
		}
	} //for 4
	//5th/last pass - find really locked non-G AND add to corral boxes ALL LOCKED BOXES outside the corral
	for (uint16_t nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
		uint8_t nBoxPos = m_Sokoban.CellPos(ptBox);
		if (btsLocked.test(nBoxPos)) {
			llExternalLocked |= 1ll << nBoxPos;
			if (!IsG_(ptBox))
				bRet = true; //"good" PI Corral!
		}
	}//for 5
	return bRet;//not found
}

bool CStageCorrals::CanUnblockLR_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT int64_t& llBoxes) const {
	assert(m_aCells[nRow][nCol] == CBOX);//debug

	int64_t llBox = 1ll << m_Sokoban.CellPos({ nRow,nCol });
	if (llBoxes & llBox)
		return false;//already checked!
	llBoxes |= llBox;//add bit - if its inner corral's box its OK!

	if (IsNonCorralSpace_(nRow, nCol-1, nCIdx)) {		//left is another corral
		if (IsNonCorralSpace_(nRow, nCol + 1, nCIdx))	//...and right is another corral
			return !m_Sokoban.IsDeadPos(nRow, nCol + 1) || !m_Sokoban.IsDeadPos(nRow, nCol - 1);
		//else, check an obstacle on the right
		if (m_aCells[nRow][nCol + 1] == CBOX)
			return CanUnblockUD_(nRow, nCol + 1, nCIdx, llBoxes);
	} 
	else if (IsNonCorralSpace_(nRow, nCol + 1, nCIdx)) {//only right is free
		//check left box
		if (m_aCells[nRow][nCol - 1] == CBOX)
			return CanUnblockUD_(nRow, nCol - 1, nCIdx, llBoxes);
	}
	//else we cannot push this box L/R
	return false;
}

bool CStageCorrals::CanUnblockUD_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT int64_t& llBoxes) const {
	assert(m_aCells[nRow][nCol] == CBOX);//debug

	if (IsNonCorralSpace_(nRow - 1,nCol, nCIdx)) { //top is free
		if (IsNonCorralSpace_(nRow + 1, nCol, nCIdx))		//...and btm is free
			return !m_Sokoban.IsDeadPos(nRow + 1, nCol) || !m_Sokoban.IsDeadPos(nRow - 1, nCol);
		//else, check an obstacle on the bottom
		if (m_aCells[nRow+1][nCol] == CBOX)
			return CanUnblockLR_(nRow+1, nCol, nCIdx, llBoxes);
	}
	else if (IsNonCorralSpace_(nRow + 1, nCol, nCIdx)) {//only btm is free
		//check top box
		if (m_aCells[nRow-1][nCol] == CBOX)
			return CanUnblockLR_(nRow-1, nCol, nCIdx, llBoxes);
	}
	//else we cannot push this box U/D
	return false;
}

