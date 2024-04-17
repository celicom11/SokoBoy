#include "StdAfx.h"
#include "Corral.h"
#include "Sokoban.h"

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
	assert(m_aCells[ptCell.nRow][ptCell.nCol] == 0);
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
		if (IsPICorral_(nCIdx))
			return nCIdx;
	}
	return 0;//not found
}
void CStageCorrals::OnInnerCorralAdded_() {
	Point ptCell;
	//go recursevely through newly added boxes and add/merge neighbour Corrals! 
	//m_nBoxCount grows after each AddInnerCorral_
	for (uint8_t nBox = 0; nBox < m_nBoxCount; ++nBox) {
		Point ptBox = m_aBoxes[nBox];
		//can push box FROM unkn Corral to the current/last Corral
		//from UP
		if (m_aCells[ptBox.nRow - 1][ptBox.nCol] == CUNKN && m_aCells[ptBox.nRow + 1][ptBox.nCol] == m_nLastCIdx &&
			m_Sokoban.IsSpace(ptBox.nRow - 1, ptBox.nCol) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow - 1, ptBox.nCol)) {
			ptCell = ptBox; --ptCell.nRow;
			AddInnerCorral_(ptCell, m_nLastCIdx);//MERGE to current inner Corral!
		}
		//from DN
		else if (m_aCells[ptBox.nRow + 1][ptBox.nCol] == CUNKN && m_aCells[ptBox.nRow - 1][ptBox.nCol] == m_nLastCIdx &&
			m_Sokoban.IsSpace(ptBox.nRow + 1, ptBox.nCol) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow + 1, ptBox.nCol)) {
			ptCell = ptBox; ++ptCell.nRow;
			AddInnerCorral_(ptCell, m_nLastCIdx);
		}
		//from LT
		if (m_aCells[ptBox.nRow][ptBox.nCol - 1] == CUNKN && m_aCells[ptBox.nRow][ptBox.nCol + 1] == m_nLastCIdx &&
			m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol - 1) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol - 1)) {
			ptCell = ptBox; --ptCell.nCol;
			AddInnerCorral_(ptCell, m_nLastCIdx);
		}
		//from RT
		else if (m_aCells[ptBox.nRow][ptBox.nCol + 1] == CUNKN && m_aCells[ptBox.nRow][ptBox.nCol - 1] == m_nLastCIdx &&
			m_Sokoban.IsSpace(ptBox.nRow, ptBox.nCol + 1) && !m_Sokoban.HasBox(*m_pStage, ptBox.nRow, ptBox.nCol + 1)) {
			ptCell = ptBox; ++ptCell.nCol;
			AddInnerCorral_(ptCell, m_nLastCIdx);
		}
	}
}

