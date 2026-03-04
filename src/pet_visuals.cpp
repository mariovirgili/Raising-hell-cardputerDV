#include "pet_visuals.h"

#include "graphics_assets.h" // for your actual sprite dimensions

// -------------------------------------------
// VISUAL PROFILES FOR EACH PET TYPE
// -------------------------------------------
// These define:
//   • sprite width/height
//   • eye position offsets
//   • pupil radius
//   • eye spacing
//
// They ensure pet_renderer() + pet_eyes() work correctly
// -------------------------------------------

const PetVisualProfile PET_PROFILES[8] = {

    // PET_DEVIL
    {96, 96, 0, -6, 24, 7, 3, 0, 20},

    // PET_KAIJU
    {128, 128, 0, -10, 28, 8, 4, 0, 24},

    // PET_ELDRITCH
    {96, 96, 0, -8, 22, 6, 3, 0, 18},

    // PET_ALIEN
    {96, 96, 0, -6, 24, 7, 3, 0, 20},

    // PET_ANUBIS
    {96, 96, 0, -8, 26, 8, 3, 0, 18},

    // PET_AXOLOTL
    {96, 96, 0, -6, 24, 7, 3, 0, 20},

    // PET_PHOENIX
    {96, 96, 0, -8, 26, 7, 3, 0, 18},

    // PET_BANSHEE
    {96, 96, 0, -4, 20, 8, 4, 0, 22}
};
