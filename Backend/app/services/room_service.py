from app.models import db
from app.models.room import Room, RoomMember
from app.models.account import Account, Profile
from app.models.match import Match, MatchSettings, MatchPlayer
from datetime import datetime

class RoomService:
    
    @staticmethod
    def create_room(account_id, room_data):
        """
        Create a new room and add host as first member
        
        Args:
            account_id (int): ID of the host account
            room_data (dict): {
                'name': str,
                'visibility': 'public' | 'private'
            }
        
        Returns:
            dict: {'success': bool, 'room_id': int, 'room': dict}
        """
        try:
            # Validate account exists
            account = Account.query.get(account_id)
            if not account:
                return {'success': False, 'error': 'Account not found'}
            
            if not account.profile:
                return {'success': False, 'error': 'Profile not found'}
            
            # Validate visibility
            visibility = room_data.get('visibility', 'public')
            if visibility not in ['public', 'private']:
                return {'success': False, 'error': 'Invalid visibility'}
            
            # Create room
            new_room = Room(
                host_id=account_id,
                name=room_data.get('name', f"Room by {account.profile.name}"),
                visibility=visibility,
                status='waiting'
            )
            
            db.session.add(new_room)
            db.session.flush()  # Get room ID
            
            # Add host as first member
            host_member = RoomMember(
                room_id=new_room.id,
                account_id=account_id,
                role='host',
                ready=True
            )
            
            db.session.add(host_member)
            db.session.commit()
            
            return {
                'success': True,
                'room_id': new_room.id,
                'room': new_room.to_dict(),
                'message': 'Room created successfully'
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def delete_room(room_id, account_id):
        """
        Delete a room (close it, set status='closed')
        Only host can delete. Cannot delete if game is in progress.
        
        Args:
            room_id (int): ID of the room
            account_id (int): ID of account requesting deletion
        
        Returns:
            dict: {'success': bool, 'message': str}
        """
        try:
            # Find room
            room = Room.query.filter_by(id=room_id, status__in=['waiting', 'in_game']).first()
            
            if not room:
                return {'success': False, 'error': 'Room not found or already closed'}
            
            # Check if user is host
            if room.host_id != account_id:
                return {'success': False, 'error': 'Only host can delete room'}
            
            # Check if game is in progress
            if room.status == 'in_game':
                return {'success': False, 'error': 'Cannot delete room while game is in progress'}
            
            # Close room
            room.status = 'closed'
            room.updated_at = datetime.utcnow()
            
            # Mark all members as left
            for member in room.members:
                if not member.left_at:
                    member.left_at = datetime.utcnow()
            
            db.session.commit()
            
            return {
                'success': True,
                'message': f'Room "{room.name}" closed successfully'
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def list_rooms(status='waiting', visibility='public'):
        """
        List all available rooms
        
        Args:
            status (str): Filter by status
            visibility (str): Filter by visibility
        
        Returns:
            dict: {'success': bool, 'rooms': list}
        """
        try:
            query = Room.query
            
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
    
    @staticmethod
    def get_room_details(room_id):
        """Get room with members"""
        try:
            room = Room.query.get(room_id)
            
            if not room:
                return {'success': False, 'error': 'Room not found'}
            
            room_data = room.to_dict()
            room_data['members'] = [
                member.to_dict() 
                for member in room.members 
                if not member.kicked and not member.left_at
            ]
            
            return {'success': True, 'room': room_data}
            
        except Exception as e:
            return {'success': False, 'error': str(e)}