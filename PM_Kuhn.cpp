#include "StdAfx.h"
#include "PM_Kuhn.h"


bool CPM_Kuhn::dfs_(uint16_t nLeft, uint16_t nIter) {
	if (m_aUsed[nLeft] == nIter)
		return false;
	m_aUsed[nLeft] = nIter;
	for (int16_t nRIdx : m_vL2R[nLeft]) {
		if (m_aR2L[nRIdx] == -1 || dfs_(m_aR2L[nRIdx], nIter)) {
			m_aR2L[nRIdx] = nLeft;
			return true;
		}
	}
	return false;
}
bool CPM_Kuhn::HasPM(const vector<vector<int16_t>>& vvL2R) {
	//INIT
	m_nLC = 0;
	for (const vector<int16_t>& vRNodes : vvL2R) {
		m_vL2R[m_nLC++] = vRNodes;
	}
	::memset(m_aUsed, 0, sizeof(m_aUsed));//used left nodes based on iter idx!
	::memset(m_aR2L, -1, sizeof(m_aR2L)); //Left node for this R nodein the current matching
	//RUN DFS
	for (uint16_t nLeft = 0; nLeft < m_nLC; ++nLeft) {
		if(!dfs_(nLeft, nLeft + 1))
			return false;
	}
	return true;
}
