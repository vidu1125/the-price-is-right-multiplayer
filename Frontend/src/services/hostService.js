/**
 * hostService.js - Host use cases with BINARY payload encoding
 * 
 * IMPORTANT: All payloads must match C struct definitions in payloads.h
 */

import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

//==============================================================================
// MODULE STATE: Cache latest snapshots for race condition fix
//==============================================================================
let latestRulesSnapshot = null;
let latestPlayersSnapshot = null;

export function getLatestRulesSnapshot() {
    return latestRulesSnapshot;
}

export function getLatestPlayersSnapshot() {
    return latestPlayersSnapshot;
}

export function clearSnapshots() {
    latestRulesSnapshot = null;
    latestPlayersSnapshot = null;
}

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
 * - uint8_t wager_enabled
 * - uint8_t reserved[4]
 */
export function createRoom(roomData) {
    const { name, visibility, mode, maxPlayers, wagerEnabled } = roomData;

    const buffer = new ArrayBuffer(72);
    const view = new DataView(buffer);

    // Encode name (64 bytes, null-terminated)
    const nameBytes = encodeString(name || "New Room", 64);
    new Uint8Array(buffer, 0, 64).set(nameBytes);

    // Pack fields
    view.setUint8(64, visibility === 'private' ? 1 : 0);
    view.setUint8(65, mode === 'elimination' ? 1 : 0);
    view.setUint8(66, maxPlayers || 6);
    view.setUint8(67, wagerEnabled ? 1 : 0);
    // reserved[4] = already zeros

    console.log('[Host] Creating room:', { name, visibility, mode, maxPlayers, wagerEnabled });
    sendPacket(OPCODE.CMD_CREATE_ROOM, buffer);
}

// NOTE: RES_ROOM_CREATED is handled in RoomPanel.js to avoid navigation conflicts

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
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));

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
 * - uint8_t wager_enabled
 * - uint8_t visibility
 */
export function setRules(roomId, rules) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false); // Network byte order
    view.setUint8(4, rules.mode === 'elimination' ? 1 : 0);
    view.setUint8(5, rules.maxPlayers || 6);
    view.setUint8(6, rules.wagerMode ? 1 : 0);
    view.setUint8(7, rules.visibility === 'private' ? 1 : 0);

    console.log('[Host] Setting rules:', rules);
    sendPacket(OPCODE.CMD_SET_RULE, buffer);
}

registerHandler(OPCODE.RES_RULES_UPDATED, (payload) => {
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));

        if (response.success) {
            console.log('[Host] Rules update request accepted');
            // Don't update UI here - wait for NTF_RULES_CHANGED
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
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));

        if (response.success) {
            console.log('[Host] Member kicked successfully, refreshing room state');
            
            // Pull fresh snapshot from DB to update UI
            const roomId = parseInt(localStorage.getItem('room_id'));
            if (roomId) {
                getRoomState(roomId);
            }
            
            alert('Member kicked successfully');
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

// Error handler (backup for legacy error codes)
registerHandler(OPCODE.ERR_BAD_REQUEST, (payload) => {
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));
        
        console.error('[Host] Start game error:', response);
        
        // Dispatch error event
        window.dispatchEvent(new CustomEvent('game-start-error', { 
            detail: { message: response.error || 'Unknown error' }
        }));
    } catch (e) {
        console.error('[Host] Parse error:', e);
        window.dispatchEvent(new CustomEvent('game-start-error', { 
            detail: { message: 'Failed to start game' }
        }));
    }
});

registerHandler(OPCODE.RES_GAME_STARTED, (payload) => {
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));

        if (response.success) {
            console.log('[Host] Game started successfully');
            
            // Dispatch event to show modal
            window.dispatchEvent(new CustomEvent('game-starting', { 
                detail: response 
            }));
        } else {
            console.error('[Host] Start game failed:', response.error);
            
            // Dispatch error event
            window.dispatchEvent(new CustomEvent('game-start-error', { 
                detail: { message: response.error }
            }));
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
});

//==============================================================================
// 6. GET ROOM STATE (Pull snapshot from DB)
//==============================================================================

/**
 * RoomIDPayload struct (4 bytes):
 * - uint32_t room_id
 */
export function getRoomState(roomId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);

    view.setUint32(0, roomId, false);

    console.log('[Host] Getting room state:', roomId);
    sendPacket(OPCODE.CMD_GET_ROOM_STATE, buffer);
}

