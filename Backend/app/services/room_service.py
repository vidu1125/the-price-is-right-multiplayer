from app.models import db
from app.models.room import Room, RoomMember
from datetime import datetime

class RoomService:
    
    @staticmethod
    def create_room(host_id, room_data):
        """
        Create a new room and add host as first member
        
        Args:
            host_id (str): ID of the host user
            room_data (dict): {
                'name': str,
                'visibility': 'public' | 'private',
                'mode': 'scoring' | 'elimination',
                'max_players': int (4-8),
                'wager_enabled': bool,
                'round_time_sec': int
            }
        
        Returns:
            dict: {'success': bool, 'room_id': int, 'message': str, 'error': str}
        """
        try:
            # Validate max_players
            max_players = room_data.get('max_players', 4)
            if not (4 <= max_players <= 8):
                return {'success': False, 'error': 'max_players must be between 4 and 8'}
            
            # Validate mode
            mode = room_data.get('mode', 'scoring')
            if mode not in ['scoring', 'elimination']:
                return {'success': False, 'error': 'Invalid game mode'}
            
            # Validate visibility
            visibility = room_data.get('visibility', 'public')
            if visibility not in ['public', 'private']:
                return {'success': False, 'error': 'Invalid visibility'}
            
            # Create room
            new_room = Room(
                host_id=host_id,
                name=room_data.get('name', f'Room by {host_id}'),
                visibility=visibility,
                mode=mode,
                max_players=max_players,
                wager_enabled=room_data.get('wager_enabled', False),
                round_time_sec=room_data.get('round_time_sec', 15),
                status='waiting'
            )
            
            db.session.add(new_room)
            db.session.flush()  # Get room_id before commit
            
            # Add host as first member with role='host'
            host_member = RoomMember(
                room_id=new_room.room_id,
                user_id=host_id,
                role='host',
                is_ready=True  # Host is always ready
            )
            
            db.session.add(host_member)
            db.session.commit()
            
            return {
                'success': True,
                'room_id': new_room.room_id,
                'room': new_room.to_dict(),
                'message': 'Room created successfully'
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def delete_room(room_id, user_id):
        """
        Delete a room (soft delete by setting deleted_at)
        Only host can delete. Cannot delete if game is in progress.
        
        Args:
            room_id (int): ID of the room to delete
            user_id (str): ID of user requesting deletion
        
        Returns:
            dict: {'success': bool, 'message': str, 'error': str}
        """
        try:
            # Find room
            room = Room.query.filter_by(room_id=room_id, deleted_at=None).first()
            
            if not room:
                return {'success': False, 'error': 'Room not found or already deleted'}
            
            # Check if user is host
            if room.host_id != user_id:
                return {'success': False, 'error': 'Only host can delete room'}
            
            # Check room status
            if room.status == 'in_game':
                return {'success': False, 'error': 'Cannot delete room while game is in progress'}
            
            # Soft delete: set deleted_at timestamp
            room.deleted_at = datetime.utcnow()
            room.status = 'closed'
            
            db.session.commit()
            
            return {
                'success': True,
                'message': f'Room "{room.name}" deleted successfully'
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def get_room_details(room_id):
        """
        Get detailed information about a room including members
        
        Args:
            room_id (int): ID of the room
        
        Returns:
            dict: Room data with members list
        """
        try:
            room = Room.query.filter_by(room_id=room_id, deleted_at=None).first()
            
            if not room:
                return {'success': False, 'error': 'Room not found'}
            
            room_data = room.to_dict()
            room_data['members'] = [
                member.to_dict() 
                for member in room.members 
                if not member.is_kicked
            ]
            
            return {'success': True, 'room': room_data}
            
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def list_rooms(status='waiting', visibility='public'):
        """
        List all available rooms
        
        Args:
            status (str): Filter by status (waiting, in_game, closed)
            visibility (str): Filter by visibility (public, private)
        
        Returns:
            dict: {'success': bool, 'rooms': list}
        """
        try:
            query = Room.query.filter_by(deleted_at=None)
            
            if status:
                query = query.filter_by(status=status)
            
            if visibility:
                query = query.filter_by(visibility=visibility)
            
            rooms = query.order_by(Room.created_at.desc()).all()
            
            return {
                'success': True,
                'rooms': [room.to_dict() for room in rooms]
            }
            
        except Exception as e:
            return {'success': False, 'error': str(e)}