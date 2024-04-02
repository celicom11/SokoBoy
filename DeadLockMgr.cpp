#include "StdAfx.h"
#include "DeadLockMgr.h"
#include "BitMgr45.h"

bool CDLMgr::Init() {
  m_vDeadPIC.clear();
  m_btsDL23.reset();
  //NN
  //Nb
  for (int8_t nB0 : {0, 1}) {
    for (int8_t nB1 : {1, 2, 3}) {
      for (int8_t nB2 : {1,2,3}) {
        for (int8_t nB3 : {1, 2, 3}) {
          m_btsDL23.set((nB0 << 6) | (nB1 << 4) | (nB2 << 2) | nB3);
        }
      }
    }
  }
  //ADD
  // O
  //Ob
  m_btsDL23.set((0 << 6) | (1 << 4) | (0 << 2) | 1);
  //CLEAR
  //GG OO OG GG GO GO OO OG
  //Gg Og Og Og Og Gg Gg Gg

  for (int8_t nB1 : {1, 3}) {
    for (int8_t nB2 : {1, 3}) {
      for (int8_t nB3 : {1, 3}) {
        m_btsDL23.set((1 << 6) | (nB1 << 4) | (nB2 << 2) | nB3, false);
      }
    }
  }
  return true;
}
bool CDLMgr::IsDeadLock_UP(const Stage& stage, IN OUT Corral& clrDDL) const {
  const uint8_t nRow = stage.ptR.nRow - 1, nCol = stage.ptR.nCol; //box pos
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  if (m_Sokoban.IsDeadHWall(stage, nRow, nCol))
    return true;
  //2x3 DL check
  CBM45_Up bm45(m_Sokoban, stage);
  if (IsDeadlock23_(&bm45))
    return true;
  //5 pos tobe checked for 3x3 DL after pushing UP:
  //cbc
  //c c
  if (IsDeadLock33_(stage, nRow, nCol) || IsDeadLock33_(stage, nRow, nCol-1) || IsDeadLock33_(stage, nRow, nCol+1) ||
    IsDeadLock33_(stage, nRow+1, nCol-1) || IsDeadLock33_(stage, nRow+1, nCol+1))
    return true;
  Corral crlDDLE;//existing
  if (GetDeadCorral_(stage, crlDDLE)) {//exclude pushed Box!
    crlDDLE.llBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow , nCol }));
    clrDDL.Union(crlDDLE);
    return true;
  }
  return false;
}
bool CDLMgr::IsDeadLock_DN(const Stage& stage, IN OUT Corral& clrDDL) const {
  const uint8_t nRow = stage.ptR.nRow + 1, nCol = stage.ptR.nCol; //box pos
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  if (m_Sokoban.IsDeadHWall(stage, nRow, nCol))
    return true;
  CBM45_Dn bm45(m_Sokoban, stage);
  if (IsDeadlock23_(&bm45))
    return true;
  //5 pos tobe checked for 3x3 DL after pushing DN, excluding b!:
  // b 
  //c c
  //ccc
  if (IsDeadLock33_(stage, nRow+2, nCol) || IsDeadLock33_(stage, nRow+1, nCol - 1) || IsDeadLock33_(stage, nRow+1, nCol + 1) ||
    IsDeadLock33_(stage, nRow + 2, nCol - 1) || IsDeadLock33_(stage, nRow + 2, nCol + 1))
    return true;
  Corral crlDDLE;//existing
  if (GetDeadCorral_(stage, crlDDLE)) {//exclude pushed Box!
    crlDDLE.llBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow , nCol }));
    clrDDL.Union(crlDDLE);
    return true;
  }
  return false;
}
bool CDLMgr::IsDeadLock_LT(const Stage& stage, IN OUT Corral& clrDDL) const {
  const uint8_t nRow = stage.ptR.nRow, nCol = stage.ptR.nCol-1; //box pos
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  if (m_Sokoban.IsDeadVWall(stage, nRow, nCol))
    return true;
  CBM45_Lt bm45(m_Sokoban, stage);
  if (IsDeadlock23_(&bm45))
    return true;
  //5 pos tobe checked for 3x3 DL after pushing Left:
  //cb 
  //c 
  //cc
  if (IsDeadLock33_(stage, nRow, nCol) || IsDeadLock33_(stage, nRow, nCol - 1) || IsDeadLock33_(stage, nRow + 1, nCol - 1) ||
    IsDeadLock33_(stage, nRow + 2, nCol - 1) || IsDeadLock33_(stage, nRow + 2, nCol))
    return true;
  Corral crlDDLE;//existing
  if (GetDeadCorral_(stage, crlDDLE)) {//exclude pushed Box!
    crlDDLE.llBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow , nCol }));
    clrDDL.Union(crlDDLE);
    return true;
  }
  return false;
}
bool CDLMgr::IsDeadLock_RT(const Stage& stage, IN OUT Corral& clrDDL) const {
  const uint8_t nRow = stage.ptR.nRow, nCol = stage.ptR.nCol+1; //box pos
  assert(m_Sokoban.HasBox(stage, nRow, nCol));
  if (m_Sokoban.IsDeadVWall(stage, nRow, nCol))
    return true;

  CBM45_Rt bm45(m_Sokoban, stage);
  if (IsDeadlock23_(&bm45))
    return true;
  //5 pos tobe checked for 3x3 DL after pushing RT:
  //bc
  // c
  //cc
  if (IsDeadLock33_(stage, nRow, nCol) || IsDeadLock33_(stage, nRow, nCol + 1) || IsDeadLock33_(stage, nRow + 1, nCol + 1) ||
    IsDeadLock33_(stage, nRow + 2, nCol + 1) || IsDeadLock33_(stage, nRow + 2, nCol))
    return true;
  Corral crlDDLE;//existing
  if (GetDeadCorral_(stage, crlDDLE)) {//exclude pushed Box!
    crlDDLE.llBoxes &= ~(1ll << m_Sokoban.CellPos({ nRow , nCol }));
    clrDDL.Union(crlDDLE);
    return true;
  }
  return false;
}
bool CDLMgr::IsDeadlock23_(IBitMgr45* pBMgr) const {
  //1. get 6 x 7bits pack for DL 2x3 check
  int64_t llPack23 = pBMgr->GetBits23();
  assert(llPack23 >= 0);
  while (llPack23) {
    int8_t nBits = int8_t(llPack23 & 127);
    if (m_btsDL23.test(nBits))
      return true;
    llPack23 >>= 7;
  }
  return false;
}
bool CDLMgr::IsDeadPIC(const Stage& stage) const {
  for (const Corral& crl : m_vDeadPIC) {
    if ((crl.llBoxes & stage.llBoxPos) == crl.llBoxes && ((1ll << m_Sokoban.CellPos(stage.ptR)) & crl.llCells) == 0)//R must be outside!
      return true;
  }
  return false;
}
bool CDLMgr::GetDeadCorral_(const Stage& stage, OUT Corral& clrDDL) const {
  for (const Corral& crl : m_vDeadPIC) {
    if ((crl.llBoxes & stage.llBoxPos) == crl.llBoxes && ((1ll << m_Sokoban.CellPos(stage.ptR)) & crl.llCells) == 0) {//R must be outside!
      clrDDL = crl;
      return true;
    }
  }
  return false;
}

