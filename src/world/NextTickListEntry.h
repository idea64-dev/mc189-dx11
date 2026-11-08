#pragma once

// Original: net/minecraft/world/NextTickListEntry.java and block/BlockEventData.java

#include "block/Block.h"
#include "util/BlockPos.h"

#include <cstdint>

namespace mc {

struct NextTickListEntry {
    NextTickListEntry(const BlockPos& positionIn, Block* blockIn)
        : position(positionIn)
        , block(blockIn)
        , tickEntryID(nextTickEntryID++)
    {
    }

    NextTickListEntry& setScheduledTime(int64_t scheduledTimeIn)
    {
        scheduledTime = scheduledTimeIn;
        return *this;
    }
    void setPriority(int priorityIn) { priority = priorityIn; }
    Block* getBlock() const { return block; }
    // Java: equals -- position and an associated block.
    bool equals(const NextTickListEntry& other) const { return position == other.position && Block::isEqualTo(block, other.block); }
    // Java: compareTo -- the TreeSet order.
    bool operator<(const NextTickListEntry& other) const
    {
        if (scheduledTime != other.scheduledTime)
            return scheduledTime < other.scheduledTime;
        if (priority != other.priority)
            return priority < other.priority;
        return tickEntryID < other.tickEntryID;
    }

    BlockPos position;
    int64_t scheduledTime = 0;
    int priority = 0;

private:
    // Java: a static counter shared by every world of the server.
    static inline int64_t nextTickEntryID = 0;

    Block* block;
    int64_t tickEntryID;
};

class BlockEventData {
public:
    BlockEventData(const BlockPos& pos, Block* blockType, int eventId, int eventParameter)
        : position(pos)
        , eventID(eventId)
        , eventParameter(eventParameter)
        , blockType(blockType)
    {
    }

    const BlockPos& getPosition() const { return position; }
    int getEventID() const { return eventID; }
    int getEventParameter() const { return eventParameter; }
    Block* getBlock() const { return blockType; }
    bool operator==(const BlockEventData& other) const
    {
        return position == other.position && eventID == other.eventID && eventParameter == other.eventParameter && blockType == other.blockType;
    }

private:
    BlockPos position;
    int eventID;
    int eventParameter;
    Block* blockType;
};

} // namespace mc
