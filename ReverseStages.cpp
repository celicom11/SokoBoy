#include "StdAfx.h"
#include "Sokoban.h"
#include "ReverseStages.h"
#include "Hungarian1616.h"
#include "StageQueue.h"

namespace { //static helpers
	void _CheckAddStage(IN OUT Stage& temp, IN OUT deque<Stage>& dqOpened, const vector<Stage>& vClosed, uint32_t nPIdx) {
		if (std::find(dqOpened.begin(), dqOpened.end(), temp) == dqOpened.end() &&
			std::find(vClosed.begin(), vClosed.end(), temp) == vClosed.end()) {
			temp.nPIdx = nPIdx;
			//_ASSERT(temp.llBoxPos != vClosed[nPIdx - 1].llBoxPos);
			dqOpened.push_back(temp);
		}
	}
	uint32_t _Depth(const Stage& stage, const vector<Stage>& vClosed) {
		uint32_t nRet = 0;
		const Stage* pStage = &stage;
		while (pStage && pStage->nPIdx) {
			pStage = &vClosed[pStage->nPIdx - 1];
			++nRet;
		}
		return nRet;
	}
#define BOX_TOP	0x01
#define BOX_BTM	0x02
#define BOX_LT	0x04
#define BOX_RT	0x08
	int _BoxEdges(Point ptBox, const vector<Point>& vCells) {
		int nRet = 0;
		for (Point ptCell : vCells) {
			if (ptCell.nRow + 1 == ptBox.nRow && ptCell.nCol == ptBox.nCol)
				nRet |= BOX_TOP;
			else if (ptCell.nRow - 1 == ptBox.nRow && ptCell.nCol == ptBox.nCol)
				nRet |= BOX_BTM;
			else if (ptCell.nRow == ptBox.nRow && ptCell.nCol+1 == ptBox.nCol)
				nRet |= BOX_LT;
			else if (ptCell.nRow == ptBox.nRow && ptCell.nCol-1 == ptBox.nCol)
				nRet |= BOX_RT;
		}
		return nRet;
	}
	//bcXd...
	//Abcd...
	//expected ret {2,0}
	//Abcd...
	//bcXd...
	//expected ret {0,2}
	pair<int, int> _GetDiffIndexes(const uint8_t* pA1,const uint8_t* pA2, uint16_t nBoxCount) {
		pair<int, int> ret{ -1,-1 };
		for (int nIdx = 0; nIdx < nBoxCount; ++nIdx) {
			if (pA1[nIdx] != pA2[nIdx]) {
				ret.first = nIdx;
				break;
			}
		}
		_ASSERT_RET(ret.first != -1, ret);
		for (int nIdx = nBoxCount; nIdx > 0; --nIdx) {
			if (pA1[nIdx-1] != pA2[nIdx-1]) {
				ret.second = nIdx-1;
				break;
			}
		}
		_ASSERT_RET(ret.second != -1, ret);
		if (ret.second != ret.first && pA1[ret.first] > pA2[ret.first])
			std::swap(ret.first,ret.second);
		return ret;
	}

}

bool CRStages::CanPullUp_(const Stage& stage) const {
	return !m_Sokoban.NotSpace(stage, stage.ptR.nRow - 1, stage.ptR.nCol) &&
				m_Sokoban.HasBox(stage, stage.ptR.nRow + 1, stage.ptR.nCol);
}
bool CRStages::CanPullDown_(const Stage& stage) const {
	return !m_Sokoban.NotSpace(stage, stage.ptR.nRow + 1, stage.ptR.nCol) &&
				m_Sokoban.HasBox(stage, stage.ptR.nRow - 1, stage.ptR.nCol);
}
bool CRStages::CanPullLeft_(const Stage& stage) const {
	return !m_Sokoban.NotSpace(stage, stage.ptR.nRow, stage.ptR.nCol - 1) &&
				m_Sokoban.HasBox(stage, stage.ptR.nRow, stage.ptR.nCol + 1);
}
bool CRStages::CanPullRight_(const Stage& stage) const {
	return !m_Sokoban.NotSpace(stage, stage.ptR.nRow, stage.ptR.nCol + 1) &&
				m_Sokoban.HasBox(stage, stage.ptR.nRow, stage.ptR.nCol - 1);
}
Stage CRStages::PullUp_(const Stage& stage) const {
	Stage ret = stage; 
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol, true, ret);
	m_Sokoban.SetBox(ret.ptR.nRow + 1, ret.ptR.nCol, false, ret);
	--ret.ptR.nRow;
	return ret;
}
Stage CRStages::PullDown_(const Stage& stage) const {
	Stage ret = stage;
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol, true, ret);
	m_Sokoban.SetBox(ret.ptR.nRow - 1, ret.ptR.nCol, false, ret);
	++ret.ptR.nRow;
	return ret;
}
Stage CRStages::PullLeft_(const Stage& stage) const {
	Stage ret = stage;
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol, true, ret);
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol + 1, false, ret);
	--ret.ptR.nCol;
	return ret;
}
Stage CRStages::PullRight_(const Stage& stage) const {
	Stage ret = stage;
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol, true, ret);
	m_Sokoban.SetBox(ret.ptR.nRow, ret.ptR.nCol - 1, false, ret);
	++ret.ptR.nCol;
	return ret;
}

