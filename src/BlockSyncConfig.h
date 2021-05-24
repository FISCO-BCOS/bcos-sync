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
 * @file BlockSyncConfig.h
 * @author: yujiechen
 * @date 2021-05-24
 */
#pragma once
#include "interfaces/BlockSyncMsgFactory.h"
#include <bcos-framework/interfaces/crypto/KeyInterface.h>
#include <bcos-framework/interfaces/ledger/LedgerInterface.h>
#include <bcos-framework/interfaces/protocol/BlockFactory.h>
namespace bcos
{
namespace sync
{
class BlockSyncConfig
{
public:
    using Ptr = std::shared_ptr<BlockSyncConfig>;
    BlockSyncConfig(bcos::ledger::LedgerInterface::Ptr _ledger, bcos::crypto::PublicPtr _nodeId,
        bcos::protocol::BlockFactory::Ptr _blockFactory, BlockSyncMsgFactory::Ptr _msgFactory)
      : m_ledger(_ledger),
        m_nodeId(_nodeId),
        m_blockFactory(_blockFactory),
        m_msgFactory(_msgFactory)
    {}

    bcos::ledger::LedgerInterface::Ptr ledger() { return m_ledger; }
    bcos::crypto::PublicPtr nodeId() { return m_nodeId; }
    bcos::protocol::BlockFactory::Ptr blockFactory() { return m_blockFactory; }
    BlockSyncMsgFactory::Ptr msgFactory() { return m_msgFactory; }

    bcos::crypto::HashType const& genesisHash() const { return m_genesisHash; }
    void setGenesisHash(bcos::crypto::HashType const& _hash)
    {
        m_genesisHash = _hash;
        if (knownLatestHash() == bcos::crypto::HashType())
        {
            setKnownLatestHash(m_genesisHash);
        }
    }

    bcos::protocol::BlockNumber blockNumber() const { return m_blockNumber; }
    void setBlockNumber(bcos::protocol::BlockNumber _blockNumber)
    {
        m_blockNumber = _blockNumber;
        if (m_knownHighestNumber < _blockNumber)
        {
            m_knownHighestNumber = _blockNumber;
        }
    }

    virtual void setKnownHighestNumber(bcos::protocol::BlockNumber _highestNumber)
    {
        m_knownHighestNumber = _highestNumber;
    }
    virtual bcos::protocol::BlockNumber knownHighestNumber() { return m_knownHighestNumber; }

    virtual void setKnownLatestHash(bcos::crypto::HashType const& _hash)
    {
        WriteGuard l(x_knownLatestHash);
        m_knownLatestHash = _hash;
    }

    virtual bcos::crypto::HashType const& knownLatestHash()
    {
        ReadGuard l(x_knownLatestHash);
        return m_knownLatestHash;
    }


    size_t maxDownloadingBlockQueueSize() const { return m_maxDownloadingBlockQueueSize; }
    void setMaxDownloadingBlockQueueSize(size_t _maxDownloadingBlockQueueSize)
    {
        m_maxDownloadingBlockQueueSize = _maxDownloadingBlockQueueSize;
    }

    void setMaxDownloadRequestQueueSize(size_t _maxDownloadRequestQueueSize)
    {
        m_maxDownloadRequestQueueSize = _maxDownloadRequestQueueSize;
    }

    size_t maxDownloadRequestQueueSize() const { return m_maxDownloadRequestQueueSize; }

private:
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    bcos::crypto::PublicPtr m_nodeId;
    bcos::protocol::BlockFactory::Ptr m_blockFactory;
    BlockSyncMsgFactory::Ptr m_msgFactory;

    bcos::crypto::HashType m_genesisHash;
    std::atomic<bcos::protocol::BlockNumber> m_blockNumber = {0};

    std::atomic<bcos::protocol::BlockNumber> m_knownHighestNumber = {0};
    bcos::crypto::HashType m_knownLatestHash;
    mutable SharedMutex x_knownLatestHash;

    std::atomic<size_t> m_maxDownloadingBlockQueueSize = 256;
    std::atomic<size_t> m_maxDownloadRequestQueueSize = 1000;
};
}  // namespace sync
}  // namespace bcos