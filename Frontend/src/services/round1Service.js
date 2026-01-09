import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Opcodes (MUST match C header)
const OPCODE = {
    C2S_ROUND1_READY: 0x0601,
    S2C_ROUND1_START: 0x0611,
    C2S_ROUND1_GET_QUESTION: 0x0602,
    S2C_ROUND1_QUESTION: 0x0612,
    C2S_ROUND1_ANSWER: 0x0604,
    S2C_ROUND1_RESULT: 0x0614,
    C2S_ROUND1_END: 0x0605,
    S2C_ROUND1_END: 0x0615,
    // New sync opcodes
    C2S_ROUND1_PLAYER_READY: 0x0606,
    S2C_ROUND1_READY_STATUS: 0x0616,
    S2C_ROUND1_ALL_READY: 0x0617,
    C2S_ROUND1_FINISHED: 0x0607,
    S2C_ROUND1_WAITING: 0x0618,
    S2C_ROUND1_ALL_FINISHED: 0x0619,
    // Notification opcodes
    NTF_PLAYER_LEFT: 0x02BD,       // 701 decimal - player disconnected
    NTF_PLAYER_RECONNECTED: 0x02BE // 702 decimal - player reconnected
};

//==============================================================================
// 1. START ROUND
//==============================================================================
export function startRound1(matchId, roomId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, roomId, false);   // big-endian
    view.setUint32(4, matchId, false);
    
    console.log('[Round1] Starting round', { matchId, roomId });
    sendPacket(OPCODE.C2S_ROUND1_READY, buffer);
}

registerHandler(OPCODE.S2C_ROUND1_START, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] Round started:', data);
    
    // Trigger event for UI
    window.dispatchEvent(new CustomEvent('round1Started', { detail: data }));
});

//==============================================================================
// 2. GET QUESTION
//==============================================================================
export function getQuestion(matchId, questionIdx) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);

    // MUST match C GetQuestionPayload: { uint32_t match_id; uint32_t question_idx; }
    view.setUint32(0, Number(matchId), false);
    view.setUint32(4, Number(questionIdx), false);

    console.log('[Round1] Request question', { matchId, questionIdx });
    console.log('[Round1] Binary payload:', new Uint8Array(buffer));

    sendPacket(OPCODE.C2S_ROUND1_GET_QUESTION, buffer);
}

registerHandler(OPCODE.S2C_ROUND1_QUESTION, (payload) => {
    console.log('[Round1] Raw payload received:', new Uint8Array(payload));
    
    const text = new TextDecoder().decode(payload);
    console.log('[Round1] Decoded text:', text);
    
    try {
        const data = JSON.parse(text);
        console.log('[Round1] Parsed question:', data);
        
        window.dispatchEvent(new CustomEvent('round1Question', { detail: data }));
    } catch (e) {
        console.error('[Round1] JSON parse error:', e);
        console.error('[Round1] Invalid JSON:', text);
    }
});

//==============================================================================
// 3. SUBMIT ANSWER
//==============================================================================
export function submitAnswer(matchId, questionIdx, choiceIndex, timeTakenMs) {
    const buffer = new ArrayBuffer(13);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, questionIdx, false);
    view.setUint8(8, choiceIndex);
    view.setUint32(9, timeTakenMs, false);
    
    console.log('[Round1] Submit answer', { matchId, questionIdx, choiceIndex, timeTakenMs });
    sendPacket(OPCODE.C2S_ROUND1_ANSWER, buffer);
}

registerHandler(OPCODE.S2C_ROUND1_RESULT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] Answer result:', data);
    
    window.dispatchEvent(new CustomEvent('round1Result', { detail: data }));
});

//==============================================================================
// 4. END ROUND
//==============================================================================
export function endRound1(matchId, playerId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, playerId || 1, false);
    
    console.log('[Round1] End round', { matchId, playerId });
    sendPacket(OPCODE.C2S_ROUND1_END, buffer);
}

registerHandler(OPCODE.S2C_ROUND1_END, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] Round ended:', data);
    
    window.dispatchEvent(new CustomEvent('round1Ended', { detail: data }));
});

//==============================================================================
// 5. PLAYER READY (New multiplayer system)
//==============================================================================
export function playerReady(matchId, playerId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, playerId, false);
    
    console.log('[Round1] Player ready', { matchId, playerId });
    sendPacket(OPCODE.C2S_ROUND1_PLAYER_READY, buffer);
}

// Ready status update (broadcast to all players)
registerHandler(OPCODE.S2C_ROUND1_READY_STATUS, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] Ready status update:', data);
    window.dispatchEvent(new CustomEvent('round1ReadyStatus', { detail: data }));
});

// All players ready - game starts
registerHandler(OPCODE.S2C_ROUND1_ALL_READY, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] All players ready, starting game:', data);
    window.dispatchEvent(new CustomEvent('round1AllReady', { detail: data }));
});

// Waiting for other players
registerHandler(OPCODE.S2C_ROUND1_WAITING, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] Waiting for others:', data);
    window.dispatchEvent(new CustomEvent('round1Waiting', { detail: data }));
});

// All players finished - show final scoreboard
registerHandler(OPCODE.S2C_ROUND1_ALL_FINISHED, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round1] All players finished, final scoreboard:', data);
    window.dispatchEvent(new CustomEvent('round1AllFinished', { detail: data }));
});

// Player disconnected notification (NTF_PLAYER_LEFT = 701 = 0x02BD)
console.log('[Round1Service] Registering NTF_PLAYER_LEFT handler with opcode:', OPCODE.NTF_PLAYER_LEFT);
registerHandler(OPCODE.NTF_PLAYER_LEFT, (payload) => {
    const text = new TextDecoder().decode(payload);
    console.log('[Round1Service] 🔴🔴🔴 NTF_PLAYER_LEFT received! Raw:', text);
    try {
        const data = JSON.parse(text);
        console.log('[Round1Service] Player disconnect data parsed:', data);
        const eventDetail = { 
            playerId: data.player_id || data.playerId,
            playerName: data.name || data.playerName || `Player${data.player_id}`
        };
        console.log('[Round1Service] Dispatching playerDisconnected event:', eventDetail);
        window.dispatchEvent(new CustomEvent('playerDisconnected', { detail: eventDetail }));
        console.log('[Round1Service] ✅ playerDisconnected event dispatched!');
    } catch (e) {
        console.error('[Round1Service] Failed to parse disconnect data:', e, 'Raw text:', text);
    }
});

// Player reconnected notification (NTF_PLAYER_RECONNECTED = 702 = 0x02BE)
console.log('[Round1Service] Registering NTF_PLAYER_RECONNECTED handler with opcode:', OPCODE.NTF_PLAYER_RECONNECTED);
registerHandler(OPCODE.NTF_PLAYER_RECONNECTED, (payload) => {
    const text = new TextDecoder().decode(payload);
    console.log('[Round1Service] 🔄 NTF_PLAYER_RECONNECTED received!', text);
    try {
        const data = JSON.parse(text);
        const eventDetail = { 
            playerId: data.player_id || data.playerId,
            playerName: data.name || data.playerName || `Player${data.player_id}`
        };
        console.log('[Round1Service] Player reconnected:', eventDetail);
        window.dispatchEvent(new CustomEvent('playerReconnected', { detail: eventDetail }));
    } catch (e) {
        console.error('[Round1Service] Failed to parse reconnect data:', e);
    }
});