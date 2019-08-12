// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "arith_uint256.h"
#include "pow.h"
#include "chain.h"
#include "chainparams.h"
#include "validation.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util/system.h"

#include <math.h>

////////////////////////////////////////////////////////
uint256 GetProofOfStakeLimit(int nHeight)
{
    uint256 bnProofOfStakeLimit = uint256S("0x00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    uint256 bnProofOfStakeLimitV2 = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // lol
    if (IsProtocolV2(nHeight))
        return bnProofOfStakeLimitV2;
    else
        return bnProofOfStakeLimit;
}
////////////////////////////////////////////////////////

// Lux modified: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& consenusParams, bool fProofOfStake)
{
    int64_t nTargetSpacing = consenusParams.nPowTargetSpacing;
    int64_t nTargetTimespan = consenusParams.nPowTargetTimespan;
    uint256 bnTargetLimit(Params().ProofOfWorkLimit());
    if(fProofOfStake) {
        bnTargetLimit = GetProofOfStakeLimit(pindexLast->nHeight);
    }

    if (pindexLast == nullptr) // Lux Modified
        return UintToArith256(bnTargetLimit).GetCompact();

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == nullptr)
        return UintToArith256(bnTargetLimit).GetCompact();

    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == nullptr)
        return UintToArith256(bnTargetLimit).GetCompact();

    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits); // Replaced pindexLast to avoid bugs

    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > UintToArith256(bnTargetLimit))
        bnNew = UintToArith256(bnTargetLimit);

    return UintToArith256(bnTargetLimit).GetCompact();
}

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& consensusParams)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(Params().ProofOfWorkLimit())) return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget) return false; 

    return true;
}
