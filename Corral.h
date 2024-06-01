#pragma once

class Sokoban;
//Corral cells
#define CUNKN   0  //
#define CWALL   1  //
#define CBOX    2  //
#define CIDX0   3 //first corral index
class CStageCorrals {
//DATA
  //CONTEXT
  const Sokoban&      m_Sokoban;
  const Stage*        m_pStage{nullptr};        //external, dnd!
  //
  uint8_t             m_nLastCIdx;
  uint8_t             m_nBoxCount{ 0 };         //reachable boxes
  uint8_t             m_nC0BoxCount{ 0 };       //Corral0 box count
  uint8_t             m_nLastPICIdx{ 0 };       //cache
  int64_t             m_llExternalLocked{ 0 };  //last PIC is special!
  Point               m_aBoxes[MAX_BOXES];      //boxes reachable from C0
  //Stage cells: 0-Unkn, 1-Wall, 2-Box, 3+: Corral idx of the empty cell
  uint8_t             m_aCells[MAX_DIM][MAX_DIM]{ 0 };
public:
//CTOR/DTOR
  CStageCorrals(const Sokoban& sb) : m_Sokoban(sb){};
  ~CStageCorrals() = default;
//ATTS
  uint8_t C0BoxCount() const { return m_nC0BoxCount; }
  Point GetBox(uint8_t nBoxIdx) const { return m_aBoxes[nBoxIdx]; }
  bool CanPushUp(Point ptBox, uint8_t nCIdx) const;
  bool CanPushDown(Point ptBox, uint8_t nCIdx) const;
  bool CanPushLeft(Point ptBox, uint8_t nCIdx) const;
  bool CanPushRight(Point ptBox, uint8_t nCIdx) const;
  //METHODS
  void Init(const Stage* pStage);
  int8_t GetPICorral();
  Corral GetCorral(uint8_t nCIdx) const;

private:
  bool HasBox_(Point ptCell) const {
    return m_aCells[ptCell.nRow][ptCell.nCol] == CBOX;
  }
  inline bool NotSpace_(Point ptCell) const;
  inline bool IsDead_(Point ptCell) const;
  inline bool IsWall_(Point ptCell) const;
  inline bool IsG_(Point ptCell) const;
  inline bool IsNonCorralSpace_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const;
  inline bool IsOtherCorralSpace_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const;

  bool HasCorralEdge_(Point ptBox, uint8_t nCIdx) const {
    return m_aCells[ptBox.nRow - 1][ptBox.nCol] == nCIdx || m_aCells[ptBox.nRow + 1][ptBox.nCol] == nCIdx ||
      m_aCells[ptBox.nRow][ptBox.nCol - 1] == nCIdx || m_aCells[ptBox.nRow][ptBox.nCol + 1] == nCIdx;
  }
  bool HasOtherCorralEdge_(Point ptBox, uint8_t nCIdx) const {
    return (m_aCells[ptBox.nRow - 1][ptBox.nCol] >= CIDX0 && m_aCells[ptBox.nRow - 1][ptBox.nCol] != nCIdx) ||
      (m_aCells[ptBox.nRow + 1][ptBox.nCol] >= CIDX0 && m_aCells[ptBox.nRow + 1][ptBox.nCol] != nCIdx) ||
      (m_aCells[ptBox.nRow][ptBox.nCol - 1] >= CIDX0 && m_aCells[ptBox.nRow][ptBox.nCol - 1] != nCIdx) ||
      (m_aCells[ptBox.nRow][ptBox.nCol + 1] >= CIDX0 && m_aCells[ptBox.nRow][ptBox.nCol + 1] != nCIdx);
  }
  bool CanUnblockUD_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT int64_t& llBoxes) const;
  bool CanUnblockLR_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT int64_t& llBoxes) const;
  bool CalcLockedNonG_(uint8_t nCIdx, IN OUT int64_t& llExternalLocked) const;
  void GetBoxNeighb_(Point ptBox, OUT uint8_t (&aCC)[4]) const;
  //
  void InitBoxCell_(uint8_t nRow, uint8_t nCol, bool bC0);
  void AddCorral_(Point ptCell, uint8_t nCIdx);//helper
  void AddInnerCorral_(Point ptCell, uint8_t nCIdx) {
    AddCorral_(ptCell, nCIdx);          //may inc m_nBoxCount!
    Point ptCellMerge = OnInnerCorralAdded_();
    //recursevely merge inner corral with others if possible
    if (ptCellMerge.nRow && ptCellMerge.nCol)
      AddInnerCorral_(ptCellMerge, nCIdx);
  }
  Point OnInnerCorralAdded_();
  bool IsPICorral_(uint8_t nCIdx, OUT int64_t& llExternalLocked) const;
  bool IsSingleCorral_() const;
};