/**
 * hostService.js - Host use cases with BINARY payload encoding
 * 
 * IMPORTANT: All payloads must match C struct definitions in payloads.h
 */

import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

//==============================================================================
// HELPER: Encode string to fixed-size buffer (null-terminated)
//==============================================================================
function encodeString(str, maxLength) {
    const encoder = new TextEncoder();
    const truncated = str.substring(0, maxLength - 1); // Reserve 1 byte for \0
    const bytes = encoder.encode(truncated);

    const buffer = new Uint8Array(maxLength);
    buffer.fill(0); // Zero out entire array
    buffer.set(bytes); // Copy string bytes (auto null-terminated)

    return buffer;
}

//==============================================================================
// 1. CREATE ROOM
//==============================================================================

/**
 * CreateRoomPayload struct (36 bytes total) - UPDATED:
 * - char name[32]
 * - uint8_t mode
 * - uint8_t max_players
 * - uint8_t visibility
 * - uint8_t wager_mode
 */
export function createRoom(roomData) {
    const { name, visibility, mode, maxPlayers, wagerEnabled } = roomData;

    const buffer = new ArrayBuffer(36);
    const view = new DataView(buffer);

    // Encode name (32 bytes, null-terminated)
    const nameBytes = encodeString(name || "New Room", 32);
    new Uint8Array(buffer, 0, 32).set(nameBytes);

    // Pack fields (order: mode, max_players, visibility, wager_mode)
    view.setUint8(32, mode === 'elimination' ? 0 : 1);  // MODE_ELIMINATION=0, MODE_SCORING=1
    view.setUint8(33, maxPlayers || 6);
    view.setUint8(34, visibility === 'private' ? 1 : 0); // ROOM_PUBLIC=0, ROOM_PRIVATE=1
    view.setUint8(35, wagerEnabled ? 1 : 0);

    console.log('[CLIENT] [CREATE_ROOM] Sending:', { name, visibility, mode, maxPlayers, wagerEnabled, payloadSize: 36 });
    sendPacket(OPCODE.CMD_CREATE_ROOM, buffer);
}

registerHandler(OPCODE.RES_ROOM_CREATED, (payload) => {
    console.log('[CLIENT] [CREATE_ROOM] Response received, payload size:', payload.byteLength);

    // Validate payload size
    if (payload.byteLength !== 12) {
        console.error('[CLIENT] [CREATE_ROOM] Invalid payload size:', payload.byteLength, 'expected 12');
        alert('Failed to create room: Invalid server response');
        return;
    }

    // Parse binary payload
    // payload is ArrayBuffer (from receiver.js slice)
    const view = new DataView(payload);

    // Read room_id (4 bytes, network byte order = big-endian)
    const roomId = view.getUint32(0, false);

    // Read room_code (8 bytes, null-terminated string)
    // Create Uint8Array view regarding the same buffer to read bytes
    const codeBytes = new Uint8Array(payload, 4, 8);
    const roomCode = new TextDecoder()
        .decode(codeBytes)
        .replace(/\0/g, ''); // Remove null terminators

    console.log('[CLIENT] [CREATE_ROOM] ✅ SUCCESS:', { roomId, roomCode });

    // Store room info
    localStorage.setItem('current_room_id', roomId.toString());
    sessionStorage.setItem('room_code', roomCode);

    // Dispatch event for UI to handle navigation (using React Router)
    // using window.location.href would cause a reload and disconnect the socket!
    window.dispatchEvent(new CustomEvent('room_created', {
        detail: { roomId, roomCode }
    }));
});

//==============================================================================
// 2. CLOSE ROOM
//==============================================================================

/**
 * RoomIDPayload struct (4 bytes):
 * - uint32_t room_id (network byte order)
 */
export function closeRoom(roomId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false); // Big-endian (network byte order)

    console.log('[Host] Closing room:', roomId);
    sendPacket(OPCODE.CMD_CLOSE_ROOM, buffer);
}

registerHandler(OPCODE.RES_ROOM_CLOSED, (payload) => {
    const text = new TextDecoder().decode(payload);
    let jsonStr = text.substring(text.indexOf('\r\n\r\n') + 4);

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Host] Room closed');
            localStorage.removeItem('current_room_id');
            window.location.href = '/lobby';
        } else {
            alert('Failed to close room: ' + response.error);
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
});

//==============================================================================
// 3. SET RULES
//==============================================================================

/**
 * SetRulesPayload struct (4 bytes):
 * - uint8_t mode
 * - uint8_t max_players
 * - uint8_t visibility
 * - uint8_t wager_mode
 */
