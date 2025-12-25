from app.models import db
from .base import Base
from sqlalchemy.sql import func

class Match(Base):
    __tablename__ = "matches"

    id = db.Column(db.Integer, primary_key=True)

    room_id = db.Column(
        db.Integer,
        db.ForeignKey("rooms.id"),
        nullable=False
    )

    mode = db.Column(db.String(20), nullable=False)
    max_players = db.Column(db.Integer, nullable=False)

    advanced = db.Column(db.JSON)   # 👈 QUAN TRỌNG (xem lưu ý dưới)

    round_time = db.Column(db.Integer)

    started_at = db.Column(db.DateTime)
    ended_at = db.Column(db.DateTime)


class MatchPlayer(Base):
    __tablename__ = "match_players"

    id = db.Column(db.Integer, primary_key=True)

    match_id = db.Column(
        db.Integer,
        db.ForeignKey("matches.id"),
        nullable=False
    )

    account_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        nullable=False
    )

    score = db.Column(db.Integer, default=0)

    eliminated = db.Column(db.Boolean, default=False)
    forfeited = db.Column(db.Boolean, default=False)
    winner = db.Column(db.Boolean, default=False)

    joined_at = db.Column(db.DateTime, server_default=func.now())
