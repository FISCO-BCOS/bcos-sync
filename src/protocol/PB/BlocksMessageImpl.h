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
 * @brief PB implementation for BlocksMessageInterface
 * @file BlocksMessageImpl.h
 * @author: yujiechen
 * @date 2021-05-24
 */
#pragma once
#include "interfaces/BlocksMessageInterface.h"
#include "protocol/proto/BlockSync.pb.h"
#include <bcos-framework/libprotocol/Common.h>
namespace bcos
{
namespace sync
{
class BlocksMessageImpl : public BlocksMessageInterface
{
public:
    using Ptr = std::shared_ptr<BlocksMessageImpl>;
    BlocksMessageImpl() : m_blocksMessage(std::shared_ptr<BlocksMessage>()) {}
    BlocksMessageImpl(bytesConstRef _data) : BlocksMessageImpl() { decode(_data); }
    ~BlocksMessageImpl() override {}


    bytesPointer encode() const override { return bcos::protocol::encodePBObject(m_blocksMessage); }
    void decode(bytesConstRef _data) override
    {
        bcos::protocol::decodePBObject(m_blocksMessage, _data);
    }

    int32_t version() const override { return m_blocksMessage->version(); }
    size_t blocksSize() const override { return m_blocksMessage->blocksdata_size(); }
    bytesConstRef blockData(size_t _index) const override
    {
        auto const& blockData = m_blocksMessage->blocks_data(_index);
        return bytesConstRef((byte const*)blockData.data(), blockData.size());
    }

    void setVersion(int32_t _version) override { m_blocksMessage->set_version(_version); }
    void appendBlockData(bytes&& _blockData) override
    {
        auto index = blocksSize();
        auto blockSize = _blockData.size();
        m_blocksMessage->add_blocksdata();
        m_blocksMessage->set_mutable_blocksdata(index, std::move(_blockData), blockSize);
    }

private:
    std::shared_ptr<BlocksMessage> m_blocksMessage;
};
}  // namespace sync
}  // namespace bcos