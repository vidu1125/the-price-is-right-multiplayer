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
 * CreateRoomPayload struct (72 bytes total):
 * - char name[64]
 * - uint8_t visibility
 * - uint8_t mode
 * - uint8_t max_players
 * - uint8_t round_time
 * - uint8_t wager_enabled
 * - uint8_t reserved[3]
 */
export function createRoom(roomData) {
    const { name, visibility, mode, maxPlayers, roundTime, wagerEnabled } = roomData;

    const buffer = new ArrayBuffer(72);
    const view = new DataView(buffer);

    // Encode name (64 bytes, null-terminated)
    const nameBytes = encodeString(name || "New Room", 64);
    new Uint8Array(buffer, 0, 64).set(nameBytes);

    // Pack fields
    view.setUint8(64, visibility === 'private' ? 1 : 0);
    view.setUint8(65, mode === 'elimination' ? 1 : 0);
    view.setUint8(66, maxPlayers || 6);
    view.setUint8(67, roundTime || 15);
    view.setUint8(68, wagerEnabled ? 1 : 0);
    // reserved[3] = already zeros

    console.log('[Host] Creating room:', { name, visibility, mode, maxPlayers, roundTime, wagerEnabled });
    sendPacket(OPCODE.CMD_CREATE_ROOM, buffer);
}

registerHandler(OPCODE.RES_ROOM_CREATED, (payload) => {
    const text = new TextDecoder().decode(payload);

    // Skip HTTP headers if present
    let jsonStr = text;
    const bodyStart = text.indexOf('\r\n\r\n');
    if (bodyStart !== -1) {
        jsonStr = text.substring(bodyStart + 4);
    }

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Host] Room created:', response);
            localStorage.setItem('current_room_id', response.room_id.toString());
            // Store code too if needed
            sessionStorage.setItem('room_code', response.room_code);
            window.location.href = '/waitingroom';
        } else {
            alert('Failed to create room: ' + response.error);
        }
    } catch (e) {
        console.error('[Host] Parse error:', e, jsonStr);
    }
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
 * SetRulesPayload struct (8 bytes):
 * - uint32_t room_id
 * - uint8_t mode
 * - uint8_t max_players
 * - uint8_t round_time
 * - uint8_t wager_enabled
 */
export function setRules(roomId, rules) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false); // Network byte order
    view.setUint8(4, rules.mode === 'elimination' ? 1 : 0);
    view.setUint8(5, rules.maxPlayers || 6);
    view.setUint8(6, rules.roundTime || 15);
    view.setUint8(7, rules.wagerEnabled ? 1 : 0);

    console.log('[Host] Setting rules:', rules);
    sendPacket(OPCODE.CMD_SET_RULE, buffer);
}

registerHandler(OPCODE.RES_RULES_UPDATED, (payload) => {
    const text = new TextDecoder().decode(payload);
    let jsonStr = text.substring(text.indexOf('\r\n\r\n') + 4);

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Host] Rules updated:', response.rules);
            // TODO: Update UI to reflect new rules
            alert('Rules updated successfully!');
        } else {
            alert('Failed to update rules: ' + response.error);
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
});

//==============================================================================
// 4. KICK MEMBER
//==============================================================================

/**
 * KickMemberPayload struct (8 bytes):
 * - uint32_t room_id
 * - uint32_t target_id
 */
export function kickMember(roomId, targetId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false);
    view.setUint32(4, targetId, false);

    console.log('[Host] Kicking member:', targetId);
    sendPacket(OPCODE.CMD_KICK, buffer);
}

registerHandler(OPCODE.RES_MEMBER_KICKED, (payload) => {
    const text = new TextDecoder().decode(payload);
    let jsonStr = text.substring(text.indexOf('\r\n\r\n') + 4);

    try {
        const response = JSON.parse(jsonStr);

        if (response.success) {
            console.log('[Host] Member kicked');
            alert('Member kicked successfully');
            // TODO: Refresh member list
        } else {
            alert('Failed to kick member: ' + response.error);
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
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
