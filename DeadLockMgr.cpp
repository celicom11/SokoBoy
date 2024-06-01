#include "StdAfx.h"
#include "DeadLockMgr.h"
//#include "BitMgr45.h" depricated
#include "Corral.h"
#include "ReverseStages.h"
#include "Sokoban.h"

bool CDLMgr::Init() {
  InitDeadWalls_();
  m_vDeadPICs.clear();
  m_FGs.Init();
  return true;
}
void CDLMgr::InitDeadWalls_() {
  std::memset(m_aHDWIdx, 0, sizeof(m_aHDWIdx));
  std::memset(m_aVDWIdx, 0, sizeof(m_aVDWIdx));
  m_vHDWs.clear(); m_vVDWs.clear();
  DeadWall dwcand;
  for (uint8_t i = 0; i < m_Sokoban.Dim().nRows; i++) {
    for (uint8_t j = 0; j < m_Sokoban.Dim().nCols; j++) {
      if (m_Sokoban.IsCorner(i, j)) {
        //check a horiz dead wall candidate
        if (m_Sokoban.IsWall(i,j-1) && !m_aHDWIdx[m_Sokoban.CellPos({ i, j })]) {
          dwcand.nMaxBoxes = 0;
          dwcand.llBoxes = 0;
          for (uint8_t nCol = j; nCol < m_Sokoban.Dim().nCols; ++nCol) {
            dwcand.llBoxes |= 1ll << m_Sokoban.CellPos({ i, nCol });
            if (m_Sokoban.IsSpace(i - 1, nCol) && m_Sokoban.IsSpace(i + 1, nCol))
              break;//not a dw, though it could be another vert DW!
            if (m_Sokoban.IsStorage(i, nCol))
              ++dwcand.nMaxBoxes;
            if (m_Sokoban.IsWall(i, nCol + 1)) {//done!
              m_vHDWs.push_back(dwcand);
              for (uint8_t nCol1 = j; nCol1 <= nCol; ++nCol1) {
                m_aHDWIdx[m_Sokoban.CellPos({ i, nCol1 })] = (uint16_t)m_vHDWs.size();//1 based!
                if (!dwcand.nMaxBoxes)
                  m_Sokoban.SetDeadPos(i, nCol1);
              }
              break;
            }
          }
        }
        //check a vert dead wall candidate
        if (m_Sokoban.IsWall(i-1, j) && !m_aVDWIdx[m_Sokoban.CellPos({ i, j })]) {
          dwcand.nMaxBoxes = 0;
          dwcand.llBoxes = 0;
          for (uint8_t nRow = i; nRow < m_Sokoban.Dim().nRows; ++nRow) {
            dwcand.llBoxes |= 1ll << m_Sokoban.CellPos({ nRow, j});
            if (m_Sokoban.IsSpace(nRow, j - 1) && m_Sokoban.IsSpace(nRow, j + 1))
              break;//not a dw
            if (m_Sokoban.IsStorage(nRow, j))
              ++dwcand.nMaxBoxes;
            if (m_Sokoban.IsWall(nRow + 1, j)) {//done!
              m_vVDWs.push_back(dwcand);
              for (uint8_t nRow1 = i; nRow1 <= nRow; ++nRow1) {
                m_aVDWIdx[m_Sokoban.CellPos({ nRow1, j })] = (uint16_t)m_vVDWs.size();//1 based!
                if (!dwcand.nMaxBoxes)
                  m_Sokoban.SetDeadPos(nRow1, j);
              }
              break;
            }
          }
        }
      }//if dw candidate
    }
  }
}

