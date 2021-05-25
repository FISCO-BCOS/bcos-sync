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
 * @brief block sync implementation
 * @file BlockSync.cpp
 * @author: yujiechen
 * @date 2021-05-24
 */
#include "BlockSync.h"

using namespace bcos;
using namespace bcos::sync;
using namespace bcos::protocol;
using namespace bcos::crypto;
using namespace bcos::ledger;

BlockSync::BlockSync(BlockSyncConfig::Ptr _config)
  : Worker("syncWorker", 0),
    m_config(_config),
    m_syncStatus(std::make_shared<SyncPeerStatus>(_config))
{
    // TODO: create the timer and add the timeout logic
}

void BlockSync::start()
{
    if (m_running)
    {
        BLKSYNC_LOG(INFO) << LOG_DESC("BlockSync has already been started");
        return;
    }
    startWorking();
    m_running = true;
    BLKSYNC_LOG(INFO) << LOG_DESC("Start BlockSync");
}

void BlockSync::stop()
{
    if (!m_running)
    {
        BLKSYNC_LOG(INFO) << LOG_DESC("BlockSync has already been stopped");
        return;
    }
    BLKSYNC_LOG(INFO) << LOG_DESC("Stop BlockSync");
    m_running = false;
    finishWorker();
    if (isWorking())
    {
        // stop the worker thread
        stopWorking();
        terminate();
    }
}

void BlockSync::asyncNotifyBlockSyncMessage(Error::Ptr _error, NodeIDPtr _nodeID,
    bytesConstRef _data, std::function<void(bytesConstRef _respData)>,
    std::function<void(Error::Ptr _error)> _onRecv)
{
    if (_error != nullptr)
    {
        BLKSYNC_LOG(WARNING) << LOG_DESC("asyncNotifyBlockSyncMessage error")
                             << LOG_KV("code", _error->errorCode())
                             << LOG_KV("msg", _error->errorMessage());
        _onRecv(nullptr);
        return;
    }
    try
    {
        auto syncMsg = m_config->msgFactory()->createBlockSyncMsg(_data);
        switch (syncMsg->packetType())
        {
        case BlockSyncPacketType::BlockStatusPacket:
        {
            onPeerStatus(_nodeID, syncMsg);
            break;
        }
        case BlockSyncPacketType::BlockRequestPacket:
        {
            onPeerBlocksRequest(_nodeID, syncMsg);
            break;
        }
        case BlockSyncPacketType::BlockResponsePacket:
        {
            onPeerBlocks(_nodeID, syncMsg);
            break;
        }
        default:
        {
            BLKSYNC_LOG(WARNING) << LOG_DESC(
                                        "asyncNotifyBlockSyncMessage: unknown block sync message")
                                 << LOG_KV("type", syncMsg->packetType())
                                 << LOG_KV("peer", _nodeID->shortHex());
            break;
        }
        }
    }
    catch (std::exception const& e)
    {
        BLKSYNC_LOG(WARNING) << LOG_DESC("asyncNotifyBlockSyncMessage exception")
                             << LOG_KV("error", boost::diagnostic_information(e))
                             << LOG_KV("peer", _nodeID->shortHex());
    }
    _onRecv(nullptr);
}

void BlockSync::onPeerStatus(NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg)
{
    auto statusMsg = m_config->msgFactory()->createBlockSyncStatusMsg(_syncMsg);
    m_syncStatus->updatePeerStatus(_nodeID, statusMsg);
}

void BlockSync::onPeerBlocks(NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg)
{
    auto blockMsg = m_config->msgFactory()->createBlocksMsg(_syncMsg);
    BLKSYNC_LOG(DEBUG) << LOG_BADGE("Download") << LOG_BADGE("BlockSync")
                       << LOG_DESC("Receive peer block packet")
                       << LOG_KV("peer", _nodeID->shortHex());
    m_downloadingQueue->push(blockMsg);
    // TODO: notify worker to handle the request
}

