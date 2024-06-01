#include "StdAfx.h"
#include "DeadLockMgr.h"
#include "ReverseStages.h"
#include "Reporter.h"

class SQVec;
class Sokoban {//field+....
//DATA
	SokoCfg							m_Cfg;														//Config
	mutable CReporter		m_Reporter;
	//Board info
	uint16_t						m_nBoxes{ 0 };										//=Storages
	uint16_t						m_nSpaces{ 0 };										//64 MAX!
	Dimens							m_dim{ 0,0 };											//board dimensions
	int64_t							m_llStgPos{ 0 };
	Stage								m_stInit;													//initial stage
	Point								m_aSpaces[MAX_SPACES];						//precalc map bitpos to free cells
	uint8_t							m_aField[MAX_DIM][MAX_DIM]{ 0 };	//0-brick, 1-space, 2-goal
	uint8_t							m_aBitPos[MAX_DIM][MAX_DIM]{ 0 };	//precalc space/cell bitpos, 1 based; 1 for space,0 for walls
	//Pre-search analysis
	//uint16_t			m_aDeadHWalls[MAX_DIM][MAX_DIM]{ 0 };	//precalc Horiz Dead Walls packed Left/Right; 0 if not a dead wall;
	//uint16_t			m_aDeadVWalls[MAX_DIM][MAX_DIM]{ 0 };	//precalc Vert Dead Walls packed Top/Down; 0 if not a dead wall;
	bitset<MAX_SPACES>	m_btsDeadPos;											//static dead pos: corners + dead walls w/o goals;
	bitset<MAX_SPACES>	m_btsTnlPosUP;										//Tunnel pos on push UP
	bitset<MAX_SPACES>	m_btsTnlPosDN;										//Tunnel pos on push DOWN
	bitset<MAX_SPACES>	m_btsTnlPosLT;										//Tunnel pos on push LEFT
	bitset<MAX_SPACES>	m_btsTnlPosRT;										//Tunnel pos on push RIGHT
	CDLMgr							m_DLM;														//DeadlockMgr
	CRStages						m_RSM;														//Reverse stages
	vector<Storage>			m_vStg;														//TODO:sort/anaylize orders
	SQVec*							m_pClosedStgs{nullptr};						//temp member to provide node's Depth check for DFS


public:
//CTOR/DTOR
	Sokoban();
	~Sokoban();
//ATTS
	const SokoCfg& Cfg() const { return m_Cfg; }
	CReporter& Reporter() const { return m_Reporter; }
	const Dimens& Dim() const { return m_dim; }
	uint16_t BoxCount() const { return m_nBoxes; }
	uint16_t FreeCells() const { return m_nSpaces; }
	const Stage& InitStage() const { return m_stInit; }
	int64_t FinalBoxPos() const { return m_llStgPos; }

	bool IsWall(int nRow, int nCol) const {
		if (nRow <= 0 || nRow >= m_dim.nRows || nCol <= 0 || nCol >= m_dim.nCols)
			return true;
		return m_aField[nRow][nCol] == 0 || m_aField[nRow][nCol] == 4;//4-imposed wall!
	}
	bool IsSpace(int nRow, int nCol) const {
		return !IsWall(nRow, nCol);
	}
	bool IsCorner(int nRow, int nCol) const {
		return IsSpace(nRow, nCol) &&
			(IsWall(nRow, nCol - 1) || IsWall(nRow, nCol + 1)) &&
			(IsWall(nRow - 1, nCol) || IsWall(nRow + 1, nCol));
	}