//NEW STATIC DL TEST
bool CDLMgr::HasNonG_(int64_t llBoxes) const {
  return ((~m_Sokoban.FinalBoxPos()) & llBoxes) != 0;
}
int8_t CDLMgr::GetCellCode_(const Stage& stage, int nRow, int nCol) const {
  if (m_Sokoban.IsWall(nRow, nCol))
    return 1;
  if (m_Sokoban.HasBox(stage, nRow, nCol))
    return 2;
  if (m_Sokoban.IsDeadPos(nRow, nCol))
    return 3;
  return 0; //space
}
//More generic static deadlock test: chain of box cannot be moved and one of boxes is not a G
// Example: ANY of the boxes brings a StaticDL - otherwise all b boxes could pushed away
//   #
//  bb
// bb
//#b
//# 
//###
// Complications are caused by G boxes in the chain...(todo: elaborate)
//We need to return all locked boxes for Dead PICorral record (DynamicDL) as PIC DLs can be affected by external boxes
bool CDLMgr::IsStaticDL_(const Stage& stage, uint8_t nRow, uint8_t nCol, OUT int64_t& llBoxes) const {
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  llBoxes = 1ll << m_Sokoban.CellPos({ nRow,nCol });
  int8_t aUDLR[4]{ GetCellCode_(stage, nRow - 1, nCol), GetCellCode_(stage, nRow + 1, nCol),
                   GetCellCode_(stage, nRow, nCol - 1), GetCellCode_(stage, nRow, nCol + 1) };
  bool bHasNG = !m_Sokoban.IsStorage(nRow, nCol);
  //8 cases
  // N
  //Nb

  // N
  // bN

  // N
  //DbD
  if ((aUDLR[0]==1 || aUDLR[0] == 2) && (aUDLR[2] || aUDLR[3])) {
    if (aUDLR[0] == 1 && (aUDLR[2] == 1 || aUDLR[3] == 1) && bHasNG)
      return true;//corner/near 2 corners
    if (aUDLR[0] == 1 || IsLockedLR_(stage, nRow - 1, nCol, llBoxes)) {
      bHasNG = HasNonG_(llBoxes);//update
      if (aUDLR[2] == 3 && aUDLR[3] == 3 && bHasNG)
        return true;
      //check LR boxes (NOTE: R may be more preferred than L in some cases, but its too much to check...)
      if ((aUDLR[2] == 1 && bHasNG) || (aUDLR[2] == 2 && IsLockedTB_(stage, nRow, nCol - 1, llBoxes) && HasNonG_(llBoxes)))
        return true;
      if ((aUDLR[3] == 1 && bHasNG) || (aUDLR[3] == 2 && IsLockedTB_(stage, nRow, nCol + 1, llBoxes) && HasNonG_(llBoxes)))
        return true;
    }
  }
  //Nb
  // N
  
  // bN
  // N
  
  //DbD
  // N
  if ((aUDLR[1] == 1 || aUDLR[1] == 2) && (aUDLR[2] || aUDLR[3])) {
    if (aUDLR[1] == 1 && (aUDLR[2] == 1 || aUDLR[3] == 1) && bHasNG)
      return true;//corner/near 2 corners
    if (aUDLR[1] == 1 || IsLockedLR_(stage, nRow + 1, nCol, llBoxes)) {
      bHasNG = HasNonG_(llBoxes);//update
      if (aUDLR[2] == 3 && aUDLR[3] == 3 && bHasNG)
        return true;
      //check LR boxes (NOTE: R may be more preferred than L in some cases, but its too much to check...)
      if ((aUDLR[2] == 1 && bHasNG) || (aUDLR[2] == 2 && IsLockedTB_(stage, nRow, nCol - 1, llBoxes) && HasNonG_(llBoxes)))
        return true;
      if ((aUDLR[3] == 1 && bHasNG) || (aUDLR[3] == 2 && IsLockedTB_(stage, nRow, nCol + 1, llBoxes) && HasNonG_(llBoxes)))
        return true;
    }
  }

  // D
  //Nb
  // D

  // D
  // bN
  // D
  if (aUDLR[0] == 3 && aUDLR[1] == 3) {
    //check LR boxes (NOTE: R may be more preferred than L in some cases, but its too much to check...)
    if ((aUDLR[2] == 1 && bHasNG) || (aUDLR[2] == 2 && IsLockedTB_(stage, nRow, nCol - 1, llBoxes) && HasNonG_(llBoxes)))
      return true;
    if ((aUDLR[3] == 1 && bHasNG) || (aUDLR[3] == 2 && IsLockedTB_(stage, nRow, nCol + 1, llBoxes) && HasNonG_(llBoxes)))
      return true;
  }
  return false;
}
//recursive!
bool CDLMgr::IsLockedLR_(const Stage& stage, uint8_t nRow, uint8_t nCol, IN OUT int64_t& llBoxes) const {
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  int64_t llBox = 1ll << m_Sokoban.CellPos({ nRow,nCol });
  if (llBoxes & llBox)
    return true;//already locked
  llBoxes |= llBox;//addbit
  bool bLocked = false;

  int8_t nLCode = GetCellCode_(stage, nRow, nCol - 1);
  int8_t nRCode = GetCellCode_(stage, nRow, nCol + 1);
  if ((nLCode == 1 || nRCode == 1 || (nLCode == 3 && nRCode == 3))) {
    bLocked = true;
    if(HasNonG_(llBoxes))
      return true;//DONE!
  }
  //check LR boxes (NOTE: R may be more preferred than L in some cases, but its too much to check...)
  if (nLCode == 2 && IsLockedTB_(stage, nRow, nCol - 1, llBoxes)) {
    bLocked = true;
    if (HasNonG_(llBoxes))
      return true;//DONE!
  }
  if (nRCode == 2 && IsLockedTB_(stage, nRow, nCol + 1, llBoxes)) {
    bLocked = true;
    if (HasNonG_(llBoxes))
      return true;//DONE!
  }

  if (bLocked)
    return true; //we return TRUE even if HasNoG!
  llBoxes ^= llBox;//remove back!
  return false;
}
bool CDLMgr::IsLockedTB_(const Stage& stage, uint8_t nRow, uint8_t nCol, IN OUT int64_t& llBoxes) const {
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  int64_t llBox = 1ll << m_Sokoban.CellPos({ nRow,nCol });
  if (llBoxes & llBox)
    return true;//already checked!
  llBoxes |= llBox;//addbit
  bool bLocked = false;

  int8_t nTCode = GetCellCode_(stage, nRow-1, nCol);
  int8_t nBCode = GetCellCode_(stage, nRow+1, nCol);
  if (nTCode == 1 || nBCode == 1 || (nTCode == 3 && nBCode == 3)) {
    bLocked = true;
    if (HasNonG_(llBoxes))
      return true;//DONE!
  }
  //check LR boxes (NOTE: R may be more preferred than L in some cases, but its too much to check...)
  if (nTCode == 2 && IsLockedLR_(stage, nRow-1, nCol, llBoxes)) {
    bLocked = true;
    if (HasNonG_(llBoxes))
      return true;//DONE!
  }
  if (nBCode == 2 && IsLockedLR_(stage, nRow+1, nCol, llBoxes)) {
    bLocked = true;
    if (HasNonG_(llBoxes))
      return true;//DONE!
  }
  if (bLocked)
    return true;
  llBoxes ^= llBox;//remove back!
  return false;
}
bool CDLMgr::IsStaticDeadLock(const Stage& stage, int8_t nLastPush) const {
  //box pos
  uint8_t nRow = stage.ptR.nRow, nCol = stage.ptR.nCol;
  if (nLastPush == SB_UP) --nRow;
  else if (nLastPush == SB_DN) ++nRow;
  else if (nLastPush == SB_LT) --nCol;
  else if (nLastPush == SB_RT) ++nCol;
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  int64_t llLockedBoxes = 0;
  if (IsStaticDL_(stage, nRow, nCol, llLockedBoxes)) 
    return true;
  if (GetDeadWallDL_(stage, nLastPush, nRow, nCol))
    return true;
  return false;
}
//SDL check for CorralMgr!
//it should return true IIF that all boxes in the locked chain are fixed w or w/o pushed box 
bool CDLMgr::IsFixedDeadLock(const Stage& stage, int8_t nLastPush) const {
  //box pos
  uint8_t nRow = stage.ptR.nRow, nCol = stage.ptR.nCol;
  if (nLastPush == SB_UP) --nRow;
  else if (nLastPush == SB_DN) ++nRow;
  else if (nLastPush == SB_LT) --nCol;
  else if (nLastPush == SB_RT) ++nCol;
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  int64_t llLockedBoxes = 0;
  if (!IsStaticDL_(stage, nRow, nCol, llLockedBoxes))
    return false;
  //verify that ALL boxes in the LC except pushed one are FixedGoals/FGs!
  //NOTE: it could be more complicated- elaborate!
  llLockedBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow, nCol }));
  if (llLockedBoxes != (llLockedBoxes & m_Sokoban.FinalBoxPos()))
    return false;
  //we need to check if they are really fixed
  vector<Point> vCells;
  m_Sokoban.Pos2Points(llLockedBoxes, vCells);
  return m_Sokoban.AreFixedGoals(vCells);
}