uint16_t CRStages::GetMinDist(const Stage& stage) const {
	uint16_t nRet = 0xFFFF;//inf
	for (const RStageNode& rsn : m_vRSNodes) {
		uint16_t nDist = LBDist_(rsn, stage);
		if (nRet > nDist)
			nRet = nDist;
	}
	return nRet;
}
bool CRStages::HasStage_(const RStageNode& rsnode, const Stage& stage) const {
	if(rsnode.stage.llBoxPos != stage.llBoxPos)
		return false;
	//check robots Corral!
	int64_t llRPos = 1ll << m_Sokoban.CellPos(stage.ptR);
	uint8_t aBoxesPos[16]{ 0 };//0..63
	rsnode.stage.GetBoxesPos(aBoxesPos);
	for (int nBox = 0; nBox < m_Sokoban.BoxCount(); ++nBox) {
		const DistExt& dstex = rsnode.aDistExt[nBox][aBoxesPos[nBox]];
		if (dstex.aCellPos[0] & llRPos) {
			assert(dstex.aPulls[0] == 0);
			return true;
		}
		if (dstex.aCellPos[1] & llRPos) { //could be in the 2nd corral!
			//assert(dstex.aPulls[1] == 0);
			return true;
		}
		if (dstex.aCellPos[2] & llRPos) { //could be in the 3rd corral!
			//assert(dstex.aPulls[2] == 0);
			return true;
		}
	}
	return false;
}
bool CRStages::CompletePath(IN OUT Stage& current, IN OUT IStageQueue* pSQClosed) const {
	//1. find RStageNode with our closed Stage
	for (const RStageNode& rsn : m_vRSNodes) {
		if (HasStage_(rsn, current)) {
			//we have found the point/Stage on the radius - lets build the path to center
			pSQClosed->Push(current);
			Stage rsnStage = rsn.stage, temp;
			uint8_t aS1BoxPos[MAX_BOXES]{ 0 };
			uint8_t aS2BoxPos[MAX_BOXES]{ 0 };
			do {
				rsnStage.GetBoxesPos(aS1BoxPos);
				rsnStage = m_vClosedStages[rsnStage.nPIdx - 1]; //rsn parent
				temp = rsnStage; temp.nPIdx = pSQClosed->Size();//1 based!
				//to fix player / robot position of the RStage,
				//find box in prev stage which is not in temp and place robot there
				temp.GetBoxesPos(aS2BoxPos);
				pair<uint8_t, uint8_t> pairBoxes = _GetDiffIndexes(aS1BoxPos, aS2BoxPos, m_Sokoban.BoxCount());
				temp.ptR = m_Sokoban.CellPoint(aS1BoxPos[pairBoxes.first]);
				Point ptBox = m_Sokoban.CellPoint(aS2BoxPos[pairBoxes.second]);
				_ASSERT_RET(temp.ptR.nRow == ptBox.nRow || temp.ptR.nCol == ptBox.nCol, false);
				_ASSERT_RET(temp.ptR.nRow != ptBox.nRow || temp.ptR.nCol != ptBox.nCol, false);
				//fix tunnel macro-moves
				if (temp.ptR.nCol == ptBox.nCol)
					temp.ptR.nRow = temp.ptR.nRow > ptBox.nRow ? (ptBox.nRow + 1) : (ptBox.nRow - 1);
				else
					temp.ptR.nCol = temp.ptR.nCol > ptBox.nCol ? (ptBox.nCol + 1) : (ptBox.nCol - 1);
				pSQClosed->Push(temp);
			} while (rsnStage.nPIdx);
			current = temp;
			return true;
		}
	}
	assert(0);
	return false;//snbh?
}
//following ideas from https://www.researchgate.net/publication/2305703_Pushing_the_Limits_New_Developments_in_Single-Agent_Search . 4.3. Lower Bound Heuristic
uint16_t CRStages::LBDist_(const RStageNode& rsnode, const Stage& stage) const {
	uint8_t aDisMatrix[MAX_DIM][MAX_DIM]; //dist matrix
	std::memset(aDisMatrix, 0xFF, MAX_DIM * MAX_DIM);
	//
	const uint16_t nBoxes = m_Sokoban.BoxCount();
	const int64_t llRPos = 1ll << m_Sokoban.CellPos(stage.ptR);
	uint8_t aStgBoxesPos[MAX_BOXES]{ 0 };//0..MAX_SPACES-1
	stage.GetBoxesPos(aStgBoxesPos);

	for (uint8_t nRStgBox = 0; nRStgBox < nBoxes; ++nRStgBox) {
		bool bHasPath = false;
		for (uint8_t nStgBox = 0; nStgBox < nBoxes; ++nStgBox) {
			uint8_t nStgBoxesPos = aStgBoxesPos[nStgBox];
			const DistExt& dstex = rsnode.aDistExt[nRStgBox][nStgBoxesPos];
			for (int nCIdx = 0; nCIdx < _countof(dstex.aCellPos); ++nCIdx) {
				if (dstex.aCellPos[nCIdx] & llRPos) {
					aDisMatrix[nRStgBox][nStgBox] = dstex.aPulls[nCIdx];
					bHasPath = true;
					break;
				}
			}
		}
		if (!bHasPath)
			return 0xFFFF;
	}
	for (uint8_t nStgBox = 0; nStgBox < nBoxes; ++nStgBox) {
		bool bHasPath = false;
		for (uint8_t nRStgBox = 0; nRStgBox < nBoxes; ++nRStgBox) {
			if (aDisMatrix[nRStgBox][nStgBox] < 0xFF) {
				bHasPath = true;
				break;
			}
		}
		if (!bHasPath)
			return 0xFFFF;
	}

	return Hungarian::Min1616(nBoxes, nBoxes, aDisMatrix);
}