void BlockSync::onPeerBlocksRequest(NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg)
{
    auto blockRequest = m_config->msgFactory()->createBlockRequest(_syncMsg);
    BLKSYNC_LOG(INFO) << LOG_BADGE("Download") << LOG_BADGE("onPeerBlocksRequest")
                      << LOG_DESC("Receive block request") << LOG_KV("peer", _nodeID->shortHex())
                      << LOG_KV("from", blockRequest->number())
                      << LOG_KV("size", blockRequest->size());
    auto peerStatus = m_syncStatus->peerStatus(_nodeID);
    if (peerStatus)
    {
        peerStatus->downloadRequests()->push(blockRequest->number(), blockRequest->size());
        // TODO: notify worker to handle the request
    }
}

void BlockSync::tryToRequestBlocks()
{
    // TODO: add timeout logic
    auto requestToNumber = m_config->knownHighestNumber();
    auto topBlock = m_downloadingQueue->top();
    // The block in BlockQueue is not nextBlock(the BlockQueue missing some block)
    if (topBlock && topBlock->blockHeader()->number() > m_config->nextBlock())
    {
        requestToNumber =
            std::min(m_config->knownHighestNumber(), (topBlock->blockHeader()->number() - 1));
    }
    auto currentNumber = m_config->blockNumber();
    // no need to request blocks
    if (currentNumber >= requestToNumber)
    {
        return;
    }
    requestBlocks(currentNumber, requestToNumber);
}

void BlockSync::requestBlocks(BlockNumber _from, BlockNumber _to)
{
    auto blockSizePerShard = m_config->maxRequestBlocks();
    auto shardNumber = (_to - _from + blockSizePerShard - 1) / blockSizePerShard;
    size_t shard = 0;
    // at most request `maxShardPerPeer` shards every time
    for (size_t loop = 0; loop < m_config->maxShardPerPeer() && shard < shardNumber; loop++)
    {
        bool findPeer = false;
        m_syncStatus->foreachPeerRandom([&](PeerStatus::Ptr _p) {
            if (_p->number() < m_config->knownHighestNumber())
            {
                // Only send request to nodes which are not syncing(has max number)
                return true;
            }
            // shard: [from, to]
            BlockNumber from = _from + 1 + shard * blockSizePerShard;
            BlockNumber to = std::min((BlockNumber)(from + blockSizePerShard - 1), _to);
            if (_p->number() < to)
            {
                return true;  // to next peer
            }
            // found a peer
            findPeer = true;
            auto blockRequest = m_config->msgFactory()->createBlockRequest();
            blockRequest->setNumber(from);
            blockRequest->setSize(to - from + 1);
            auto encodedData = blockRequest->encode();
            m_config->frontService()->asyncSendMessageByNodeID(
                ModuleID::BlockSync, _p->nodeId(), ref(*encodedData), 0, nullptr);

            BLKSYNC_LOG(INFO) << LOG_BADGE("Download") << LOG_BADGE("Request")
                              << LOG_DESC("Request blocks") << LOG_KV("from", from)
                              << LOG_KV("to", to) << LOG_KV("peer", _p->nodeId()->shortHex());

            ++shard;  // shard move
            return shard < shardNumber;
        });
        if (!findPeer)
        {
            BlockNumber from = _from + shard * blockSizePerShard;
            BlockNumber to = std::min((BlockNumber)(from + blockSizePerShard - 1), _to);
            BLKSYNC_LOG(WARNING) << LOG_BADGE("Download") << LOG_BADGE("Request")
                                 << LOG_DESC("Couldn't find any peers to request blocks")
                                 << LOG_KV("from", from) << LOG_KV("to", to);
            break;
        }
    }
}

