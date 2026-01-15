import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

// Fallback opcodes if definition is missing in the generated file
const CMD_INVITE_FRIEND = OPCODE.CMD_INVITE_FRIEND ?? 0x0205;
const RES_SUCCESS = OPCODE.RES_SUCCESS ?? 0x00C8;
const ERR_BAD_REQUEST = OPCODE.ERR_BAD_REQUEST ?? 0x0190;
const ERR_NOT_LOGGED_IN = OPCODE.ERR_NOT_LOGGED_IN ?? 0x0191;
const ERR_NOT_FOUND = OPCODE.ERR_NOT_FOUND ?? 0x0194;

// Callback storage for async responses
const pendingCallbacks = new Map();

// Sequence number tracking for local request matching
// Note: The server sends generic RES_SUCCESS, so we rely on FIFO ordering for simple success/fail confirmation
// just like friendService.js does.
let lastSeqNum = 0;

function trackAndSend(sendFn) {
    const currentSeqNum = ++lastSeqNum;
    console.log("[InviteService] Tracking request #" + currentSeqNum);
    sendFn();
    return currentSeqNum;
}

function waitForResponse(requestId, timeoutMs = 10000) {
    let timeoutId;
    return new Promise((resolve) => {
        timeoutId = setTimeout(() => {
            pendingCallbacks.delete(requestId);
            console.error("[InviteService] Request timeout for #" + requestId);
            resolve({ success: false, error: "Request timeout" });
        }, timeoutMs);

        pendingCallbacks.set(requestId, { resolve, timeoutId });
    });
}

function resolvePending(response, typeString) {
    if (pendingCallbacks.size > 0) {
        // Resolve the oldest pending callback (FIFO)
        const [key, callback] = pendingCallbacks.entries().next().value;
        pendingCallbacks.delete(key);
        clearTimeout(callback.timeoutId);
        callback.resolve(response);
        console.log(`[InviteService] Resolved request #${key} with ${typeString}`);
    } else {
        // It's possible friendService or others consume RES_SUCCESS too.
        // If we get here, it means we have no pending invites, so ignore.
        // console.warn(`[InviteService] Received ${typeString} but no pending requests`);
    }
}

// Register Handlers
function setupHandlers() {
    // Success Handler
    registerHandler(RES_SUCCESS, (payload) => {
        // Invite Handler returns RES_SUCCESS on success with a plain text message "Invitation sent"
        const decoder = new TextDecoder();
        let message = "Success";
        try {
            const text = decoder.decode(payload);
            // Try to parse as JSON first (some successes might be JSON)
            try {
                const json = JSON.parse(text);
                message = json.message || text;
            } catch (e) {
                // Not JSON, use plain text
                message = text;
            }
        } catch (e) {
            console.warn("[InviteService] Failed to decode success payload");
        }

        resolvePending({ success: true, message }, "RES_SUCCESS");
    });

    // Error Handlers
    const errorOpcodes = [ERR_BAD_REQUEST, ERR_NOT_LOGGED_IN, ERR_NOT_FOUND];
    errorOpcodes.forEach(op => {
        registerHandler(op, (payload) => {
            // Helper to decode error message if strict format is used
            const decoder = new TextDecoder();
            const msg = decoder.decode(payload);
            resolvePending({ success: false, error: msg || "Error occurred" }, `Error 0x${op.toString(16)}`);
        });
    });
}

// Initialize handlers
setupHandlers();

/**
 * Invite a player to a room
 * @param {number} targetId - Account ID of the player to invite
 * @param {number} roomId - Room ID to invite them to
 * @returns {Promise<{success: boolean, error?: string}>}
 */
export async function invitePlayer(targetId, roomId) {
    console.log(`[InviteService] Inviting player ${targetId} to room ${roomId}`);

    if (!targetId || !roomId) {
        return { success: false, error: "Invalid target or room ID" };
    }

    // Ensure inputs are numbers
    const targetIdNum = parseInt(targetId);
    const roomIdNum = parseInt(roomId);

    if (isNaN(targetIdNum) || isNaN(roomIdNum)) {
        return { success: false, error: "IDs must be numeric" };
    }

    const payloadSize = 8; // 4 bytes target + 4 bytes room
    const buffer = new ArrayBuffer(payloadSize);
    const view = new DataView(buffer);

    // Layout: [target_id (4)] [room_id (4)] - Big Endian
    view.setInt32(0, targetIdNum, false);
    view.setUint32(4, roomIdNum, false);

    const sendFn = () => {
        sendPacket(CMD_INVITE_FRIEND, new Uint8Array(buffer));
    };

    const requestId = trackAndSend(sendFn);
    return waitForResponse(requestId);
}
