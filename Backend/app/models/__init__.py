from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()

# Import all models
from app.models.account import Account, Profile
from app.models.room import Room, RoomMember
from app.models.match import Match, MatchSettings, MatchPlayer

__all__ = [
    'db',
    'Account',
    'Profile',
    'Room',
    'RoomMember',
    'Match',
    'MatchSettings',
    'MatchPlayer'
]