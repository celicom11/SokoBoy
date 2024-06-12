#pragma once
#include "PM_Kuhn.h"
class Sokoban;
//FixedGoals Deadlock Checker
class CFixedGoals {
	//TYPES
	struct FGFreeGInfo {
		int64_t llFreeGPos{ 0 };					//G box position
		int64_t llBoxRCells{ 0 };					//cells this box can reach by pulling
	};
	struct FixedGoals {
		uint16_t						nBits{ 0 };		//popcnt/cache
		int64_t							llBoxes{ 0 };	//fixed G boxes
		int64_t							llDeadPos{ 0 }; //cells  unreachable by any FreeG
		vector<int64_t>			vIFGPos;			//Imposed FixedGoals; llBoxes = ifgs boxes + base FG llBoxes
		vector<FGFreeGInfo>	vFreeGI;			//FreeGInfo for each pullable/non-fixed  box
	};
	//DATA
	const Sokoban&     m_Sokoban;
	mutable CPM_Kuhn   m_PMK;//cached
	//DEAD POS/GOALS INDUCED BY FIXED GOAL/STG
	vector<FixedGoals> m_vFG;

public:
//CTOR/DTOR/Init
	CFixedGoals(const Sokoban& sb) : m_Sokoban(sb) {};
	~CFixedGoals() = default;
//ATTS
	uint32_t GetCount() const { return (uint32_t)m_vFG.size(); }
//METHODS
	void Init();
	uint32_t GetFixedGoalDL(const Stage& stage, OUT int64_t& llFGBoxes) const;
	bool IsFixedGoalDL(const Stage& stage, uint32_t nFGIdx) const;
	uint16_t NearestFGLSize(const Stage& stage) const {
		uint32_t nIdx = GetNearestFG_(stage);
		return nIdx ? m_vFG[nIdx-1].nBits : 0;
	}
//INTERNALS
private:
	uint32_t GetNearestFG_(const Stage& stage) const;
	void AddFixedGoal_(const vector<Point>& vStgPts);
	bool HasPerfectMatch_(int64_t llAllBoxes, const vector<int64_t>& vRBoxes) const;

};
