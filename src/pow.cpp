// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();
	int64_t nTargetTimespan = Params().TargetTimespan();
	int64_t nTargetSpacing = Params().TargetSpacing();
    int64_t nInterval = nTargetTimespan / nTargetSpacing;	// 300
	
	
	
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;
	
    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        // Special difficulty rule for testnet:
        if (Params().AllowMinDifficultyBlocks())
        {
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
		
        return pindexLast->nBits;
    }
	
    // Fastcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = nInterval-1;
    if ((pindexLast->nHeight+1) != nInterval)
        blockstogoback = nInterval;
	
    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);
	
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    //printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);
	
	if ((pindexLast->nHeight+1) < 1250)
	{
		if (nActualTimespan < nTargetTimespan/32)
			nActualTimespan = nTargetTimespan/32;
	}
	else if ((pindexLast->nHeight+1) < 4000)
	{
		if (nActualTimespan < nTargetTimespan/8)
			nActualTimespan = nTargetTimespan/8;
	}
	else
	{
		if (nActualTimespan < nTargetTimespan/4)
			nActualTimespan = nTargetTimespan/4;
	}
	
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;
	
    // Retarget
    uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
	
    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();
	
	
    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

	return true;
	
    if (Params().SkipProofOfWorkCheck())
       return true;

   bnTarget.SetCompact(nBits);

    // Check range
	if (bnTarget <= 0 || bnTarget > Params().ProofOfWorkLimit())
		return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
