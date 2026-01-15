/* eslint-disable no-undef */
/**
 * endGameService.js - End Game Service
 * 
 * PURPOSE:
 * Handles communication for the End Game screen.
 * Receives final rankings and allows players to return to lobby.
 * 
 * SCORING MODE:
 * - Case 1: No bonus → Rankings by score
 * - Case 2: With bonus → Bonus winner = TOP 1
 * 
 * ELIMINATION MODE:
 * - Rankings by elimination order (winner first, then by round eliminated)
 */

import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Opcodes (MUST match C header opcode.h)
const OPCODE = {
    // End Game (0x0680 - 0x068F)
    C2S_END_GAME_READY: 0x0680,
    C2S_END_GAME_BACK_LOBBY: 0x0681,
    S2C_END_GAME_RESULT: 0x0690,
    S2C_END_GAME_RANKINGS: 0x0691,
    S2C_END_GAME_BACK_ACK: 0x0692
};

//==============================================================================
// 1. END GAME READY - Request current end game state
//==============================================================================
export function endGameReady(matchId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    
    console.log('[SERVICE] <endGame> endGameReady called', { matchId });
    sendPacket(OPCODE.C2S_END_GAME_READY, buffer);
}

// End game result response
registerHandler(OPCODE.S2C_END_GAME_RESULT, (payload) => {
    try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        
        console.log('[SERVICE] <endGame> End game result received:', data);
        window.dispatchEvent(new CustomEvent('endGameResult', { detail: data }));
    } catch (err) {
        console.error('[SERVICE] <endGame> Failed to parse end game result:', err);
    }
});

// End game rankings (alternative event)
registerHandler(OPCODE.S2C_END_GAME_RANKINGS, (payload) => {
    try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        
        console.log('[SERVICE] <endGame> End game rankings received:', data);
        window.dispatchEvent(new CustomEvent('endGameRankings', { detail: data }));
    } catch (err) {
        console.error('[SERVICE] <endGame> Failed to parse rankings:', err);
    }
});

//==============================================================================
// 2. BACK TO LOBBY
//==============================================================================
export function backToLobby(matchId) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    view.setUint32(0, matchId, false);
    
    console.log('[SERVICE] <endGame> backToLobby called', { matchId });
    sendPacket(OPCODE.C2S_END_GAME_BACK_LOBBY, buffer);
}

// Back to lobby acknowledgment
registerHandler(OPCODE.S2C_END_GAME_BACK_ACK, (payload) => {
    try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        
        console.log('[SERVICE] <endGame> Back to lobby acknowledged:', data);
        window.dispatchEvent(new CustomEvent('endGameBackAck', { detail: data }));
    } catch (err) {
        console.error('[SERVICE] <endGame> Failed to parse back ack:', err);
    }
});

console.log('[SERVICE] <endGame> EndGameService loaded and handlers registered');
