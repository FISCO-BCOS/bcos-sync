/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief config for the block sync
 * @file BlockSyncConfig.cpp
 * @author: yujiechen
 * @date 2021-05-25
 */
#include "BlockSyncConfig.h"
using namespace bcos;
using namespace bcos::sync;
using namespace bcos::crypto;
using namespace bcos::protocol;

void BlockSyncConfig::setGenesisHash(HashType const& _hash)
{
    m_genesisHash = _hash;
    if (knownLatestHash() == HashType())
    {
        setKnownLatestHash(m_genesisHash);
    }
}

void BlockSyncConfig::setBlockNumber(BlockNumber _blockNumber)
{
    m_blockNumber = _blockNumber;
    m_nextBlock = m_blockNumber + 1;
    if (m_knownHighestNumber < _blockNumber)
    {
        m_knownHighestNumber = _blockNumber;
    }
    if (_blockNumber > m_executedBlock)
    {
        m_executedBlock = _blockNumber;
    }
}


void BlockSyncConfig::setKnownHighestNumber(BlockNumber _highestNumber)
{
    m_knownHighestNumber = _highestNumber;
}

void BlockSyncConfig::setKnownLatestHash(HashType const& _hash)
{
    WriteGuard l(x_knownLatestHash);
    m_knownLatestHash = _hash;
}

HashType const& BlockSyncConfig::knownLatestHash()
{
    ReadGuard l(x_knownLatestHash);
    return m_knownLatestHash;
}

void BlockSyncConfig::setMaxDownloadingBlockQueueSize(size_t _maxDownloadingBlockQueueSize)
{
    m_maxDownloadingBlockQueueSize = _maxDownloadingBlockQueueSize;
}

void BlockSyncConfig::setMaxDownloadRequestQueueSize(size_t _maxDownloadRequestQueueSize)
{
    m_maxDownloadRequestQueueSize = _maxDownloadRequestQueueSize;
}

void BlockSyncConfig::setExecutedBlock(BlockNumber _executedBlock)
{
    if (m_blockNumber <= _executedBlock)
    {
        m_executedBlock = _executedBlock;
    }
}