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
  Point               m_aBoxes[16];             //boxes reachable from C0
  //Stage cells: 0-Unkn, 1-Wall, 2-Box, 3+: Corral idx of the empty cell
  uint8_t             m_aCells[16][16]{ 0 };
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
  bool CanMoveInCorral_(Point ptBox, uint8_t nCIdx) const {
    return (m_aCells[ptBox.nRow - 1][ptBox.nCol] == nCIdx && m_aCells[ptBox.nRow + 1][ptBox.nCol] == nCIdx) ||
           (m_aCells[ptBox.nRow][ptBox.nCol - 1] == nCIdx && m_aCells[ptBox.nRow][ptBox.nCol + 1] == nCIdx);
  }
  bool CanMoveOutsideCorral_(Point ptBox, uint8_t nCIdx) const {
    return (m_aCells[ptBox.nRow - 1][ptBox.nCol] >= CIDX0 && m_aCells[ptBox.nRow + 1][ptBox.nCol] >= CIDX0 &&
            m_aCells[ptBox.nRow - 1][ptBox.nCol] != nCIdx && m_aCells[ptBox.nRow + 1][ptBox.nCol] != nCIdx) ||
          (m_aCells[ptBox.nRow][ptBox.nCol - 1] >= CIDX0 && m_aCells[ptBox.nRow][ptBox.nCol + 1] >= CIDX0 &&
           m_aCells[ptBox.nRow][ptBox.nCol - 1] != nCIdx && m_aCells[ptBox.nRow][ptBox.nCol + 1] != nCIdx);
  }
  bool IntCanUnblockLR_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT bitset<256>& btsBoxMark) const;//recursive
  bool IntCanUnblockUD_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx, IN OUT bitset<256>& btsBoxMark) const;//recursive
  bool CanUnblockUD_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const {
    bitset<256> btsBoxMark; btsBoxMark.set(nRow * 16 + nCol);
    return IntCanUnblockUD_(nRow, nCol, nCIdx, btsBoxMark);
  }
  bool CanUnblockLR_(uint8_t nRow, uint8_t nCol, uint8_t nCIdx) const {
    bitset<256> btsBoxMark; btsBoxMark.set(nRow * 16 + nCol);
    return IntCanUnblockLR_(nRow, nCol, nCIdx, btsBoxMark);
  }
  void InitBoxCell_(uint8_t nRow, uint8_t nCol, bool bC0);
  void AddCorral_(Point ptCell, uint8_t nCIdx);//helper
  void AddInnerCorral_(Point ptCell, uint8_t nCIdx) {
    AddCorral_(ptCell, nCIdx);          //may inc m_nBoxCount!
    OnInnerCorralAdded_(); //immediately merge inner corral with others if possible
  }
  void OnInnerCorralAdded_();
  bool IsPICorral_(uint8_t nCIdx) const;
};