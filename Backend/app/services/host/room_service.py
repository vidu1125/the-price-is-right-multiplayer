# Backend/app/services/host/room_service.py
"""
Host Room Service - Business logic for room management
Handles: Create Room, Delete Room, Start Game
"""
from app.models import db, Room, RoomMember, Match, MatchPlayer, Account
from app.utils.validation import validate_room_name, validate_room_settings
from app.utils.time_sync import get_server_time_ms
from datetime import datetime
import random
import string

def generate_room_code():
    """Generate unique 6-character room code"""
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=6))

def create_room(user_id, room_data):
    """
    Create a new game room
    
    Args:
        user_id (int): Host user ID
        room_data (dict): {
            'name': str,
            'visibility': str ('public'/'private'),
            'mode': str ('scoring'/'elimination'),
            'max_players': int (4-6),
            'round_time': int (15-60),
            'wager_enabled': bool
        }
    
    Returns:
        dict: {
            'success': bool,
            'room_id': int,
            'room_code': str,
            'timestamp': int,
            'error': str (if failed)
        }
    """
    try:
        # Validate user exists
        user = Account.query.get(user_id)
        if not user:
            return {'success': False, 'error': 'User not found'}
        
        # Extract and validate data
        name = room_data.get('name', '').strip()
        if not validate_room_name(name):
            return {'success': False, 'error': 'Invalid room name (3-63 characters)'}
        
        # Parse mode and visibility
        mode_str = room_data.get('mode', 'scoring')
        visibility_str = room_data.get('visibility', 'public')
        
        if mode_str not in ['scoring', 'elimination']:
            return {'success': False, 'error': 'Invalid mode'}
        
        if visibility_str not in ['public', 'private']:
            return {'success': False, 'error': 'Invalid visibility'}
        
        max_players = int(room_data.get('max_players', 4))
        round_time = int(room_data.get('round_time', 15))
        wager_enabled = bool(room_data.get('wager_enabled', False))
        
        # Validate settings
        if not validate_room_settings(mode_str, max_players, round_time):
            return {'success': False, 'error': 'Invalid settings'}
        
        # Generate unique code
        code = generate_room_code()
        attempts = 0
        while Room.query.filter_by(code=code).first() and attempts < 10:
            code = generate_room_code()
            attempts += 1
        
        if attempts >= 10:
            return {'success': False, 'error': 'Failed to generate unique code'}
        
        # Create room
        room = Room(
            code=code,
            name=name,
            visibility=visibility_str,
            host_id=user_id,
            status='waiting',
            created_at=datetime.utcnow(),
            updated_at=datetime.utcnow()
        )
        db.session.add(room)
        db.session.flush()  # Get room.id
        
        # Add host as first member
        host_member = RoomMember(
            room_id=room.id,
            account_id=user_id,
            ready=True,  # Host is always ready
            joined_at=datetime.utcnow()
        )
        db.session.add(host_member)
        
        # Create match settings
        match = Match(
            room_id=room.id,
            mode=mode_str,
            max_players=max_players,
            wager=wager_enabled,
            round_time=round_time
        )
        db.session.add(match)
        
        db.session.commit()
        
        return {
            'success': True,
            'room_id': room.id,
            'room_code': code,
            'timestamp': get_server_time_ms()
        }
        
    except Exception as e:
        db.session.rollback()
        print(f"[ERROR] create_room: {str(e)}")
        return {'success': False, 'error': f'Database error: {str(e)}'}

def delete_room(user_id, room_data):
    """
    Delete a room (host only)
    
    Args:
        user_id (int): User ID attempting to delete
        room_data (dict): {'room_id': int}
    
    Returns:
        dict: {
            'success': bool,
            'member_ids': list[int],  # For broadcast
            'timestamp': int,
            'error': str (if failed)
        }
    """
    try:
        room_id = room_data.get('room_id')
        if not room_id:
            return {'success': False, 'error': 'Room ID required'}
        
        room = Room.query.get(room_id)
        if not room:
            return {'success': False, 'error': 'Room not found'}
        
        # Permission check
        if room.host_id != user_id:
            return {'success': False, 'error': 'Not host'}
        
        # Status check
        if room.status == 'playing':
            return {'success': False, 'error': 'Cannot delete room during game'}
        
        # Get all members for broadcast
        members = RoomMember.query.filter_by(room_id=room_id).all()
        member_ids = [m.account_id for m in members]
        
        # Delete room (cascade will handle room_members and matches)
        db.session.delete(room)
        db.session.commit()
        
        return {
            'success': True,
            'member_ids': member_ids,
            'timestamp': get_server_time_ms()
        }
        
    except Exception as e:
        db.session.rollback()
        print(f"[ERROR] delete_room: {str(e)}")
        return {'success': False, 'error': f'Database error: {str(e)}'}

def start_game(user_id, room_data):
    """
    Start game (host only)
    
    Args:
        user_id (int): Host user ID
        room_data (dict): {'room_id': int}
    
    Returns:
        dict: {
            'success': bool,
            'match_id': int,
            'countdown': int,
            'server_timestamp': int,
            'round_start_time': int,
            'round_end_time': int,
            'time_limit': int,
            'member_ids': list[int],
            'error': str (if failed)
        }
    """
    try:
        room_id = room_data.get('room_id')
        if not room_id:
            return {'success': False, 'error': 'Room ID required'}
        
        room = Room.query.get(room_id)
        if not room:
            return {'success': False, 'error': 'Room not found'}
        
        # Permission check
        if room.host_id != user_id:
            return {'success': False, 'error': 'Not host'}
        
        # Check player count
        members = RoomMember.query.filter_by(room_id=room_id).all()
        if len(members) < 4:
            return {'success': False, 'error': 'Minimum 4 players required'}
        
        # Check all ready
        not_ready = [m for m in members if not m.ready]
        if not_ready:
            return {'success': False, 'error': f'{len(not_ready)} player(s) not ready'}
        
        # Get match settings
        match = Match.query.filter_by(room_id=room_id, ended_at=None).first()
        if not match:
            return {'success': False, 'error': 'Match not found'}
        
        # Update match status
        match.started_at = datetime.utcnow()
        room.status = 'playing'
        
        # Create match players
        for member in members:
            match_player = MatchPlayer(
                match_id=match.id,
                account_id=member.account_id,
                score=0,
                eliminated=False,
                forfeited=False,
                winner=False,
                joined_at=datetime.utcnow()
            )
            db.session.add(match_player)
        
        db.session.commit()
        
        # Calculate timings
        now = get_server_time_ms()
        round_start_time = now + 3000  # 3 second countdown
        round_end_time = round_start_time + (match.round_time * 1000)
        
        return {
            'success': True,
            'match_id': match.id,
            'countdown': 3,
            'server_timestamp': now,
            'round_start_time': round_start_time,
            'round_end_time': round_end_time,
            'time_limit': match.round_time,
            'member_ids': [m.account_id for m in members]
        }
        
    except Exception as e:
        db.session.rollback()
        print(f"[ERROR] start_game: {str(e)}")
        return {'success': False, 'error': f'Database error: {str(e)}'}