//IsDeadLock33 check ONLY this config:
//***
//***
//*N*, N can be O!
bool CDLMgr::IsDeadLock33_(const Stage& stage, uint8_t nRow, uint8_t nCol) const {
  if (nRow < 2 || nRow >= m_Sokoban.Dim().nRows || nCol < 1 || nCol >= m_Sokoban.Dim().nCols - 1)
    return false;
  if (!m_Sokoban.NotSpace(stage, nRow, nCol))
    return false;
  if (m_Sokoban.NotSpace(stage, nRow - 1, nCol)) {
    //static DL check
    //**N N**
    //*NN NN*
    //Ob* *bO
    //DL if cannot move B0(B2) && B7(B5) and at least one Box is not in Goal 
    if (m_Sokoban.IsWall(nRow, nCol - 1) && m_Sokoban.NotSpace(stage, nRow - 1, nCol + 1) && m_Sokoban.NotSpace(stage, nRow - 2, nCol + 1)) {
      if (m_Sokoban.IsWall(nRow - 2, nCol + 1) || m_Sokoban.IsWall(nRow - 2, nCol + 2) || m_Sokoban.NotSpace(stage, nRow - 2, nCol) ||
        (m_Sokoban.IsDeadPos(nRow - 2, nCol) && m_Sokoban.IsDeadPos(nRow - 2, nCol + 2))
        ) {
        const Point aPt[4]{ {nRow, nCol},{uint8_t(nRow - 1), nCol},{uint8_t(nRow - 1), uint8_t(nCol + 1)},{uint8_t(nRow - 2), uint8_t(nCol + 1)} };
        for (Point pt : aPt) {
          if (!m_Sokoban.IsWall(pt.nRow, pt.nCol) && !m_Sokoban.IsStorage(pt.nRow, pt.nCol))
            return true;//found box not in STG!
        }
      }
    }
    if (m_Sokoban.IsWall(nRow, nCol + 1) && m_Sokoban.NotSpace(stage, nRow - 1, nCol - 1) && m_Sokoban.NotSpace(stage, nRow - 2, nCol - 1)) {
      if (m_Sokoban.IsWall(nRow - 2, nCol - 1) || m_Sokoban.IsWall(nRow - 2, nCol - 2) || m_Sokoban.NotSpace(stage, nRow - 2, nCol) ||
        (m_Sokoban.IsDeadPos(nRow - 2, nCol) && m_Sokoban.IsDeadPos(nRow - 2, nCol - 2))
        ) {
        const Point aPt[4]{ {nRow, nCol},{uint8_t(nRow - 1), nCol},{uint8_t(nRow - 1), uint8_t(nCol - 1)},{uint8_t(nRow - 2), uint8_t(nCol - 1)} };
        for (Point pt : aPt) {
          if (!m_Sokoban.IsWall(pt.nRow, pt.nCol) && !m_Sokoban.IsStorage(pt.nRow, pt.nCol))
            return true;//found box not in STG!
        }
      }
    }
  }
  else {
    //*N*
    //NsN
    //*b*
    if (m_Sokoban.NotSpace(stage, nRow - 1, nCol - 1) && m_Sokoban.NotSpace(stage, nRow - 1, nCol + 1) &&
      m_Sokoban.NotSpace(stage, nRow-2, nCol)) {
      //mini corral?
      if (TestDL23OnPushUP_(stage, nRow, nCol) && TestDL23OnPushLT_(stage, nRow - 1, nCol + 1) &&
        TestDL23OnPushDN_(stage, nRow - 2, nCol) && TestDL23OnPushRT_(stage, nRow - 1, nCol - 1)
        ) {
        bool bHasUBox = false;//pushable box-non-in-goal which could be locked
        const Point aPtBox[4]{ {nRow, nCol},
                            {uint8_t(nRow - 1), uint8_t(nCol + 1)},
                            {uint8_t(nRow - 2), nCol},
                            {uint8_t(nRow - 1), uint8_t(nCol - 1)} };
        for (Point ptBox : aPtBox) {
          if (!m_Sokoban.IsWall(ptBox.nRow, ptBox.nCol) && !m_Sokoban.IsStorage(ptBox.nRow, ptBox.nCol))
            bHasUBox = true;//found box not in STG!
        }
        if(bHasUBox)
          return true;//3x3 DL detected!
      }
    }
  }
  return false;
}