bool CRStages::Init(uint16_t nDepth) {
	m_vClosedStages.clear();
	m_vRSNodes.clear();
	//1. BFS for reverse agent
	Stage temp; temp.llBoxPos = m_Sokoban.FinalBoxPos();
	deque<Stage> queueStages; //BFS
  vector<Point> vCells;     //reachable cells
	//PROLOG
	//we need to add all root Stages with correct player/robot from all corrals/rooms of the final pos!
	bitset<MAX_DIM * MAX_DIM> btsCells; //unattended free cells
	bool bValid = false;
	for (uint8_t nRow = 0; nRow < m_Sokoban.Dim().nRows; ++nRow) {
		for (uint8_t nCol = 0; nCol < m_Sokoban.Dim().nCols; ++nCol) {
			if (btsCells.test(nRow * m_Sokoban.Dim().nCols + nCol) || m_Sokoban.NotSpace(temp, nRow, nCol))
				continue;
			temp.ptR = { nRow , nCol};
			m_Sokoban.GetReachableCells(temp, vCells);
			for (Point ptCell : vCells) {
				btsCells.set(ptCell.nRow * m_Sokoban.Dim().nCols + ptCell.nCol);
				if (!bValid) {
					temp.ptR = ptCell;
					bValid = (CanPullUp_(temp) || CanPullLeft_(temp) || CanPullDown_(temp) || CanPullRight_(temp));
				}
			}
			if (bValid) {
				queueStages.push_back(temp);
				bValid = false;
			}
		}
	}
  _ASSERT_RET(!queueStages.empty(),false);//unreachable final pos!

	Stage current;
	//MAIN LOOP
	while (queueStages.size()) {
		current = queueStages.front(); queueStages.pop_front();//BFS wants FIFO
		if (_Depth(current, m_vClosedStages) > nDepth)
			break;//done
		m_vClosedStages.push_back(current);
		uint32_t nPIdx = (uint32_t)m_vClosedStages.size();//1 based!
		//recalc reachable cells
		m_Sokoban.GetReachableCells(current, vCells);
		//m_Sokoban.Display(current); //DEBUG
		for (Point ptR : vCells) {
			//UP
			temp = current; temp.ptR = ptR;
			if (CanPullUp_(temp)) {
				do {//Tunneling/Macromove!
					temp = PullUp_(temp);
				} while (IsPullTunnelUP_(temp) && CanPullUp_(temp));
				_CheckAddStage(temp, queueStages, m_vClosedStages, nPIdx);
				temp = current; temp.ptR = ptR; //restore temp stage
			}
			//LEFT
			if (CanPullLeft_(temp)) {
				do {
					temp = PullLeft_(temp);
				} while (IsPullTunnelLT_(temp) && CanPullLeft_(temp));
				_CheckAddStage(temp, queueStages, m_vClosedStages, nPIdx);
				temp = current; temp.ptR = ptR; //restore temp stage
			}
			//DN
			if (CanPullDown_(temp)) {
				do {
					temp = PullDown_(temp);
				} while (IsPullTunnelDN_(temp) && CanPullDown_(temp));
				_CheckAddStage(temp, queueStages, m_vClosedStages, nPIdx);
				temp = current; temp.ptR = ptR; //restore temp stage
			}
			//RIGHT
			if (CanPullRight_(temp)) {
				do {
					temp = PullRight_(temp);
				} while (IsPullTunnelRT_(temp) && CanPullRight_(temp));
				_CheckAddStage(temp, queueStages, m_vClosedStages, nPIdx);
			}
		}
	}//while
	//2. Calc Corral0+Dist map for each RSNode
	for (auto itrS = m_vClosedStages.rbegin(); itrS != m_vClosedStages.rend(); ++itrS) {
		const Stage& stage = *itrS;
		if (_Depth(stage, m_vClosedStages) < nDepth)
			break;
		m_vRSNodes.emplace_back(stage);
		InitRSNode_(m_vRSNodes.back());
	}
	return true;
}
void CRStages::InitRSNode_(IN OUT RStageNode& rsnode) const {
	uint8_t aBoxesPos[MAX_BOXES]{ 0 };//0..63/0 based!
	rsnode.stage.GetBoxesPos(aBoxesPos);
	const uint16_t nBoxes = m_Sokoban.BoxCount();
	for (uint16_t nBox = 0; nBox < nBoxes; ++nBox) {
		Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
		assert(m_Sokoban.HasBox(rsnode.stage, ptBox.nRow, ptBox.nCol));
		CalcBoxDistEx_(rsnode.stage, ptBox, rsnode.aDistExt[nBox]);
	}
	//2nd pass:
	// Testing showed, that measuring the distance from the specific box to any cell while fixing all other boxes is TOO restrictive :
	// most of the stages would have an INF distance to a given one. 
	// After measuring the "strict distances" we "re-measure" the distances to the unreachable cells with a small penalty by "removing" one reachable+movable box.
	// Understandebly, this is not a strict "Lower-Bound" pull-distance from the free cell to a box, but something heuristically close to a real one...
	// PROBLEM: Non-DeadCorral DL: Boxes C2, E2 are fixed unless C1 or E1 are REMOVED, which should be prohibited as B1/D1 are reachable only by C1/E1 so 
	// LBDist_ must return INF!
	//  ABCDE
	// ########  
	//1# $.$. # 
	//2#  . * # 
	//3## #$# #
	//........
	// Deadlock (non-Corral!) from Sasquatch_iii_12.
	//
	//
	if (m_Sokoban.Cfg().nRSM_GBRelax == 1) { //2 is not impl: todo!
		for (uint8_t nBox = 0; nBox < nBoxes; ++nBox) {
			Point ptBox = m_Sokoban.CellPoint(aBoxesPos[nBox]);
			for (uint8_t nRBox = 0; nRBox < nBoxes; ++nRBox) {
				if (nRBox == nBox || !IsBoxPushable_(rsnode, nRBox, aBoxesPos[nRBox]))
					continue; //not directly removable...
				Stage temp = rsnode.stage;
				//assert(temp.llBoxPos & (1ll << aBoxesPos[nRBox]));
				temp.llBoxPos ^= (1ll << aBoxesPos[nRBox]); //remove box
				//update distances for unreachable cells (ONLY)
				DistExt aDistExt[64];
				CalcBoxDistEx_(temp, ptBox, aDistExt);
				for (uint8_t nCell = 0; nCell < m_Sokoban.FreeCells(); ++nCell) {
					DistExt& dex = rsnode.aDistExt[nBox][nCell];
					if (!dex.aCellPos[0] && aDistExt[nCell].aCellPos[0]) {
						dex.aCellPos[0] = aDistExt[nCell].aCellPos[0];
						dex.aPulls[0] = aDistExt[nCell].aPulls[0] + 4;//penalty
					}
					if (!dex.aCellPos[1] && aDistExt[nCell].aCellPos[1]) {
						dex.aCellPos[1] = aDistExt[nCell].aCellPos[1];
						dex.aPulls[1] = aDistExt[nCell].aPulls[1] + 4;//penalty
					}
					if (!dex.aCellPos[2] && aDistExt[nCell].aCellPos[2]) {
						dex.aCellPos[2] = aDistExt[nCell].aCellPos[2];
						dex.aPulls[2] = aDistExt[nCell].aPulls[2] + 4;//penalty
					}
				}
			}
		}
	}
}
bool CRStages::IsBoxPushable_(const RStageNode& rsnode, uint8_t nRBox, uint8_t nBoxPos) const {
	//as distances are already measured - check if cells around has distances
	const Point ptBox = m_Sokoban.CellPoint(nBoxPos);
	Point aPtUDLR[4] = {{uint8_t(ptBox.nRow - 1), ptBox.nCol}, {uint8_t(ptBox.nRow + 1), ptBox.nCol}, 
											{ptBox.nRow, uint8_t(ptBox.nCol - 1)}, {ptBox.nRow, uint8_t(ptBox.nCol + 1)} };
	for (Point ptCell : aPtUDLR) {
		if (m_Sokoban.IsSpace(ptCell.nRow, ptCell.nCol)) {
			int8_t nCell = m_Sokoban.CellPos(ptCell);
			if (rsnode.aDistExt[nRBox][nCell].aCellPos[0])
				return true;
		}
	}
	return false;
}
void CRStages::CalcBoxDistEx_(const Stage& stage, Point ptBox0, OUT DistExt (&aDistExt)[MAX_SPACES]) const {
	queue<Stage> queueStages;//BFS
	Stage current = stage, temp;
	vector<Point> vCells;
	current.nWeight = (ptBox0.nRow << 8) | ptBox0.nCol;	//use weight to encode PtBox
	current.nPIdx = 0;																	//use nPIdx for Depth
	queueStages.push(current);

	while (!queueStages.empty()) {
		current = queueStages.front(); queueStages.pop();
		m_Sokoban.GetReachableCells(current, vCells);
		if (vCells.size() < MIN_RC0_SPACE) //minimal non-pull deadlock space
			continue;
		//get our box
		Point ptBox = { uint8_t(current.nWeight >> 8), uint8_t(current.nWeight & 0xFF) };
		const int nBoxEdges = _BoxEdges(ptBox, vCells);
		if (!nBoxEdges && !(ptBox0 == ptBox)) //box is unreachable!
			continue;
		uint8_t nBoxPos = m_Sokoban.CellPos(ptBox); //<MAX_SPACES
		int64_t llCellPos = m_Sokoban.CellsPos(vCells);
		uint32_t nIdx = 0;
		bool bAttended = false;
		while (nIdx < _countof(aDistExt->aCellPos)) {
			if (aDistExt[nBoxPos].aCellPos[nIdx]) { //already attended in this corral?
				if (aDistExt[nBoxPos].aCellPos[nIdx] == llCellPos) {
					bAttended = true; //been there...
					break;
				}
			}
			else
				break;//new place
			++nIdx;
		}//while nIdx
		if (bAttended)
			continue;
		if (nIdx == _countof(aDistExt[0].aCellPos)) {
			m_Sokoban.Reporter().PC("Unexpected 4th corral when pulling box from ").P(ptBox0).PC(" to ").P(ptBox).PEol();
			m_Sokoban.Display(stage);
			assert(0);
			abort();
		}
		aDistExt[nBoxPos].aPulls[nIdx] = current.nPIdx;
		aDistExt[nBoxPos].aCellPos[nIdx] = llCellPos;
		if (nBoxEdges & BOX_TOP) {
			temp = current; temp.ptR = { uint8_t(ptBox.nRow-1), ptBox.nCol };
			if (CanPullUp_(temp)) {
				do {//Tunneling/Macromove!
					temp = PullUp_(temp);
				} while (IsPullTunnelUP_(temp) && CanPullUp_(temp));
				++temp.nPIdx;	//inc distance BY ONE even for macromoves
				temp.nWeight = ((temp.ptR.nRow + 1) << 8) | ptBox.nCol;
				queueStages.push(temp);
			}
		}
		if (nBoxEdges & BOX_LT) {
			temp = current; temp.ptR = { ptBox.nRow, uint8_t(ptBox.nCol-1) };
			if (CanPullLeft_(temp)) {
				do {
					temp = PullLeft_(temp);
				} while (IsPullTunnelLT_(temp) && CanPullLeft_(temp));
				++temp.nPIdx;
				temp.nWeight = (ptBox.nRow << 8) | (temp.ptR.nCol + 1);
				queueStages.push(temp);
			}
		}
		if (nBoxEdges & BOX_BTM) {
			temp = current; temp.ptR = { uint8_t(ptBox.nRow + 1), ptBox.nCol };
			if (CanPullDown_(temp)) {
				do {
					temp = PullDown_(temp);
				} while (IsPullTunnelDN_(temp) && CanPullDown_(temp));
				++temp.nPIdx;
				temp.nWeight = ((temp.ptR.nRow - 1) << 8) | ptBox.nCol;
				queueStages.push(temp);
			}
		}
		if (nBoxEdges & BOX_RT) {
			temp = current; temp.ptR = { ptBox.nRow, uint8_t(ptBox.nCol + 1) };
			if (CanPullRight_(temp)) {
				do {
					temp = PullRight_(temp);
				} while (IsPullTunnelRT_(temp) && CanPullRight_(temp));
				++temp.nPIdx;
				temp.nWeight = (ptBox.nRow << 8) | (temp.ptR.nCol - 1);
				queueStages.push(temp);
			}
		}
	}//while
}

