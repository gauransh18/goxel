/*
 * Goxel Script: Hostel Door
 * Creates a hostel-style door with:
 * - Inside handle and lock
 * - Outside knob
 */

function setBlock(volume, start, end, color) {
    for (let x = start[0]; x <= end[0]; x++) {
        for (let y = start[1]; y <= end[1]; y++) {
            for (let z = start[2]; z <= end[2]; z++) {
                volume.setAt([x, y, z], color);
            }
        }
    }
}

goxel.registerScript({
    name: 'CreateHostelDoor',
    description: 'Generates a larger detailed hostel door with inside handle+lock and outside knob',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // Matches the door opening from CreateHostelWalls (east wall side)
        const doorX = 14;
        const y1 = -3;
        const y2 = 2;
        const z1 = 1;
        const z2 = 7;

        // Colors (RGBA)
        const C_WOOD      = [92, 62, 40, 255];
        const C_WOOD_DARK = [72, 47, 30, 255];
        const C_WOOD_MID  = [82, 54, 35, 255];
        const C_METAL     = [170, 170, 175, 255];
        const C_LOCK      = [110, 110, 118, 255];
        const C_METAL_DARK = [130, 130, 136, 255];

        // 1) Door slab (1-voxel thick, larger)
        for (let y = y1; y <= y2; y++) {
            for (let z = z1; z <= z2; z++) {
                const grain = ((y * 7 + z * 5) % 5 < 2) ? C_WOOD_DARK : C_WOOD;
                volume.setAt([doorX, y, z], grain);
            }
        }

        // 2) Door edge bands and panel details
        // Perimeter trim on slab face
        for (let y = y1; y <= y2; y++) {
            volume.setAt([doorX, y, z1], C_WOOD_DARK);
            volume.setAt([doorX, y, z2], C_WOOD_DARK);
        }
        for (let z = z1; z <= z2; z++) {
            volume.setAt([doorX, y1, z], C_WOOD_DARK);
            volume.setAt([doorX, y2, z], C_WOOD_DARK);
        }

        // Recessed central panels
        for (let y = y1 + 1; y <= y2 - 1; y++) {
            for (let z = z1 + 1; z <= z2 - 1; z++) {
                const inUpperPanel = (z >= 4 && z <= 6);
                const inLowerPanel = (z >= 2 && z <= 3);
                if (inUpperPanel || inLowerPanel) {
                    volume.setAt([doorX, y, z], C_WOOD_MID);
                }
            }
        }

        // 3) Hinges on left side of door (inside-facing)
        volume.setAt([doorX - 1, y1, 2], C_METAL_DARK);
        volume.setAt([doorX - 1, y1, 4], C_METAL_DARK);
        volume.setAt([doorX - 1, y1, 6], C_METAL_DARK);

        // 4) Inside hardware (inside is toward lower X, room interior)
        // Lever handle with short stem
        setBlock(volume, [doorX - 1, 1, 3], [doorX - 1, 1, 4], C_METAL);
        volume.setAt([doorX - 1, 2, 3], C_METAL_DARK);

        // Lock plate + key slot
        setBlock(volume, [doorX - 1, 1, 2], [doorX - 1, 1, 2], C_LOCK);
        volume.setAt([doorX - 1, 0, 2], C_METAL_DARK);

        // 5) Outside hardware (outside is toward higher X)
        // Knob cluster + stem
        volume.setAt([doorX + 1, 1, 3], C_METAL);
        volume.setAt([doorX + 1, 1, 4], C_METAL);
        volume.setAt([doorX + 1, 2, 3], C_METAL);
        volume.setAt([doorX + 2, 1, 3], C_METAL_DARK);

        // 6) Vent strip near top (common utilitarian hostel detail)
        for (let y = y1 + 1; y <= y2 - 1; y++) {
            if (((y + 20) % 2) === 0) {
                volume.setAt([doorX, y, z2 - 1], C_WOOD_DARK);
            }
        }

        // 7) Slight worn strip near base
        for (let y = y1; y <= y2; y++) {
            if (((y + 10) % 2) === 0 || y === y1 || y === y2) {
                volume.setAt([doorX, y, z1], C_WOOD_DARK);
            }
        }
    }
});
