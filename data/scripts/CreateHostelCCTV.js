/*
 * Goxel Script: Hostel CCTV Camera
 * Generates a detailed CCTV camera mounted near the room corner.
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
    name: 'CreateHostelCCTV',
    description: 'Generates a detailed hostel CCTV camera with mount and wiring',
    onExecute: function() {
        let volume = goxel.image.activeLayer.volume;

        // Positioned near north-east inner corner, facing inward
        const baseX = 13;
        const baseY = 9;
        const baseZ = 8;

        // Colors (RGBA)
        const C_METAL_LIGHT = [175, 178, 184, 255];
        const C_METAL_DARK  = [118, 122, 130, 255];
        const C_PLASTIC     = [88, 92, 100, 255];
        const C_BLACK       = [25, 25, 28, 255];
        const C_GLASS       = [85, 145, 190, 255];
        const C_LED_RED     = [220, 45, 45, 255];
        const C_LED_IR      = [185, 165, 165, 255];
        const C_CABLE       = [62, 58, 50, 255];

        // 1) Wall mounting plate
        setBlock(volume, [baseX, baseY, baseZ], [baseX, baseY, baseZ + 2], C_METAL_DARK);

        // Bolt heads on plate
        volume.setAt([baseX, baseY, baseZ], C_METAL_LIGHT);
        volume.setAt([baseX, baseY, baseZ + 2], C_METAL_LIGHT);

        // 2) Swivel joint + arm
        volume.setAt([baseX - 1, baseY, baseZ + 1], C_METAL_LIGHT);
        setBlock(volume, [baseX - 2, baseY, baseZ + 1], [baseX - 3, baseY, baseZ + 1], C_METAL_DARK);
        volume.setAt([baseX - 4, baseY, baseZ + 1], C_METAL_LIGHT); // neck joint

        // 3) Camera body (box style)
        setBlock(volume, [baseX - 8, baseY - 1, baseZ], [baseX - 5, baseY + 1, baseZ + 2], C_PLASTIC);

        // Top shade/hood for weatherproof look
        setBlock(volume, [baseX - 9, baseY - 1, baseZ + 3], [baseX - 5, baseY + 1, baseZ + 3], C_METAL_DARK);

        // Side panel accents
        setBlock(volume, [baseX - 8, baseY - 1, baseZ], [baseX - 8, baseY - 1, baseZ + 2], C_METAL_DARK);
        setBlock(volume, [baseX - 8, baseY + 1, baseZ], [baseX - 8, baseY + 1, baseZ + 2], C_METAL_DARK);

        // 4) Lens assembly (front face points toward -X)
        setBlock(volume, [baseX - 9, baseY, baseZ + 1], [baseX - 9, baseY, baseZ + 1], C_GLASS);
        setBlock(volume, [baseX - 10, baseY, baseZ + 1], [baseX - 10, baseY, baseZ + 1], C_BLACK);

        // IR LED ring around lens
        volume.setAt([baseX - 9, baseY - 1, baseZ + 1], C_LED_IR);
        volume.setAt([baseX - 9, baseY + 1, baseZ + 1], C_LED_IR);
        volume.setAt([baseX - 9, baseY, baseZ], C_LED_IR);
        volume.setAt([baseX - 9, baseY, baseZ + 2], C_LED_IR);

        // 5) Rear status LED + access panel
        volume.setAt([baseX - 5, baseY + 1, baseZ + 1], C_LED_RED);
        setBlock(volume, [baseX - 5, baseY - 1, baseZ], [baseX - 5, baseY - 1, baseZ + 2], C_METAL_DARK);

        // 6) Cable from plate upward to ceiling direction
        setBlock(volume, [baseX, baseY, baseZ + 2], [baseX, baseY, baseZ + 4], C_CABLE);
        setBlock(volume, [baseX, baseY, baseZ + 4], [baseX, baseY - 2, baseZ + 4], C_CABLE);

        // 7) Small anti-tamper bracket under the arm
        setBlock(volume, [baseX - 3, baseY, baseZ], [baseX - 2, baseY, baseZ], C_METAL_DARK);
    }
});