//NEW DDLs 
bool CDLMgr::IsDeadLock(const Stage& stage, int8_t nLastPush, IN OUT DeadPIC& dpic) const {
  //box pos
  uint8_t nRow = stage.ptR.nRow, nCol = stage.ptR.nCol;
  if (nLastPush == SB_UP) --nRow;
  else if (nLastPush == SB_DN) ++nRow;
  else if (nLastPush == SB_LT) --nCol;
  else if (nLastPush == SB_RT) ++nCol;
  assert(m_Sokoban.HasBox(stage, nRow, nCol));

#if 0//_DEBUG
  if (m_Sokoban.HasBox(stage, 1, 7) && m_Sokoban.HasBox(stage, 2, 7) && m_Sokoban.HasBox(stage, 2, 8)) {
    __debugbreak();
  }
#endif
  int64_t llLockedBoxes = 0;
  if (IsStaticDL_(stage, nRow, nCol, llLockedBoxes)) {
    //add SDLC but w/o pushed box!
    dpic.llBoxes |= (llLockedBoxes & ~(1ll << m_Sokoban.CellPos({ nRow , nCol })));
    return true;
  }
  Push2DDL p2ddl{ m_Sokoban.CellPos({nRow,nCol}), nLastPush };

  p2ddl.nDDLIdx = GetDeadWallDL_(stage, nLastPush, nRow, nCol);
  if (p2ddl.nDDLIdx) {
    p2ddl.nDDLType = DDL_DW;
    AddDDL_(p2ddl, dpic);
    return true;
  }
  p2ddl.nDDLIdx = GetDDL_(stage);
  if (p2ddl.nDDLIdx) {
    p2ddl.nDDLType = DDL_DDL;
    AddDDL_(p2ddl, dpic);
    //exclude pushed Box in case existing Corrral was merged
    dpic.llBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow , nCol }));
    return true;
  }
  //FixedGoal DDL
  llLockedBoxes = 0;
  p2ddl.nDDLIdx = m_FGs.GetFixedGoalDL(stage, llLockedBoxes);
  if (p2ddl.nDDLIdx) {
    p2ddl.nDDLType = DDL_FG;
    AddDDL_(p2ddl, dpic);
    //add SDLC but w/o pushed box!
    dpic.llBoxes |= (llLockedBoxes & ~(1ll << m_Sokoban.CellPos({ nRow , nCol })));
    return true;
  }
  return false;

}
void CDLMgr::AddDeadPIC(Corral crlDDLNew, IN DeadPIC& dpic) {
  //merge corral
  dpic.llBoxes |= crlDDLNew.llBoxes;
  dpic.llCells |= crlDDLNew.llCells;
  //find existing DDL
  for (DeadPIC& dpicE : m_vDeadPICs) {
    if (dpicE.llBoxes == dpic.llBoxes && dpicE.llCells == dpic.llCells) {
      //check if P2DDL could be merged
      vector<Push2DDL> vNewP2DDLs;
      for (const Push2DDL& p2ddlNew : dpic.vP2DLs) {
        for (const Push2DDL& p2ddl : dpicE.vP2DLs) {
          if (!(p2ddlNew == p2ddl))
            vNewP2DDLs.push_back(p2ddlNew);
        }
      }
      if (vNewP2DDLs.empty())
        assert(0);//snbh!
      else
        dpic.vP2DLs.insert(dpic.vP2DLs.end(), vNewP2DDLs.begin(), vNewP2DDLs.end());
      return; //merged to current DDL
    }
  }
  //else, add new one
  m_vDeadPICs.push_back(dpic);
}