registerHandler(OPCODE.RES_ROOM_STATE, (payload) => {
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Host] Room state received:', response);

        // Extract rules with proper field mapping
        if (response.rules) {
            const rules = {
                mode: response.rules.mode,
                maxPlayers: response.rules.maxPlayers || response.rules.max_players,
                wagerMode: response.rules.wagerMode !== undefined ? response.rules.wagerMode : response.rules.wager_mode,
                visibility: response.rules.visibility
            };
            window.dispatchEvent(new CustomEvent('rules-changed', { detail: rules }));
        }
        
        // Extract and normalize players
        if (response.players) {
            // Normalize: remove duplicates by account_id
            const uniquePlayers = Object.values(
                response.players.reduce((acc, player) => {
                    acc[player.account_id] = player;
                    return acc;
                }, {})
            );
            
            // Sort: host first, then by join order
            const sortedPlayers = uniquePlayers.sort((a, b) => {
                if (a.is_host) return -1;
                if (b.is_host) return 1;
                return 0;
            });
            
            console.log('[Host] Dispatching normalized player list:', sortedPlayers);
            window.dispatchEvent(new CustomEvent('player-list', { detail: sortedPlayers }));
        }
    } catch (e) {
        console.error('[Host] Parse error:', e);
    }
});

//==============================================================================
// 7. LEAVE ROOM (Member)
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
    try {
        const response = JSON.parse(new TextDecoder().decode(payload));

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
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Notification] Game starting:', data);
        // Show countdown UI: 3... 2... 1...
    } catch (e) {
        console.error('[NTF] Game start parse error:', e);
    }
});

registerHandler(OPCODE.NTF_ROUND_START, (payload) => {
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Notification] Round started:', data);
        localStorage.setItem('current_round', data.round);
        // Navigate to game screen
        window.location.href = '/round';
    } catch (e) {
        console.error('[NTF] Round start parse error:', e);
    }
});

registerHandler(OPCODE.NTF_RULES_CHANGED, (payload) => {
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Notification] Rules changed:', data);

        // Cache snapshot
        const rulesSnapshot = {
            mode: data.mode,
            maxPlayers: data.max_players,
            wagerMode: data.wager_mode,
            visibility: data.visibility
        };
        latestRulesSnapshot = rulesSnapshot;

        // Dispatch custom event for WaitingRoom to catch
        window.dispatchEvent(new CustomEvent('rules-changed', {
            detail: rulesSnapshot
        }));
    } catch (e) {
        console.error('[NTF] Rules changed parse error:', e);
    }
});

registerHandler(OPCODE.NTF_MEMBER_KICKED, (payload) => {
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        const myAccountId = parseInt(localStorage.getItem('account_id') || '1');
        
        console.log('[Notification] Member kicked:', data.account_id, 'Me:', myAccountId);
        
        // Only redirect if I was the one kicked
        if (data.account_id === myAccountId) {
            alert('You have been kicked from the room');
            localStorage.removeItem('room_id');
            localStorage.removeItem('room_code');
            localStorage.removeItem('room_name');
            localStorage.removeItem('is_host');
            window.location.href = '/lobby';
        }
        // Other members just wait for NTF_PLAYER_LIST update
    } catch (e) {
        console.error('[NTF] Member kicked parse error:', e);
    }
});

registerHandler(OPCODE.NTF_ROOM_CLOSED, (payload) => {
    console.log('[Notification] Room closed by host');
    alert('Room has been closed by the host');
    localStorage.removeItem('current_room_id');
    window.location.href = '/lobby';
});

registerHandler(OPCODE.NTF_PLAYER_LEFT, (payload) => {
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Notification] Player left:', data);

        // Dispatch event for WaitingRoom
        window.dispatchEvent(new CustomEvent('player-left', { detail: data }));
    } catch (e) {
        console.error('[NTF] Player left parse error:', e);
    }
});

registerHandler(OPCODE.NTF_PLAYER_LIST, (payload) => {
    try {
        const data = JSON.parse(new TextDecoder().decode(payload));
        console.log('[Notification] Player list raw:', data);
        
        const players = data.players || data || [];
        
        // Normalize: remove duplicates by account_id
        const uniquePlayers = Object.values(
            players.reduce((acc, player) => {
                acc[player.account_id] = player;
                return acc;
            }, {})
        );
        
        // Sort: host first, then by join order
        const sortedPlayers = uniquePlayers.sort((a, b) => {
            if (a.is_host) return -1;
            if (b.is_host) return 1;
            return 0;
        });
        
        console.log('[Notification] Normalized player list:', sortedPlayers);

        // Cache snapshot
        latestPlayersSnapshot = sortedPlayers;

        // Dispatch event for MemberListPanel (REPLACE state)
        window.dispatchEvent(new CustomEvent('player-list', {
            detail: sortedPlayers
        }));
    } catch (e) {
        console.error('[NTF] Player list parse error:', e);
    }
});
