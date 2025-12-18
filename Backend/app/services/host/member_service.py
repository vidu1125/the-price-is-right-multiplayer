# Backend/app/services/host/member_service.py
"""
Host Member Service - Business logic for member management
Handles: Kick Member
"""
from app.models import db, Room, RoomMember, Account
from app.utils.time_sync import get_server_time_ms

def kick_member(user_id, kick_data):
    """
    Kick member from room (host only)
    
    Args:
        user_id (int): Host user ID
        kick_data (dict): {
            'room_id': int,
            'target_user_id': int
        }
    
    Returns:
        dict: {
            'success': bool,
            'target_user_id': int,
            'target_username': str,
            'new_player_count': int,
            'member_ids': list[int],  # Remaining members for broadcast
            'timestamp': int,
            'error': str (if failed)
        }
    """
    try:
        room_id = kick_data.get('room_id')
        target_user_id = kick_data.get('target_user_id')
        
        if not room_id or not target_user_id:
            return {'success': False, 'error': 'Room ID and target user ID required'}
        
        room = Room.query.get(room_id)
        if not room:
            return {'success': False, 'error': 'Room not found'}
        
        # Permission check
        if room.host_id != user_id:
            return {'success': False, 'error': 'Not host'}
        
        # Self-kick prevention
        if target_user_id == user_id:
            return {'success': False, 'error': 'Cannot kick yourself'}
        
        # Check target is in room
        target_member = RoomMember.query.filter_by(
            room_id=room_id,
            account_id=target_user_id
        ).first()
        
        if not target_member:
            return {'success': False, 'error': 'Player not in room'}
        
        # Get target username
        target_user = Account.query.get(target_user_id)
        target_username = target_user.profile.name if target_user and target_user.profile else "Unknown"
        
        # Remove from room
        db.session.delete(target_member)
        db.session.commit()
        
        # Get remaining members
        remaining_members = RoomMember.query.filter_by(room_id=room_id).all()
        member_ids = [m.account_id for m in remaining_members]
        
        return {
            'success': True,
            'target_user_id': target_user_id,
            'target_username': target_username,
            'new_player_count': len(remaining_members),
            'member_ids': member_ids,
            'timestamp': get_server_time_ms()
        }
        
    except Exception as e:
        db.session.rollback()
        print(f"[ERROR] kick_member: {str(e)}")
        return {'success': False, 'error': f'Database error: {str(e)}'}
