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
#include "bcos-sync/BlockSync.h"
#include <boost/bind.hpp>

using namespace bcos;
using namespace bcos::sync;
using namespace bcos::protocol;
using namespace bcos::crypto;
using namespace bcos::ledger;

BlockSync::BlockSync(BlockSyncConfig::Ptr _config, unsigned _idleWaitMs)
  : Worker("syncWorker", _idleWaitMs),
    m_config(_config),
    m_syncStatus(std::make_shared<SyncPeerStatus>(_config)),
    m_downloadingQueue(std::make_shared<DownloadingQueue>(_config))
{
    m_downloadBlockProcessor = std::make_shared<bcos::ThreadPool>("Download", 1);
    m_sendBlockProcessor = std::make_shared<bcos::ThreadPool>("SyncSend", 1);
    m_downloadingTimer = std::make_shared<Timer>(m_config->downloadTimeout());
    m_downloadingTimer->registerTimeoutHandler(boost::bind(&BlockSync::onDownloadTimeout, this));
    m_downloadingQueue->registerNewBlockHandler(boost::bind(&BlockSync::onNewBlock, this, _1));
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

void BlockSync::printSyncInfo()
{
    auto peers = m_syncStatus->peers();
    std::string peer_str;
    for (auto const& peer : *peers)
    {
        peer_str += peer->shortHex() + "/";
    }
    BLKSYNC_LOG(TRACE) << "\n[Sync Info] --------------------------------------------\n"
                       << "            IsSyncing:    " << isSyncing() << "\n"
                       << "            Block number: " << m_config->blockNumber() << "\n"
                       << "            Block hash:   " << m_config->hash().abridged() << "\n"
                       << "            Genesis hash: " << m_config->genesisHash().abridged() << "\n"
                       << "            Peers size:   " << peers->size() << "\n"
                       << "[Peer Info] --------------------------------------------\n"
                       << "    Host: " << m_config->nodeID()->shortHex() << "\n"
                       << "    Peer: " << peer_str << "\n"
                       << "            --------------------------------------------";
}

void BlockSync::executeWorker()
{
    if (isSyncing())
    {
        printSyncInfo();
    }
    // maintain the connections between observers/sealers
    maintainPeersConnection();
    m_downloadBlockProcessor->enqueue([this]() {
        try
        {
            // flush downloaded buffer into downloading queue
            maintainDownloadingBuffer();
            maintainDownloadingQueue();

            // send block-download-request to peers if this node is behind others
            tryToRequestBlocks();
        }
        catch (std::exception const& e)
        {
            BLKSYNC_LOG(ERROR) << LOG_DESC(
                                      "maintainDownloadingQueue or maintainPeersStatus exception")
                               << LOG_KV("errorInfo", boost::diagnostic_information(e));
        }
    });
    // send block to other nodes
    m_sendBlockProcessor->enqueue([this]() {
        try
        {
            maintainBlockRequest();
        }
        catch (std::exception const& e)
        {
            BLKSYNC_LOG(ERROR) << LOG_DESC("maintainBlockRequest exception")
                               << LOG_KV("errorInfo", boost::diagnostic_information(e));
        }
    });
}

void BlockSync::workerProcessLoop()
{
    while (workerState() == WorkerState::Started)
    {
        executeWorker();
        if (idleWaitMs())
        {
            boost::unique_lock<boost::mutex> l(x_signalled);
            m_signalled.wait_for(l, boost::chrono::milliseconds(idleWaitMs()));
        }
    }
}

bool BlockSync::shouldSyncing()
{
    if (m_config->blockNumber() >= m_config->knownHighestNumber())
    {
        return false;
    }
    if (m_config->executedBlock() >= m_config->knownHighestNumber())
    {
        return false;
    }
    return true;
}

bool BlockSync::isSyncing()
{
    return (m_state == SyncState::Downloading);
}

void BlockSync::maintainDownloadingBuffer()
{
    if (m_downloadingQueue->size() == 0)
    {
        return;
    }
    if (!shouldSyncing())
    {
        m_downloadingQueue->clear();
        return;
    }
    m_downloadingQueue->clearFullQueueIfNotHas(m_config->nextBlock());
    m_downloadingQueue->flushBufferToQueue();
}

void BlockSync::asyncNotifyBlockSyncMessage(Error::Ptr _error, NodeIDPtr _nodeID,
    bytesConstRef _data, std::function<void(bytesConstRef _respData)>,
    std::function<void(Error::Ptr _error)> _onRecv)
{
    if (_onRecv)
    {
        _onRecv(nullptr);
    }
    if (_error != nullptr)
    {
        BLKSYNC_LOG(WARNING) << LOG_DESC("asyncNotifyBlockSyncMessage error")
                             << LOG_KV("code", _error->errorCode())
                             << LOG_KV("msg", _error->errorMessage());
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
}

void BlockSync::asyncNotifyNewBlock(
    LedgerConfig::Ptr _ledgerConfig, std::function<void(Error::Ptr)> _onRecv)
{
    if (_onRecv)
    {
        _onRecv(nullptr);
    }
    BLKSYNC_LOG(DEBUG) << LOG_DESC("asyncNotifyNewBlock: receive new block info")
                       << LOG_KV("number", _ledgerConfig->blockNumber())
                       << LOG_KV("hash", _ledgerConfig->hash().abridged());
    if (_ledgerConfig->blockNumber() > m_config->blockNumber())
    {
        onNewBlock(_ledgerConfig);
    }
}

void BlockSync::onNewBlock(bcos::ledger::LedgerConfig::Ptr _ledgerConfig)
{
    m_config->resetConfig(_ledgerConfig);
    broadcastSyncStatus();
    m_downloadingQueue->clearExpiredQueueCache();
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
    m_signalled.notify_all();
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
        m_signalled.notify_all();
    }
}

void BlockSync::onDownloadTimeout()
{
    // stop the timer and reset the state to idle
    m_downloadingTimer->stop();
    m_state = SyncState::Idle;
}

void BlockSync::downloadFinish()
{
    m_downloadingTimer->stop();
    m_state = SyncState::Idle;
}

void BlockSync::tryToRequestBlocks()
{
    if (m_maxRequestNumber <= m_config->blockNumber() ||
        m_maxRequestNumber <= m_config->executedBlock())
    {
        downloadFinish();
    }
    if (!shouldSyncing() || isSyncing())
    {
        return;
    }
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
    m_state = SyncState::Downloading;
    m_downloadingTimer->start();

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

            m_maxRequestNumber = std::max(m_maxRequestNumber.load(), to);

            BLKSYNC_LOG(INFO) << LOG_BADGE("Download") << LOG_BADGE("Request")
                              << LOG_DESC("Request blocks") << LOG_KV("from", from)
                              << LOG_KV("to", to) << LOG_KV("peer", _p->nodeId()->shortHex())
                              << LOG_KV("node", m_config->nodeID()->shortHex());

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
    if (!shouldSyncing())
    {
        m_downloadingQueue->clear();
        downloadFinish();
        return;
    }
    auto executedBlock = m_config->executedBlock();
    // remove the expired block
    while (m_downloadingQueue->top() &&
           m_downloadingQueue->top()->blockHeader()->number() <= executedBlock)
    {
        m_downloadingQueue->pop();
    }
    if (!m_downloadingQueue->top())
    {
        return;
    }
    auto expectedBlock = executedBlock + 1;
    auto topNumber = m_downloadingQueue->top()->blockHeader()->number();
    if (topNumber > (expectedBlock))
    {
        BLKSYNC_LOG(WARNING) << LOG_DESC("Discontinuous block") << LOG_KV("topNumber", topNumber)
                             << LOG_KV("curNumber", m_config->blockNumber())
                             << LOG_KV("expectedBlock", expectedBlock)
                             << LOG_KV("node", m_config->nodeID()->shortHex());
        return;
    }
    // execute the expected block
    while (m_downloadingQueue->top() &&
           m_downloadingQueue->top()->blockHeader()->number() == (executedBlock + 1))
    {
        auto block = m_downloadingQueue->top();
        m_downloadingQueue->pop();
        m_state = SyncState::Downloading;
        auto blockNumber = block->blockHeader()->number();
        m_downloadingQueue->applyBlock(block);
        m_config->setExecutedBlock(blockNumber);
        executedBlock = blockNumber;
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
                               << LOG_KV("size", blocksReq->size()) << LOG_KV("to", numberLimit - 1)
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
                blocksReq->setNumber(_number);
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

void BlockSync::maintainPeersConnection()
{
    if (!m_config->existsInGroup())
    {
        return;
    }
    // Delete uncorrelated peers
    NodeIDs peersToDelete;
    m_syncStatus->foreachPeer([&](PeerStatus::Ptr _p) {
        if (!m_config->existsInGroup(_p->nodeId()) && m_config->blockNumber() >= _p->number())
        {
            // Only delete outsider whose number is smaller than myself
            peersToDelete.emplace_back(_p->nodeId());
        }
        return true;
    });
    // delete the invalid peer
    for (auto node : peersToDelete)
    {
        m_syncStatus->deletePeer(node);
    }
    // Add new peers
    auto groupNodeList = m_config->groupNodeList();
    for (auto node : groupNodeList)
    {
        // skip the node-self
        if (node->data() == m_config->nodeID()->data())
        {
            continue;
        }
        // create a peer
        auto newPeerStatus = m_config->msgFactory()->createBlockSyncStatusMsg(
            m_config->blockNumber(), m_config->hash(), m_config->genesisHash());
        m_syncStatus->updatePeerStatus(m_config->nodeID(), newPeerStatus);
        BLKSYNC_LOG(DEBUG) << LOG_BADGE("Status") << LOG_DESC("Send current status to new peer")
                           << LOG_KV("number", newPeerStatus->number())
                           << LOG_KV("genesisHash", newPeerStatus->genesisHash().abridged())
                           << LOG_KV("currentHash", newPeerStatus->hash().abridged())
                           << LOG_KV("peer", node->shortHex())
                           << LOG_KV("node", m_config->nodeID()->shortHex());
        auto encodedData = newPeerStatus->encode();
        m_config->frontService()->asyncSendMessageByNodeID(
            ModuleID::BlockSync, node, ref(*encodedData), 0, nullptr);
    }
}

void BlockSync::broadcastSyncStatus()
{
    auto peers = m_syncStatus->peers();
    for (auto const& _peer : *peers)
    {
        if (_peer->data() == m_config->nodeID()->data())
        {
            continue;
        }
        auto statusMsg = m_config->msgFactory()->createBlockSyncStatusMsg(
            m_config->blockNumber(), m_config->hash(), m_config->genesisHash());
        auto encodedData = statusMsg->encode();
        BLKSYNC_LOG(DEBUG) << LOG_BADGE("Status") << LOG_DESC("Send current status")
                           << LOG_KV("number", statusMsg->number())
                           << LOG_KV("genesisHash", statusMsg->genesisHash().abridged())
                           << LOG_KV("currentHash", statusMsg->hash().abridged())
                           << LOG_KV("peer", _peer->shortHex());
        m_config->frontService()->asyncSendMessageByNodeID(
            ModuleID::BlockSync, _peer, ref(*encodedData), 0, nullptr);
    }
}