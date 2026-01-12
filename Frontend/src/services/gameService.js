import { request } from "./dispatcher";

/**
 * Start a match in the specified room
 * @param {number} roomId - The room ID
 * @returns {Promise<{success: boolean, match_id?: number, error?: string}>}
 */
export async function startGame(roomId) {
    if (!roomId || roomId <= 0) {
        console.error("[gameService] Invalid room ID:", roomId);
        return { success: false, error: "Invalid room ID" };
    }

    console.log(`[gameService] Starting game in room ${roomId}...`);

    // Build binary payload: uint32_t room_id (Big Endian)
    const payload = new ArrayBuffer(4);
    const view = new DataView(payload);
    view.setUint32(0, roomId, false); // Big Endian (network byte order)

    try {
        const response = await request(0x0300, payload); // CMD_START_GAME

        if (response.command === 0x0301) {
            // RES_GAME_STARTED (assuming 0x0301)
            console.log("✅ [gameService] Game started successfully");

            // Parse response payload if needed (e.g., match_id)
            // For now, assume success if we get the response
            return { success: true };
        } else {
            // Error response
            const decoder = new TextDecoder();
            const errorMsg = decoder.decode(response.payload);
            console.error("❌ [gameService] Start game failed:", errorMsg);
            return { success: false, error: errorMsg };
        }
    } catch (error) {
        console.error("❌ [gameService] Start game error:", error);
        return { success: false, error: error.message };
    }
}

/**
 * Leave/end the current match
 * @param {number} matchId - The match ID
 * @returns {Promise<{success: boolean}>}
 */
export async function endMatch(matchId) {
    console.log(`[gameService] Ending match ${matchId}...`);

    // TODO: Implement end match logic
    // For now, return placeholder
    return { success: false, error: "Not implemented" };
}
