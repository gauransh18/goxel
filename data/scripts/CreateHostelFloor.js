/*
 * Goxel Script: Hostel Room Floor
 * Creates a voxel-style floor with a concrete border,
 * checker tiles, and a simple rug accent.
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
    name: 'CreateHostelFloor',
    description: 'Generates a voxel-style hostel room floor',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // Room footprint (single-floor plane at z = 0)
        const xMin = -14;
        const xMax = 14;
        const yMin = -10;
        const yMax = 10;
        const z = 0;

        // Colors (RGBA)
        const C_CONCRETE = [120, 120, 125, 255];
        const C_TILE_A   = [168, 152, 132, 255];
        const C_TILE_B   = [146, 132, 114, 255];
        const C_WORN     = [126, 114, 100, 255];
        const C_RUG      = [110, 40, 45, 255];
        const C_RUG_EDGE = [160, 120, 85, 255];

        // 1) Outer concrete border
        setBlock(volume, [xMin, yMin, z], [xMax, yMax, z], C_CONCRETE);

        // 2) Inner tile area (checker pattern)
        for (let x = xMin + 1; x <= xMax - 1; x++) {
            for (let y = yMin + 1; y <= yMax - 1; y++) {
                const isEven = ((x + y) & 1) === 0;
                volume.setAt([x, y, z], isEven ? C_TILE_A : C_TILE_B);
            }
        }

        // 3) Worn patches for hostel look
        const wornSpots = [
            [-9, -6], [-8, -6], [-3, -1], [0, 0], [4, 3], [7, -2], [8, -2], [10, 5]
        ];
        for (let i = 0; i < wornSpots.length; i++) {
            const p = wornSpots[i];
            volume.setAt([p[0], p[1], z], C_WORN);
        }

        // 4) Narrow bedside rug strip
        setBlock(volume, [xMin + 2, yMax - 3, z], [xMin + 9, yMax - 2, z], C_RUG);
        setBlock(volume, [xMin + 1, yMax - 3, z], [xMin + 1, yMax - 2, z], C_RUG_EDGE);
        setBlock(volume, [xMin + 10, yMax - 3, z], [xMin + 10, yMax - 2, z], C_RUG_EDGE);
    }
});