	bool IsStorage(int nRow, int nCol) const {
		return m_aField[nRow][nCol] == 2;
	}
	uint8_t CellPos(Point ptCell) const { //0=based!
		uint8_t nBitPos = m_aBitPos[ptCell.nRow][ptCell.nCol];//1 based!
		assert(nBitPos);
		return --nBitPos;
	}
	Point CellPoint(uint8_t nBitPos) const {
		return m_aSpaces[nBitPos];
	}
	int64_t CellsPos(const vector<Point>& vCells) const {
		int64_t llRet = 0;
		for (Point ptCell : vCells) {
			llRet |= (1ll << CellPos(ptCell));
		}
		return llRet;
	}
	void Pos2Points(int64_t llCells, OUT vector<Point>& vCells) const {
		vCells.clear();
		int8_t nBitPos = 0; //0-based
		while (llCells) {
			if (llCells & 1)
				vCells.push_back(m_aSpaces[nBitPos]);
			++nBitPos;
			llCells >>= 1;
		}
	}
	void Pos2Bits(int64_t llCells, OUT vector<uint8_t>& vBits) const {
		vBits.clear();
		int8_t nBitPos = 0; //0-based
		while (llCells) {
			if (llCells & 1)
				vBits.push_back(nBitPos);
			++nBitPos;
			llCells >>= 1;
		}
	}
	bool HasBox(const Stage& stage, int nRow, int nCol) const {
		int8_t nBitPos = m_aBitPos[nRow][nCol];//1 based!
		return (nBitPos == 0 ? 0 : (stage.llBoxPos & (1ll << (nBitPos - 1))) != 0);
	}
	void SetBox(int nRow, int nCol, bool bSet, IN OUT Stage& stage) const {
		int8_t nBitPos = m_aBitPos[nRow][nCol];//1 based!
		if (!nBitPos) { //snbh!
			assert(0);
			abort();
		}
		 if(bSet) 
			 stage.llBoxPos |= 1ll << (nBitPos - 1);
		 else
			 stage.llBoxPos &= ~(1ll << (nBitPos - 1));
	}
	bool NotSpace(const Stage& stage, int nRow, int nCol) const {
		return IsWall(nRow, nCol) || HasBox(stage, nRow, nCol);
	}
	const Storage& Goal(int nIdx) const {
		return m_vStg[nIdx];
	}

	bool IsDeadPos(int nRow, int nCol) const {
		return m_btsDeadPos.test(m_aBitPos[nRow][nCol] - 1);
	}
	void SetDeadPos(int nRow, int nCol) {
		assert(IsSpace(nRow, nCol));
		m_btsDeadPos.set(m_aBitPos[nRow][nCol] - 1);
	}
	bool IsTunnelPos_UP(int nRow, int nCol) const {
		return m_btsTnlPosUP.test(m_aBitPos[nRow][nCol] - 1);
	}
	bool IsTunnelPos_DN(int nRow, int nCol) const {
		return m_btsTnlPosDN.test(m_aBitPos[nRow][nCol] - 1);
	}
	bool IsTunnelPos_LT(int nRow, int nCol) const {
		return m_btsTnlPosLT.test(m_aBitPos[nRow][nCol] - 1);
	}
	bool IsTunnelPos_RT(int nRow, int nCol) const {
		return m_btsTnlPosRT.test(m_aBitPos[nRow][nCol] - 1);
	}
	char GetCode(const Stage& stage, int nRow, int nCol) const;
	//2bits, 0-3: space=0,wall=1,box=2,box-in-goal=3; space+goal is 0 OR treated separately!
	int8_t GetBits2(const Stage& stage, int nRow, int nCol) const {
		if(IsWall(nRow, nCol))
			return 1;
		if (HasBox(stage, nRow, nCol))
			return IsStorage(nRow, nCol) ? 3 : 2;
		return 0;
	}

//ROBOT MOVINGs
	[[nodiscard]] bool CanWalkUp(const Stage& stage) const { 
		return stage.ptR.nRow > 1 && IsSpace(stage.ptR.nRow-1, stage.ptR.nCol) && !HasBox(stage, stage.ptR.nRow - 1, stage.ptR.nCol);
	}
	[[nodiscard]] bool CanWalkDown(const Stage& stage) const { 
		return stage.ptR.nRow < m_dim.nRows-2 && IsSpace(stage.ptR.nRow+1, stage.ptR.nCol) && !HasBox(stage, stage.ptR.nRow + 1, stage.ptR.nCol);
	}
	[[nodiscard]] bool CanWalkLeft(const Stage& stage) const {
		return stage.ptR.nCol > 1 && IsSpace(stage.ptR.nRow, stage.ptR.nCol - 1) && !HasBox(stage, stage.ptR.nRow, stage.ptR.nCol - 1);
	}
	[[nodiscard]] bool CanWalkRight(const Stage& stage) const {
		return stage.ptR.nCol < m_dim.nCols - 2 && IsSpace(stage.ptR.nRow, stage.ptR.nCol+1) && !HasBox(stage, stage.ptR.nRow, stage.ptR.nCol+1);
	}

