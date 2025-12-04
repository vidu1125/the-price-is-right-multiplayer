from app.models import db
from datetime import datetime

class Account(db.Model):
    __tablename__ = 'accounts'
    
    id = db.Column(db.Integer, primary_key=True)
    email = db.Column(db.String(100), unique=True, nullable=False)
    password = db.Column(db.String(255), nullable=False)
    provider = db.Column(db.String(20), default='local')
    status = db.Column(db.String(20), default='active')
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    profile = db.relationship('Profile', backref='account', uselist=False, cascade='all, delete-orphan')
    hosted_rooms = db.relationship('Room', backref='host', lazy=True, foreign_keys='Room.host_id')
    room_memberships = db.relationship('RoomMember', backref='account', lazy=True)
    
    def to_dict(self):
        return {
            'id': self.id,
            'email': self.email,
            'provider': self.provider,
            'status': self.status,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }


class Profile(db.Model):
    __tablename__ = 'profiles'
    
    id = db.Column(db.Integer, primary_key=True)
    account_id = db.Column(db.Integer, db.ForeignKey('accounts.id'), unique=True, nullable=False)
    name = db.Column(db.String(50), nullable=False)
    avatar = db.Column(db.Text)
    bio = db.Column(db.Text)
    matches = db.Column(db.Integer, default=0)
    wins = db.Column(db.Integer, default=0)
    points = db.Column(db.Integer, default=0)
    badges = db.Column(db.JSON, default=[])
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    def to_dict(self):
        return {
            'id': self.id,
            'account_id': self.account_id,
            'name': self.name,
            'avatar': self.avatar,
            'bio': self.bio,
            'matches': self.matches,
            'wins': self.wins,
            'points': self.points,
            'badges': self.badges,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }