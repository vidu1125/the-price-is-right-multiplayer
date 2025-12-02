from app.models import db
from datetime import datetime

class Room(db.Model):
    __tablename__ = 'rooms'
    
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    visibility = db.Column(db.String(20), default='public')
    host_id = db.Column(db.Integer, db.ForeignKey('accounts.id'), nullable=False)
    status = db.Column(db.String(20), default='waiting')
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    members = db.relationship('RoomMember', backref='room', lazy=True, cascade='all, delete-orphan')
    matches = db.relationship('Match', backref='room', lazy=True)


    
    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'visibility': self.visibility,
            'host_id': self.host_id,
            'host_name': self.host.profile.name if self.host and self.host.profile else 'Unknown',
            'status': self.status,
            'current_players': len([m for m in self.members if not m.kicked]),
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None
        }


class RoomMember(db.Model):
    __tablename__ = 'room_members'
    
    room_id = db.Column(db.Integer, db.ForeignKey('rooms.id'), primary_key=True)
    account_id = db.Column(db.Integer, db.ForeignKey('accounts.id'), primary_key=True)
    role = db.Column(db.String(20), default='member')
    ready = db.Column(db.Boolean, default=False)
    kicked = db.Column(db.Boolean, default=False)
    joined_at = db.Column(db.DateTime, default=datetime.utcnow)
    left_at = db.Column(db.DateTime, nullable=True)
    
    def to_dict(self):
        profile = self.account.profile if self.account and self.account.profile else None
        return {
            'room_id': self.room_id,
            'account_id': self.account_id,
            'name': profile.name if profile else 'Unknown',
            'avatar': profile.avatar if profile else None,
            'role': self.role,
            'ready': self.ready,
            'kicked': self.kicked,
            'joined_at': self.joined_at.isoformat() if self.joined_at else None
        }
