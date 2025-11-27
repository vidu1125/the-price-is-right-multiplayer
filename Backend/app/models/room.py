from app.models import db
from datetime import datetime

class Room(db.Model):
    __tablename__ = 'rooms'
    
    room_id = db.Column(db.Integer, primary_key=True)
    host_id = db.Column(db.String(100), db.ForeignKey('users.user_id'), nullable=False)
    name = db.Column(db.String(100), nullable=False)
    visibility = db.Column(db.String(20), default='public')
    status = db.Column(db.String(20), default='waiting')
    mode = db.Column(db.String(20), default='scoring')
    max_players = db.Column(db.Integer, default=4)
    wager_enabled = db.Column(db.Boolean, default=False)
    round_time_sec = db.Column(db.Integer, default=15)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    deleted_at = db.Column(db.DateTime, nullable=True)
    
    # Relationships
    members = db.relationship('RoomMember', backref='room', lazy=True, cascade='all, delete-orphan')
    
    def to_dict(self):
        """Convert room object to dictionary"""
        return {
            'room_id': self.room_id,
            'name': self.name,
            'host_id': self.host_id,
            'visibility': self.visibility,
            'status': self.status,
            'mode': self.mode,
            'max_players': self.max_players,
            'current_players': len([m for m in self.members if not m.is_kicked]),
            'wager_enabled': self.wager_enabled,
            'round_time_sec': self.round_time_sec,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'is_deleted': self.deleted_at is not None
        }

class RoomMember(db.Model):
    __tablename__ = 'room_members'
    
    room_id = db.Column(db.Integer, db.ForeignKey('rooms.room_id'), primary_key=True)
    user_id = db.Column(db.String(100), db.ForeignKey('users.user_id'), primary_key=True)
    role = db.Column(db.String(20), default='member')
    is_ready = db.Column(db.Boolean, default=False)
    is_kicked = db.Column(db.Boolean, default=False)
    joined_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    def to_dict(self):
        """Convert room member to dictionary"""
        return {
            'room_id': self.room_id,
            'user_id': self.user_id,
            'username': self.user.username if hasattr(self, 'user') else None,
            'role': self.role,
            'is_ready': self.is_ready,
            'is_kicked': self.is_kicked,
            'joined_at': self.joined_at.isoformat() if self.joined_at else None
        }