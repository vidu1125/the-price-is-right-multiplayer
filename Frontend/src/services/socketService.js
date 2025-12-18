/**
 * Socket Service for Frontend → Backend → C Server Communication
 * 
 * Architecture (TUÂN THỦ APPLICATION DESIGN):
 * 
 * React UI → HTTP/JSON → Python Backend → RAW TCP SOCKET → C Server
 *                                          (Binary Protocol)
 * 
 * Flow:
 * 1. React calls this service with JSON data
 * 2. Service sends HTTP POST to Python Backend
 * 3. Backend converts JSON → Binary Protocol
 * 4. Backend sends via RAW TCP SOCKET to C Server (port 8888)
 * 5. C Server processes and returns binary response
 * 6. Backend converts Binary → JSON
 * 7. React receives JSON response
 * 
 * Note: Browsers CANNOT directly connect to raw TCP sockets (security)
 * So we use HTTP as transport layer between Frontend and Backend
 */

import axios from 'axios';

// Network server endpoint (will be proxied through Backend)
const NETWORK_API = process.env.REACT_APP_NETWORK_API || 'http://localhost:5000/api/network';

// Get account_id from localStorage
const getAccountId = () => {
    return localStorage.getItem('account_id') || '1';
};

class SocketService {
    constructor() {
        this.sessionId = null;
        this.connected = false;
    }

    /**
     * Connect to server (establish session)
     */
    async connect() {
        try {
            const response = await axios.post(`${NETWORK_API}/connect`, {
                account_id: getAccountId()
            });
            
            if (response.data.success) {
                this.sessionId = response.data.session_id;
                this.connected = true;
                console.log('[Socket] Connected, session:', this.sessionId);
                return true;
            }
            return false;
        } catch (error) {
            console.error('[Socket] Connection failed:', error);
            return false;
        }
    }

    /**
     * Create Room (CMD_CREATE_ROOM = 0x0200)
     */
    async createRoom(roomData) {
        try {
            const payload = {
                command: 'CREATE_ROOM',
                account_id: getAccountId(),
                data: {
                    room_name: roomData.name,
                    visibility: roomData.visibility === 'public' ? 0 : 1,
                    mode: roomData.mode === 'scoring' ? 0 : 1,
                    max_players: roomData.maxPlayers || 4,
                    round_time: roomData.roundTime || 30,
                    wager_enabled: roomData.wagerEnabled ? 1 : 0
                }
            };

            const response = await axios.post(`${NETWORK_API}/command`, payload);
            
            if (response.data.success) {
                console.log('[Socket] Room created:', response.data.room);
                return {
                    success: true,
                    room: {
                        id: response.data.room.room_id,
                        code: response.data.room.room_code,
                        hostId: response.data.room.host_id,
                        createdAt: response.data.room.created_at
                    }
                };
            }
            
            return { success: false, error: response.data.error };
        } catch (error) {
            console.error('[Socket] Create room failed:', error);
            return { 
                success: false, 
                error: error.response?.data?.error || 'Failed to create room' 
            };
        }
    }

    /**
     * Set Rules (CMD_SET_RULE = 0x0206)
     */
    async setRules(roomId, rules) {
        try {
            const payload = {
                command: 'SET_RULE',
                account_id: getAccountId(),
                data: {
                    room_id: roomId,
                    mode: rules.mode === 'scoring' ? 0 : 1,
                    max_players: rules.maxPlayers,
                    round_time: rules.roundTime,
                    wager_enabled: rules.wagerEnabled ? 1 : 0
                }
            };

            const response = await axios.post(`${NETWORK_API}/command`, payload);
            
            if (response.data.success) {
                console.log('[Socket] Rules updated');
                return { success: true };
            }
            
            return { success: false, error: response.data.error };
        } catch (error) {
            console.error('[Socket] Set rules failed:', error);
            return { 
                success: false, 
                error: error.response?.data?.error || 'Failed to set rules' 
            };
        }
    }

    /**
     * Kick Member (CMD_KICK = 0x0204)
     */
    async kickMember(roomId, targetUserId) {
        try {
            const payload = {
                command: 'KICK_MEMBER',
                account_id: getAccountId(),
                data: {
                    room_id: roomId,
                    target_user_id: targetUserId
                }
            };

            const response = await axios.post(`${NETWORK_API}/command`, payload);
            
            if (response.data.success) {
                console.log('[Socket] Member kicked:', targetUserId);
                return { success: true };
            }
            
            return { success: false, error: response.data.error };
        } catch (error) {
            console.error('[Socket] Kick member failed:', error);
            return { 
                success: false, 
                error: error.response?.data?.error || 'Failed to kick member' 
            };
        }
    }

    /**
     * Start Game (CMD_START_GAME = 0x0300)
     */
    async startGame(roomId) {
        try {
            const payload = {
                command: 'START_GAME',
                account_id: getAccountId(),
                data: {
                    room_id: roomId
                }
            };

            const response = await axios.post(`${NETWORK_API}/command`, payload);
            
            if (response.data.success) {
                console.log('[Socket] Game starting:', response.data);
                return {
                    success: true,
                    matchId: response.data.match_id,
                    countdown: response.data.countdown_ms,
                    serverTime: response.data.server_timestamp_ms,
                    gameStartTime: response.data.game_start_timestamp_ms
                };
            }
            
            return { success: false, error: response.data.error };
        } catch (error) {
            console.error('[Socket] Start game failed:', error);
            return { 
                success: false, 
                error: error.response?.data?.error || 'Failed to start game' 
            };
        }
    }

    /**
     * Delete Room (CMD_DELETE_ROOM = 0x0207)
     */
    async deleteRoom(roomId) {
        try {
            const payload = {
                command: 'DELETE_ROOM',
                account_id: getAccountId(),
                data: {
                    room_id: roomId
                }
            };

            const response = await axios.post(`${NETWORK_API}/command`, payload);
            
            if (response.data.success) {
                console.log('[Socket] Room deleted');
                return { success: true };
            }
            
            return { success: false, error: response.data.error };
        } catch (error) {
            console.error('[Socket] Delete room failed:', error);
            return { 
                success: false, 
                error: error.response?.data?.error || 'Failed to delete room' 
            };
        }
    }

    /**
     * Disconnect from server
     */
    disconnect() {
        this.sessionId = null;
        this.connected = false;
        console.log('[Socket] Disconnected');
    }
}

// Singleton instance
export const socketService = new SocketService();
export default socketService;