//if we try to push wall return true
//if box can be pushed up/down - return false
//otherwise push Up and test DL23
bool CDLMgr::TestDL23OnPushUP_(const Stage& stage, int nRow, int nCol) const {
  if (m_Sokoban.IsWall(nRow, nCol))
    return true;
  if (m_Sokoban.NotSpace(stage, nRow, nCol - 1) || m_Sokoban.NotSpace(stage, nRow, nCol + 1) ||
    (m_Sokoban.IsDeadPos(nRow, nCol - 1) && m_Sokoban.IsDeadPos(nRow, nCol + 1))
    ) 
  {
    if (m_Sokoban.IsWall(nRow + 1, nCol))
      return true;//can't push; add check for Locked G?
    Stage temp = stage;
    temp.ptR = { uint8_t(nRow+1), (uint8_t)nCol };
    Stage temp2 = m_Sokoban.PushUp(temp);
    //2x3 DL check
    CBM45_Up bm45(m_Sokoban, temp2);
    if (IsDeadlock23_(&bm45))
      return true;
  }
  return false;
}
bool CDLMgr::TestDL23OnPushDN_(const Stage& stage, int nRow, int nCol) const {
  if (m_Sokoban.IsWall(nRow, nCol))
    return true;
  if (m_Sokoban.NotSpace(stage, nRow, nCol - 1) || m_Sokoban.NotSpace(stage, nRow, nCol + 1) ||
    (m_Sokoban.IsDeadPos(nRow, nCol - 1) && m_Sokoban.IsDeadPos(nRow, nCol + 1))
    )
  {
    if (m_Sokoban.IsWall(nRow - 1, nCol))
      return true;//can't push; add check for Locked G?
    Stage temp = stage;
    temp.ptR = { uint8_t(nRow - 1), (uint8_t)nCol };
    Stage temp2 = m_Sokoban.PushDown(temp);
    //2x3 DL check
    CBM45_Dn bm45(m_Sokoban, temp2);
    if (IsDeadlock23_(&bm45))
      return true;
  }
  return false;
}
bool CDLMgr::TestDL23OnPushLT_(const Stage& stage, int nRow, int nCol) const {
  if (m_Sokoban.IsWall(nRow, nCol))
    return true;
  if (m_Sokoban.NotSpace(stage, nRow-1, nCol) || m_Sokoban.NotSpace(stage, nRow+1, nCol) ||
    (m_Sokoban.IsDeadPos(nRow-1, nCol) && m_Sokoban.IsDeadPos(nRow+1, nCol))
    )
  {
    if (m_Sokoban.IsWall(nRow, nCol+1))
      return true;//can't push; add check for Locked G?
    Stage temp = stage;
    temp.ptR = { uint8_t(nRow), (uint8_t)(nCol+1) };
    Stage temp2 = m_Sokoban.PushLeft(temp);
    //2x3 DL check
    CBM45_Lt bm45(m_Sokoban, temp2);
    if (IsDeadlock23_(&bm45))
      return true;
  }
  return false;
}
bool CDLMgr::TestDL23OnPushRT_(const Stage& stage, int nRow, int nCol) const {
  if (m_Sokoban.IsWall(nRow, nCol))
    return true;
  if (m_Sokoban.NotSpace(stage, nRow - 1, nCol) || m_Sokoban.NotSpace(stage, nRow + 1, nCol) ||
    (m_Sokoban.IsDeadPos(nRow - 1, nCol) && m_Sokoban.IsDeadPos(nRow + 1, nCol))
    )
  {
    if (m_Sokoban.IsWall(nRow, nCol - 1))
      return true;//can't push; add check for Locked G?
    Stage temp = stage;
    temp.ptR = { uint8_t(nRow), (uint8_t)(nCol - 1) };
    Stage temp2 = m_Sokoban.PushRight(temp);
    //2x3 DL check
    CBM45_Rt bm45(m_Sokoban, temp2);
    if (IsDeadlock23_(&bm45))
      return true;
  }
  return false;
}