//internals
uint32_t CDLMgr::GetDDL_(const Stage& stage) const {
  for (uint32_t nIdx = 0; nIdx < m_vDeadPICs.size(); ++nIdx) {
    const DeadPIC& dpic = m_vDeadPICs[nIdx];
    if( IsDeadPIC_(stage, dpic) )
      return nIdx + 1;  //valid DDL
  }
  return 0;
}
bool CDLMgr::IsDeadPIC_(const Stage& stage, const DeadPIC& dpic) const {
  if (((dpic.llBoxes | dpic.llCells) & stage.llBoxPos) == dpic.llBoxes && //FIX: stage must have all corral's boxes!
      ((1ll << m_Sokoban.CellPos(stage.ptR)) & dpic.llCells) == 0) {//R must be outside!
    for (const Push2DDL& p2ddl : dpic.vP2DLs) {
      Stage temp = stage;
      temp.ptR = m_Sokoban.CellPoint(p2ddl.nBoxPos);
      assert(m_Sokoban.IsSpace(temp.ptR.nRow, temp.ptR.nCol) && !m_Sokoban.HasBox(temp, temp.ptR.nRow, temp.ptR.nCol));
      assert(p2ddl.nDDLIdx);
      int nDDLIdx = p2ddl.nDDLIdx - 1;
      switch (p2ddl.nLastPush) {
      case SB_UP: ++temp.ptR.nRow;
        while (!m_Sokoban.HasBox(temp, temp.ptR.nRow, temp.ptR.nCol)) {
          _ASSERT(m_Sokoban.IsTunnelPos_UP(temp.ptR.nRow + 1, temp.ptR.nCol));
          ++temp.ptR.nRow;
        }
        ++temp.ptR.nRow;
        assert(m_Sokoban.IsSpace(temp.ptR.nRow, temp.ptR.nCol));
        temp = m_Sokoban.PushUp(temp);
        while (m_Sokoban.IsTunnelPos_UP(temp.ptR.nRow, temp.ptR.nCol)) {
          temp = m_Sokoban.PushUp(temp);
        }
        break;
      case SB_DN: --temp.ptR.nRow;
        while (!m_Sokoban.HasBox(temp, temp.ptR.nRow, temp.ptR.nCol)){
          _ASSERT(m_Sokoban.IsTunnelPos_DN(temp.ptR.nRow - 1, temp.ptR.nCol));
          --temp.ptR.nRow;
        }
        --temp.ptR.nRow;
        assert(m_Sokoban.IsSpace(temp.ptR.nRow, temp.ptR.nCol));
        temp = m_Sokoban.PushDown(temp);
        while (m_Sokoban.IsTunnelPos_DN(temp.ptR.nRow, temp.ptR.nCol)) {
          temp = m_Sokoban.PushDown(temp);
        }
        break;
      case SB_LT: ++temp.ptR.nCol;
        while (!m_Sokoban.HasBox(temp, temp.ptR.nRow, temp.ptR.nCol)) {
          _ASSERT(m_Sokoban.IsTunnelPos_LT(temp.ptR.nRow, temp.ptR.nCol+1));
          ++temp.ptR.nCol;
        }
        ++temp.ptR.nCol;
        assert(m_Sokoban.IsSpace(temp.ptR.nRow, temp.ptR.nCol));
        temp = m_Sokoban.PushLeft(temp);
        while (m_Sokoban.IsTunnelPos_LT(temp.ptR.nRow, temp.ptR.nCol)) {
          temp = m_Sokoban.PushLeft(temp);
        }
        break;
      case SB_RT: --temp.ptR.nCol;
        while (!m_Sokoban.HasBox(temp, temp.ptR.nRow, temp.ptR.nCol)) {
          _ASSERT(m_Sokoban.IsTunnelPos_RT(temp.ptR.nRow, temp.ptR.nCol - 1));
          --temp.ptR.nCol;
        }
        --temp.ptR.nCol;
        assert(m_Sokoban.IsSpace(temp.ptR.nRow, temp.ptR.nCol));
        temp = m_Sokoban.PushRight(temp);
        while (m_Sokoban.IsTunnelPos_RT(temp.ptR.nRow, temp.ptR.nCol)) {
          temp = m_Sokoban.PushRight(temp);
        }
        break;
      }
      if (p2ddl.nDDLType == DDL_DW) {//need to re-check if DWDL still has place
        if ((p2ddl.nLastPush== SB_UP || p2ddl.nLastPush == SB_DN) && !IsDeadWallDL_(temp, m_vHDWs[nDDLIdx]))
          return 0;//not a DDL!
        if ((p2ddl.nLastPush == SB_LT || p2ddl.nLastPush == SB_RT) && !IsDeadWallDL_(temp, m_vVDWs[nDDLIdx]))
          return 0;//not a DDL!
      }
      else if (p2ddl.nDDLType == DDL_FG) {//need to re-check if FGDL still has place
        if (!m_FGs.IsFixedGoalDL(temp, p2ddl.nDDLIdx))
          return 0;//not a DDL!
      }
      else {
        assert(p2ddl.nDDLType == DDL_DDL);
        if (!IsDeadPIC_(temp, m_vDeadPICs[nDDLIdx]))
          return false;
      }
    }
    return true;//DDL!
  }
  return false;
}
uint32_t CDLMgr::GetDeadWallDL_(const Stage& stage, int8_t nLastPush, int nRow, int nCol) const {
  uint8_t nBoxPos = m_Sokoban.CellPos({ uint8_t(nRow),uint8_t(nCol) });//0-based
  if (nLastPush == SB_UP || nLastPush == SB_DN) {
    if (m_aHDWIdx[nBoxPos] && IsDeadWallDL_(stage, m_vHDWs[m_aHDWIdx[nBoxPos] - 1]))
      return m_aHDWIdx[nBoxPos];
  }
  else {
    if (m_aVDWIdx[nBoxPos] && IsDeadWallDL_(stage, m_vVDWs[m_aVDWIdx[nBoxPos] - 1]))
      return m_aHDWIdx[nBoxPos];
  }
  return 0;//no DWDL
}
bool CDLMgr::IsDeadWallDL_(const Stage& stage, const DeadWall& dwall) const {
  int64_t llDwBoxes = dwall.llBoxes & stage.llBoxPos;
  return (llDwBoxes && _Popcnt64(llDwBoxes) > dwall.nMaxBoxes);
}
void CDLMgr::AddDDL_(const Push2DDL& p2ddl, IN OUT DeadPIC& dpic) const {
  if (!dpic.IsValid())
    return;//ntd
  if (p2ddl.nDDLType == DDL_DDL) {
    //always merge with child DDL?
    const DeadPIC& dpicE = m_vDeadPICs[p2ddl.nDDLIdx - 1];
    dpic.llBoxes |= dpicE.llBoxes;
    dpic.llCells |= dpicE.llCells;
    if(!dpicE.vP2DLs.empty())
      dpic.vP2DLs.push_back(p2ddl);//will need to check this child DDL on push
  }
  else
    dpic.vP2DLs.push_back(p2ddl);
}
