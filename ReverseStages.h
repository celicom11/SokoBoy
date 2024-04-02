#pragma once

class Sokoban;
struct DistExt {
	//max 2 different corrals are possible around any box, even if its a 0 pulls!
	uint8_t aPulls[3]{ 255, 255 };	//255 if INF
	int64_t	aCellPos[3]{ 0, 0 };		//Robot's Corral
	//int64_t	aPath[2]{ 0, 0 };			//todo: attended cells for penalty assessment...
};
struct RStageNode {
	RStageNode(const Stage& node) : stage(node) {}
	const Stage&	stage;					//external,fixed in m_vClosedStages!
	//extended pull-distances from each box to all free cells
	DistExt				aDistExt[16][64];
};
class CRStages {
//DATA
  const Sokoban&			m_Sokoban;
	vector<Stage>				m_vClosedStages;	//BFS-tree of RStages - built with all possible starting corral!
  vector<RStageNode>	m_vRSNodes;				//selected nodes on the circle of max depth radius
public:
  CRStages(const Sokoban& sb) : m_Sokoban(sb) {};
  ~CRStages() = default;
//ATTS
	uint32_t size() const { return (uint32_t)m_vRSNodes.size(); }
//METHODS
	bool Init(uint16_t nDepth);
  uint16_t GetMinDist(const Stage& stage) const;		//aka Lower Bound estimation
	bool CompletePath(IN OUT Stage& current, IN OUT IStageQueue* pSQClosed) const;	//Stage MUST be in one of the m_vCorralRS!
private:
	void GetReachableCells_(const Stage& stage, OUT vector<Point>& vCells) const;
	bool CanPullUp_(const Stage& stage) const;
	bool CanPullDown_(const Stage& stage) const;
	bool CanPullLeft_(const Stage& stage) const;
	bool CanPullRight_(const Stage& stage) const;
	Stage PullUp_(const Stage& stage) const;
	Stage PullDown_(const Stage& stage) const;
	Stage PullRight_(const Stage& stage) const;
	Stage PullLeft_(const Stage& stage) const;
	uint16_t LBDist_(const RStageNode& rsl, const Stage& stage) const;
	void InitRSNode_(IN OUT RStageNode& rsnode) const;
	void CalcBoxDistEx_(const Stage& stage, Point ptBox, OUT DistExt* pDistExt) const;
	bool HasStage_(const RStageNode& rsnode, const Stage& stage) const;
	bool IsBoxPushable_(const RStageNode& rsnode, uint8_t nRBox, uint8_t nBoxPos) const;
};