export function setRules(roomId, rules) {
    return new Promise((resolve, reject) => {
        const buffer = new ArrayBuffer(4);
        const view = new DataView(buffer);

        // Byte 0: mode (0=elimination, 1=scoring)
        view.setUint8(0, rules.mode === 'scoring' ? 1 : 0);

        // Byte 1: max_players
        view.setUint8(1, rules.maxPlayers || 4);

        // Byte 2: visibility (0=public, 1=private)
        view.setUint8(2, rules.visibility === 'private' ? 1 : 0);

        // Byte 3: wager_mode
        view.setUint8(3, rules.wagerMode ? 1 : 0);

        console.log('[HOST_SERVICE] setRules:', rules, 'payload size:', 4);

        // Register one-time handlers for this request
        const successHandler = (payload) => {
            console.log('[HOST_SERVICE] ✅ Rules updated successfully');
            resolve();
        };

        const errorHandler = (payload) => {
            const text = new TextDecoder().decode(payload);
            let errorMsg = "Failed to update rules";
            try {
                const data = JSON.parse(text);
                errorMsg = data.error || data.message || text;
            } catch (e) {
                errorMsg = text;
            }
            console.error('[HOST_SERVICE] ❌ Rules update failed:', errorMsg);
            reject(new Error(errorMsg));
        };

        // Register handlers (these will be called once)
        registerHandler(OPCODE.RES_RULES_UPDATED, successHandler);
        registerHandler(OPCODE.ERR_BAD_REQUEST, errorHandler);

        sendPacket(OPCODE.CMD_SET_RULE, new Uint8Array(buffer));
    });
}

// Remove old handler registration (now done inside setRules function)
// registerHandler(OPCODE.RES_RULES_UPDATED, (payload) => {
//     console.log('[HOST_SERVICE] ✅ Rules updated successfully');
//     // UI will be updated via NTF_RULES_CHANGED
// });

//==============================================================================
// 4. KICK MEMBER
//==============================================================================

/**
 * Kick một member khỏi phòng (chỉ host)
 * KickMemberPayload struct (4 bytes):
 * - uint32_t target_account_id (network byte order)
 */
export function kickMember(targetAccountId) {
    console.log('[HOST_SERVICE] Kicking member:', targetAccountId);

    const buffer = new ArrayBuffer(4);  // ✅ 4 bytes
    const view = new DataView(buffer);
    view.setUint32(0, targetAccountId, false);  // ✅ Chỉ target_account_id

    sendPacket(OPCODE.CMD_KICK, buffer);
}


registerHandler(OPCODE.RES_MEMBER_KICKED, (payload) => {
    console.log('[HOST_SERVICE] ✅ Member kicked successfully');
    // Payload rỗng, không cần parse
    // UI sẽ tự động cập nhật qua NTF_PLAYER_LEFT và NTF_PLAYER_LIST
});

//==============================================================================
// 5. START GAME
//==============================================================================

/**
 * StartGamePayload struct (4 bytes):
 * - uint32_t room_id
 */
export function startGame(roomId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false);

    console.log('[Host] Starting game:', roomId);
    sendPacket(OPCODE.CMD_START_GAME, buffer);
}

registerHandler(OPCODE.RES_GAME_STARTED, (payload) => {
    const text = new TextDecoder().decode(payload);
    let jsonStr = text.substring(text.indexOf('\r\n\r\n') + 4);

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Host] Game started, match_id:', response.match_id);
            localStorage.setItem('current_match_id', response.match_id);

            // Wait for NTF_GAME_START and NTF_ROUND_START
            alert('Game starting... Get ready!');
        } else {
            alert('Failed to start game: ' + response.error);
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
});

//==============================================================================
// 6. LEAVE ROOM (Member)
//==============================================================================

/**
 * RoomIDPayload struct (4 bytes):
 * - uint32_t room_id
 */
export function leaveRoom(roomId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false);

    console.log('[Member] Leaving room:', roomId);
    sendPacket(OPCODE.CMD_LEAVE_ROOM, buffer);
}

registerHandler(OPCODE.RES_ROOM_LEFT, (payload) => {
    const text = new TextDecoder().decode(payload);
    let jsonStr = text.substring(text.indexOf('\r\n\r\n') + 4);

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Member] Left room');
            localStorage.removeItem('current_room_id');
            window.location.href = '/lobby';
        }
    } catch (e) {
        console.error('[Member] Parse error:', e);
    }
});

//==============================================================================
// NOTIFICATIONS
//==============================================================================

registerHandler(OPCODE.NTF_GAME_START, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);

    console.log('[Notification] Game starting:', data);
    // Show countdown UI: 3... 2... 1...
});

registerHandler(OPCODE.NTF_ROUND_START, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);

    console.log('[Notification] Round started:', data);
    localStorage.setItem('current_round', data.round);

    // Navigate to game screen
    window.location.href = '/round';
});

registerHandler(OPCODE.NTF_RULES_CHANGED, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);

    console.log('[Notification] Rules changed:', data);
    alert('Host changed the rules!');
    // TODO: Update UI to reflect new rules
});

registerHandler(OPCODE.NTF_MEMBER_KICKED, (payload) => {
    console.log('[Notification] You were kicked!');
    alert('You have been kicked from the room');
    localStorage.removeItem('current_room_id');
    window.location.href = '/lobby';
});

registerHandler(OPCODE.NTF_ROOM_CLOSED, (payload) => {
    console.log('[Notification] Room closed by host');
    alert('Room has been closed by the host');
    localStorage.removeItem('current_room_id');
    window.location.href = '/lobby';
});

registerHandler(OPCODE.NTF_PLAYER_LEFT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);

    console.log('[Notification] Player left:', data);
    // TODO: Update member list UI
});
