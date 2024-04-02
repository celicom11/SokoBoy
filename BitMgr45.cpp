#include "StdAfx.h"
#include "BitMgr45.h"
int64_t CBM45_Up::GetBits23() {
  const int nRow = m_Stage.ptR.nRow - 1, nCol = m_Stage.ptR.nCol; //box pos/B0
  int8_t nB3 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol);
  if (!nB3)
    return 0; //ntd/cannot be DL if B3 is free space!
  int64_t llRet = 0;
  //5 packs of (1+2*3) bits, 0 if cannot be DL
  //NN? ?NN | NO ON |  O O
  //Nb? ?bN |Ob   bO| Nb bN
  //                  O   O
  int8_t nB0 = m_Sokoban.IsStorage(nRow, nCol) ? 1 : 0;
  int8_t nB1 = m_Sokoban.GetBits2(m_Stage, nRow, nCol-1);
  int8_t nB2 = m_Sokoban.GetBits2(m_Stage, nRow-1, nCol - 1);
  int8_t nB4 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol+1);
  int8_t nB5 = m_Sokoban.GetBits2(m_Stage, nRow, nCol+1);
  llRet = (nB0 << 6)|(nB1<<4)|(nB2<<2)|nB3;
  llRet <<= 7;
  llRet |= (nB0 << 6) | (nB3 << 4) | (nB4 << 2) | nB5;
  llRet <<= 7;
  if ((nB2 == 1 && nB5 == 1) || (nB1 == 1 && nB4 == 1))
    llRet |= (nB0 << 6) | (1 << 4) | (1 << 2) | nB3;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow+1,nCol-1) )
    llRet |= (nB0 << 6) | (nB1 << 4) | (1 << 2) | 1;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow + 1, nCol + 1))
    llRet |= (nB0 << 6) | (nB5 << 4) | (1 << 2) | 1;
  return llRet;
}
int64_t CBM45_Lt::GetBits23() {
  const int nRow = m_Stage.ptR.nRow, nCol = m_Stage.ptR.nCol-1; //box pos
  int8_t nB3 = m_Sokoban.GetBits2(m_Stage, nRow, nCol - 1);
  if (!nB3)
    return 0; //ntd/cannot be DL if B3 is free space!
  int64_t llRet = 0;
  //4 packs of (1+2*3) bits, 0 if no fit!
  //?? NN |  O O  | NO Ob 
  //Nb Nb | Nb Nb |Ob   NO
  //NN ?? | O   O
  int8_t nB0 = m_Sokoban.IsStorage(nRow, nCol) ? 1 : 0;
  int8_t nB1 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol);
  int8_t nB2 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol - 1);
  int8_t nB4 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol - 1);
  int8_t nB5 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol);
  llRet = (nB0 << 6) | (nB1 << 4) | (nB2 << 2) | nB3;
  llRet <<= 7;
  llRet |= (nB0 << 6) | (nB3 << 4) | (nB4 << 2) | nB5;
  llRet <<= 7;
  if ((nB2 == 1 && nB5 == 1) || (nB1 == 1 && nB4 == 1))
    llRet |= (nB0 << 6) | (1 << 4) | (1 << 2) | nB3;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow - 1, nCol + 1))
    llRet |= (nB0 << 6) | (nB5 << 4) | (1 << 2) | 1;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow + 1, nCol + 1))
    llRet |= (nB0 << 6) | (nB1 << 4) | (1 << 2) | 1;
  return llRet;
}

int64_t CBM45_Dn::GetBits23() {
  const int nRow = m_Stage.ptR.nRow + 1, nCol = m_Stage.ptR.nCol; //box pos
  int8_t nB3 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol);
  if (!nB3)
    return 0; //ntd/cannot be DL if B3 is free space!
  int64_t llRet = 0;
  //6 packs of (1+2*3) bits, 0 if no fit!
  //Nb? ?bN | bO Ob |  O O
  //NN? ?NN |ON   NO| bN Nb
  //                  O   O
  int8_t nB0 = m_Sokoban.IsStorage(nRow, nCol) ? 1 : 0;
  int8_t nB1 = m_Sokoban.GetBits2(m_Stage, nRow, nCol - 1);
  int8_t nB2 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol - 1);
  int8_t nB4 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol+1);
  int8_t nB5 = m_Sokoban.GetBits2(m_Stage, nRow, nCol + 1);
  llRet = (nB0 << 6) | (nB1 << 4) | (nB2 << 2) | nB3;
  llRet <<= 7;
  llRet |= (nB0 << 6) | (nB3 << 4) | (nB4 << 2) | nB5;
  llRet <<= 7;
  if ((nB2 == 1 && nB5 == 1) || (nB1 == 1 && nB4 == 1))
    llRet |= (nB0 << 6) | (1 << 4) | (1 << 2) | nB3;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow - 1, nCol + 1))
    llRet |= (nB0 << 6) | (nB5 << 4) | (1 << 2) | 1;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow - 1, nCol - 1))
    llRet |= (nB0 << 6) | (nB1 << 4) | (1 << 2) | 1;
  return llRet;
}

int64_t CBM45_Rt::GetBits23() {
  const int nRow = m_Stage.ptR.nRow, nCol = m_Stage.ptR.nCol + 1; //box pos
  int8_t nB3 = m_Sokoban.GetBits2(m_Stage, nRow, nCol + 1);
  if (!nB3)
    return 0; //ntd/cannot be DL if B3 is free space!
  int64_t llRet = 0;
  //4 packs of (1+2*3) bits, 0 if no fit!
  //?? NN | O   O | bO ON 
  //bN bN | bN bN |ON   bO
  //NN ?? |  O O 
  int8_t nB0 = m_Sokoban.IsStorage(nRow, nCol) ? 1 : 0;
  int8_t nB1 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol);
  int8_t nB2 = m_Sokoban.GetBits2(m_Stage, nRow + 1, nCol + 1);
  int8_t nB4 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol + 1);
  int8_t nB5 = m_Sokoban.GetBits2(m_Stage, nRow - 1, nCol);
  llRet = (nB0 << 6) | (nB1 << 4) | (nB2 << 2) | nB3;
  llRet <<= 7;
  llRet |= (nB0 << 6) | (nB3 << 4) | (nB4 << 2) | nB5;
  llRet <<= 7;
  if ((nB2 == 1 && nB5 == 1) || (nB1 == 1 && nB4 == 1))
    llRet |= (nB0 << 6) | (1 << 4) | (1 << 2) | nB3;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow + 1, nCol - 1))
    llRet |= (nB0 << 6) | (nB1 << 4) | (1 << 2) | 1;
  llRet <<= 7;
  if (nB3 == 1 && m_Sokoban.IsWall(nRow - 1, nCol - 1))
    llRet |= (nB0 << 6) | (nB5 << 4) | (1 << 2) | 1;
  return llRet;
}
