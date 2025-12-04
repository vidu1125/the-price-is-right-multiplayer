# app/models/match.py
from app.models import db
from datetime import datetime

class Match(db.Model):
    __tablename__ = "matches"

    id = db.Column(db.Integer, primary_key=True)
    room_id = db.Column(db.Integer, db.ForeignKey("rooms.id", ondelete="SET NULL"))
    mode = db.Column(db.String(20))
    max_players = db.Column(db.Integer)
    wager = db.Column(db.Boolean)
    round_time = db.Column(db.Integer)
    started_at = db.Column(db.DateTime)
    ended_at = db.Column(db.DateTime)

    players = db.relationship("MatchPlayer", back_populates="match", cascade="all, delete-orphan")

    def to_dict(self):
        return {
            "id": self.id,
            "room_id": self.room_id,
            "mode": self.mode,
            "max_players": self.max_players,
            "wager": self.wager,
            "round_time": self.round_time,
            "started_at": self.started_at.isoformat() if self.started_at else None,
            "ended_at": self.ended_at.isoformat() if self.ended_at else None,
            "players": [p.to_dict_basic() for p in self.players]
        }


class MatchPlayer(db.Model):
    __tablename__ = "match_players"

    id = db.Column(db.Integer, primary_key=True)
    match_id = db.Column(db.Integer, db.ForeignKey("matches.id", ondelete="CASCADE"))
    account_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="CASCADE"))
    score = db.Column(db.Integer)
    eliminated = db.Column(db.Boolean)
    forfeited = db.Column(db.Boolean)
    winner = db.Column(db.Boolean)
    joined_at = db.Column(db.DateTime)

    match = db.relationship("Match", back_populates="players")

    def to_dict(self):
        profile = self.account.profile if self.account else None
        return {
            "id": self.id,
            "match_id": self.match_id,
            "account_id": self.account_id,
            "name": profile.name if profile else "Unknown",
            "avatar": profile.avatar if profile else None,
            "score": self.score,
            "eliminated": self.eliminated,
            "forfeited": self.forfeited,
            "winner": self.winner,
        }

    def to_dict_basic(self):
        return {
            "id": self.id,
            "account_id": self.account_id,
            "score": self.score,
            "eliminated": self.eliminated,
            "winner": self.winner
        }
