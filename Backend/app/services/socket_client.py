"""
TCP Socket Client - Python Backend → C Server Communication
Uses RAW TCP SOCKET with BINARY PROTOCOL

Architecture:
    React UI → HTTP → Python Backend → RAW TCP SOCKET → C Server
                                         (Binary Protocol)
"""

import socket
import struct
import logging
import time

logger = logging.getLogger(__name__)

# Protocol constants
MAGIC_NUMBER = 0x4347  # 'CG' in hex
PROTOCOL_VERSION = 0x01
DEFAULT_FLAGS = 0x00

# Command codes (must match Network/protocol/commands.h)
CMD_CREATE_ROOM = 0x0200
CMD_JOIN_ROOM = 0x0201
CMD_LEAVE_ROOM = 0x0202
CMD_READY = 0x0203
CMD_KICK = 0x0204
CMD_SET_RULE = 0x0206
CMD_DELETE_ROOM = 0x0207
CMD_START_GAME = 0x0300

# Response codes
RES_SUCCESS = 0x00CA  # Success
RES_ERROR = 0x00CB    # Error
RES_ROOM_CREATED = 0x00CC
RES_GAME_STARTING = 0x00CD


class CServerSocketClient:
    """TCP Socket Client for communicating with C Server"""
    
    def __init__(self, host='localhost', port=8888, timeout=5.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.seq_number = 0
    
    def _get_next_seq(self):
        """Generate next sequence number"""
        self.seq_number = (self.seq_number + 1) % 0xFFFFFFFF
        return self.seq_number
    
    def _build_header(self, command, payload_length):
        """
        Build 16-byte packet header
        
        Format (Network byte order):
            uint16_t magic;           // 0x4347
            uint16_t version;         // 0x01
            uint8_t  flags;           // 0x00
            uint8_t  command;         // Command code (high byte)
            uint16_t command_full;    // Full command code
            uint16_t reserved;        // 0x0000
            uint32_t sequence;        // Sequence number
            uint32_t payload_length;  // Payload size
        """
        header = struct.pack(
            '!HHBBHHII',  # Network byte order
            MAGIC_NUMBER,           # magic (2 bytes)
            PROTOCOL_VERSION,       # version (2 bytes)
            DEFAULT_FLAGS,          # flags (1 byte)
            (command >> 8) & 0xFF,  # command high byte (1 byte)
            command,                # command full (2 bytes)
            0,                      # reserved (2 bytes)
            self._get_next_seq(),   # sequence (4 bytes)
            payload_length          # payload_length (4 bytes)
        )
        return header
    
    def _parse_header(self, header_bytes):
        """Parse 16-byte response header"""
        if len(header_bytes) < 16:
            raise ValueError("Invalid header length")
        
        magic, version, flags, cmd_high, command, reserved, sequence, payload_len = struct.unpack(
            '!HHBBHHII', header_bytes
        )
        
        if magic != MAGIC_NUMBER:
            raise ValueError(f"Invalid magic number: 0x{magic:04X}")
        
        return {
            'magic': magic,
            'version': version,
            'flags': flags,
            'command': command,
            'sequence': sequence,
            'payload_length': payload_len
        }
    
    def send_command(self, command, payload=b''):
        """
        Send command to C Server and receive response
        
        Args:
            command: Command code (e.g., CMD_CREATE_ROOM)
            payload: Binary payload
        
        Returns:
            dict: {'success': bool, 'command': int, 'payload': bytes}
        """
        sock = None
        try:
            # Connect to C Server
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            sock.connect((self.host, self.port))
            
            logger.info(f"[Socket] Connected to C Server {self.host}:{self.port}")
            
            # Build and send packet
            header = self._build_header(command, len(payload))
            packet = header + payload
            
            logger.debug(f"[Socket] Sending command 0x{command:04X}, payload size: {len(payload)}")
            sock.sendall(packet)
            
            # Receive response header (16 bytes)
            response_header = b''
            while len(response_header) < 16:
                chunk = sock.recv(16 - len(response_header))
                if not chunk:
                    raise ConnectionError("Connection closed while reading header")
                response_header += chunk
            
            # Parse header
            header_info = self._parse_header(response_header)
            logger.debug(f"[Socket] Response command: 0x{header_info['command']:04X}, payload: {header_info['payload_length']} bytes")
            
            # Receive payload
            response_payload = b''
            payload_length = header_info['payload_length']
            
            while len(response_payload) < payload_length:
                chunk = sock.recv(payload_length - len(response_payload))
                if not chunk:
                    raise ConnectionError("Connection closed while reading payload")
                response_payload += chunk
            
            # Check if response is success
            success = header_info['command'] in (RES_SUCCESS, RES_ROOM_CREATED, RES_GAME_STARTING)
            
            return {
                'success': success,
                'command': header_info['command'],
                'payload': response_payload,
                'sequence': header_info['sequence']
            }
            
        except socket.timeout:
            logger.error(f"[Socket] Timeout connecting to C Server")
            return {'success': False, 'error': 'Connection timeout'}
        
        except Exception as e:
            logger.error(f"[Socket] Error: {e}", exc_info=True)
            return {'success': False, 'error': str(e)}
        
        finally:
            if sock:
                sock.close()
                logger.debug("[Socket] Connection closed")
    
    def create_room(self, room_name, visibility, mode, max_players, round_time, wager_enabled):
        """
        Create room command
        
        Payload structure (72 bytes):
            char room_name[64];
            uint8_t visibility;     // 0: public, 1: private
            uint8_t mode;           // 0: scoring, 1: elimination
            uint8_t max_players;    // 4-6
            uint16_t round_time;    // 15-60 seconds
            uint8_t wager_enabled;  // 0: disabled, 1: enabled
            uint8_t padding[2];     // Alignment
        """
        # Encode room name (max 63 chars + null terminator)
        name_bytes = room_name.encode('utf-8')[:63].ljust(64, b'\x00')
        
        # Build payload
        payload = struct.pack(
            '64sBBBHBxx',  # xx = 2 bytes padding
            name_bytes,
            visibility,
            mode,
            max_players,
            round_time,
            wager_enabled
        )
        
        response = self.send_command(CMD_CREATE_ROOM, payload)
        
        if response['success']:
            # Parse response payload (CreateRoomResponse)
            # uint32_t room_id, char room_code[11], uint32_t host_id, uint64_t created_at
            payload = response['payload']
            if len(payload) >= 27:  # 4 + 11 + 4 + 8
                room_id, room_code_bytes, host_id, created_at = struct.unpack(
                    '!I11sIQ', payload[:27]
                )
                room_code = room_code_bytes.decode('utf-8').rstrip('\x00')
                
                return {
                    'success': True,
                    'room_id': room_id,
                    'room_code': room_code,
                    'host_id': host_id,
                    'created_at': created_at
                }
        
        return response
    
    def set_rule(self, room_id, mode, max_players, round_time, wager_enabled):
        """
        Set rules command
        
        Payload: SetRuleRequest (16 bytes)
        """
        payload = struct.pack(
            '!IBBBHBxx',
            room_id,
            mode,
            max_players,
            round_time,
            wager_enabled
        )
        
        return self.send_command(CMD_SET_RULE, payload)
    
    def kick_member(self, room_id, target_user_id):
        """
        Kick member command
        
        Payload: KickRequest (8 bytes)
        """
        payload = struct.pack('!II', room_id, target_user_id)
        return self.send_command(CMD_KICK, payload)
    
    def start_game(self, room_id):
        """
        Start game command
        
        Payload: StartGameRequest (4 bytes)
        """
        payload = struct.pack('!I', room_id)
        response = self.send_command(CMD_START_GAME, payload)
        
        if response['success']:
            # Parse GameStartingNotification
            # uint32_t match_id, uint32_t countdown_ms, 
            # uint64_t server_timestamp_ms, uint64_t game_start_timestamp_ms
            payload = response['payload']
            if len(payload) >= 24:  # 4 + 4 + 8 + 8
                match_id, countdown_ms, server_ts, game_start_ts = struct.unpack(
                    '!IIQQ', payload[:24]
                )
                
                return {
                    'success': True,
                    'match_id': match_id,
                    'countdown_ms': countdown_ms,
                    'server_timestamp_ms': server_ts,
                    'game_start_timestamp_ms': game_start_ts
                }
        
        return response
    
    def delete_room(self, room_id):
        """
        Delete room command
        
        Payload: DeleteRoomRequest (4 bytes)
        """
        payload = struct.pack('!I', room_id)
        return self.send_command(CMD_DELETE_ROOM, payload)


# Singleton instance
_socket_client = None

def get_socket_client():
    """Get or create socket client instance"""
    global _socket_client
    if _socket_client is None:
        _socket_client = CServerSocketClient()
    return _socket_client
