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
 * @file BlockSync.h
 * @author: yujiechen
 * @date 2021-05-24
 */
#pragma once
#include "BlockSyncConfig.h"
#include "interfaces/BlockSyncInterface.h"
#include "state/DownloadingQueue.h"
#include "state/SyncPeerStatus.h"
#include <bcos-framework/libutilities/Worker.h>
namespace bcos
{
namespace sync
{
class BlockSync : public BlockSyncInterface,
                  public Worker,
                  public std::enable_shared_from_this<BlockSync>
{
public:
    using Ptr = std::shared_ptr<BlockSync>;
    explicit BlockSync(BlockSyncConfig::Ptr _config);
    ~BlockSync() override {}

    void start() override;
    void stop() override;

    // called by the frontService to dispatch message
    // TODO: move this interface into bcos-framework
    virtual void asyncNotifyBlockSyncMessage(Error::Ptr _error, bcos::crypto::NodeIDPtr _nodeID,
        bytesConstRef _data, std::function<void(bytesConstRef _respData)> _sendResponse,
        std::function<void(Error::Ptr _error)> _onRecv);

protected:
    // for message handle
    virtual void onPeerStatus(bcos::crypto::NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg);
    virtual void onPeerBlocks(bcos::crypto::NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg);
    virtual void onPeerBlocksRequest(
        bcos::crypto::NodeIDPtr _nodeID, BlockSyncMsgInterface::Ptr _syncMsg);


    virtual bool isSyncing();
    // TODO: timeout logic
    virtual void tryToRequestBlocks();
    // block execute and submit
    virtual void maintainDownloadingQueue();
    // TODO: maintainPeersConnection
    virtual void maintainPeersConnection();
    virtual void maintainBlockRequest();

private:
    void requestBlocks(bcos::protocol::BlockNumber _from, bcos::protocol::BlockNumber _to);
    void fetchAndSendBlock(bcos::crypto::PublicPtr _peer, bcos::protocol::BlockNumber _number);

private:
    BlockSyncConfig::Ptr m_config;
    SyncPeerStatus::Ptr m_syncStatus;
    DownloadingQueue::Ptr m_downloadingQueue;

    std::atomic_bool m_running = {false};

    boost::condition_variable m_signalled;
    boost::mutex x_signalled;
};
}  // namespace sync
}  // namespace bcos