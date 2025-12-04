# app/services/room_service.py
from app.models import db
from app.models.room import Room, RoomMember
from app.models.account import Account
from datetime import datetime


class RoomService:

    @staticmethod
    def create_room(account_id, room_data):
        try:
            # Validate account exists
            acc = Account.query.get(account_id)
            if not acc:
                return {'success': False, 'error': 'Account not found'}
            
            name = (room_data.get('name') or '').strip()
            if not name:
                return {'success': False, 'error': 'Room name is required'}
            
            visibility = room_data.get('visibility', 'public')
            if visibility not in ('public', 'private'):
                return {'success': False, 'error': 'Invalid visibility'}

            # Create room
            room = Room(
                name=name,
                visibility=visibility,
                host_id=account_id,
                status='waiting',
                created_at=datetime.utcnow(),
                updated_at=datetime.utcnow()
            )
            db.session.add(room)
            db.session.flush()  # Get room.id

            # Add host as first member (NO role, kicked, left_at)
            host_member = RoomMember(
                room_id=room.id,
                account_id=account_id,
                ready=True,
                joined_at=datetime.utcnow()
            )
            db.session.add(host_member)
            db.session.commit()

            return {
                'success': True,
                'room_id': room.id,
                'room': room.to_dict(),
                'message': 'Room created successfully'
            }

        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}

    @staticmethod
    def delete_room(room_id, account_id):
        try:
            room = (
                Room.query
                .filter(Room.id == room_id, Room.status.in_(["waiting", "in_game"]))
                .first()
            )
            if not room:
                return {'success': False, 'error': 'Room not found or already closed'}

            if room.host_id != account_id:
                return {'success': False, 'error': 'Only host can delete room'}

            if room.status == 'in_game':
                return {'success': False, 'error': 'Cannot delete while in game'}

            room.status = 'closed'
            room.updated_at = datetime.utcnow()

            db.session.commit()
            return {'success': True, 'message': f'Room \"{room.name}\" closed successfully'}

        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}

    @staticmethod
    def list_rooms(status='waiting', visibility='public'):
        try:
            query = Room.query
            if status:
                query = query.filter(Room.status == status)
            if visibility:
                query = query.filter(Room.visibility == visibility)

            rooms = query.order_by(Room.created_at.desc()).all()

            return {
                'success': True,
                'rooms': [room.to_dict() for room in rooms]
            }

        except Exception as e:
            return {'success': False, 'error': str(e)}

    @staticmethod
    def get_room_details(room_id):
        try:
            room = Room.query.get(room_id)
            if not room:
                return {'success': False, 'error': 'Room not found'}

            room_data = room.to_dict()

            # RoomMember no longer has kicked / left_at fields
            room_data['members'] = [member.to_dict() for member in room.members]

            return {'success': True, 'room': room_data}

        except Exception as e:
            return {'success': False, 'error': str(e)}
