#include "world/chunk/IChunkProvider.h"

#include "world/chunk/Chunk.h"

namespace mc {

// Out of line so that users of the interface need not see Chunk.
std::unique_ptr<Chunk> IChunkProvider::generateChunk(int, int)
{
    return nullptr;
}

} // namespace mc
