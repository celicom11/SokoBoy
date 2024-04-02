#pragma once

//LIMITS
#define MAX_DIM    16
#define MAX_BOXES  16
#define MAX_SPACES 64

//COMMON TYPES
struct Point {
	uint8_t	nRow{ 0 };
	uint8_t	nCol{ 0 };
	Point() = default;
	Point(uint8_t i, uint8_t j) :nRow(i), nCol(j) {}
	bool operator==(const Point& rhs) const {
		return nRow == rhs.nRow && nCol == rhs.nCol;
	}
};
struct Dimens {
	uint16_t	nRows{ 0 };
	uint16_t	nCols{ 0 };
};
struct Corral {//POD, 128b
	int64_t llBoxes{ 0 };
	int64_t llCells{ 0 };//free cells inside Corral, to ensure R is outside!!
	Corral& Union(const Corral& clr2) {
		llBoxes |= clr2.llBoxes;
		llCells |= clr2.llCells;
		return *this;
	}
};
struct Stage {//POD, 16b
	Point			ptR;									//robot pos
	uint16_t	nWeight{ 0xFFFF };		//sum of min-MDist from each box to the highest priority available storage!
	uint32_t	nPIdx{ 0 };						//Parent Id/Idx
	int64_t		llBoxPos{ 0 };				//bits for box pos 
	//OPS
	bool operator==(const Stage& rhs) const {
		return ptR == rhs.ptR && llBoxPos == rhs.llBoxPos;
	}
	void GetBoxesPos(OUT uint8_t(&aBoxBPos)[16]) const {
		if (!llBoxPos) {
			assert(0);
			return;
		}
		int64_t llBP = llBoxPos;
		uint8_t nShift = 0, nBox = 0;
		while (llBP) {
			if (llBP & 1)
				aBoxBPos[nBox++] = nShift;
			++nShift;
			llBP >>= 1;
		}
	}
};
struct Storage {
	uint16_t					nPrty{ 1 };				//1+ priority
	Point							pt;
	//vector<uint64_t>	vDLpos;						//when final, creates ->new<- dead pos, that is cannot be the priority goal if any non-in-stg box is in that new DPos!
};
struct SokoCfg {
	bool       bRpt_Sol{ true };
	bool       bRpt_PIC_Merge{ true };
	bool       bRpt_UIMode{ true };			//show nice Unicode drawings instead of XSB codes

	uint16_t        nRSM_Depth{ 0 };
	uint16_t        nRSM_GBRelax{ 1 };
	uint16_t        nDFS_MaxDepth{ 0 };
	uint16_t        nRpt_SQInc{ 1000 };
	wstring         wsSearch;            //Algorithm
	wstring         wsRpt_Path;          //default is null/Console
	vector<wstring> vPuzzles;            //list of puzzles; >=1
};

//abstract/pure
struct __declspec(novtable) IStageQueue abstract {
	virtual ~IStageQueue() {}
	//ATTS
	virtual bool IsEmpty() const = 0;
	virtual uint32_t Size() const = 0;
	virtual bool HasStage(const Stage& stage) const = 0;
	virtual const Stage* Parent(const Stage& stage) const = 0; //null if root
	//METHODS
	virtual void Clear() = 0;
	virtual Stage Pop() = 0;
	virtual const Stage& Top() const = 0;
	virtual void Push(const Stage& stage) = 0;
};
//abstract/pure
struct __declspec(novtable) IBitMgr45 abstract {
	virtual ~IBitMgr45() {}
	virtual int64_t GetBits23() = 0;      //pack of 5 x (1+2*3)=7bits = 35 bits
};
