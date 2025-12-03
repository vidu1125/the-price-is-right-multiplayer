from datetime import datetime

from app.models import db



class Match(db.Model):
    __tablename__ = 'matches'
    
    id = db.Column(db.Integer, primary_key=True)
    room_id = db.Column(db.Integer, db.ForeignKey('rooms.id'), nullable=False)
    started_at = db.Column(db.DateTime, default=datetime.utcnow)
    ended_at = db.Column(db.DateTime, nullable=True)
    status = db.Column(db.String(20), default='playing')
    
    # Relationships
    settings = db.relationship(
        'MatchSettings',
        backref='match',
        uselist=False,
        cascade='all, delete-orphan'
    )
    players = db.relationship(
        'MatchPlayer',
        backref='match',
        lazy=True,
        cascade='all, delete-orphan'
    )
    # ⭐ Dùng trực tiếp class Round thay vì string 'Round'
    rounds = db.relationship(
        "Round",   # dùng tên class dạng string → SQLAlchemy tự resolve sau
        backref="match",
        lazy=True,
        cascade="all, delete-orphan"
    )

    
    def to_dict(self):
        return {
            'id': self.id,
            'room_id': self.room_id,
            'status': self.status,
            'started_at': self.started_at.isoformat() if self.started_at else None,
            'ended_at': self.ended_at.isoformat() if self.ended_at else None
        }


class MatchSettings(db.Model):
    __tablename__ = 'match_settings'
    
    match_id = db.Column(db.Integer, db.ForeignKey('matches.id'), primary_key=True)
    mode = db.Column(db.String(20), default='scoring')
    max_players = db.Column(db.Integer, default=4)
    wager = db.Column(db.Boolean, default=False)
    round_time = db.Column(db.Integer, default=15)
    visibility = db.Column(db.String(20), default='public')
    
    def to_dict(self):
        return {
            'match_id': self.match_id,
            'mode': self.mode,
            'max_players': self.max_players,
            'wager': self.wager,
            'round_time': self.round_time,
            'visibility': self.visibility
        }


class MatchPlayer(db.Model):
    __tablename__ = 'match_players'
    
    id = db.Column(db.Integer, primary_key=True)
    match_id = db.Column(db.Integer, db.ForeignKey('matches.id'), nullable=False)
    account_id = db.Column(db.Integer, db.ForeignKey('accounts.id'), nullable=False)
    score = db.Column(db.Integer, default=0)
    eliminated = db.Column(db.Boolean, default=False)
    forfeited = db.Column(db.Boolean, default=False)
    winner = db.Column(db.Boolean, default=False)
    connected = db.Column(db.Boolean, default=True)
    joined_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    def to_dict(self):
        return {
            'id': self.id,
            'match_id': self.match_id,
            'account_id': self.account_id,
            'score': self.score,
            'eliminated': self.eliminated,
            'winner': self.winner,
            'connected': self.connected
        }
