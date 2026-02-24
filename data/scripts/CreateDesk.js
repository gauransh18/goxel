/*
 * Goxel Script: Detailed Student Desk
 * Features a large work surface and a side drawer unit.
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
    name: 'CreateDesk',
    description: 'Generates a student desk with built-in drawers',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // --- Colors (RGBA) ---
        const C_WOOD_TOP = [255, 235, 205, 255]; // Blanched Almond
        const C_WOOD_DARK = [80, 50, 30, 255];   // Dark Wood Frame
        const C_METAL     = [180, 180, 180, 255]; // Drawer Handles

        // 1. MAIN TABLE TOP (Blanched Almond)
        setBlock(volume, [-7, -5, 8], [7, 5, 8], C_WOOD_TOP);

        // 2. SUPPORT LEGS (Left Side)
        setBlock(volume, [-7, -5, 0], [-6, -4, 7], C_WOOD_DARK);
        setBlock(volume, [-7, 4, 0], [-6, 5, 7], C_WOOD_DARK);

        // 3. DRAWER UNIT (Right Side)
        // Solid block for the cabinet
        setBlock(volume, [4, -5, 0], [7, 5, 7], C_WOOD_DARK);

        // Drawer Seams (Creating visual separation)
        for (let z_seam of [2, 5]) {
            setBlock(volume, [4, -5, z_seam], [7, 5, z_seam], [40, 20, 10, 255]);
        }

        // 4. METAL HANDLES
        // Small metallic voxels on the front of the drawers
        volume.setAt([3, 0, 1], C_METAL);
        volume.setAt([3, 0, 4], C_METAL);
        volume.setAt([3, 0, 6], C_METAL);

        // 5. STRETCHER BAR (Stability bar at the back)
        setBlock(volume, [-7, -5, 1], [4, -5, 2], C_WOOD_DARK);
    }
});