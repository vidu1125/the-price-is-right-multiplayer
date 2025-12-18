# Backend/app/services/host/rule_service.py
"""
Host Rule Service - Business logic for game rule management
Handles: Set Rules
"""
from app.models import db, Room, Match
from app.utils.validation import validate_room_settings
from app.utils.time_sync import get_server_time_ms
from datetime import datetime

def set_rule(user_id, rule_data):
    """
    Update game rules (host only)
    
    Args:
        user_id (int): User ID attempting to change rules
        rule_data (dict): {
            'room_id': int,
            'mode': str ('scoring'/'elimination'),
            'max_players': int (4-6),
            'round_time': int (15-60),
            'wager_enabled': bool
        }
    
    Returns:
        dict: {
            'success': bool,
            'timestamp': int,
            'member_ids': list[int],  # For broadcast
            'error': str (if failed)
        }
    """
    try:
        room_id = rule_data.get('room_id')
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
            return {'success': False, 'error': 'Cannot change rules during game'}
        
        # Parse new settings
        mode = rule_data.get('mode', 'scoring')
        max_players = int(rule_data.get('max_players', 4))
        round_time = int(rule_data.get('round_time', 15))
        wager_enabled = bool(rule_data.get('wager_enabled', False))
        
        # Validate
        if not validate_room_settings(mode, max_players, round_time):
            return {'success': False, 'error': 'Invalid settings'}
        
        # Get match settings
        match = Match.query.filter_by(room_id=room_id, ended_at=None).first()
        if not match:
            return {'success': False, 'error': 'Match not found'}
        
        # Update match settings
        match.mode = mode
        match.max_players = max_players
        match.round_time = round_time
        match.wager = wager_enabled
        
        room.updated_at = datetime.utcnow()
        
        db.session.commit()
        
        # Get member IDs for broadcast
        from app.models import RoomMember
        members = RoomMember.query.filter_by(room_id=room_id).all()
        member_ids = [m.account_id for m in members]
        
        return {
            'success': True,
            'timestamp': get_server_time_ms(),
            'member_ids': member_ids
        }
        
    except Exception as e:
        db.session.rollback()
        print(f"[ERROR] set_rule: {str(e)}")
        return {'success': False, 'error': f'Database error: {str(e)}'}
