from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()

from app.models.session import AccountSession
from app.models.account import Account, Profile
from app.models.friend import Friend
from app.models.room import Room, RoomMember
from app.models.match import Match, MatchPlayer
from app.models.round import Round
from app.models.round1 import R1Item, R1Answer
from app.models.question import Question
from app.models.product import Product
from app.models.wheel import Wheel
from app.models.round2 import R2Item, R2Answer
from app.models.round3 import R3Item, R3Answer
from app.models.roundfp import FPRound, FPAnswer
from app.models.history import PlayerMatchHistory


__all__ = [
    "db",
    "Account", "Profile", "AccountSession",
    "Friend",
    "Room", "RoomMember",
    "Match", "MatchPlayer",
    "Round",
    "Question", "R1Item", "R1Answer",
    "Product", "R2Item", "R2Answer",
    "Wheel", "R3Item", "R3Answer",
    "FPRound", "FPAnswer",
    "PlayerMatchHistory",
]