bool CRStages::IsPullTunnelUP_(const Stage& stage) const {
	Point ptBox{ uint8_t(stage.ptR.nRow + 1), stage.ptR.nCol };
	Point ptBoxOld{ uint8_t(stage.ptR.nRow + 2), stage.ptR.nCol };
	return m_Sokoban.IsWall(ptBox.nRow, ptBox.nCol - 1) && m_Sokoban.IsWall(ptBox.nRow, ptBox.nCol + 1) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol - 1) || m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol - 2)) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol + 1) || m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol + 2));
}
bool CRStages::IsPullTunnelDN_(const Stage& stage) const {
	Point ptBox{ uint8_t(stage.ptR.nRow - 1), stage.ptR.nCol };
	Point ptBoxOld{ uint8_t(stage.ptR.nRow - 2), stage.ptR.nCol };
	return m_Sokoban.IsWall(ptBox.nRow, ptBox.nCol - 1) && m_Sokoban.IsWall(ptBox.nRow, ptBox.nCol + 1) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol - 1) || m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol - 2)) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol + 1) || m_Sokoban.IsWall(ptBoxOld.nRow, ptBoxOld.nCol + 2));
}
bool CRStages::IsPullTunnelLT_(const Stage& stage) const {
	Point ptBox{ stage.ptR.nRow, uint8_t(stage.ptR.nCol + 1) };
	Point ptBoxOld{ stage.ptR.nRow, uint8_t(stage.ptR.nCol + 2)};
	bool bRet = m_Sokoban.IsWall(ptBox.nRow - 1, ptBox.nCol) && m_Sokoban.IsWall(ptBox.nRow + 1, ptBox.nCol) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow - 1, ptBoxOld.nCol) || m_Sokoban.IsWall(ptBoxOld.nRow - 2, ptBoxOld.nCol)) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow + 1, ptBoxOld.nCol) || m_Sokoban.IsWall(ptBoxOld.nRow + 2, ptBoxOld.nCol));
	return bRet;
}
bool CRStages::IsPullTunnelRT_(const Stage& stage) const {
	Point ptBox{ stage.ptR.nRow, uint8_t(stage.ptR.nCol - 1) };
	Point ptBoxOld{ stage.ptR.nRow, uint8_t(stage.ptR.nCol - 2) };
	return m_Sokoban.IsWall(ptBox.nRow - 1, ptBox.nCol) && m_Sokoban.IsWall(ptBox.nRow + 1, ptBox.nCol) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow - 1, ptBoxOld.nCol) || m_Sokoban.IsWall(ptBoxOld.nRow - 2, ptBoxOld.nCol)) &&
		(m_Sokoban.IsWall(ptBoxOld.nRow + 1, ptBoxOld.nCol) || m_Sokoban.IsWall(ptBoxOld.nRow + 2, ptBoxOld.nCol));
}
//special variations for DLMgr
int64_t CRStages::GetRBoxCells(int64_t llBoxes, IN OUT SBoxInfo(&aSBI)[FIXEDPC], bool bMinPulls) const {
	const int64_t llPullBox = 1ll << aSBI[0].nBoxPos;
	assert(llBoxes & llPullBox);
	int64_t llBoxRCells = 0;

	uint8_t aBEdges[MAX_SPACES]; memset(aBEdges, 0, sizeof(aBEdges)); //attended positions tracker
	if (!aSBI[0].nRPos) {
		//we have to try all possible R positions and return the combined attended cells+SBoxInfo
		Point ptBox0 = m_Sokoban.CellPoint(aSBI[0].nBoxPos);
		//put R near the the ptBox and pull it around/to every possible cell
		const Point aBoxPts[4]{ {uint8_t(ptBox0.nRow - 1),ptBox0.nCol},{uint8_t(ptBox0.nRow + 1),ptBox0.nCol},
												{ptBox0.nRow,uint8_t(ptBox0.nCol - 1)},{ptBox0.nRow,uint8_t(ptBox0.nCol + 1)} };
		vector<SBoxInfo> vSBInfo;
		for (Point ptR : aBoxPts) {
			Stage temp; temp.llBoxPos = llBoxes;
			if (m_Sokoban.NotSpace(temp, ptR.nRow, ptR.nCol))
				continue;
			aSBI[0].nRPos = m_Sokoban.CellPos(ptR) + 1;//1 based
			int64_t llBRCells = GetRBoxCells_(llBoxes, aSBI, aBEdges, bMinPulls);
			uint8_t nPC = _Popcnt64(llBRCells);
			if (nPC == 1)
				continue; //no pulls
			if (nPC > 4) //box is not fixed
				vSBInfo.resize(FIXEDPC);//stop filling it
			else if(vSBInfo.size() < FIXEDPC) {
				for (SBoxInfo& sbi : aSBI) {
					if (!sbi.nRPos)
						break;
					vSBInfo.push_back({ sbi.nRPos, sbi.nBoxPos});
					if (vSBInfo.size() >= FIXEDPC)
						break;
				}
			}
			llBoxRCells |= llBRCells;
			if (bMinPulls && vSBInfo.size() >= FIXEDPC)
				break; //abort search
		}//for
		memcpy(aSBI, vSBInfo.data(), sizeof(SBoxInfo) * min(FIXEDPC, (int)vSBInfo.size()));
	}
	//else, we have R position=>Box pos also
	else
		llBoxRCells = GetRBoxCells_(llBoxes, aSBI, aBEdges, bMinPulls);

	return llBoxRCells;
}