// I-Corral is not NOT PI-Corral if
// A) All boxes are G AND there is no free G inside!
// B) if Box cannot be pushed inside CIdx(shown as 1), but can be pushed after 1+ boxes are pushed in corral0(shown as 1)
//Example of blocked upper box 'b'
//1B1 1BB BB1
//NbN	NbN	NbN
//?2?	?2?	?2?
bool CStageCorrals::IsPICorral_(uint8_t nCIdx) const {
	bool bRet = false;//pessimistic
	for (uint8_t nBox = 0; nBox < m_nBoxCount; ++nBox) { //go through non-Corral0 boxes
		Point ptBox = m_aBoxes[nBox];
		//get box that belongs to the corral
		if (!HasCorralEdge_(ptBox, nCIdx))
			continue;
		if(!m_Sokoban.IsStorage(ptBox.nRow, ptBox.nCol))
			bRet = true;//non-G box

		//cbx xbc ,x!=CIdx
		//BbB BbB
		if (CanMoveOutsideCorral_(ptBox, nCIdx)) //its a gate/ not an I-Corral; see "rear examples" below
			return false;
		//???
		//cbc
		if (CanMoveInCorral_(ptBox, nCIdx))
			continue;
		//NOT PI-Corral if
		// A) All boxes are G
		// B) if Box cannot be pushed inside CIdx(shown as c) immediately, but can be after 1+ boxes are pushed in corral0(shown as 1)
		//BTM EDGE  = nCIdx examples:
		// B   BB BB 
		//NbN Nb   bN
		//?c? ?cN Nc?
		// Also, possible that block B can be moved to unmerged Corral x, to unblock 'b' for push 1-2
		//    NNN 
		//   Bxx.... 
		// NNbOOO 
		//  ?cc 
		//
		// Another exotic example: to unblock box 'b' we need to push B inside other unmerged Corrals x,y), to unblock 'b' for push 1-2
		//   O?O
		//   xBy....
		//  OObOOO
		//   ?c?
		//For the two examples above - we cannot treat Corral 2 as PI
		if (m_aCells[ptBox.nRow + 1][ptBox.nCol] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow + 1, ptBox.nCol) && 
			  m_aCells[ptBox.nRow - 1][ptBox.nCol] == CBOX) {
			if (CanUnblockLR_(ptBox.nRow - 1, ptBox.nCol, nCIdx))
				return false;
		}
		//TOP EDGE
		if (m_aCells[ptBox.nRow - 1][ptBox.nCol] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow - 1, ptBox.nCol) &&
			m_aCells[ptBox.nRow + 1][ptBox.nCol] == CBOX) {
			if (CanUnblockLR_(ptBox.nRow + 1, ptBox.nCol, nCIdx))
				return false;
		}
		//RT EDGE
		if (m_aCells[ptBox.nRow][ptBox.nCol+1] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol+1) &&
			m_aCells[ptBox.nRow][ptBox.nCol-1] == CBOX) { 
			if (CanUnblockUD_(ptBox.nRow, ptBox.nCol-1, nCIdx))
				return false;
		}
		//LT EDGE
		if (m_aCells[ptBox.nRow][ptBox.nCol - 1] == nCIdx && !m_Sokoban.IsDeadPos(ptBox.nRow, ptBox.nCol - 1) &&
			m_aCells[ptBox.nRow][ptBox.nCol + 1] == CBOX) { 
			if (CanUnblockUD_(ptBox.nRow, ptBox.nCol + 1, nCIdx))
				return false;
		}
	} //for nBox
	if (!bRet) {
		//we r here if all this is I-Corral with all G boxes on the border
		//however, if there is a free G cell inside - we must return TRUE!
		for (uint8_t nRow = 0; nRow < MAX_DIM; ++nRow) {
			for (uint8_t nCol = 0; nCol < MAX_DIM; ++nCol) {
				if (m_aCells[nRow][nCol] == nCIdx && m_Sokoban.IsStorage(nRow, nCol))
					return true;
			}
		}
	}
	return bRet;//good PI Corral!
}
bool CStageCorrals::IntCanUnblockLR_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT bitset<256>& btsBoxMark) const {
	assert(m_aCells[nRow][nCol] == CBOX);//debug
	//must be a box reachable in C!=nCIdx
	if (!HasOtherCorralEdge_({ nRow,nCol }, nCIdx))
		return false;

	if (m_aCells[nRow][nCol-1] >= CIDX0 && m_aCells[nRow][nCol - 1] != nCIdx) {		//left is another corral
		if (m_aCells[nRow][nCol + 1] >= CIDX0 && m_aCells[nRow][nCol + 1] != nCIdx)	//...and right is another corral
			return !m_Sokoban.IsDeadPos(nRow, nCol + 1) || !m_Sokoban.IsDeadPos(nRow, nCol - 1);
		//else, check an obstacle on the right
		if (m_aCells[nRow][nCol + 1] == CBOX && !btsBoxMark.test(nRow * MAX_DIM + nCol + 1)) {
			btsBoxMark.set(nRow * MAX_DIM + nCol + 1);
			return IntCanUnblockUD_(nRow, nCol + 1, nCIdx, btsBoxMark);
		}
	} 
	else if (m_aCells[nRow][nCol + 1] >= CIDX0 && m_aCells[nRow][nCol + 1] != nCIdx) {//only right is free
		//check an obstacle on the left
		if (m_aCells[nRow][nCol - 1] == CBOX && !btsBoxMark.test(nRow * MAX_DIM + nCol - 1)) {
			btsBoxMark.set(nRow * MAX_DIM + nCol - 1);
			return IntCanUnblockUD_(nRow, nCol - 1, nCIdx, btsBoxMark);
		}
	}
	//else we cannot push this box L/R
	return false;
}
bool CStageCorrals::IntCanUnblockUD_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT bitset<256>& btsBoxMark) const {
	assert(m_aCells[nRow][nCol] == CBOX);//debug
	//must be a box reachable in C != nCIdx
	if (!HasOtherCorralEdge_({ nRow,nCol }, nCIdx))
		return false;

	if (m_aCells[nRow - 1][nCol] >= CIDX0 && m_aCells[nRow-1][nCol] != nCIdx) { //top is free
		if (m_aCells[nRow + 1][nCol] >= CIDX0 && m_aCells[nRow + 1][nCol] != nCIdx)		//...and btm is free
			return !m_Sokoban.IsDeadPos(nRow + 1, nCol) || !m_Sokoban.IsDeadPos(nRow - 1, nCol);
		//else, check an obstacle on the bottom
		if (m_aCells[nRow+1][nCol] == CBOX && !btsBoxMark.test((nRow+1) * MAX_DIM + nCol)) {
			btsBoxMark.set((nRow+1) * MAX_DIM + nCol);
			return IntCanUnblockLR_(nRow+1, nCol, nCIdx, btsBoxMark);
		}
	}
	else if (m_aCells[nRow + 1][nCol] >= CIDX0 && m_aCells[nRow + 1][nCol] != nCIdx) {//only btm is free
		//check an obstacle on top
		if (m_aCells[nRow-1][nCol] == CBOX && !btsBoxMark.test((nRow-1) * MAX_DIM + nCol)) {
			btsBoxMark.set((nRow-1) * MAX_DIM + nCol);
			return IntCanUnblockLR_(nRow-1, nCol, nCIdx, btsBoxMark);
		}
	}
	//else we cannot push this box U/D
	return false;
}
