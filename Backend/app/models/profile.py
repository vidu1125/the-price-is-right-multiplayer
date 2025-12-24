from sqlalchemy import (
    Column, Integer, String, Text, Timestamp, ForeignKey
)
from sqlalchemy.dialects.postgresql import JSONB
from .base import Base

class Profile(Base):
    __tablename__ = "profiles"

    id = Column(Integer, primary_key=True)
    account_id = Column(Integer, ForeignKey("accounts.id"))
    name = Column(String(50))
    avatar = Column(Text)
    bio = Column(Text)

    matches = Column(Integer, default=0)
    wins = Column(Integer, default=0)
    points = Column(Integer, default=0)

    badges = Column(JSONB)

    created_at = Column(Timestamp)
    updated_at = Column(Timestamp)
