from sqlalchemy import (
    Column, Integer, String, Boolean, Timestamp, ForeignKey
)
from sqlalchemy.dialects.postgresql import JSONB
from .base import Base

class Match(Base):
    __tablename__ = "matches"

    id = Column(Integer, primary_key=True)
    room_id = Column(Integer, ForeignKey("rooms.id"))
    mode = Column(String(20))
    max_players = Column(Integer)
    advanced = Column(JSONB)
    round_time = Column(Integer)
    started_at = Column(Timestamp)
    ended_at = Column(Timestamp)


class MatchPlayer(Base):
    __tablename__ = "match_players"

    id = Column(Integer, primary_key=True)
    match_id = Column(Integer, ForeignKey("matches.id"))
    account_id = Column(Integer, ForeignKey("accounts.id"))

    score = Column(Integer, default=0)
    eliminated = Column(Boolean, default=False)
    forfeited = Column(Boolean, default=False)
    winner = Column(Boolean, default=False)

    joined_at = Column(Timestamp)
