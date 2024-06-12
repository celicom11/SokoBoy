#include "StdAfx.h"
#include "SokoTypes.h"
#include "Sokoban.h"
#include "DeadLockMgr.h"


namespace {
//TYPES
  struct SFreeG {
    int8_t  nGIdx{ 0 };        //index of the G box
    int8_t  nBoxPos{ 0 };      //current position of this box in IFG set
    bool operator==(const SFreeG& rhs) const {
      return nGIdx == rhs.nGIdx && nBoxPos == rhs.nBoxPos;
    }

  };
  struct SImpFGPos {
    int8_t         nRPos{ 0 };  //R in this IFG setup
    int64_t        llBoxes{ 0 };
    vector<SFreeG> vDynaG;      //pullable G boxes
    //
    bool operator==(const SImpFGPos& rhs) const {
      return llBoxes == rhs.llBoxes && vDynaG == rhs.vDynaG;
    }

    struct HashFunction
    {
      size_t operator()(const SImpFGPos& ifgp) const
      {
        return std::hash< int64_t>()(ifgp.llBoxes);
      }
    };
  };

}
void CFixedGoals::Init() {
  const int nMaxB = (1 << m_Sokoban.BoxCount()) - 1; //2^MAX_BOXES-1 max;case when all boxes are G is a final pos/excluded!
  m_vFG.clear();
  for (int nBMask = 1; nBMask < nMaxB; ++nBMask) {
    int64_t llBoxes = 0;
    int nTmp = nBMask, nBitPos = 0;
    vector<Point> vStgPts;
    while (nTmp) {
      if (nTmp & 1)
        vStgPts.push_back(m_Sokoban.Goal(nBitPos).pt);
      ++nBitPos;
      nTmp >>= 1;
    }
    if (m_Sokoban.AreFixedGoals(vStgPts))
      AddFixedGoal_(vStgPts);
  }//for
}
void CFixedGoals::AddFixedGoal_(const vector<Point>& vStgPts) {
  FixedGoals fgs;
  fgs.nBits = (uint16_t)vStgPts.size();
  fgs.llBoxes = m_Sokoban.CellsPos(vStgPts);
  for (int nIdx = 0; nIdx < m_Sokoban.BoxCount(); ++nIdx) {
    Point ptG = m_Sokoban.Goal(nIdx).pt;
    FGFreeGInfo fginfo;
    fginfo.llFreeGPos = 1ll << m_Sokoban.CellPos(ptG);
    if (0 == (fgs.llBoxes & fginfo.llFreeGPos)) {
      fginfo.llBoxRCells = fginfo.llFreeGPos;
      fgs.vFreeGI.push_back(fginfo);
    }
  }
  //Pass1: try to pull all boxes (except from FG) from Gs as far as possible (>4 steps) in all possible orders
  CRStages rsm(m_Sokoban);
  SBoxInfo aSBI[FIXEDPC];
  int64_t llUnmovedGoals = m_Sokoban.FinalBoxPos() ^ fgs.llBoxes;
  while (llUnmovedGoals) {
    bool bBoxRemoved = false;
    for (FGFreeGInfo& fginfo : fgs.vFreeGI) {
      if (fginfo.llFreeGPos & llUnmovedGoals) {
        aSBI[0].nRPos = 0;//unkn
        aSBI[0].nBoxPos = _Bit1Pos(fginfo.llFreeGPos);
        fginfo.llBoxRCells |= rsm.GetRBoxCells(llUnmovedGoals | fgs.llBoxes, aSBI, false);
        if (_Popcnt64(fginfo.llBoxRCells) >= FIXEDPC) {//this G box is free
          llUnmovedGoals ^= fginfo.llFreeGPos;
          bBoxRemoved = true;
          break;
        }
      }
    }
    if (!bBoxRemoved)
      break; //nothing can be pulled-away far enough
  }
  //Pass2: if some G boxes left unmoved - we MAY have ImposedFGs!
  //init current IFG candidates
  SImpFGPos ifgpCurrent;
  ifgpCurrent.llBoxes = llUnmovedGoals;
  for (int8_t nIdx = 0; nIdx < (int8_t)fgs.vFreeGI.size(); ++nIdx) {
    const FGFreeGInfo& fginfo = fgs.vFreeGI[nIdx];
    if (llUnmovedGoals & fginfo.llFreeGPos) {
      ifgpCurrent.vDynaG.emplace_back();
      SFreeG& freeg = ifgpCurrent.vDynaG.back();
      freeg.nGIdx = nIdx;
      freeg.nBoxPos = _Bit1Pos(fginfo.llFreeGPos);
    }
  }
  vector<SImpFGPos> vImpFGoals(1, ifgpCurrent);//~Stack
  unordered_set<SImpFGPos, SImpFGPos::HashFunction> usProcessed;
  while (!vImpFGoals.empty()) {
    ifgpCurrent = vImpFGoals.back(); vImpFGoals.pop_back();//pop
    if (usProcessed.find(ifgpCurrent) != usProcessed.end())
      continue;//already processed
    usProcessed.insert(ifgpCurrent);
    //try to pull again
    int nFreeGIdx = -1;
    bool bHasIFG = false;
    for (SFreeG& freeG : ifgpCurrent.vDynaG) {
      ++nFreeGIdx;
      FGFreeGInfo& fginfo = fgs.vFreeGI[freeG.nGIdx];//orig G box
      aSBI[0].nBoxPos = freeG.nBoxPos;
      aSBI[0].nRPos = ifgpCurrent.nRPos;//may be 0!
      int64_t llBoxRCells = rsm.GetRBoxCells(ifgpCurrent.llBoxes | fgs.llBoxes, aSBI, false);
      fginfo.llBoxRCells |= llBoxRCells;
      uint8_t nPCTotal = _Popcnt64(fginfo.llBoxRCells);
      if(nPCTotal < FIXEDPC)
        bHasIFG = true;
      uint8_t nPCLast = _Popcnt64(llBoxRCells);
      if (nPCLast >= FIXEDPC) {//IFG candidate box gets free->FreeG
        llUnmovedGoals &= ~fginfo.llFreeGPos;
        //we continue from this (and ONLY this) position without the freed box now 
        ifgpCurrent.llBoxes ^= 1ll << freeG.nBoxPos;
        ifgpCurrent.vDynaG.erase(ifgpCurrent.vDynaG.begin() + nFreeGIdx);
        vImpFGoals.push_back(ifgpCurrent);
        break;
      }
      //else, new RCells if any from the last pull
      
      if (nPCLast > 1) { //[2..4]
        //move box to new positions
        for (uint8_t nIdx = 1; nIdx < FIXEDPC; ++nIdx) {
          if (aSBI[nIdx].nRPos == 0)
            break;  //no more attended cells
          SImpFGPos ifgpNew = ifgpCurrent;
          SFreeG& freeGNew = ifgpNew.vDynaG[nFreeGIdx];
          ifgpNew.nRPos = aSBI[nIdx].nRPos;
          freeGNew.nBoxPos = aSBI[nIdx].nBoxPos;
          ifgpNew.llBoxes ^= 1ll << freeG.nBoxPos;
          ifgpNew.llBoxes |= 1ll << freeGNew.nBoxPos;
          vImpFGoals.push_back(ifgpNew);
        }
      }
      //else, try next freeG
    } //for
    if (!bHasIFG) {
      llUnmovedGoals = 0;//all boxes could be moved far enough!
      break;
    }
  }//while vImpFGoals

   //fill fgs.vIFGPos
  if (llUnmovedGoals) {
    unordered_set<int64_t> usUnmoved;
    for (const SImpFGPos& ifgp : usProcessed) {
      int64_t llBoxes = ifgp.llBoxes;
      //leave only real IFG boxes
      for (const SFreeG& freeG : ifgp.vDynaG) {
        FGFreeGInfo& fginfo = fgs.vFreeGI[freeG.nGIdx];//orig G box
        if (0 == (fginfo.llFreeGPos & llUnmovedGoals)) {
          assert(llBoxes & (1ll << freeG.nBoxPos));
          llBoxes ^= 1ll << freeG.nBoxPos;
        }
      }
      if(llBoxes)
        usUnmoved.insert(llBoxes);
    }
    for (int64_t llIFG : usUnmoved) {
      fgs.vIFGPos.push_back(llIFG);
    }

    //ImposedFixedGoal detection completed: remove all IFGs from FreeGI
    for (uint16_t nRIdx = (uint16_t)fgs.vFreeGI.size(); nRIdx > 0; --nRIdx) {
      if (fgs.vFreeGI[nRIdx - 1].llFreeGPos & llUnmovedGoals) {
        fgs.vFreeGI.erase(fgs.vFreeGI.begin() + nRIdx - 1);
      }
    }

    //Pass3: update FreeG possible positions as some pulled IFG boxes could free more squares for them!
    for (FGFreeGInfo& freeG : fgs.vFreeGI) {
      aSBI[0].nRPos = 0;//unkn
      aSBI[0].nBoxPos = _Bit1Pos(freeG.llFreeGPos);
      //do it for all ImposedFG position  - could be an overhead, but its accurate( whatif FreeG is unlocked during IFG detection?)
      for (int64_t llIFG : fgs.vIFGPos) {
        int64_t llBoxRCells = rsm.GetRBoxCells(fgs.llBoxes | freeG.llFreeGPos | llIFG, aSBI, false);
        freeG.llBoxRCells |= llBoxRCells;
        aSBI[0].nRPos = 0;//unkn
        aSBI[0].nBoxPos = _Bit1Pos(freeG.llFreeGPos);
      }
    }
  }
  else {//no IFG, all boxes are free-pulled:lets pull them as a single box, as exact "packing order" is unknown (todo!)
    for (FGFreeGInfo& freeG : fgs.vFreeGI) {
      aSBI[0].nRPos = 0;//unkn
      aSBI[0].nBoxPos = _Bit1Pos(freeG.llFreeGPos);
      int64_t llBoxRCells = rsm.GetRBoxCells(fgs.llBoxes | freeG.llFreeGPos, aSBI, false);
      freeG.llBoxRCells |= llBoxRCells;
    }
  }
  //init llDeadPos
  fgs.llDeadPos = ~0ll ^ m_Sokoban.FinalBoxPos();//all w/o Gs
  for (const FGFreeGInfo& freeG : fgs.vFreeGI) {
    fgs.llDeadPos &= ~freeG.llBoxRCells;//remove reachable cells
  }
  m_vFG.push_back(fgs);
  
}
uint32_t CFixedGoals::GetFixedGoalDL(const Stage& stage, OUT int64_t& llFGBoxes) const {
  uint32_t nRet = GetNearestFG_(stage);
  if (nRet && IsFixedGoalDL(stage, nRet)) {
    llFGBoxes = m_vFG[nRet - 1].llBoxes;
    return nRet;
  }
  return 0;
}
bool CFixedGoals::IsFixedGoalDL(const Stage& stage, uint32_t nFGIdx) const {
  assert(nFGIdx && nFGIdx <= m_vFG.size());
  const FixedGoals& fg = m_vFG[nFGIdx - 1];
  //check for each good IFGs
  if (!fg.vIFGPos.empty()) {
    for (int64_t llBoxes : fg.vIFGPos) { //find first IFG contained by the stage
      if ((llBoxes & stage.llBoxPos) == llBoxes) {
        int64_t llNonFixedBoxes = stage.llBoxPos ^ (fg.llBoxes|llBoxes); //exclude FGs and IFGs
        //check FreeG/non-fixed boxes
        if (llNonFixedBoxes & fg.llDeadPos)
          continue;//try another IFG combination
        vector<int64_t> vRBoxes;
        int64_t llAllBoxes = 0;
        uint8_t nNFBoxesCount = _Popcnt64(llNonFixedBoxes), nMinRBoxes = MAX_BOXES;
        assert(fg.vFreeGI.size() == nNFBoxesCount);
        for (const FGFreeGInfo& fginfo : fg.vFreeGI) {
          // pull-reachable stage boxes for this free (non-FG/non-IFG) G
          const int64_t llRBoxes = llNonFixedBoxes & fginfo.llBoxRCells;
          if (!llRBoxes)
            continue; //FG DL, as non of the boxes can be pushed to this G! TODO: check for box influences/fill order?
          uint8_t nPC = _Popcnt64(llRBoxes);
          if (nPC < nNFBoxesCount) { //ignore G which can reached by any box
            llAllBoxes |= llRBoxes;
            vRBoxes.push_back(llRBoxes);
            if (nMinRBoxes > nPC)
              nMinRBoxes = nPC;
          }
        }
        if (nMinRBoxes < vRBoxes.size() && !HasPerfectMatch_(llAllBoxes, vRBoxes)) {
          //DEBUG
          //m_Sokoban.Reporter().PC(L">>>>>>FG NoPM4Goals DL:").PEol();
          //m_Sokoban.Display(stage);
          continue;
        }
        return false; //not an FG DL!
      }
    }
    return true;//good IFG no found
  }
  //else
  //check FreeG/non-fixed boxes
  int64_t llNonFixedBoxes = stage.llBoxPos ^ fg.llBoxes; //exclude FGs
  if (llNonFixedBoxes & fg.llDeadPos) {
#if 0 //DEBUG
    m_Sokoban.Reporter().PC(L">>>>>>FG DeadPos DL:").PEol();
    m_Sokoban.Display(stage);
#endif
    return true; //some box is in FG DeadPos!
  }
  int64_t llAllBoxes = 0;
  vector<int64_t> vRBoxes;
  uint8_t nNFBoxesCount = _Popcnt64(llNonFixedBoxes), nMinRBoxes = MAX_BOXES;
  assert(fg.vFreeGI.size() == nNFBoxesCount);
  for (const FGFreeGInfo& fginfo : fg.vFreeGI) {
    // pull-reachable stage boxes for this free (non-FG/non-IFG) G
    const int64_t llRBoxes = llNonFixedBoxes & fginfo.llBoxRCells;
    if (!llRBoxes)
      return true; //FG DL, as non of the boxes can be pushed to this G! TODO: check for box influences/fill order?
    uint8_t nPC = _Popcnt64(llRBoxes);
    if (nPC < nNFBoxesCount) { //if any box can reach this G - we exclude this G from GBP check
      llAllBoxes |= llRBoxes;
      vRBoxes.push_back(llRBoxes);
      if (nMinRBoxes > nPC)
        nMinRBoxes = nPC;
    }
  }
  if (nMinRBoxes < vRBoxes.size() && !HasPerfectMatch_(llAllBoxes, vRBoxes)) {
    //DEBUG
    //m_Sokoban.Reporter().PC(L">>>>>>FG NoPM4Goals DL:").PEol();
    //m_Sokoban.Display(stage);
    return true;
  }
  return false;
}
//check if each Goal can be reached at least by 1 box (~ perfect match in bipartite graph) 
bool CFixedGoals::HasPerfectMatch_(int64_t llAllBoxes, const vector<int64_t>& vRBoxes) const {
  //transform G->llBoxes to Graph structure/G->RBoxes
  vector<vector<int16_t>> vvL2R(vRBoxes.size());
  int16_t nBoxIdx = 0;
  int64_t llBit = 1;
  while (llAllBoxes) {
    if (llAllBoxes & 1) { //new box
      for (size_t nIdx = 0; nIdx < vRBoxes.size(); ++nIdx) {
        if (vRBoxes[nIdx] & llBit)
          vvL2R[nIdx].push_back(nBoxIdx);
      }
      ++nBoxIdx;
    }
    llAllBoxes >>= 1;
    llBit <<= 1;
  }
  return m_PMK.HasPM(vvL2R);
}

//brute-force search: todo min-hashing?
uint32_t CFixedGoals::GetNearestFG_(const Stage& stage) const {
  uint16_t nMaxBits = 0;
  uint32_t nIdx = 0, nRet=0;
  for (const FixedGoals& fg : m_vFG) {
    ++nIdx;//1 based!
    if (fg.llBoxes == (fg.llBoxes & stage.llBoxPos)) {//stage contains all FG boxes
      if (nMaxBits < fg.nBits) {
        nMaxBits = fg.nBits;
        nRet = nIdx;
      }
    }
  }
  return nRet;
}

