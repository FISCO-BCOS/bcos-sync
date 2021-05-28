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
 * @file BlockSyncFactory.cpp
 * @author: yujiechen
 * @date 2021-05-28
 */
#include "BlockSyncFactory.h"
#include "protocol/PB/BlockSyncMsgFactoryImpl.h"
#include <bcos-framework/libtool/LedgerConfigFetcher.h>
using namespace bcos;
using namespace bcos::sync;
using namespace bcos::tool;

BlockSyncFactory::BlockSyncFactory(bcos::crypto::PublicPtr _nodeId,
    bcos::protocol::BlockFactory::Ptr _blockFactory, bcos::ledger::LedgerInterface::Ptr _ledger,
    bcos::front::FrontServiceInterface::Ptr _frontService,
    bcos::dispatcher::DispatcherInterface::Ptr _dispatcher,
    bcos::consensus::ConsensusInterface::Ptr _consensus)
  : m_ledger(_ledger)
{
    auto msgFactory = std::make_shared<BlockSyncMsgFactoryImpl>();
    m_syncConfig = std::make_shared<BlockSyncConfig>(
        _nodeId, _ledger, _blockFactory, _frontService, _dispatcher, _consensus, msgFactory);
    m_sync = std::make_shared<BlockSync>(m_syncConfig);
}

void BlockSyncFactory::init()
{
    auto fetcher = std::make_shared<LedgerConfigFetcher>(m_ledger);
    BLKSYNC_LOG(INFO) << LOG_DESC("start fetch the ledger config for block sync module");
    fetcher->fetchBlockNumberAndHash();
    fetcher->fetchConsensusNodeList();
    fetcher->fetchObserverNodeList();
    fetcher->waitFetchFinished();
    BLKSYNC_LOG(INFO) << LOG_DESC("fetch the ledger config for block sync module success")
                      << LOG_KV("number", fetcher->ledgerConfig()->blockNumber())
                      << LOG_KV("latestHash", fetcher->ledgerConfig()->hash().abridged());

    // set the syncConfig
    m_syncConfig->resetConfig(fetcher->ledgerConfig());
    BLKSYNC_LOG(INFO) << LOG_DESC("init block sync success");
}
