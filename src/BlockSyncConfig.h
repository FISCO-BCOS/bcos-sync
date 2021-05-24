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
#include <bcos-framework/interfaces/dispatcher/DispatcherInterface.h>
#include <bcos-framework/interfaces/front/FrontServiceInterface.h>
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
        bcos::protocol::BlockFactory::Ptr _blockFactory,
        bcos::front::FrontServiceInterface::Ptr _frontService,
        bcos::dispatcher::DispatcherInterface::Ptr _dispatcher,
        BlockSyncMsgFactory::Ptr _msgFactory)
      : m_ledger(_ledger),
        m_nodeId(_nodeId),
        m_blockFactory(_blockFactory),
        m_frontService(_frontService),
        m_dispatcher(_dispatcher),
        m_msgFactory(_msgFactory)
    {}

    bcos::ledger::LedgerInterface::Ptr ledger() { return m_ledger; }
    bcos::crypto::PublicPtr nodeId() { return m_nodeId; }
    bcos::protocol::BlockFactory::Ptr blockFactory() { return m_blockFactory; }
    bcos::front::FrontServiceInterface::Ptr frontService() { return m_frontService; }
    bcos::dispatcher::DispatcherInterface::Ptr dispatcher() { return m_dispatcher; }
    BlockSyncMsgFactory::Ptr msgFactory() { return m_msgFactory; }

    bcos::crypto::HashType const& genesisHash() const { return m_genesisHash; }
    void setGenesisHash(bcos::crypto::HashType const& _hash);

    bcos::protocol::BlockNumber blockNumber() const { return m_blockNumber; }
    bcos::protocol::BlockNumber nextBlock() const { return m_nextBlock; }
    void setBlockNumber(bcos::protocol::BlockNumber _blockNumber);

    void setKnownHighestNumber(bcos::protocol::BlockNumber _highestNumber);
    bcos::protocol::BlockNumber knownHighestNumber() { return m_knownHighestNumber; }

    void setKnownLatestHash(bcos::crypto::HashType const& _hash);

    bcos::crypto::HashType const& knownLatestHash();

    size_t maxDownloadingBlockQueueSize() const { return m_maxDownloadingBlockQueueSize; }
    void setMaxDownloadingBlockQueueSize(size_t _maxDownloadingBlockQueueSize);

    void setMaxDownloadRequestQueueSize(size_t _maxDownloadRequestQueueSize);

    size_t maxDownloadRequestQueueSize() const { return m_maxDownloadRequestQueueSize; }

    size_t downloadTimeout() const { return m_downloadTimeout; }

    size_t maxRequestBlocks() const { return m_maxRequestBlocks; }
    size_t maxShardPerPeer() const { return m_maxShardPerPeer; }

    void setExecutedBlock(bcos::protocol::BlockNumber _executedBlock);
    bcos::protocol::BlockNumber executedBlock() { return m_executedBlock; }

private:
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    bcos::crypto::PublicPtr m_nodeId;
    bcos::protocol::BlockFactory::Ptr m_blockFactory;
    bcos::front::FrontServiceInterface::Ptr m_frontService;
    bcos::dispatcher::DispatcherInterface::Ptr m_dispatcher;
    BlockSyncMsgFactory::Ptr m_msgFactory;

    bcos::crypto::HashType m_genesisHash;
    std::atomic<bcos::protocol::BlockNumber> m_blockNumber = {0};
    std::atomic<bcos::protocol::BlockNumber> m_nextBlock = {0};
    std::atomic<bcos::protocol::BlockNumber> m_executedBlock = {0};

    std::atomic<bcos::protocol::BlockNumber> m_knownHighestNumber = {0};
    bcos::crypto::HashType m_knownLatestHash;
    mutable SharedMutex x_knownLatestHash;

    std::atomic<size_t> m_maxDownloadingBlockQueueSize = 256;
    std::atomic<size_t> m_maxDownloadRequestQueueSize = 1000;
    std::atomic<size_t> m_downloadTimeout = (200 * m_maxDownloadingBlockQueueSize);
    // the max number of blocks this node can requested to
    std::atomic<size_t> m_maxRequestBlocks = {8};

    std::atomic<size_t> m_maxShardPerPeer = {2};
};
}  // namespace sync
}  // namespace bcos