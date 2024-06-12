#pragma once
#include "FixedGoals.h"

class Sokoban;
struct Stage;
///////////////////DeadLock Types, addressing "Hidden pattern" problem ////////////////////
// 1. Static Locked Chain(SLC): chain locked of boxes, w at least one nonG box, nothing can be pushed OR pushing any box leads to another SLC
// 1a.(todo?) Dynamic LC(DLC) - chain locked of boxes, w at least one nonG box, pushing any box leads to DDL!
// 2. DeadWall DL(DWDL): depends on >external< boxes!
// 3. Static DeadPIC(SDPIC): PI-Corral where any push leads to SLC or another SDPIC (which MUST be subset/merged with this SDPIC!)
// 4. FixedGoals DL(FGSDL), may depend on >external< boxes!
// 5. DynamicDL(DDL): set of boxes+RCells where any push leads to SLC|SDPIC|DWDL|FGSDL|existing DDL

//Abstract DeadWall: set of cells where number of boxes must be less or eq nMaxBoxes
struct DeadWall {//POD 128b
	int16_t	nMaxBoxes{ 0 };
	int16_t	nP1{ 0 };			//dummy/padding for alignment
	int32_t	nP2{ 0 };
	int64_t llBoxes{ 0 };
};

#define DDL_STATIC	0
#define DDL_DW			1
#define DDL_FG			2
#define DDL_DDL			3
struct Push2DDL {//POD
	uint8_t  nBoxPos{ 0 };  //0- based!
	int8_t   nLastPush{ 0 };//SB_...
	uint16_t nDDLType{ DDL_STATIC };
	uint32_t nDDLIdx{ 0 };
	bool operator==(const Push2DDL& rhs) const {
		return nDDLType == rhs.nDDLType && nDDLIdx == rhs.nDDLIdx && nBoxPos == rhs.nBoxPos; //nLastPush is ignored
	}
};

//Currently DDL is based on DeadPIC, DLC are not supported
struct DeadPIC {
//DATA
	int64_t						llBoxes{ 0 };			//DDL boxes
	int64_t						llCells{ 0 };			//Corral cells; R must be is outside!!
	int64_t						llBoxMask{ 0 };		//Boxes+Cells with G (TEST)
	vector<Push2DDL>	vP2DLs;						//empty for SDPIC
//ATTS
	bool IsValid() const { return llCells != ~0ll;}
	void Disable() { llCells = ~0ll; }
};
///////////////////////////////////////////////////////////////
class CDLMgr {
//DATA
  Sokoban&								m_Sokoban;
	uint16_t								m_aHDWIdx[MAX_SPACES];	//1-based indexes
	uint16_t								m_aVDWIdx[MAX_SPACES];	
	vector<DeadWall>				m_vHDWs;								//Horizontal DeadWalls
	vector<DeadWall>				m_vVDWs;								//Horizontal DeadWalls
	CFixedGoals							m_FGs;
	//DDLs!
	vector<DeadPIC>				m_vDeadPICs;
public:
//CTOR/DTOR
  CDLMgr(Sokoban& sb) : m_Sokoban(sb), m_FGs(sb) {
	};
  ~CDLMgr() = default;
//ATTS
	uint32_t DDLCount() const { return (uint32_t)m_vDeadPICs.size(); }
	uint32_t FGSCount() const { return m_FGs.GetCount(); }
	uint16_t NearestFGLSize(const Stage& stage) const {
		return m_FGs.NearestFGLSize(stage);
	}
//METHODS
  bool Init();
	//DL checks
	bool IsStaticDeadLock(const Stage& stage, int8_t nLastPush) const; //SDL check for FG!
	bool IsFixedDeadLock(const Stage& stage, int8_t nLastPush) const;  //SDL check for CorralMgr!
	bool IsDDL(const Stage& stage) const { return GetDDL_(stage) != 0; }
	bool IsDeadLock(const Stage& stage, int8_t nLastPush, IN OUT DeadPIC& dpic) const;
//Dynamic Deadlocks /DDLs
	void AddDeadPIC(Corral crlDDLNew, IN DeadPIC& dpic);
//internals
private:
	inline int8_t GetCellCode_(const Stage& stage, int nRow, int nCol) const;
	void InitDeadWalls_();
//New Static DL test
	inline bool HasNonG_(int64_t llBoxes) const;
	bool IsStaticDL_(const Stage& stage, uint8_t nRow, uint8_t nCol, OUT int64_t& llBoxes) const;
	bool IsLockedLR_(const Stage& stage, uint8_t nRow, uint8_t nCol, IN OUT int64_t& llBoxes) const;
	bool IsLockedTB_(const Stage& stage, uint8_t nRow, uint8_t nCol, IN OUT int64_t& llBoxes) const;
	//existing DDLs getters return 1-based Idx if true, 0 otherwise
	uint32_t GetDeadWallDL_(const Stage& stage, int8_t nLastPush, int nRow, int nCol) const;
	inline bool IsDeadWallDL_(const Stage& stage, const DeadWall& dwall) const;
	uint32_t GetDDL_(const Stage& stage) const;
	void AddDDL_(const Push2DDL& p2ddl, IN OUT DeadPIC& dpic) const;
	bool IsDeadPIC_(const Stage& stage, const DeadPIC& dpic) const;
};