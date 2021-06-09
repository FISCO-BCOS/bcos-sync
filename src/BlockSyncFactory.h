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
 * @brief factory to create BlockSync
 * @file BlockSyncFactory.h
 * @author: yujiechen
 * @date 2021-05-28
 */
#pragma once
#include "BlockSync.h"
#include "BlockSyncConfig.h"

namespace bcos
{
namespace sync
{
class BlockSyncFactory
{
public:
    using Ptr = std::shared_ptr<BlockSyncFactory>;
    BlockSyncFactory(bcos::crypto::PublicPtr _nodeId,
        bcos::protocol::BlockFactory::Ptr _blockFactory, bcos::ledger::LedgerInterface::Ptr _ledger,
        bcos::front::FrontServiceInterface::Ptr _frontService,
        bcos::dispatcher::DispatcherInterface::Ptr _dispatcher,
        bcos::consensus::ConsensusInterface::Ptr _consensus);
    virtual ~BlockSyncFactory() {}

    virtual void init();
    BlockSyncInterface::Ptr sync() { return m_sync; }

    BlockSyncConfig::Ptr syncConfig() { return m_syncConfig; }

protected:
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    BlockSync::Ptr m_sync;
    BlockSyncConfig::Ptr m_syncConfig;
};
}  // namespace sync
}  // namespace bcos