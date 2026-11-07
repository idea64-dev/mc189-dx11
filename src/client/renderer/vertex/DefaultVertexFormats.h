#pragma once

// Original: net/minecraft/client/renderer/vertex/DefaultVertexFormats.java

#include "client/renderer/vertex/VertexFormat.h"

namespace mc {

class DefaultVertexFormats {
    using E = VertexFormatElement;
    using T = VertexFormatElement::EnumType;
    using U = VertexFormatElement::EnumUsage;

public:
    static inline const E POSITION_3F { 0, T::FLOAT, U::POSITION, 3 };
    static inline const E COLOR_4UB { 0, T::UBYTE, U::COLOR, 4 };
    static inline const E TEX_2F { 0, T::FLOAT, U::UV, 2 };
    static inline const E TEX_2S { 1, T::SHORT, U::UV, 2 };
    static inline const E NORMAL_3B { 0, T::BYTE, U::NORMAL, 3 };
    static inline const E PADDING_1B { 0, T::BYTE, U::PADDING, 1 };

    static inline const VertexFormat BLOCK { POSITION_3F, COLOR_4UB, TEX_2F, TEX_2S };
    static inline const VertexFormat ITEM { POSITION_3F, COLOR_4UB, TEX_2F, NORMAL_3B, PADDING_1B };
    static inline const VertexFormat OLDMODEL_POSITION_TEX_NORMAL { POSITION_3F, TEX_2F, NORMAL_3B, PADDING_1B };
    static inline const VertexFormat PARTICLE_POSITION_TEX_COLOR_LMAP { POSITION_3F, TEX_2F, COLOR_4UB, TEX_2S };
    static inline const VertexFormat POSITION { POSITION_3F };
    static inline const VertexFormat POSITION_COLOR { POSITION_3F, COLOR_4UB };
    static inline const VertexFormat POSITION_TEX { POSITION_3F, TEX_2F };
    static inline const VertexFormat POSITION_NORMAL { POSITION_3F, NORMAL_3B, PADDING_1B };
    static inline const VertexFormat POSITION_TEX_COLOR { POSITION_3F, TEX_2F, COLOR_4UB };
    static inline const VertexFormat POSITION_TEX_NORMAL { POSITION_3F, TEX_2F, NORMAL_3B, PADDING_1B };
    static inline const VertexFormat POSITION_TEX_LMAP_COLOR { POSITION_3F, TEX_2F, TEX_2S, COLOR_4UB };
    static inline const VertexFormat POSITION_TEX_COLOR_NORMAL { POSITION_3F, TEX_2F, COLOR_4UB, NORMAL_3B, PADDING_1B };
};

} // namespace mc