void BlockSync::maintainDownloadingQueue()
{
    // TODO: wait operation
    if (m_config->blockNumber() >= m_config->knownHighestNumber())
    {
        m_downloadingQueue->clear();
        return;
    }
    auto block = m_downloadingQueue->top();
    if (block->blockHeader()->number() > m_config->nextBlock())
    {
        BLKSYNC_LOG(DEBUG) << LOG_DESC("Discontinuous block")
                           << LOG_KV("topNumber", block->blockHeader()->number())
                           << LOG_KV("curNumber", m_config->blockNumber());
        return;
    }
    auto executedBlock = m_config->executedBlock();
    // remove the expired block
    while (block->blockHeader()->number() <= executedBlock)
    {
        m_downloadingQueue->pop();
    }
    // execute the expected block
    block = m_downloadingQueue->top();
    while (block->blockHeader()->number() == (executedBlock + 1))
    {
        auto blockNumber = block->blockHeader()->number();
        m_downloadingQueue->applyBlock(block);
        m_config->setExecutedBlock(blockNumber);
        executedBlock = blockNumber;
        m_downloadingQueue->pop();
        block = m_downloadingQueue->top();
    }
}

void BlockSync::maintainBlockRequest()
{
    m_syncStatus->foreachPeerRandom([&](PeerStatus::Ptr _p) {
        auto reqQueue = _p->downloadRequests();
        // no need to respond
        if (reqQueue->empty())
        {
            return true;
        }
        while (!reqQueue->empty())
        {
            auto blocksReq = reqQueue->topAndPop();
            BlockNumber numberLimit = blocksReq->fromNumber() + blocksReq->size();
            BLKSYNC_LOG(DEBUG) << LOG_BADGE("Download Request: response blocks")
                               << LOG_KV("from", blocksReq->fromNumber())
                               << LOG_KV("size", blocksReq->size()) << LOG_KV("to", numberLimit)
                               << LOG_KV("peer", _p->nodeId()->shortHex());
            for (BlockNumber number = blocksReq->fromNumber(); number < numberLimit; number++)
            {
                fetchAndSendBlock(_p->nodeId(), number);
            }
        }
        return true;
    });
}

void BlockSync::fetchAndSendBlock(PublicPtr _peer, BlockNumber _number)
{
    // only fetch blockHeader and transactions
    auto blockFlag = HEADER | TRANSACTIONS;
    auto self = std::weak_ptr<BlockSync>(shared_from_this());
    m_config->ledger()->asyncGetBlockDataByNumber(
        _number, blockFlag, [self, _peer, _number](Error::Ptr _error, Block::Ptr _block) {
            if (_error != nullptr)
            {
                BLKSYNC_LOG(WARNING)
                    << LOG_DESC("fetchAndSendBlock failed for asyncGetBlockDataByNumber failed")
                    << LOG_KV("number", _number) << LOG_KV("errorCode", _error->errorCode())
                    << LOG_KV("errorMessage", _error->errorMessage());
                // TODO: do something when fetch failed
                return;
            }
            try
            {
                auto sync = self.lock();
                if (!sync)
                {
                    return;
                }
                auto config = sync->m_config;
                auto blocksReq = config->msgFactory()->createBlocksMsg();
                bytesPointer blockData = std::make_shared<bytes>();
                _block->encode(*blockData);
                blocksReq->appendBlockData(std::move(*blockData));
                config->frontService()->asyncSendMessageByNodeID(
                    ModuleID::BlockSync, _peer, ref(*(blocksReq->encode())), 0, nullptr);
                BLKSYNC_LOG(DEBUG)
                    << LOG_DESC("fetchAndSendBlock: response block")
                    << LOG_KV("toPeer", _peer->shortHex()) << LOG_KV("number", _number);
            }
            catch (std::exception const& e)
            {
                BLKSYNC_LOG(WARNING)
                    << LOG_DESC("fetchAndSendBlock exception") << LOG_KV("number", _number)
                    << LOG_KV("error", boost::diagnostic_information(e));
            }
        });
}

void BlockSync::maintainPeersConnection() {}