# app/services/room_service.py
from app.models import db
from app.models.room import Room, RoomMember
from app.models.account import Account
from datetime import datetime
import random
import string


class RoomService:

    @staticmethod
    def _generate_room_code():
        """Generate unique 6-char room code (A-Z0-9)"""
        while True:
            code = ''.join(random.choices(string.ascii_uppercase + string.digits, k=6))
            # Check uniqueness
            if not Room.query.filter_by(code=code).first():
                return code

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

            # Game settings from room_data
            mode = room_data.get('mode', 'scoring')
            max_players = room_data.get('maxPlayers', 5)
            round_time = room_data.get('roundTime', 'normal')
            wager_mode = room_data.get('wagerEnabled', False)

            # Create room
            room = Room(
                name=name,
                code=RoomService._generate_room_code(),
                visibility=visibility,
                host_id=account_id,
                status='waiting',
                mode=mode,
                max_players=max_players,
                round_time=round_time,
                wager_mode=wager_mode,
                created_at=datetime.utcnow(),
                updated_at=datetime.utcnow()
            )
            db.session.add(room)
            db.session.flush()  # Get room.id

            # Add host as first member with ready status
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
    def kick_member(room_id, member_account_id, requester_account_id):
        """
        Kick a member from room (host only)
        """
        try:
            # Get room
            room = Room.query.get(room_id)
            if not room:
                return {'success': False, 'error': 'Room not found'}

            # Check room status
            if room.status != 'waiting':
                return {'success': False, 'error': 'Cannot kick from active game'}

            # Verify requester is host
            if room.host_id != requester_account_id:
                return {'success': False, 'error': 'Only host can kick members'}

            # Cannot kick self
            if member_account_id == requester_account_id:
                return {'success': False, 'error': 'Host cannot kick themselves'}

            # Find and remove member
            member = RoomMember.query.filter_by(
                room_id=room_id,
                account_id=member_account_id
            ).first()

            if not member:
                return {'success': False, 'error': 'Member not found in room'}

            db.session.delete(member)
            db.session.commit()

            return {
                'success': True,
                'message': 'Member kicked successfully',
                'kicked_account_id': member_account_id
            }

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

            # Serialize members with ready field
            room_data['members'] = []
            for member in room.members:
                member_dict = member.to_dict()
                # Temporarily hardcode host as ready until DB migration
                member_dict['ready'] = (member.account_id == room.host_id)
                room_data['members'].append(member_dict)

            return {'success': True, 'room': room_data, 'members': room_data['members']}

        except Exception as e:
            return {'success': False, 'error': str(e)}

    @staticmethod
    def update_rules(room_id, account_id, rules):
        """Host updates room rules/settings"""
        try:
            room = Room.query.get(room_id)
            if not room:
                return {'success': False, 'error': 'Room not found'}
            
            if room.host_id != account_id:
                return {'success': False, 'error': 'Only host can change rules'}
            
            if room.status != 'waiting':
                return {'success': False, 'error': 'Cannot change rules during game'}
            
            # Update room settings in database
            if 'mode' in rules:
                room.mode = rules['mode']
            if 'maxPlayers' in rules:
                room.max_players = rules['maxPlayers']
            if 'roundTime' in rules:
                room.round_time = rules['roundTime']
            if 'wagerMode' in rules:
                room.wager_mode = rules['wagerMode']
            if 'visibility' in rules:
                room.visibility = rules['visibility']
            
            room.updated_at = datetime.utcnow()
            db.session.commit()
            
            return {'success': True, 'rules': room.to_dict()}
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}

    @staticmethod
    def kick_member(room_id, host_id, target_id):
        """Host kicks a member from room"""
        try:
            room = Room.query.get(room_id)
            if not room or room.host_id != host_id:
                return {'success': False, 'error': 'Only host can kick'}
            
            if room.status == 'in_game':
                return {'success': False, 'error': 'Cannot kick during game'}
            
            if target_id == host_id:
                return {'success': False, 'error': 'Cannot kick yourself'}
            
            member = RoomMember.query.filter_by(
                room_id=room_id, 
                account_id=target_id
            ).first()
            
            if not member:
                return {'success': False, 'error': 'Member not found'}
            
            db.session.delete(member)
            db.session.commit()
            
            return {'success': True, 'message': 'Member kicked'}
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}

    @staticmethod
    def leave_room(room_id, account_id):
        """Member leaves room"""
        try:
            member = RoomMember.query.filter_by(
                room_id=room_id,
                account_id=account_id
            ).first()
            
            if not member:
                return {'success': False, 'error': 'Not in this room'}
            
            db.session.delete(member)
            db.session.commit()
            
            return {'success': True, 'message': 'Left room'}
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
