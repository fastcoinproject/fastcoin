// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

// How many times we expect transactions after the last checkpoint to
 // be slower. This number is a compromise, as it can't be accurate for
 // every system. When reindexing from a fast disk with a slow CPU, it
 // can be up to 20, while when downloading from a slow network with a
 // fast multicore CPU, it won't be much higher than 1.
 static const double fSigcheckVerificationFactor = 5.0;

 struct CCheckpointData {
     const MapCheckpoints *mapCheckpoints;
     int64 nTimeLastCheckpoint;
     int64 nTransactionsLastCheckpoint;
     double fTransactionsPerDay;
 };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
            boost::assign::map_list_of
        (    0, uint256("0xecba185817b726ef62e53afb14241a8095bd9613d2d3df679911029b83c98e5b"))
        (    18, uint256("0x632aa100b8055344be4e765defd1a07a1e3d053eb67332c9a95045eb87b7f3ab"))
        (    70, uint256("0x9f0d4db126647e607a14da83f89b103f0fb5f29c6447c1574a011f7204b6a02f"))
        (   105, uint256("0xbf7bc864ecfb4803df34d1e05024654d9a65159c875f60fe07eeda2203ac734c"))
        (   200, uint256("0x3ba4bd313ce1a76e519ebc60c3e82154561c65c4b11ea82de1b3b0cc93f6eebb"))
        (299893, uint256("0xca07e64a675616af303ec911343b2473904334b043f8e972eb9dc03720995f01"))
        (599787, uint256("0x464cef0fed8c75ce46b48d19b97d70eb919edcca51523c793b830db1a710167e"))
        (1199646, uint256("0xce0e285eb16b6940dd7dd7b0fea58f3d93ffdfc7544ff1274ca5ff9496773903"))
        (2399216, uint256("0x4720496d86f9fea0a100c678d76192f753ba8da8f9c3d41eb739e479fa8e5bda"))
	(2899214, uint256("0x8652192d0905663bceed4c10d5759c2691a767768f1f80b30a9447cfa0978ba9"))
	(3380762, uint256("0x85207e898babf569cf7d59f06d3bbfa80f4041e2747d4b401367380d0f3cc985"))
	(3621535, uint256("0x0e69ac77ca6336a1d7faf5a37bbb68fe3305a601f1976cca35e9325bec9cd612"))
		
		
        
        ;
    static const CCheckpointData data = {
         &mapCheckpoints,
         1416303108, // * UNIX timestamp of last checkpoint block
         4483343,    // * total number of transactions between genesis and last checkpoint
                     //   (the tx=... number in the SetBestChain debug.log lines)
         7000.0     // * estimated number of transactions per day after checkpoint
     };

    const CCheckpointData &Checkpoints() {
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints

        MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
        if (i == mapCheckpoints.end()) return true;
        return hash == i->second;
    }


    // Guess how far we are in the verification process at the given block index
        double GuessVerificationProgress(CBlockIndex *pindex) {
            if (pindex==NULL)
                return 0.0;

            int64 nNow = time(NULL);

            double fWorkBefore = 0.0; // Amount of work done before pindex
            double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
            // Work is defined as: 1.0 per transaction before the last checkoint, and
            // fSigcheckVerificationFactor per transaction after.

            const CCheckpointData &data = Checkpoints();

            if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
                double nCheapBefore = pindex->nChainTx;
                double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
                double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
                fWorkBefore = nCheapBefore;
                fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
            } else {
                double nCheapBefore = data.nTransactionsLastCheckpoint;
                double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
                double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
                fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
                fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
            }

            return fWorkBefore / (fWorkBefore + fWorkAfter);
        }


    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0;
        return mapCheckpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
