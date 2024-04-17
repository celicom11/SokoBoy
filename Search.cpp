#include "StdAfx.h"
#include "Sokoban.h"
#include "Corral.h"
#include "StageQueue.h"

//
bool Sokoban::Search(IStageQueue* pSQ, IN OUT Stage& current) {
	uint32_t nQS = 0;
	uint32_t nPIdx = 1;
	Stage temp;
	CStageCorrals sc(*this);
	
	pSQ->Clear();
	m_pClosedStgs->Clear();
	m_pClosedStgs->Push(current);

	do
	{
		//DEBUG
		//if (HasBox(current, 8, 1)) {
		// __debugbreak();
		//}

		if (m_Cfg.nRpt_SQInc && abs((int)(nQS - pSQ->Size())) >= m_Cfg.nRpt_SQInc) {
			Display(current);
			m_Reporter.PC("DST: ");
			if (current.nWeight < 0xFFFF)
				m_Reporter.P(current.nWeight);
			nQS = pSQ->Size();
			m_Reporter.PC(" QS: ").P(nQS).PC(" ALL: ").P(m_pClosedStgs->Size()).PC(" DEPTH: ").P(Depth(current)).PC(" DDLs: ").P(m_DLM.DDLCount()).PEol();
		}
		//1. Start stage corrals analysis
		sc.Init(&current);
		//2.Analyze corrals for PI-Corral pruning
		uint8_t nCIdx = sc.GetPICorral();
		bool bPushed = false;
		//if on push an existing DDL PIC is created, latter needs to be merged with the current PIC w/o the pushed box!
		Corral crlDDL;
		//3.Pushing
		for (uint8_t nBox = 0; nBox < sc.C0BoxCount(); ++nBox) {
			Point ptBox = sc.GetBox(nBox);
			//UP
			if (sc.CanPushUp(ptBox, nCIdx)) {
				temp = current; temp.ptR = ptBox; ++temp.ptR.nRow;
				//tunneling
				temp = PushUp(temp);
				while (IsTunnelPos_UP(temp.ptR.nRow, temp.ptR.nCol) && CanPushUp(temp)) {
					temp = PushUp(temp);
				}
				if (!IsTunnelPos_UP(temp.ptR.nRow, temp.ptR.nCol) && !m_DLM.IsDeadLock_UP(temp, crlDDL)) {
					bPushed = true;
					if (!pSQ->HasStage(temp) && !m_pClosedStgs->HasStage(temp)) {
						temp.nPIdx = nPIdx;
						pSQ->Push(temp);
					}
				}
			}
			//LEFT
			if (sc.CanPushLeft(ptBox, nCIdx)) {
				temp = current; temp.ptR = ptBox; ++temp.ptR.nCol;
				//tunneling
				temp = PushLeft(temp);
				while (IsTunnelPos_LT(temp.ptR.nRow, temp.ptR.nCol) && CanPushLeft(temp)) {
					temp = PushLeft(temp);
				}
				if (!IsTunnelPos_LT(temp.ptR.nRow, temp.ptR.nCol) && !m_DLM.IsDeadLock_LT(temp, crlDDL)) {
					bPushed = true;
					if (!pSQ->HasStage(temp) && !m_pClosedStgs->HasStage(temp)) {
						temp.nPIdx = nPIdx;
						pSQ->Push(temp);
					}
				}
			}
			//DOWN
			if (sc.CanPushDown(ptBox, nCIdx)) {
				temp = current; temp.ptR = ptBox; --temp.ptR.nRow;
				//tunneling
				temp = PushDown(temp);
				while (IsTunnelPos_DN(temp.ptR.nRow, temp.ptR.nCol) && CanPushDown(temp)) {
					temp = PushDown(temp);
				}
				if (!IsTunnelPos_DN(temp.ptR.nRow, temp.ptR.nCol) && !m_DLM.IsDeadLock_DN(temp, crlDDL)) {
					bPushed = true;
					if (!pSQ->HasStage(temp) && !m_pClosedStgs->HasStage(temp)) {
						temp.nPIdx = nPIdx;
						pSQ->Push(temp);
					}
				}
			}
			//RIGHT
			if (sc.CanPushRight(ptBox, nCIdx)) {
				temp = current; temp.ptR = ptBox; --temp.ptR.nCol;
				//tunneling
				temp = PushRight(temp);
				while (IsTunnelPos_RT(temp.ptR.nRow, temp.ptR.nCol) && CanPushRight(temp)) {
					temp = PushRight(temp);
				}
				if (!IsTunnelPos_RT(temp.ptR.nRow, temp.ptR.nCol) && !m_DLM.IsDeadLock_RT(temp, crlDDL)) {
					bPushed = true;
					if (!pSQ->HasStage(temp) && !m_pClosedStgs->HasStage(temp)) {
						temp.nPIdx = nPIdx;
						pSQ->Push(temp);
					}
				}
			}
		}
		if (nCIdx && !bPushed) { //new dead PICorral/DDL!
			Corral crlDDLNew = sc.GetCorral(nCIdx);
			if (m_Cfg.bRpt_PIC_Merge && crlDDLNew.llBoxes != (crlDDLNew.llBoxes | crlDDL.llBoxes)) {
				Display(current);
				m_Reporter.PC("DST: ");
				if (pSQ->Top().nWeight < 0xFFFF)
					m_Reporter.P(pSQ->Top().nWeight);
				nQS = pSQ->Size();
				m_Reporter.PC(" QS: ").P(nQS).PC(" ALL: ").P(m_pClosedStgs->Size()).PC(" DEPTH: ").P(Depth(current)).PC(" DDLs: ").P(m_DLM.DDLCount()).PEol();
				m_Reporter.PC("Merging PI Corrals").PEol();
				DisplayCorral(current.ptR, crlDDLNew);
				DisplayCorral(current.ptR, crlDDL);
			}
			m_DLM.AddDeadPIC(crlDDLNew.Union(crlDDL));
		}
		//because of DDLs, check if parent node is already dead
		while (1) {
			if (!pSQ->IsEmpty()) {
				current = pSQ->Pop();
				if (0 == current.nWeight) {//the rest of the path is in m_RSM!
					m_RSM.CompletePath(current, m_pClosedStgs);
					return true;
				}
				if (m_DLM.IsDeadPIC(current))
					continue;//prunned by DDL!
				m_pClosedStgs->Push(current);
				nPIdx = m_pClosedStgs->Size();//1 based!
				break; //back to main loop
			}
			else
				return false;
		}
	} while (current.llBoxPos != m_llStgPos);
	return true;
}