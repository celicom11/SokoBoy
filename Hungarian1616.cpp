#include "StdAfx.h"
#include "Hungarian1616.h"
//https://e-maxx.ru/algo/assignment_hungary
namespace Hungarian {
#define INF 0xFFFF
#define MAXROWS 16
#define MAXCOLS 16
	uint16_t Min1616(int nRows, int nCols, IN OUT uint8_t(&aDisMatrix)[16][16]) {
		int aU[MAXROWS + 1], aV[MAXCOLS + 1], aP[MAXCOLS + 1], aWay[MAXCOLS + 1], aMinV[MAXCOLS + 1];
		memset(aU, 0, sizeof(aU));
		memset(aV, 0, sizeof(aV));
		memset(aP, 0, sizeof(aP));
		memset(aWay, 0, sizeof(aWay));
		for (int nR = 1; nR <= nRows; ++nR) {
			aP[0] = nR;
			int j0 = 0;
			for (int& nVal : aMinV) nVal = INF;
			bitset<16 + 1> bstUsed;
			do {
				bstUsed.set(j0);
				int i0 = aP[j0], delta = INF, j1 = 0;
				for (int j = 1; j <= nCols; ++j) {
					if (!bstUsed.test(j)) {
						int cur = aDisMatrix[i0 - 1][j - 1] - (aU[i0] + aV[j]);
						if (cur < aMinV[j]) {
							aMinV[j] = cur;
							aWay[j] = j0;
						}
						if (aMinV[j] < delta) {
							delta = aMinV[j];
							j1 = j;
						}
					}
				}//for
				//if (delta == 255)
				//	return INF;
				for (int j = 0; j <= nCols; ++j) {
					if (bstUsed.test(j)) {
						aU[aP[j]] += delta;
						aV[j] -= delta;
					}
					else {
						//if (aMinV[j] == INF)
						//	return 0xFFFF;
						aMinV[j] -= delta;
					}
				}
				if (aV[0] < -255)
					return INF;//cutoff
				j0 = j1;
			} while (aP[j0] != 0);

			do {
				int j1 = aWay[j0];
				aP[j0] = aP[j1];
				j0 = j1;
			} while (j0);
		}//for nR
		return uint16_t(-aV[0]);
	}
};