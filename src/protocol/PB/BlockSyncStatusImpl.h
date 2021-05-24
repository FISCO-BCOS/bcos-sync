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
 * @brief implementation for the block sync status packet
 * @file BlockSyncStatusImpl.h
 * @author: yujiechen
 * @date 2021-05-23
 */
#pragma once
#include "interfaces/BlockSyncStatusInterface.h"
#include "protocol/proto/BlockSync.pb.h"
namespace bcos
{
namespace sync
{
class BlockSyncStatusImpl : public BlockSyncStatusInterface
{
public:
    using Ptr = std::shared_ptr<BlockSyncStatusImpl>;
    BlockSyncStatusImpl() : m_blockSyncStatus(std::make_shared<BlockSyncStatus>()) {}
    explicit BlockSyncStatusImpl(bytesConstRef _data);
    ~BlockSyncStatusImpl() override {}

    bytesPointer encode() override;
    void decode(bytesConstRef _data) override;

    bcos::protocol::BlockNumber number() override;
    bcos::crypto::HashType const& hash() override;
    bcos::crypto::HashType const& genesisHash() override;

    void setNumber(bcos::protocol::BlockNumber _number) override;
    void setHash(bcos::crypto::HashType const& _hash) override;
    void setGenesisHash(bcos::crypto::HashType const& _gensisHash) override;

protected:
    virtual void deserializeObject();

private:
    std::shared_ptr<BlockSyncStatus> m_blockSyncStatus;
    bcos::crypto::HashType m_hash;
    bcos::crypto::HashType m_genesisHash;
};
}  // namespace sync
}  // namespace bcos