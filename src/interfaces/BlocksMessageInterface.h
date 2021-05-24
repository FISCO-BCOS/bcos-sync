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
 * @brief interface for the message contains blockData
 * @file BlocksMessageInterface.h
 * @author: yujiechen
 * @date 2021-05-24
 */
#pragma once
namespace bcos
{
namespace sync
{
class BlocksMessageInterface
{
public:
    using Ptr = std::shared_ptr<BlocksMessageInterface>;
    BlocksMessageInterface() = default;
    virtual ~BlocksMessageInterface() {}

    virtual bytesPointer encode() const = 0;
    virtual void decode(bytesConstRef) = 0;

    virtual int32_t version() const = 0;
    virtual size_t blocksSize() const = 0;
    virtual bytesConstRef blockData(size_t _index) const = 0;

    virtual void setVersion(int32_t _version) = 0;
    virtual void appendBlockData(bytes&& _blockData) = 0;
};
using BlocksMessageList = std::vector<BlocksMessageInterface::Ptr>;
using BlocksMessageListPtr = std::shared_ptr<BlocksMessageList>;
}  // namespace sync
}  // namespace bcos