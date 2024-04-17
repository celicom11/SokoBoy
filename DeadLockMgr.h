#pragma once

class Sokoban;
struct Stage;
struct IBitMgr45;

class CDLMgr {
//TYPES
	struct FixedGoals {
		uint16_t					nBits{ 0 };			//popcnt
		int64_t						llDeadPos{ 0 };	//common unreachable cells (if any) for all non-FG G-boxes
		int64_t						llFGPos{ 0 };		//pos of the fixed Goals
		vector<FGStgInfo>	vGI;						//BoxInfo for each box not in FG 
	};
//DATA
  const Sokoban&					m_Sokoban;
	//Deadlock tables
	bitset<128>							m_btsDL23;						//1+2*3 = 7 bits for 2x3 DLs 
	//deadlocked PI Corrals
	vector<Corral>					m_vDeadPIC;
	//DEAD POS/GOALS INDUCED BY FIXED GOAL/STG
	vector<FixedGoals>			m_vFG;								//all FixedGoals which induce extra DFG
public:
//CTOR/DTOR
  CDLMgr(const Sokoban& sb) : m_Sokoban(sb) {};
  ~CDLMgr() = default;
//ATTS
	uint32_t DDLCount() const { return (uint32_t)m_vDeadPIC.size(); }
//METHODS
  bool Init();
	bool IsDeadLock_UP(const Stage& stage, IN OUT Corral& crlDDL) const;
	bool IsDeadLock_DN(const Stage& stage, IN OUT Corral& crlDDL) const;
	bool IsDeadLock_LT(const Stage& stage, IN OUT Corral& crlDDL) const;
	bool IsDeadLock_RT(const Stage& stage, IN OUT Corral& crlDDL) const;
//Dynamic Deadlocks
	void AddDeadPIC(const Corral& crlNew) {
		for (const Corral& crl : m_vDeadPIC) {
			if ((crl.llBoxes & crlNew.llBoxes) == crl.llBoxes && (crl.llCells && crlNew.llCells) == crl.llCells) //existing sub-Corral?
				return;//already exist!
		}
		m_vDeadPIC.push_back(crlNew);
	}
	bool IsDeadPIC(const Stage& stage) const;
	uint16_t GetFGLBits(const Stage& stage) const;
//internals
private:
	void InitFixedGoals_();
	bool IsDeadlock23_(IBitMgr45* pBMgr) const;
	bool IsDeadLock33_(const Stage& stage, uint8_t nRow, uint8_t nCol) const;
	bool TestDL23OnPushUP_(const Stage& stage, int nRow, int nCol) const;
	bool TestDL23OnPushDN_(const Stage& stage, int nRow, int nCol) const;
	bool TestDL23OnPushLT_(const Stage& stage, int nRow, int nCol) const;
	bool TestDL23OnPushRT_(const Stage& stage, int nRow, int nCol) const;
	inline bool GetDeadCorral_(const Stage& stage, OUT Corral& crlDDL) const;
	bool IsFixedGoalDL_(const Stage& stage) const;
	void AddFixedGoal_(const vector<Point>& vStgPts);
};