	[[nodiscard]] bool CanPushUp(const Stage& stage) const {
		return stage.ptR.nRow > 1 && IsSpace(stage.ptR.nRow - 2, stage.ptR.nCol) &&
			HasBox(stage, stage.ptR.nRow - 1, stage.ptR.nCol) && !HasBox(stage, stage.ptR.nRow - 2, stage.ptR.nCol) &&
			!IsDeadPos(stage.ptR.nRow - 2, stage.ptR.nCol);
	}
	[[nodiscard]] bool CanPushDown(const Stage& stage) const {
		return IsSpace(stage.ptR.nRow + 2, stage.ptR.nCol) &&
			HasBox(stage, stage.ptR.nRow + 1, stage.ptR.nCol) && !HasBox(stage, stage.ptR.nRow + 2, stage.ptR.nCol) &&
			!IsDeadPos(stage.ptR.nRow + 2, stage.ptR.nCol);
	}
	[[nodiscard]] bool CanPushLeft(const Stage& stage) const {
		return stage.ptR.nCol > 1 && IsSpace(stage.ptR.nRow, stage.ptR.nCol - 2) &&
			HasBox(stage, stage.ptR.nRow, stage.ptR.nCol -1) && !HasBox(stage, stage.ptR.nRow, stage.ptR.nCol - 2) &&
			!IsDeadPos(stage.ptR.nRow, stage.ptR.nCol-2);
	}
	[[nodiscard]] bool CanPushRight(const Stage& stage) const {
		return IsSpace(stage.ptR.nRow, stage.ptR.nCol + 2) &&
			HasBox(stage, stage.ptR.nRow, stage.ptR.nCol + 1) && !HasBox(stage, stage.ptR.nRow, stage.ptR.nCol + 2) &&
			!IsDeadPos(stage.ptR.nRow, stage.ptR.nCol + 2);
	}
	[[nodiscard]] Stage WalkUp(const Stage& stage) const {
		Stage ret = stage; --ret.ptR.nRow; return ret;
	}
	[[nodiscard]] Stage WalkDown(const Stage& stage) const {
		Stage ret = stage; ++ret.ptR.nRow; return ret;
	}
	[[nodiscard]] Stage WalkLeft(const Stage& stage) const {
		Stage ret = stage; --ret.ptR.nCol; return ret;
	}
	[[nodiscard]] Stage WalkRight(const Stage& stage) const {
		Stage ret = stage; ++ret.ptR.nCol; return ret;
	}

	[[nodiscard]] Stage PushUp(const Stage& stage) const {
		Stage ret = stage; --ret.ptR.nRow; 
		SetBox(ret.ptR.nRow, ret.ptR.nCol, false, ret);
		SetBox(ret.ptR.nRow-1, ret.ptR.nCol, true, ret);
		return ret;
	}
	[[nodiscard]] Stage PushDown(const Stage& stage) const {
		Stage ret = stage; ++ret.ptR.nRow;
		SetBox(ret.ptR.nRow, ret.ptR.nCol, false, ret);
		SetBox(ret.ptR.nRow + 1, ret.ptR.nCol, true, ret);
		return ret;
	}
	[[nodiscard]] Stage PushRight(const Stage& stage) const {
		Stage ret = stage; ++ret.ptR.nCol;
		SetBox(ret.ptR.nRow, ret.ptR.nCol, false, ret);
		SetBox(ret.ptR.nRow, ret.ptR.nCol+1, true, ret);
		return ret;
	}
	[[nodiscard]] Stage PushLeft(const Stage& stage) const {
		Stage ret = stage; --ret.ptR.nCol;
		SetBox(ret.ptR.nRow, ret.ptR.nCol, false, ret);
		SetBox(ret.ptR.nRow, ret.ptR.nCol - 1, true, ret);
		return ret;
	}
	//specal addition for CorralMgr
	bool IsFixedDeadLock(const Stage& stage, int8_t nLastPush) const {
		return m_DLM.IsFixedDeadLock(stage, nLastPush);
	}
//METHODS
	bool Run();
	//Reporting helpers
	bool ReportSolution(const Stage& last, PCWSTR wszPath, double dblTime);
	void Display(const Stage& stage) const;
	void DisplayCorral(Point ptR, const Corral& crl); //DEBUG
	//Searching
	bool Search(IStageQueue* pSQ, IN OUT Stage& current);
	uint32_t Depth(const Stage& stage) const;
	void UpdateStageWeight(IN OUT Stage& stage) const;
	uint16_t ParentWeight(const Stage& stage) const;
	void GetReachableCells(const Stage& stage, OUT vector<Point>& vCells) const;
	bool AreFixedGoals(const vector<Point>& vStgPts) const;
	bool CanPushAnyBox_(const Stage& stage) const;
private:
	bool InitCfg_();
	bool Initialize_(PCWSTR wszPuzzlePath);
	void AddDeadCornerPos_();
	void InitTunnelPos_();			//update m_btsTnlPos
	//void InitStgDist_();				//update m_aMDist with distances from stg to any cell in empty field 
	//Pre-move
	//uint16_t MinWalk_(Point pt1, Point pt2) const;
	bool IsDummySpace_(uint8_t nRow, uint8_t nCol, OUT Point& ptFreeCell) const;
};