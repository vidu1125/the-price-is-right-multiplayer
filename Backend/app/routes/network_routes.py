"""
Backend Network Proxy Routes

Architecture:
    Frontend → HTTP/JSON → This Proxy → RAW TCP SOCKET → C Server
    
Flow:
    1. React UI sends HTTP POST with JSON
    2. This proxy converts JSON to Binary Protocol
    3. Sends via RAW TCP SOCKET to C Server (port 8888)
    4. C Server processes and returns binary response
    5. Proxy converts binary to JSON
    6. Returns JSON to React UI
"""

from flask import Blueprint, request, jsonify
from app.services.socket_client import get_socket_client
import logging

logger = logging.getLogger(__name__)

network_bp = Blueprint('network', __name__, url_prefix='/api/network')

@network_bp.route('/connect', methods=['POST'])
def connect_session():
    """
    Establish session (for future WebSocket upgrade)
    Currently returns mock session_id
    """
    try:
        data = request.get_json()
        account_id = data.get('account_id')
        
        if not account_id:
            return jsonify({'success': False, 'error': 'account_id required'}), 400
        
        # TODO: Implement real session management
        session_id = f"session_{account_id}"
        
        logger.info(f"[Network] Session connected: {session_id}")
        return jsonify({'success': True, 'session_id': session_id})
        
    except Exception as e:
        logger.error(f"[Network] Connect error: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500


@network_bp.route('/command', methods=['POST'])
def handle_command():
    """
    Route incoming commands via RAW TCP SOCKET to C Server
    
    Request format:
    {
        "command": "CREATE_ROOM" | "SET_RULE" | "KICK_MEMBER" | "START_GAME" | "DELETE_ROOM",
        "account_id": 123,
        "data": { ... command-specific data ... }
    }
    """
    try:
        payload = request.get_json()
        command = payload.get('command')
        account_id = payload.get('account_id')
        data = payload.get('data', {})
        
        if not command or not account_id:
            return jsonify({'success': False, 'error': 'command and account_id required'}), 400
        
        logger.info(f"[Network] Command received: {command} from user {account_id}")
        logger.info(f"[Network] Forwarding to C Server via RAW TCP SOCKET...")
        
        # Get socket client
        socket_client = get_socket_client()
        
        # Route to appropriate command
        if command == 'CREATE_ROOM':
            # Convert to binary protocol parameters
            room_name = data.get('room_name', 'New Room')
            visibility = 0 if data.get('visibility') == 0 else 1
            mode = 0 if data.get('mode') == 0 else 1
            max_players = data.get('max_players', 4)
            round_time = data.get('round_time', 30)
            wager_enabled = 1 if data.get('wager_enabled') else 0
            
            # Send via RAW TCP SOCKET
            result = socket_client.create_room(
                room_name, visibility, mode, max_players, round_time, wager_enabled
            )
            
            if result.get('success'):
                return jsonify({
                    'success': True,
                    'room': {
                        'room_id': result['room_id'],
                        'room_code': result['room_code'],
                        'host_id': result['host_id'],
                        'created_at': result['created_at']
                    }
                })
            return jsonify(result), 400
            
        elif command == 'SET_RULE':
            room_id = data.get('room_id')
            mode = 0 if data.get('mode') == 0 else 1
            max_players = data.get('max_players', 4)
            round_time = data.get('round_time', 30)
            wager_enabled = 1 if data.get('wager_enabled') else 0
            
            result = socket_client.set_rule(
                room_id, mode, max_players, round_time, wager_enabled
            )
            
            if result.get('success'):
                return jsonify({'success': True})
            return jsonify(result), 400
            
        elif command == 'KICK_MEMBER':
            room_id = data.get('room_id')
            target_user_id = data.get('target_user_id')
            
            result = socket_client.kick_member(room_id, target_user_id)
            
            if result.get('success'):
                return jsonify({'success': True})
            return jsonify(result), 400
            
        elif command == 'START_GAME':
            room_id = data.get('room_id')
            
            result = socket_client.start_game(room_id)
            
            if result.get('success'):
                return jsonify({
                    'success': True,
                    'match_id': result['match_id'],
                    'countdown_ms': result['countdown_ms'],
                    'server_timestamp_ms': result['server_timestamp_ms'],
                    'game_start_timestamp_ms': result['game_start_timestamp_ms']
                })
            return jsonify(result), 400
            
        elif command == 'DELETE_ROOM':
            room_id = data.get('room_id')
            
            result = socket_client.delete_room(room_id)
            
            if result.get('success'):
                return jsonify({'success': True})
            return jsonify(result), 400
            
        else:
            return jsonify({'success': False, 'error': f'Unknown command: {command}'}), 400
            
    except Exception as e:
        logger.error(f"[Network] Command error: {e}", exc_info=True)
        return jsonify({'success': False, 'error': str(e)}), 500


@network_bp.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({'status': 'ok', 'service': 'network-proxy'})
