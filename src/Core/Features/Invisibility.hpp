#pragma once
#include <Includes/Includes.hpp>

// ============================================================
//  Fluffy-FiveM | Invisibility
//  Sets the local player CPed's m_alpha (opacity) to 0,
//  making them invisible to other players on screen.
//  Alpha offset: 0x2A (standard GTA V / FiveM CPed offset)
// ============================================================

namespace Core::Features {

    struct cInvisibility {

        // CPed + 0x2A = m_alpha (uint8, 0 = invisible, 255 = fully visible)
        static constexpr int AlphaOffset = 0x2A;

        inline static bool Enabled = false;
        inline static bool Running = false;

        static void Apply(bool invisible) {
            CPed* pLocal = Core::SDK::Pointers::pLocalPlayer;
            if (!pLocal) return;

            uint8_t alpha = invisible ? 0u : 255u;
            Mem.Write<uint8_t>((uintptr_t)pLocal + AlphaOffset, alpha);
        }

        static void Start() {
            Running = true;
            bool lastState = false;

            while (Running) {
                if (Enabled != lastState) {
                    Apply(Enabled);
                    lastState = Enabled;
                }

                // Re-apply every 500ms to fight server resets
                if (Enabled) {
                    Apply(true);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    };

    inline cInvisibility g_Invisibility;
}
