# Backend/app/ipc/socket_server.py
"""
IPC Server - Communication bridge between C Network Layer and Python Backend
Uses Unix Domain Socket for low-latency IPC
"""
import socket
import json
import os
from threading import Thread
import logging

logger = logging.getLogger(__name__)

class IPCServer:
    """IPC Server for handling requests from C Network Layer"""
    
    def __init__(self, socket_path='/tmp/tpir_backend.sock'):
        self.socket_path = socket_path
        self.sock = None
        self.running = False
        
    def start(self):
        """Start IPC server"""
        # Remove existing socket file
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
        
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.bind(self.socket_path)
        self.sock.listen(5)
        self.running = True
        
        logger.info(f"[IPC] Server listening on {self.socket_path}")
        
        while self.running:
            try:
                conn, _ = self.sock.accept()
                Thread(target=self.handle_client, args=(conn,), daemon=True).start()
            except Exception as e:
                if self.running:
                    logger.error(f"[IPC] Accept error: {e}")
    
    def stop(self):
        """Stop IPC server"""
        self.running = False
        if self.sock:
            self.sock.close()
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
    
    def handle_client(self, conn):
        """Handle single client connection"""
        try:
            # Read message length (4 bytes, big-endian)
            length_bytes = conn.recv(4)
            if not length_bytes or len(length_bytes) < 4:
                return
            
            msg_length = int.from_bytes(length_bytes, 'big')
            
            # Read full message
            data = b''
            while len(data) < msg_length:
                chunk = conn.recv(min(4096, msg_length - len(data)))
                if not chunk:
                    return
                data += chunk
            
            # Parse JSON request
            try:
                request = json.loads(data.decode('utf-8'))
            except json.JSONDecodeError as e:
                logger.error(f"[IPC] JSON decode error: {e}")
                self.send_response(conn, {'success': False, 'error': 'Invalid JSON'})
                return
            
            # Route request
            response = self.route_request(request)
            
            # Send response
            self.send_response(conn, response)
            
        except Exception as e:
            logger.error(f"[IPC] Client handler error: {e}")
            try:
                self.send_response(conn, {'success': False, 'error': str(e)})
            except:
                pass
        finally:
            conn.close()
    
    def send_response(self, conn, response):
        """Send JSON response to client"""
        response_data = json.dumps(response).encode('utf-8')
        length = len(response_data)
        conn.sendall(length.to_bytes(4, 'big'))
        conn.sendall(response_data)
    
    def route_request(self, request):
        """
        Route request to appropriate service
        
        Request format:
        {
            'action': str,
            'user_id': int,
            'data': dict
        }
        """
        action = request.get('action')
        user_id = request.get('user_id')
        data = request.get('data', {})
        
        logger.info(f"[IPC] Request: action={action}, user_id={user_id}")
        
        try:
            # Host features
            if action == 'CREATE_ROOM':
                from app.services.host.room_service import create_room
                return create_room(user_id, data)
            
            elif action == 'SET_RULE':
                from app.services.host.rule_service import set_rule
                return set_rule(user_id, data)
            
            elif action == 'KICK_MEMBER':
                from app.services.host.member_service import kick_member
                return kick_member(user_id, data)
            
            elif action == 'DELETE_ROOM':
                from app.services.host.room_service import delete_room
                return delete_room(user_id, data)
            
            elif action == 'START_GAME':
                from app.services.host.room_service import start_game
                return start_game(user_id, data)
            
            # Add more actions here as needed
            # elif action == 'JOIN_ROOM':
            #     from app.services.lobby.join_service import join_room
            #     return join_room(user_id, data)
            
            else:
                logger.warning(f"[IPC] Unknown action: {action}")
                return {'success': False, 'error': f'Unknown action: {action}'}
        
        except Exception as e:
            logger.error(f"[IPC] Route error: {e}", exc_info=True)
            return {'success': False, 'error': str(e)}

# Global IPC server instance
_ipc_server = None

def start_ipc_server():
    """Start IPC server in background thread"""
    global _ipc_server
    _ipc_server = IPCServer()
    Thread(target=_ipc_server.start, daemon=True).start()
    logger.info("[IPC] Server started in background")

def stop_ipc_server():
    """Stop IPC server"""
    global _ipc_server
    if _ipc_server:
        _ipc_server.stop()
        _ipc_server = None
