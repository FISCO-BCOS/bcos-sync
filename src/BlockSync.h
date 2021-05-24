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
namespace bcos
{
namespace sync
{
class BlockSync : public BlockSyncInterface
{
public:
    using Ptr = std::shared_ptr<BlockSync>;
    explicit BlockSync(BlockSyncConfig::Ptr _config) : m_config(_config) {}
    ~BlockSync() override {}

    void start() override;
    void stop() override;

private:
    BlockSyncConfig::Ptr m_config;
};
}  // namespace sync
}  // namespace bcos