int64_t CRStages::GetRBoxCells_(int64_t llBoxes, IN OUT SBoxInfo(&aSBI)[FIXEDPC], IN OUT uint8_t(&aBEdges)[MAX_SPACES], bool bMinPulls) const {
	assert(aSBI[0].nRPos);
	const int64_t llPullBox = 1ll << aSBI[0].nBoxPos;
	assert(llBoxes & llPullBox);
	aSBI[1] = aSBI[2] = aSBI[3] = aSBI[4] = { 0,0 };

	//Reverse Stage for FGs calcs
	struct RStageFG {
		uint8_t		nLastBoxPos{ 0 };				//1-based cell with the last pulled box
		Point			ptR;
		int64_t		llBoxes{ 0 };					//boxes
	};
	RStageFG rstgCurrent{ aSBI[0].nBoxPos, m_Sokoban.CellPoint(aSBI[0].nRPos-1), llBoxes };
	Point ptBox0 = m_Sokoban.CellPoint(rstgCurrent.nLastBoxPos);
	int64_t llRBoxCells = llPullBox; //initial pos
	vector<Point> vC0Pts;
	queue<RStageFG> queueRStages;//BFS
	queueRStages.push(rstgCurrent);
	Stage current, temp;
	int nStep = 0;
	while (!queueRStages.empty()) {
		rstgCurrent = queueRStages.front(); queueRStages.pop();
		current.llBoxPos = rstgCurrent.llBoxes;
		current.ptR = rstgCurrent.ptR;
		m_Sokoban.GetReachableCells(current, vC0Pts);
		if (vC0Pts.size() < MIN_RC0_SPACE) //minimal cells in the non-pull deadlock corral
			continue; //TODO: check if it is q pull-DL when vC0Pts.size() >= MIN_RC0_SPACE!
		//get our box
		Point ptBox = m_Sokoban.CellPoint(rstgCurrent.nLastBoxPos);
		int nBoxEdges = _BoxEdges(ptBox, vC0Pts);
		if (!nBoxEdges) { //at leat 1 edge of the box is reachable IF pull it here
			assert(nStep == 0); //otherwise it could be ptBox0 blocked from all sides!
			return llPullBox;
		}
		//check if we already been here
		if (aBEdges[rstgCurrent.nLastBoxPos] & nBoxEdges)
			continue;//already attended
		aBEdges[rstgCurrent.nLastBoxPos] |= nBoxEdges;//mark as attended
		if (nStep && nStep < FIXEDPC) {
			aSBI[nStep].nRPos = m_Sokoban.CellPos(rstgCurrent.ptR) + 1;//1 based
			aSBI[nStep].nBoxPos = rstgCurrent.nLastBoxPos;
		}
		++nStep;
		llRBoxCells |= 1ll << rstgCurrent.nLastBoxPos;
		if (bMinPulls && nStep >= FIXEDPC)
			break; //abort search
		if (nBoxEdges & BOX_TOP) {
			temp = current; temp.ptR = { uint8_t(ptBox.nRow - 1), ptBox.nCol };
			rstgCurrent.nLastBoxPos = m_Sokoban.CellPos(temp.ptR);
			if (CanPullUp_(temp)) {
				temp = PullUp_(temp);
				rstgCurrent.ptR = temp.ptR;
				rstgCurrent.llBoxes = temp.llBoxPos;
				queueRStages.push(rstgCurrent);
			}
		}
		if (nBoxEdges & BOX_LT) {
			temp = current; temp.ptR = { ptBox.nRow, uint8_t(ptBox.nCol - 1) };
			rstgCurrent.nLastBoxPos = m_Sokoban.CellPos(temp.ptR);
			if (CanPullLeft_(temp)) {
				temp = PullLeft_(temp);
				rstgCurrent.ptR = temp.ptR;
				rstgCurrent.llBoxes = temp.llBoxPos;
				queueRStages.push(rstgCurrent);
			}
		}
		if (nBoxEdges & BOX_BTM) {
			temp = current; temp.ptR = { uint8_t(ptBox.nRow + 1), ptBox.nCol };
			rstgCurrent.nLastBoxPos = m_Sokoban.CellPos(temp.ptR);
			if (CanPullDown_(temp)) {
				temp = PullDown_(temp);
				rstgCurrent.ptR = temp.ptR;
				rstgCurrent.llBoxes = temp.llBoxPos;
				queueRStages.push(rstgCurrent);
			}
		}
		if (nBoxEdges & BOX_RT) {
			temp = current; temp.ptR = { ptBox.nRow, uint8_t(ptBox.nCol + 1) };
			rstgCurrent.nLastBoxPos = m_Sokoban.CellPos(temp.ptR);
			if (CanPullRight_(temp)) {
				temp = PullRight_(temp);
				rstgCurrent.ptR = temp.ptR;
				rstgCurrent.llBoxes = temp.llBoxPos;
				queueRStages.push(rstgCurrent);
			}
		}
	}//while
	return llRBoxCells;
}


