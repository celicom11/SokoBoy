#pragma once
#define MAX_LEFT 16
#define MAX_RIGHT 16
class CPM_Kuhn {
//DATA
  uint16_t        m_nLC{ 0 };         //nodes on the left side
  uint16_t        m_aUsed[MAX_LEFT];  //number of iter of the used/marked left node
  int16_t         m_aR2L[MAX_RIGHT];  //single matching Left node
  vector<int16_t> m_vL2R[MAX_LEFT];   //vectors of Right nodes for every Left node; RIdx<MAX_RIGHT!
public:
//CTOR/DTOR
  CPM_Kuhn() = default;
//METHODS
  bool HasPM(const vector<vector<int16_t>>& vvL2R);
private:
  bool dfs_(uint16_t nLeft, uint16_t nIter);
};