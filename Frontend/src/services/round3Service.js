/* eslint-disable no-undef */
import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Opcodes (MUST match C header opcode.h)
const OPCODE = {
    // Round 3 - Bonus Wheel (0x0640 - 0x065F)
    C2S_ROUND3_READY: 0x0640,
    C2S_ROUND3_PLAYER_READY: 0x0641,
    S2C_ROUND3_READY_STATUS: 0x0650,
    S2C_ROUND3_ALL_READY: 0x0651,
    C2S_ROUND3_SPIN: 0x0642,
    S2C_ROUND3_SPIN_RESULT: 0x0652,
    C2S_ROUND3_DECISION: 0x0643,
    S2C_ROUND3_DECISION_ACK: 0x0653,
    S2C_ROUND3_FINAL_RESULT: 0x0654,
    S2C_ROUND3_ALL_FINISHED: 0x0655,
    // Notification opcodes
    NTF_PLAYER_LEFT: 0x02BD
};

//==============================================================================
// 1. PLAYER READY
//==============================================================================
export function playerReady(matchId, playerId) {
    const buffer = new ArrayBuffer(8);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint32(4, playerId, false);
    
    console.log('[Round3] Player ready', { matchId, playerId });
    sendPacket(OPCODE.C2S_ROUND3_PLAYER_READY, buffer);
}

// Ready status update
registerHandler(OPCODE.S2C_ROUND3_READY_STATUS, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] Ready status update:', data);
    window.dispatchEvent(new CustomEvent('round3ReadyStatus', { detail: data }));
});

// All players ready - round starts
registerHandler(OPCODE.S2C_ROUND3_ALL_READY, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] All players ready, starting round:', data);
    window.dispatchEvent(new CustomEvent('round3AllReady', { detail: data }));
});

//==============================================================================
// 2. SPIN
//==============================================================================
export function requestSpin(matchId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    
    console.log('[Round3] Request spin', { matchId });
    sendPacket(OPCODE.C2S_ROUND3_SPIN, buffer);
}

// Spin result
registerHandler(OPCODE.S2C_ROUND3_SPIN_RESULT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] Spin result:', data);
    window.dispatchEvent(new CustomEvent('round3SpinResult', { detail: data }));
});

//==============================================================================
// 3. DECISION (Continue or Stop)
//==============================================================================
export function submitDecision(matchId, decision) {
    // decision: 0 = Stop, 1 = Continue
    const buffer = new ArrayBuffer(5);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    view.setUint8(4, decision ? 1 : 0);
    
    console.log('[Round3] Submit decision:', { matchId, decision: decision ? 'continue' : 'stop' });
    sendPacket(OPCODE.C2S_ROUND3_DECISION, buffer);
}

// Decision acknowledgment / Prompt
registerHandler(OPCODE.S2C_ROUND3_DECISION_ACK, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] Decision ack/prompt:', data);
    window.dispatchEvent(new CustomEvent('round3DecisionPrompt', { detail: data }));
});

// Final result (bonus calculated)
registerHandler(OPCODE.S2C_ROUND3_FINAL_RESULT, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] Final result:', data);
    window.dispatchEvent(new CustomEvent('round3FinalResult', { detail: data }));
    
    // Dispatch score update for GameContainer
    if (data.total_score !== undefined) {
        window.dispatchEvent(new CustomEvent('round3ScoreUpdate', {
            detail: { score: data.bonus, totalScore: data.total_score }
        }));
    }
});

//==============================================================================
// 4. ROUND FINISHED
//==============================================================================
registerHandler(OPCODE.S2C_ROUND3_ALL_FINISHED, (payload) => {
    const text = new TextDecoder().decode(payload);
    const data = JSON.parse(text);
    
    console.log('[Round3] All finished, final scoreboard:', data);
    window.dispatchEvent(new CustomEvent('round3AllFinished', { detail: data }));
});

console.log('[Round3Service] Loaded and handlers registered');
